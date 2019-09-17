// Copyright (c) 2017-2019 The Elastos Foundation
// Use of this source code is governed by an MIT
// license that can be found in the LICENSE file.
//

package state

import (
	"bytes"
	"errors"
	"fmt"
	"github.com/elastos/Elastos.ELA/utils"
	"sort"
	"sync"

	"github.com/elastos/Elastos.ELA/common"
	"github.com/elastos/Elastos.ELA/common/config"
	"github.com/elastos/Elastos.ELA/core/types"
	"github.com/elastos/Elastos.ELA/core/types/payload"
)

type Committee struct {
	KeyFrame
	mtx     sync.RWMutex
	state   *State
	params  *config.Params
	manager *ProposalManager

	getCheckpoint func(height uint32) *Checkpoint
}

func (c *Committee) GetState() *State {
	return c.state
}

func (c *Committee) GetProposalManager() *ProposalManager {
	return c.manager
}

func (c *Committee) ExistCR(programCode []byte) bool {
	existCandidate := c.state.ExistCandidate(programCode)
	if existCandidate {
		return true
	}

	return c.IsCRMember(programCode)
}

func (c *Committee) IsCRMember(programCode []byte) bool {
	c.mtx.RLock()
	defer c.mtx.RUnlock()
	for _, v := range c.Members {
		if bytes.Equal(programCode, v.Info.Code) {
			return true
		}
	}
	return false
}

func (c *Committee) IsInVotingPeriod(height uint32) bool {
	c.mtx.RLock()
	defer c.mtx.RUnlock()
	return c.isInVotingPeriod(height)
}

func (c *Committee) IsInElectionPeriod() bool {
	c.mtx.RLock()
	defer c.mtx.RUnlock()
	return c.InElectionPeriod
}

func (c *Committee) GetMembersDIDs() []common.Uint168 {
	c.mtx.RLock()
	defer c.mtx.RUnlock()

	result := make([]common.Uint168, 0, len(c.Members))
	for _, v := range c.Members {
		result = append(result, v.Info.DID)
	}
	return result
}

//get all CRMembers
func (c *Committee) GetAllMembers() []*CRMember {
	c.mtx.RLock()
	defer c.mtx.RUnlock()

	return getCRMembers(c.Members)
}

func (c *Committee) GetMembersCodes() [][]byte {
	c.mtx.RLock()
	defer c.mtx.RUnlock()

	result := make([][]byte, 0, len(c.Members))
	for _, v := range c.Members {
		result = append(result, v.Info.Code)
	}
	return result
}

func (c *Committee) ProcessBlock(block *types.Block, confirm *payload.Confirm) {
	c.mtx.Lock()
	defer c.mtx.Unlock()

	if block.Height < c.params.CRVotingStartHeight {
		return
	}

	// If reached the voting start height, record the last voting start
	// height.
	c.recordLastVotingStartHeight(block.Height)

	// If in election period and not in voting period, deal with TransferAsset
	// ReturnCRDepositCoin CRCProposal type of transaction only.
	isVoting := c.isInVotingPeriod(block.Height)
	if isVoting {
		c.state.ProcessBlock(block, confirm)
	} else {
		c.state.ProcessElectionBlock(block)
	}

	if c.shouldChange(block.Height) {
		committeeDIDs, err := c.changeCommitteeMembers(block.Height)
		if err != nil {
			log.Error("[ProcessBlock] change committee members error: ", err)
			return
		}

		checkpoint := Checkpoint{
			KeyFrame: c.KeyFrame,
		}
		checkpoint.StateKeyFrame = *c.state.FinishVoting(committeeDIDs)
	}
}

func (c *Committee) recordLastVotingStartHeight(height uint32) {
	// Update last voting start height one block ahead.
	if height == c.LastCommitteeHeight+c.params.CRDutyPeriod-
		c.params.CRVotingPeriod-1 {
		lastVotingStartHeight := c.LastVotingStartHeight
		c.state.history.Append(height, func() {
			c.LastVotingStartHeight = height + 1
		}, func() {
			c.LastVotingStartHeight = lastVotingStartHeight
		})
	}
}

func (c *Committee) updateMembers(height uint32) {
	if !c.InElectionPeriod {
		return
	}

	// Check vote of CRCImpeachment and change the member if in election period.
	// todo get current circulation by calculation
	circulation := common.Fixed64(3300 * 10000 * 100000000)
	changeMembers := make(map[common.Uint168]*CRMember)
	for k, v := range c.Members {
		if v.ImpeachmentVotes >= circulation/10 {
			changeMembers[k] = v
		}
	}

	if uint32(len(changeMembers)) < c.params.CRAgreementCount {
		changeMembers = c.Members
		lastVotingStartHeight := c.LastVotingStartHeight

		c.state.history.Append(height, func() {
			c.InElectionPeriod = false
			c.LastVotingStartHeight = height
		}, func() {
			c.InElectionPeriod = true
			c.LastVotingStartHeight = lastVotingStartHeight
		})
	}

	for _, v := range changeMembers {
		c.changeToCandidate(height, v)
	}
}

func (c *Committee) processImpeachment(height uint32, member []byte,
	votes common.Fixed64, history *utils.History) {
	for _, v := range c.Members {
		if bytes.Equal(v.Info.Code, member) {
			history.Append(height, func() {
				v.ImpeachmentVotes += votes
			}, func() {
				v.ImpeachmentVotes -= votes
			})
			return
		}
	}
}

func (c *Committee) RollbackTo(height uint32) error {
	c.mtx.Lock()
	defer c.mtx.Unlock()
	lastCommitHeight := c.LastCommitteeHeight

	if height >= lastCommitHeight {
		if height > c.state.history.Height() {
			return fmt.Errorf("can't rollback to height: %d", height)
		}

		if err := c.state.RollbackTo(height); err != nil {
			log.Warn("state rollback err: ", err)
		}
	} else {
		point := c.getCheckpoint(height)
		if point == nil {
			return errors.New("can't find checkpoint")
		}

		c.state.StateKeyFrame = point.StateKeyFrame
		c.KeyFrame = point.KeyFrame
	}
	return nil
}

func (c *Committee) Recover(checkpoint *Checkpoint) {
	c.mtx.Lock()
	defer c.mtx.Unlock()
	c.state.StateKeyFrame = checkpoint.StateKeyFrame
	c.KeyFrame = checkpoint.KeyFrame
}

func (c *Committee) shouldChange(height uint32) bool {
	if height == c.params.CRCommitteeStartHeight {
		return true
	}

	return height == c.LastVotingStartHeight+c.params.CRVotingPeriod
}

func (c *Committee) isInVotingPeriod(height uint32) bool {
	//todo consider emergency election later
	inVotingPeriod := func(committeeUpdateHeight uint32) bool {
		return height >= committeeUpdateHeight-c.params.CRVotingPeriod &&
			height < committeeUpdateHeight
	}

	if c.LastCommitteeHeight < c.params.CRCommitteeStartHeight {
		return height >= c.params.CRVotingStartHeight &&
			height < c.params.CRCommitteeStartHeight
	} else {
		if !c.InElectionPeriod {
			return height < c.LastVotingStartHeight+c.params.CRVotingPeriod
		}
		return inVotingPeriod(c.LastCommitteeHeight + c.params.CRDutyPeriod)
	}
}

func (c *Committee) changeCommitteeMembers(height uint32) (
	[]common.Uint168, error) {
	candidates, err := c.getActiveCRCandidatesDesc()
	if err != nil {
		c.InElectionPeriod = false
		c.LastVotingStartHeight = height
		return nil, err
	}

	result := make([]common.Uint168, 0, c.params.CRMemberCount)
	c.Members = make(map[common.Uint168]*CRMember, c.params.CRMemberCount)
	for i := 0; i < int(c.params.CRMemberCount); i++ {
		c.Members[candidates[i].info.DID] = c.generateMember(candidates[i])
		result = append(result, candidates[i].info.DID)
	}

	c.InElectionPeriod = true
	c.LastCommitteeHeight = height
	return result, nil
}

func (c *Committee) generateMember(candidate *Candidate) *CRMember {
	return &CRMember{
		Info:             candidate.info,
		ImpeachmentVotes: 0,
		DepositHash:      candidate.depositHash,
		DepositAmount:    candidate.depositAmount,
		Penalty:          candidate.penalty,
	}
}

func (c *Committee) changeToCandidate(height uint32, member *CRMember) {
	// Calculate penalty by election block count.
	electionCount := height - c.LastCommitteeHeight
	electionRate := float64(electionCount) / float64(c.params.CRDutyPeriod)
	notElectionPenalty := MinDepositAmount * common.Fixed64(1-electionRate)

	// Calculate penalty by vote proposal count.
	// todo change penalty of member according to vote proposal count
	notVoteProposalPenalty := common.Fixed64(0)

	// Calculate the final penalty.
	penalty := member.Penalty
	finalPenalty := penalty + notElectionPenalty + notVoteProposalPenalty

	c.state.history.Append(height, func() {
		// Add candidate to impeached candidates map.
		candidate := c.generateCandidate(height, member)
		candidate.penalty = finalPenalty
		c.state.ImpeachedCandidates[member.Info.DID] = candidate

		// Remove member from members map.
		delete(c.Members, member.Info.DID)
	}, func() {
		// Add member into members map.
		c.Members[member.Info.DID] = member

		// Remove candidate from impeached candidates map.
		delete(c.state.ImpeachedCandidates, member.Info.DID)
	})
}

func (c *Committee) generateCandidate(height uint32, member *CRMember) *Candidate {
	return &Candidate{
		info:          member.Info,
		state:         Canceled,
		cancelHeight:  height,
		depositAmount: member.DepositAmount,
		depositHash:   member.DepositHash,
		penalty:       member.Penalty,
	}
}

func (c *Committee) getActiveCRCandidatesDesc() ([]*Candidate, error) {
	candidates := c.state.GetCandidates(Active)
	if uint32(len(candidates)) < c.params.CRMemberCount {
		return nil, errors.New("candidates count less than required count")
	}

	sort.Slice(candidates, func(i, j int) bool {
		if candidates[i].votes == candidates[j].votes {
			iCRInfo := candidates[i].Info()
			jCRInfo := candidates[j].Info()
			return iCRInfo.GetCodeHash().Compare(jCRInfo.GetCodeHash()) < 0
		}
		return candidates[i].votes > candidates[j].votes
	})
	return candidates, nil
}

func NewCommittee(params *config.Params) *Committee {
	committee := &Committee{
		state:    NewState(params),
		params:   params,
		KeyFrame: *NewKeyFrame(),
		manager:  NewProposalManager(params),
	}
	committee.state.SetManager(committee.manager)
	committee.state.SetUpdateMembers(committee.updateMembers)
	params.CkpManager.Register(NewCheckpoint(committee))
	return committee
}
