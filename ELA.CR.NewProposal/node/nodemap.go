package node

import (
	"fmt"
	"sync"

	"github.com/elastos/Elastos.ELA/protocol"

	"github.com/elastos/Elastos.ELA.Utility/p2p"
)

// The neigbor node list
type neighbourNodes struct {
	sync.RWMutex
	// Todo using the Pool structure
	List map[uint64]*node
}

func (nm *neighbourNodes) NodeExisted(uid uint64) bool {
	_, ok := nm.List[uid]
	return ok
}

func (nm *neighbourNodes) AddNbrNode(n protocol.Noder) {
	nm.Lock()
	defer nm.Unlock()

	if nm.NodeExisted(n.ID()) {
		fmt.Printf("Insert a existed node\n")
	} else {
		node, err := n.(*node)
		if err == false {
			fmt.Println("Convert the noder error when add node")
			return
		}
		nm.List[n.ID()] = node
	}
}

func (nm *neighbourNodes) DelNbrNode(id uint64) (protocol.Noder, bool) {
	nm.Lock()
	defer nm.Unlock()

	n, ok := nm.List[id]
	if ok == false {
		return nil, false
	}
	delete(nm.List, id)
	return n, true
}

func (nm *neighbourNodes) GetConnectionCount() uint {
	nm.RLock()
	defer nm.RUnlock()

	var cnt uint
	for _, node := range nm.List {
		if node.State() == p2p.ESTABLISH {
			cnt++
		}
	}
	return cnt
}

func (nm *neighbourNodes) init() {
	nm.List = make(map[uint64]*node)
}

func (nm *neighbourNodes) NodeEstablished(id uint64) bool {
	nm.RLock()
	defer nm.RUnlock()

	n, ok := nm.List[id]
	if ok == false {
		return false
	}

	if n.State() != p2p.ESTABLISH {
		return false
	}

	return true
}

func (node *node) GetNeighbourAddress() []p2p.NetAddress {
	node.neighbourNodes.RLock()
	defer node.neighbourNodes.RUnlock()

	var addrs []p2p.NetAddress
	for _, n := range node.neighbourNodes.List {
		if n.State() != p2p.ESTABLISH {
			continue
		}
		var addr p2p.NetAddress
		addr.IP, _ = n.Addr16()
		addr.Time = n.GetTime()
		addr.Services = n.Services()
		addr.Port = n.Port()
		addr.ID = n.ID()
		addrs = append(addrs, addr)
	}

	return addrs
}

func (node *node) GetNeighborHeights() []uint64 {
	node.neighbourNodes.RLock()
	defer node.neighbourNodes.RUnlock()

	heights := make([]uint64, 0, len(node.neighbourNodes.List))
	for _, n := range node.neighbourNodes.List {
		if n.State() == p2p.ESTABLISH {
			height := n.Height()
			heights = append(heights, height)
		}
	}
	return heights
}

func (node *node) GetNeighborNoder() []protocol.Noder {
	node.neighbourNodes.RLock()
	defer node.neighbourNodes.RUnlock()

	nodes := make([]protocol.Noder, 0, len(node.neighbourNodes.List))
	for _, n := range node.neighbourNodes.List {
		if n.State() == p2p.ESTABLISH {
			node := n
			nodes = append(nodes, node)
		}
	}
	return nodes
}

func (node *node) GetNeighbourCount() uint32 {
	node.neighbourNodes.RLock()
	defer node.neighbourNodes.RUnlock()
	var count uint32
	for _, n := range node.neighbourNodes.List {
		if n.State() == p2p.ESTABLISH {
			count++
		}
	}
	return count
}

func (node *node) GetANeighbourRandomly() protocol.Noder {
	node.neighbourNodes.RLock()
	defer node.neighbourNodes.RUnlock()
	for _, n := range node.neighbourNodes.List {
		if n.State() == p2p.ESTABLISH {
			return n
		}
	}
	return nil
}

func (node *node) IsNeighborNoder(n protocol.Noder) bool {
	node.neighbourNodes.RLock()
	defer node.neighbourNodes.RUnlock()

	_, ok := node.neighbourNodes.List[n.ID()]
	return ok
}
