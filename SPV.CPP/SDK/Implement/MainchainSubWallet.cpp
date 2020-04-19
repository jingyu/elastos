/*
 * Copyright (c) 2019 Elastos Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "MainchainSubWallet.h"
#include "MasterWallet.h"

#include <Common/Utils.h>
#include <Common/ErrorChecker.h>
#include <WalletCore/Key.h>
#include <WalletCore/CoinInfo.h>
#include <Wallet/UTXO.h>
#include <Plugin/Transaction/Asset.h>
#include <Plugin/Transaction/Payload/TransferCrossChainAsset.h>
#include <Plugin/Transaction/Payload/ProducerInfo.h>
#include <Plugin/Transaction/Payload/CancelProducer.h>
#include <Plugin/Transaction/Payload/OutputPayload/PayloadVote.h>
#include <Plugin/Transaction/Payload/CRInfo.h>
#include <Plugin/Transaction/Payload/UnregisterCR.h>
#include <Plugin/Transaction/Payload/CRCProposal.h>
#include <Plugin/Transaction/Payload/CRCProposalReview.h>
#include <Plugin/Transaction/Payload/CRCProposalTracking.h>
#include <Plugin/Transaction/Payload/CRCProposalWithdraw.h>
#include <Plugin/Transaction/TransactionInput.h>
#include <Plugin/Transaction/TransactionOutput.h>
#include <SpvService/Config.h>
#include <Plugin/Transaction/Payload/ReturnDepositCoin.h>
#include <CMakeConfig.h>

#include <vector>
#include <map>
#include <boost/scoped_ptr.hpp>

namespace Elastos {
	namespace ElaWallet {

#define DEPOSIT_MIN_ELA 5000

		MainchainSubWallet::MainchainSubWallet(const CoinInfoPtr &info,
											   const ChainConfigPtr &config,
											   MasterWallet *parent,
											   const std::string &netType) :
				SubWallet(info, config, parent, netType) {
		}

		MainchainSubWallet::~MainchainSubWallet() {
		}

		nlohmann::json MainchainSubWallet::CreateDepositTransaction(const std::string &fromAddress,
																	const std::string &sideChainID,
																	const std::string &amount,
																	const std::string &sideChainAddress,
																	const std::string &memo) {
			WalletPtr wallet = _walletManager->GetWallet();
			ArgInfo("{} {}", wallet->GetWalletID(), GetFunName());
			ArgInfo("fromAddr: {}", fromAddress);
			ArgInfo("sideChainID: {}", sideChainID);
			ArgInfo("amount: {}", amount);
			ArgInfo("sideChainAddr: {}", sideChainAddress);
			ArgInfo("memo: {}", memo);

			ErrorChecker::CheckBigIntAmount(amount);
			ErrorChecker::CheckParam(sideChainID == CHAINID_MAINCHAIN, Error::InvalidArgument, "can not be mainChain");

			BigInt value;
			value.setDec(amount);

			TransferInfo info(sideChainAddress, 0, value);
			PayloadPtr payload = PayloadPtr(new TransferCrossChainAsset({info}));

			ChainConfigPtr configPtr =  _parent->GetChainConfig(sideChainID);
			OutputArray outputs;
			Address receiveAddr(configPtr->GenesisAddress());
			outputs.emplace_back(OutputPtr(new TransactionOutput(value + _config->MinFee(), receiveAddr)));
			AddressPtr fromAddr(new Address(fromAddress));

			TransactionPtr tx = wallet->CreateTransaction(Transaction::transferCrossChainAsset, payload, fromAddr, outputs, memo);

			nlohmann::json result;
			EncodeTx(result, tx);

			ArgInfo("r => {}", result.dump());
			return result;
		}

		TransactionPtr MainchainSubWallet::CreateVoteTx(const VoteContent &voteContent, const std::string &memo,
		                                                bool max, VoteContentArray &dropedVotes) {
			std::string m;

			if (!memo.empty())
				m = "type:text,msg:" + memo;

			TransactionPtr tx = _walletManager->GetWallet()->Vote(voteContent, m, max, dropedVotes);

			if (_info->GetChainID() == "ELA")
				tx->SetVersion(Transaction::TxVersion::V09);

			tx->FixIndex();

			return tx;
		}

		nlohmann::json MainchainSubWallet::GenerateProducerPayload(
			const std::string &ownerPublicKey,
			const std::string &nodePublicKey,
			const std::string &nickName,
			const std::string &url,
			const std::string &ipAddress,
			uint64_t location,
			const std::string &payPasswd) const {

			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("ownerPubKey: {}", ownerPublicKey);
			ArgInfo("nodePubKey: {}", nodePublicKey);
			ArgInfo("nickName: {}", nickName);
			ArgInfo("url: {}", url);
			ArgInfo("ipAddress: {}", ipAddress);
			ArgInfo("location: {}", location);
			ArgInfo("payPasswd: *");

			ErrorChecker::CheckPassword(payPasswd, "Generate payload");

			Key verifyPubKey;
			bytes_t ownerPubKey = bytes_t(ownerPublicKey);
			verifyPubKey.SetPubKey(ownerPubKey);

			bytes_t nodePubKey = bytes_t(nodePublicKey);
			verifyPubKey.SetPubKey(nodePubKey);

			ProducerInfo pr;
			pr.SetPublicKey(ownerPubKey);
			pr.SetNodePublicKey(nodePubKey);
			pr.SetNickName(nickName);
			pr.SetUrl(url);
			pr.SetAddress(ipAddress);
			pr.SetLocation(location);

			ByteStream ostream;
			pr.SerializeUnsigned(ostream, 0);
			bytes_t prUnsigned = ostream.GetBytes();

			pr.SetSignature(_walletManager->GetWallet()->SignWithOwnerKey(prUnsigned, payPasswd));

			nlohmann::json payloadJson = pr.ToJson(0);

			ArgInfo("r => {}", payloadJson.dump());
			return payloadJson;
		}

		nlohmann::json MainchainSubWallet::GenerateCancelProducerPayload(
			const std::string &ownerPublicKey,
			const std::string &payPasswd) const {

			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("ownerPubKey: {}", ownerPublicKey);
			ArgInfo("payPasswd: *");

			ErrorChecker::CheckPassword(payPasswd, "Generate payload");
			size_t pubKeyLen = ownerPublicKey.size() >> 1;
			ErrorChecker::CheckParam(pubKeyLen != 33 && pubKeyLen != 65, Error::PubKeyLength,
									 "Public key length should be 33 or 65 bytes");

			CancelProducer pc;
			pc.SetPublicKey(ownerPublicKey);

			ByteStream ostream;
			pc.SerializeUnsigned(ostream, 0);
			bytes_t pcUnsigned = ostream.GetBytes();

			pc.SetSignature(_walletManager->GetWallet()->SignWithOwnerKey(pcUnsigned, payPasswd));

			nlohmann::json payloadJson = pc.ToJson(0);
			ArgInfo("r => {}", payloadJson.dump());
			return payloadJson;
		}

		nlohmann::json MainchainSubWallet::CreateRegisterProducerTransaction(
			const std::string &fromAddress,
			const nlohmann::json &payloadJson,
			const std::string &amount,
			const std::string &memo) {
			WalletPtr wallet = _walletManager->GetWallet();

			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("fromAddr: {}", fromAddress);
			ArgInfo("payload: {}", payloadJson.dump());
			ArgInfo("amount: {}", amount);
			ArgInfo("memo: {}", memo);

			ErrorChecker::CheckBigIntAmount(amount);
			BigInt bgAmount, minAmount(DEPOSIT_MIN_ELA);
			bgAmount.setDec(amount);

			minAmount *= SELA_PER_ELA;

			ErrorChecker::CheckParam(bgAmount < minAmount, Error::DepositAmountInsufficient,
									 "Producer deposit amount is insufficient");

			PayloadPtr payload = PayloadPtr(new ProducerInfo());
			try {
				payload->FromJson(payloadJson, 0);
			} catch (const nlohmann::detail::exception &e) {
				ErrorChecker::ThrowParamException(Error::JsonFormatError,
												  "Payload format err: " + std::string(e.what()));
			}

			bytes_t pubkey = static_cast<ProducerInfo *>(payload.get())->GetPublicKey();

			OutputArray outputs;
			Address receiveAddr(PrefixDeposit, pubkey);
			outputs.push_back(OutputPtr(new TransactionOutput(bgAmount, receiveAddr)));
			AddressPtr fromAddr(new Address(fromAddress));

			TransactionPtr tx = wallet->CreateTransaction(Transaction::registerProducer, payload, fromAddr, outputs, memo);

			nlohmann::json result;
			EncodeTx(result, tx);

			ArgInfo("r => {}", result.dump());
			return result;
		}

		nlohmann::json MainchainSubWallet::CreateUpdateProducerTransaction(
			const std::string &fromAddress,
			const nlohmann::json &payloadJson,
			const std::string &memo) {

			WalletPtr wallet = _walletManager->GetWallet();
			ArgInfo("{} {}", wallet->GetWalletID(), GetFunName());
			ArgInfo("fromAddr: {}", fromAddress);
			ArgInfo("payload: {}", payloadJson.dump());
			ArgInfo("memo: {}", memo);

			PayloadPtr payload = PayloadPtr(new ProducerInfo());
			try {
				payload->FromJson(payloadJson, 0);
			} catch (const nlohmann::detail::exception &e) {
				ErrorChecker::ThrowParamException(Error::JsonFormatError,
												  "Payload format err: " + std::string(e.what()));
			}

			OutputArray outputs;
			AddressPtr receiveAddr = wallet->GetReceiveAddress();
			outputs.push_back(OutputPtr(new TransactionOutput(BigInt(0), *receiveAddr)));
			AddressPtr fromAddr(new Address(fromAddress));

			TransactionPtr tx = wallet->CreateTransaction(Transaction::updateProducer, payload, fromAddr, outputs, memo);

			if (tx->GetOutputs().size() > 1) {
				tx->RemoveOutput(tx->GetOutputs().front());
				tx->FixIndex();
			}

			nlohmann::json result;
			EncodeTx(result, tx);

			ArgInfo("r => {}", result.dump());
			return result;
		}

		nlohmann::json MainchainSubWallet::CreateCancelProducerTransaction(
			const std::string &fromAddress,
			const nlohmann::json &payloadJson,
			const std::string &memo) {

			WalletPtr wallet = _walletManager->GetWallet();
			ArgInfo("{} {}", wallet->GetWalletID(), GetFunName());
			ArgInfo("fromAddr: {}", fromAddress);
			ArgInfo("payload: {}", payloadJson.dump());
			ArgInfo("memo: {}", memo);

			PayloadPtr payload = PayloadPtr(new CancelProducer());
			try {
				payload->FromJson(payloadJson, 0);
			} catch (const nlohmann::detail::exception &e) {
				ErrorChecker::ThrowParamException(Error::JsonFormatError,
												  "Payload format err: " + std::string(e.what()));
			}

			OutputArray outputs;
			AddressPtr receiveAddr = wallet->GetReceiveAddress();
			outputs.push_back(OutputPtr(new TransactionOutput(BigInt(0), *receiveAddr)));
			AddressPtr fromAddr(new Address(fromAddress));

			TransactionPtr tx = wallet->CreateTransaction(Transaction::cancelProducer, payload, fromAddr, outputs, memo);

			if (tx->GetOutputs().size() > 1) {
				tx->RemoveOutput(tx->GetOutputs().front());
				tx->FixIndex();
			}

			nlohmann::json result;
			EncodeTx(result, tx);

			ArgInfo("r => {}", result.dump());
			return result;
		}

		nlohmann::json MainchainSubWallet::CreateRetrieveDepositTransaction(
			const std::string &amount,
			const std::string &memo) {

			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("amount: {}", amount);
			ArgInfo("memo: {}", memo);

			ErrorChecker::CheckBigIntAmount(amount);
			BigInt bgAmount;
			bgAmount.setDec(amount);

			ErrorChecker::CheckParam(bgAmount <= 0, Error::CreateTransaction, "output amount should big than zero");

			AddressPtr fromAddress = _walletManager->GetWallet()->GetOwnerDepositAddress();

			PayloadPtr payload = PayloadPtr(new ReturnDepositCoin());
			TransactionPtr tx = _walletManager->GetWallet()->CreateRetrieveTransaction(
				Transaction::returnDepositCoin, payload, bgAmount, fromAddress, memo);

			nlohmann::json result;
			EncodeTx(result, tx);

			ArgInfo("r => {}", result.dump());
			return result;
		}

		std::string MainchainSubWallet::GetOwnerPublicKey() const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			std::string publicKey = _walletManager->GetWallet()->GetOwnerPublilcKey().getHex();
			ArgInfo("r => {}", publicKey);
			return publicKey;
		}

		std::string MainchainSubWallet::GetOwnerAddress() const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());

			std::string address = _walletManager->GetWallet()->GetOwnerAddress()->String();

			ArgInfo("r => {}", address);

			return address;
		}

		void MainchainSubWallet::FilterVoteCandidates(TransactionPtr &tx,
		                                              const nlohmann::json &invalidCandidates) const {
			OutputPayloadPtr &outputPayload = tx->GetOutputs()[0]->GetPayload();
			PayloadVote *pv = dynamic_cast<PayloadVote *>(outputPayload.get());
			ErrorChecker::CheckCondition(!pv, Error::InvalidTransaction, "invalid vote tx");

			nlohmann::json result;
			std::vector<VoteContent> voteContent = pv->GetVoteContent();
			if (voteContent.size() <= 1 || invalidCandidates.size() <= 0) {
				return;
			}

			for (std::vector<VoteContent>::iterator it = voteContent.begin(); it != voteContent.end();) {
				std::string type = (*it).GetTypeString();

				std::set<std::string> invalidList;
				for (size_t i = 0; i < invalidCandidates.size(); ++i) {
					if ((invalidCandidates[i]["Type"].get<std::string>()) == type) {
						invalidList = invalidCandidates[i]["Candidates"].get<std::set<std::string>>();
						break;
					}
				}

				if (invalidList.empty()) {
					++it;
					continue;
				}

				std::vector<CandidateVotes> candidatesVotes = (*it).GetCandidateVotes();
				for (std::vector<CandidateVotes>::iterator i = candidatesVotes.begin(); i != candidatesVotes.end();) {
					std::string candidate = (*i).GetCandidate().getHex();
					if ((*it).GetType() == VoteContent::CRC || (*it).GetType() == VoteContent::CRCImpeachment) {
						uint168 programHash((*i).GetCandidate());
						Address didAddress(programHash);
						candidate = didAddress.String();
					}

					if (invalidList.find(candidate) != invalidList.end()) {
						i = candidatesVotes.erase(i);
					} else {
						++i;
					}
				}

				if (candidatesVotes.empty()) {
					it = voteContent.erase(it);
				} else {
					(*it).SetCandidateVotes(candidatesVotes);
					++it;
				}
			}

			pv->SetVoteContent(voteContent);
		}

		nlohmann::json MainchainSubWallet::CreateVoteProducerTransaction(
			const std::string &fromAddress,
			const std::string &stake,
			const nlohmann::json &publicKeys,
			const std::string &memo,
			const nlohmann::json &invalidCandidates) {

			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("fromAddr: {}", fromAddress);
			ArgInfo("stake: {}", stake);
			ArgInfo("pubkeys: {}", publicKeys.dump());
			ArgInfo("memo: {}", memo);
			ArgInfo("invalidCandidates: {}", invalidCandidates.dump());

			bool max = false;
			BigInt bgStake;
			if (stake == "-1") {
				max = true;
				bgStake = 0;
			} else {
				bgStake.setDec(stake);
			}

			ErrorChecker::CheckJsonArray(publicKeys, 1, "Candidates public keys");
			// -1 means max
			ErrorChecker::CheckParam(bgStake <= 0 && !max, Error::Code::VoteStakeError, "Vote stake should not be zero");

			ErrorChecker::CheckJsonArray(invalidCandidates, 0, "invalidCandidates is error json format");

			VoteContent voteContent(VoteContent::Delegate);
			for (nlohmann::json::const_iterator it = publicKeys.cbegin(); it != publicKeys.cend(); ++it) {
				if (!(*it).is_string()) {
					ErrorChecker::ThrowParamException(Error::Code::JsonFormatError,
													  "Vote produce public keys is not string");
				}
				// Check public key is valid later
				voteContent.AddCandidate(CandidateVotes((*it).get<std::string>(), bgStake));
			}

			ErrorChecker::CheckParam(voteContent.GetCandidateVotes().empty(), Error::InvalidArgument,
									 "Candidate vote list should not be empty");

			VoteContentArray dropedList;
			TransactionPtr tx = CreateVoteTx(voteContent, memo, max, dropedList);
			FilterVoteCandidates(tx, invalidCandidates);

			nlohmann::json result;
			EncodeTx(result, tx);

			std::vector<std::string> dropedTypes;
			for(VoteContentArray::iterator it = dropedList.begin(); it != dropedList.end(); ++it) {
				dropedTypes.push_back((*it).GetTypeString());
			}
			result["DropVotes"] = dropedTypes;

			ArgInfo("r => {}", result.dump());
			return result;
		}

		nlohmann::json MainchainSubWallet::GetVotedProducerList() const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());

			WalletPtr wallet = _walletManager->GetWallet();
			UTXOArray utxos = wallet->GetVoteUTXO();
			nlohmann::json j;
			std::map<std::string, BigInt> votedList;

			for (size_t i = 0; i < utxos.size(); ++i) {
				const OutputPtr &output = utxos[i]->Output();
				if (output->GetType() != TransactionOutput::VoteOutput) {
					continue;
				}

				const PayloadVote *pv = dynamic_cast<const PayloadVote *>(output->GetPayload().get());
				if (pv == nullptr) {
					continue;
				}

				BigInt stake = output->Amount();
				uint8_t version = pv->Version();
				const std::vector<VoteContent> &voteContents = pv->GetVoteContent();
				std::for_each(voteContents.cbegin(), voteContents.cend(),
							  [&votedList, &stake, &version](const VoteContent &vc) {
								  if (vc.GetType() == VoteContent::Type::Delegate) {
									  std::for_each(vc.GetCandidateVotes().cbegin(), vc.GetCandidateVotes().cend(),
													[&votedList, &stake, &version](const CandidateVotes &cvs) {
														std::string c = cvs.GetCandidate().getHex();
														BigInt votes;

														if (version == VOTE_PRODUCER_CR_VERSION)
															votes = cvs.GetVotes();
														else
															votes = stake;

														if (votedList.find(c) != votedList.end()) {
															votedList[c] += votes;
														} else {
															votedList[c] = votes;
														}
													});
								  }
							  });

			}

			for (std::map<std::string, BigInt>::iterator it = votedList.begin(); it != votedList.end(); ++it)
				j[(*it).first] = (*it).second.getDec();

			ArgInfo("r => {}", j.dump());

			return j;
		}

		nlohmann::json MainchainSubWallet::GetRegisteredProducerInfo() const {
			WalletPtr wallet = _walletManager->GetWallet();
			ArgInfo("{} {}", wallet->GetWalletID(), GetFunName());

			nlohmann::json j, info;
			bytes_t types;

			types.push_back(Transaction::registerProducer);
			types.push_back(Transaction::cancelProducer);
			types.push_back(Transaction::updateProducer);
			types.push_back(Transaction::returnDepositCoin);

			std::vector<TransactionPtr> list = _walletManager->GetWallet()->GetTransactions(types);

			j["Status"] = "Unregistered";
			j["Info"] = nlohmann::json();
			for (const TransactionPtr &tx : list) {
				if (tx->GetBlockHeight() == TX_UNCONFIRMED)
					continue;

				if (tx->GetTransactionType() == Transaction::registerProducer ||
				    tx->GetTransactionType() == Transaction::updateProducer) {
					const ProducerInfo *pinfo = dynamic_cast<const ProducerInfo *>(tx->GetPayload());
					if (pinfo) {
						info["OwnerPublicKey"] = pinfo->GetPublicKey().getHex();
						info["NodePublicKey"] = pinfo->GetNodePublicKey().getHex();
						info["NickName"] = pinfo->GetNickName();
						info["URL"] = pinfo->GetUrl();
						info["Location"] = pinfo->GetLocation();
						info["Address"] = pinfo->GetAddress();
						info["Confirms"] = tx->GetConfirms(wallet->LastBlockHeight());

						j["Status"] = "Registered";
						j["Info"] = info;
					}
				} else if (tx->GetTransactionType() == Transaction::cancelProducer) {
					const CancelProducer *pc = dynamic_cast<const CancelProducer *>(tx->GetPayload());
					if (pc) {
						info["Confirms"] = tx->GetConfirms(wallet->LastBlockHeight());

						j["Status"] = "Canceled";
						j["Info"] = info;
					}
				} else if (tx->GetTransactionType() == Transaction::returnDepositCoin) {
					info["Confirms"] = tx->GetConfirms(wallet->LastBlockHeight());

					j["Status"] = "ReturnDeposit";
					j["Info"] = info;
				}
			}

			ArgInfo("r => {}", j.dump());
			return j;
		}

		nlohmann::json MainchainSubWallet::GenerateCRInfoPayload(
				const std::string &crPublicKey,
				const std::string &did,
				const std::string &nickName,
				const std::string &url,
				uint64_t location) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("crPublicKey: {}", crPublicKey);
			ArgInfo("did: {}", did);
			ArgInfo("nickName: {}", nickName);
			ArgInfo("url: {}", url);
			ArgInfo("location: {}", location);

			size_t pubKeyLen = crPublicKey.size() >> 1;
			ErrorChecker::CheckParam(pubKeyLen != 33 && pubKeyLen != 65, Error::PubKeyLength,
			                         "Public key length should be 33 or 65 bytes");

			bytes_t pubkey(crPublicKey);

			Address didAddress(did);
			Address address(PrefixStandard, pubkey);

			CRInfo crInfo;
			crInfo.SetCode(address.RedeemScript());
			crInfo.SetDID(didAddress.ProgramHash());
			crInfo.SetNickName(nickName);
			crInfo.SetUrl(url);
			crInfo.SetLocation(location);

			Address cid;
			cid.SetRedeemScript(PrefixIDChain, crInfo.GetCode());
			crInfo.SetCID(cid.ProgramHash());

			ByteStream ostream;
			crInfo.SerializeUnsigned(ostream, CRInfoDIDVersion);
			uint256 digest(sha256(ostream.GetBytes()));

			nlohmann::json payloadJson = crInfo.ToJson(CRInfoDIDVersion);
			payloadJson["Digest"] = digest.GetHex();

			ArgInfo("r => {}", payloadJson.dump());
			return payloadJson;
		}

		nlohmann::json MainchainSubWallet::GenerateUnregisterCRPayload(const std::string &CID) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("CID: {}", CID);

			Address cid(CID);
			ErrorChecker::CheckParam(!cid.Valid(), Error::InvalidArgument, "invalid crDID");

			UnregisterCR unregisterCR;
			unregisterCR.SetCID(cid.ProgramHash());

			ByteStream ostream;
			unregisterCR.SerializeUnsigned(ostream, 0);
			uint256 digest(sha256(ostream.GetBytes()));

			nlohmann::json payloadJson = unregisterCR.ToJson(0);
			payloadJson["Digest"] = digest.GetHex();

			ArgInfo("r => {}", payloadJson.dump());
			return payloadJson;
		}

		nlohmann::json MainchainSubWallet::CreateRegisterCRTransaction(
				const std::string &fromAddress,
				const nlohmann::json &payloadJSON,
				const std::string &amount,
				const std::string &memo) {

			WalletPtr wallet = _walletManager->GetWallet();
			ArgInfo("{} {}", wallet->GetWalletID(), GetFunName());
			ArgInfo("fromAddr: {}", fromAddress);
			ArgInfo("payload: {}", payloadJSON.dump());
			ArgInfo("amount: {}", amount);
			ArgInfo("memo: {}", memo);

			ErrorChecker::CheckBigIntAmount(amount);
			BigInt bgAmount, minAmount(DEPOSIT_MIN_ELA);
			bgAmount.setDec(amount);

			minAmount *= SELA_PER_ELA;

			ErrorChecker::CheckParam(bgAmount < minAmount, Error::DepositAmountInsufficient,
			                         "cr deposit amount is insufficient");

			ErrorChecker::CheckParam(payloadJSON.find("Signature") == payloadJSON.end(), Error::InvalidArgument,
			                         "Signature can not be empty");

			PayloadPtr payload = PayloadPtr(new CRInfo());
			try {
				payload->FromJson(payloadJSON, CRInfoDIDVersion);
				ErrorChecker::CheckParam(!payload->IsValid(CRInfoDIDVersion), Error::InvalidArgument, "verify signature failed");
			} catch (const nlohmann::detail::exception &e) {
				ErrorChecker::ThrowParamException(Error::JsonFormatError,
				                                  "Payload format err: " + std::string(e.what()));
			}

			bytes_t code = static_cast<CRInfo *>(payload.get())->GetCode();
			Address receiveAddr;
			receiveAddr.SetRedeemScript(PrefixDeposit, code);
			AddressPtr fromAddr(new Address(fromAddress));

			OutputArray outputs;
			outputs.push_back(OutputPtr(new TransactionOutput(bgAmount, receiveAddr)));

			TransactionPtr tx = wallet->CreateTransaction(Transaction::registerCR, payload, fromAddr, outputs, memo);
			tx->SetPayloadVersion(CRInfoDIDVersion);

			nlohmann::json result;
			EncodeTx(result, tx);

			ArgInfo("r => {}", result.dump());
			return result;
		}

		nlohmann::json MainchainSubWallet::CreateUpdateCRTransaction(
				const std::string &fromAddress,
				const nlohmann::json &payloadJSON,
				const std::string &memo) {
			WalletPtr wallet = _walletManager->GetWallet();
			ArgInfo("{} {}", wallet->GetWalletID(), GetFunName());
			ArgInfo("fromAddr: {}", fromAddress);
			ArgInfo("payload: {}", payloadJSON.dump());
			ArgInfo("memo: {}", memo);

			PayloadPtr payload = PayloadPtr(new CRInfo());
			try {
				payload->FromJson(payloadJSON, CRInfoDIDVersion);
			} catch (const nlohmann::detail::exception &e) {
				ErrorChecker::ThrowParamException(Error::JsonFormatError,
				                                  "Payload format err: " + std::string(e.what()));
			}

			OutputArray outputs;
			AddressPtr receiveAddr = wallet->GetReceiveAddress();
			outputs.push_back(OutputPtr(new TransactionOutput(BigInt(0), *receiveAddr)));
			AddressPtr fromAddr(new Address(fromAddress));

			TransactionPtr tx = wallet->CreateTransaction(Transaction::updateCR, payload, fromAddr, outputs, memo);
			tx->SetPayloadVersion(CRInfoDIDVersion);

			if (tx->GetOutputs().size() > 1) {
				tx->RemoveOutput(tx->GetOutputs().front());
				tx->FixIndex();
			}

			nlohmann::json result;
			EncodeTx(result, tx);

			ArgInfo("r => {}", result.dump());
			return result;

		}

		nlohmann::json MainchainSubWallet::CreateUnregisterCRTransaction(
				const std::string &fromAddress,
				const nlohmann::json &payloadJSON,
				const std::string &memo) {
			WalletPtr wallet = _walletManager->GetWallet();
			ArgInfo("{} {}", wallet->GetWalletID(), GetFunName());
			ArgInfo("fromAddr: {}", fromAddress);
			ArgInfo("payload: {}", payloadJSON.dump());
			ArgInfo("memo: {}", memo);

			ErrorChecker::CheckParam(payloadJSON.find("Signature") == payloadJSON.end() ||
			                         payloadJSON["Signature"].get<std::string>() == "",
			                         Error::InvalidArgument, "invalied signature");

			PayloadPtr payload = PayloadPtr(new UnregisterCR());
			try {
				payload->FromJson(payloadJSON, 0);
			} catch (const nlohmann::detail::exception &e) {
				ErrorChecker::ThrowParamException(Error::JsonFormatError,
				                                  "Payload format err: " + std::string(e.what()));
			}

			OutputArray outputs;
			AddressPtr receiveAddr = wallet->GetReceiveAddress();
			outputs.push_back(OutputPtr(new TransactionOutput(BigInt(0), *receiveAddr)));
			AddressPtr fromAddr(new Address(fromAddress));

			TransactionPtr tx = wallet->CreateTransaction(Transaction::unregisterCR, payload, fromAddr, outputs, memo);

			if (tx->GetOutputs().size() > 1) {
				tx->RemoveOutput(tx->GetOutputs().front());
				tx->FixIndex();
			}

			nlohmann::json result;
			EncodeTx(result, tx);

			ArgInfo("r => {}", result.dump());
			return result;
		}

		nlohmann::json MainchainSubWallet::CreateRetrieveCRDepositTransaction(
				const std::string &crPublicKey,
				const std::string &amount,
				const std::string &memo) {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("pubkey: {}", crPublicKey);
			ArgInfo("amount: {}", amount);
			ArgInfo("memo: {}", memo);

			ErrorChecker::CheckBigIntAmount(amount);
			BigInt bgAmount;
			bgAmount.setDec(amount);

			AddressPtr fromAddress(new Address(PrefixDeposit, bytes_t(crPublicKey)));
			ErrorChecker::CheckParam(!fromAddress->Valid(), Error::InvalidArgument, "invalid crPublicKey");
			ErrorChecker::CheckParam(bgAmount <= 0, Error::CreateTransaction, "output amount should big than zero");

			PayloadPtr payload = PayloadPtr(new ReturnDepositCoin());
			TransactionPtr tx = _walletManager->GetWallet()->CreateRetrieveTransaction(
				Transaction::returnCRDepositCoin, payload, bgAmount, fromAddress, memo);

			nlohmann::json result;
			EncodeTx(result, tx);
			ArgInfo("r => {}", result.dump());
			return result;
		}

		nlohmann::json MainchainSubWallet::CreateVoteCRTransaction(
				const std::string &fromAddress,
				const nlohmann::json &votes,
				const std::string &memo,
				const nlohmann::json &invalidCandidates) {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("fromAddr: {}", fromAddress);
			ArgInfo("votes: {}", votes.dump());
			ArgInfo("memo: {}", memo);
			ArgInfo("invalidCandidates: {}", invalidCandidates.dump());

			ErrorChecker::CheckParam(!votes.is_object(), Error::Code::JsonFormatError, "votes is error json format");
			ErrorChecker::CheckJsonArray(invalidCandidates, 0, "invalidCandidates is error json format");

			BigInt bgStake = 0;

			VoteContent voteContent(VoteContent::CRC);
			std::vector<CandidateVotes> candidates;
			std::string key;
			bytes_t candidate;
			BigInt value;
			for (nlohmann::json::const_iterator it = votes.cbegin(); it != votes.cend(); ++it) {
				ErrorChecker::CheckParam(!it.value().is_string(), Error::InvalidArgument, "stake value should be big int string");
				std::string voteAmount = it.value().get<std::string>();
				ErrorChecker::CheckBigIntAmount(voteAmount);

				key = it.key();
				Address cidAddress(key);
				ErrorChecker::CheckParam(!cidAddress.Valid(), Error::InvalidArgument, "invalid candidate cid");
				candidate = cidAddress.ProgramHash().bytes();

				value.setDec(voteAmount);
				ErrorChecker::CheckParam(value <= 0, Error::InvalidArgument, "stake value should larger than 0");

				voteContent.AddCandidate(CandidateVotes(candidate, value));
			}

			VoteContentArray dropedList;
			TransactionPtr tx = CreateVoteTx(voteContent, memo, false, dropedList);

			FilterVoteCandidates(tx, invalidCandidates);

			nlohmann::json result;
			EncodeTx(result, tx);

			std::vector<std::string> dropedTypes;
			for(VoteContentArray::iterator it = dropedList.begin(); it != dropedList.end(); ++it) {
				dropedTypes.push_back((*it).GetTypeString());
			}
			result["DropVotes"] = dropedTypes;

			ArgInfo("r => {}", result.dump());

			return result;
		}

		nlohmann::json MainchainSubWallet::GetVotedCRList() const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());

			WalletPtr wallet = _walletManager->GetWallet();
			UTXOArray utxos = wallet->GetVoteUTXO();
			nlohmann::json j;
			std::map<std::string, BigInt> votedList;

			for (size_t i = 0; i < utxos.size(); ++i) {
				const OutputPtr &output = utxos[i]->Output();
				if (output->GetType() != TransactionOutput::VoteOutput) {
					continue;
				}

				const PayloadVote *pv = dynamic_cast<const PayloadVote *>(output->GetPayload().get());
				if (pv == nullptr) {
					continue;
				}

				const std::vector<VoteContent> &voteContents = pv->GetVoteContent();
				std::for_each(voteContents.cbegin(), voteContents.cend(),
				              [&votedList](const VoteContent &vc) {
					              if (vc.GetType() == VoteContent::Type::CRC) {
						              std::for_each(vc.GetCandidateVotes().cbegin(), vc.GetCandidateVotes().cend(),
						                            [&votedList](const CandidateVotes &cv) {
							                            std::string cid = Address(uint168(cv.GetCandidate())).String();
							                            if (votedList.find(cid) != votedList.end()) {
								                            votedList[cid] += cv.GetVotes();
							                            } else {
								                            votedList[cid] = cv.GetVotes();
							                            }
						                            });
					              }
				              });

			}

			for (std::map<std::string, BigInt>::iterator it = votedList.begin(); it != votedList.end(); ++it)
				j[(*it).first] = (*it).second.getDec();

			ArgInfo("r => {}", j.dump());

			return j;
		}

		nlohmann::json MainchainSubWallet::GetRegisteredCRInfo() const {
			WalletPtr wallet = _walletManager->GetWallet();
			ArgInfo("{} {}", wallet->GetWalletID(), GetFunName());

			nlohmann::json j, info;
			bytes_t types;

			types.push_back(Transaction::registerCR);
			types.push_back(Transaction::unregisterCR);
			types.push_back(Transaction::updateCR);
			types.push_back(Transaction::returnCRDepositCoin);

			std::vector<TransactionPtr> list = _walletManager->GetWallet()->GetTransactions(types);

			j["Status"] = "Unregistered";
			j["Info"] = nlohmann::json();
			for (const TransactionPtr &tx : list) {
				if (tx->GetBlockHeight() == TX_UNCONFIRMED) {
					continue;
				}

				if (tx->GetTransactionType() == Transaction::registerCR ||
				    tx->GetTransactionType() == Transaction::updateCR) {
					const CRInfo *pinfo = dynamic_cast<const CRInfo *>(tx->GetPayload());
					if (pinfo) {
						ByteStream stream(pinfo->GetCode());
						bytes_t pubKey;
						stream.ReadVarBytes(pubKey);
						Address cid(pinfo->GetCID());
						Address did(pinfo->GetDID());
						bool bondedDID = !pinfo->GetDID().bytes().isZero();

						info["CROwnerPublicKey"] = pubKey.getHex();
						info["CID"] = cid.String();
						info["DID"] = bondedDID ? did.String() : "";
						info["BondedDID"] = !did.ProgramHash().bytes().isZero();
						info["NickName"] = pinfo->GetNickName();
						info["URL"] = pinfo->GetUrl();
						info["Location"] = pinfo->GetLocation();
						info["Confirms"] = tx->GetConfirms(wallet->LastBlockHeight());

						j["Status"] = "Registered";
						j["Info"] = info;
					}
				} else if (tx->GetTransactionType() == Transaction::unregisterCR) {
					const UnregisterCR *pc = dynamic_cast<const UnregisterCR *>(tx->GetPayload());
					if (pc) {
						info["Confirms"] = tx->GetConfirms(wallet->LastBlockHeight());

						j["Status"] = "Canceled";
						j["Info"] = info;

					}
				} else if (tx->GetTransactionType() == Transaction::returnCRDepositCoin) {
					info["Confirms"] = tx->GetConfirms(wallet->LastBlockHeight());

					j["Status"] = "ReturnDeposit";
					j["Info"] = info;
				}
			}

			ArgInfo("r => {}", j.dump());
			return j;
		}

		nlohmann::json MainchainSubWallet::GetVoteInfo(const std::string &type) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());

			WalletPtr wallet = _walletManager->GetWallet();
			UTXOArray utxos = wallet->GetVoteUTXO();
			nlohmann::json jinfo;
			time_t timestamp;

			for (UTXOArray::iterator u = utxos.begin(); u != utxos.end(); ++u) {
				const OutputPtr &output = (*u)->Output();
				if (output->GetType() != TransactionOutput::VoteOutput) {
					continue;
				}

				TransactionPtr tx = wallet->TransactionForHash((*u)->Hash());
				assert(tx != nullptr);
				timestamp = tx->GetTimestamp();

				const PayloadVote *pv = dynamic_cast<const PayloadVote *>(output->GetPayload().get());
				if (pv == nullptr) {
					continue;
				}

				const std::vector<VoteContent> &voteContents = pv->GetVoteContent();
				std::for_each(voteContents.cbegin(), voteContents.cend(),
							  [&jinfo, &type, &timestamp](const VoteContent &vc) {
								  nlohmann::json j;
								  if (type.empty() || type == vc.GetTypeString()) {
									  if (vc.GetType() == VoteContent::CRC)
										  j["Amount"] = vc.GetTotalVoteAmount().getDec();
									  else if (vc.GetType() == VoteContent::Delegate)
										  j["Amount"] = vc.GetMaxVoteAmount().getDec();
									  j["Type"] = vc.GetTypeString();
									  j["Timestamp"] = timestamp;
									  j["Expiry"] = nlohmann::json();
									  nlohmann::json candidateVotes;
									  std::for_each(vc.GetCandidateVotes().cbegin(), vc.GetCandidateVotes().cend(),
													[&vc, &candidateVotes](const CandidateVotes &cv) {
														std::string c;
														if (vc.GetType() == VoteContent::CRC) {
															c = Address(uint168(cv.GetCandidate())).String();
														} else {
															c = cv.GetCandidate().getHex();
														}
														candidateVotes[c] = cv.GetVotes().getDec();
													});
									  j["Votes"] = candidateVotes;
									  jinfo.push_back(j);
								  }
							  });

			}

			ArgInfo("r => {}", jinfo.dump());

			return jinfo;
		}

		std::string MainchainSubWallet::ProposalOwnerDigest(const nlohmann::json &payload) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("payload: {}", payload.dump());

			CRCProposal proposal;
			try {
				proposal.FromJsonOwnerUnsigned(payload, CRCProposalDefaultVersion);
			} catch (const std::exception &e) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "convert from json");
			}

			ErrorChecker::CheckParam(!proposal.IsValidOwnerUnsigned(CRCProposalDefaultVersion),
									 Error::InvalidArgument, "invalid payload");

			std::string digest = proposal.DigestOwnerUnsigned(CRCProposalDefaultVersion).GetHex();

			ArgInfo("r => {}", digest);
			return digest;
		}

		std::string MainchainSubWallet::ProposalCRCommitteeDigest(const nlohmann::json &payload) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("payload: {}", payload.dump());

			CRCProposal proposal;
			try {
				proposal.FromJsonCRCommitteeUnsigned(payload, CRCProposalDefaultVersion);
			} catch (const std::exception &e) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "convert from json");
			}

			ErrorChecker::CheckParam(!proposal.IsValidCRCommitteeUnsigned(CRCProposalDefaultVersion),
									 Error::InvalidArgument, "invalid payload");

			std::string digest = proposal.DigestCRCommitteeUnsigned(CRCProposalDefaultVersion).GetHex();

			ArgInfo("r => {}", digest);
			return digest;
		}

		nlohmann::json MainchainSubWallet::CreateProposalTransaction(const nlohmann::json &payload,
																	 const std::string &memo) {
			WalletPtr wallet = _walletManager->GetWallet();
			ArgInfo("{} {}", wallet->GetWalletID(), GetFunName());
			ArgInfo("payload: {}", payload.dump());
			ArgInfo("memo: {}", memo);

			PayloadPtr p = PayloadPtr(new CRCProposal());
			try {
				p->FromJson(payload, 0);
			} catch (const std::exception &e) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "convert from json");
			}

			ErrorChecker::CheckParam(p->IsValid(CRCProposalDefaultVersion), Error::InvalidArgument, "invalid payload");

			AddressPtr receiveAddr = wallet->GetReceiveAddress();
			OutputArray outputs;
			outputs.push_back(OutputPtr(new TransactionOutput(BigInt(0), *receiveAddr)));

			TransactionPtr tx = wallet->CreateTransaction(Transaction::crcProposal, p,
														  nullptr, outputs, memo);

			if (tx->GetOutputs().size() < 2) {
				ErrorChecker::ThrowLogicException(Error::BalanceNotEnough, "balanace not enough");
			}

			tx->RemoveOutput(tx->GetOutputs().front());
			tx->FixIndex();

			nlohmann::json result;
			EncodeTx(result, tx);

			ArgInfo("r => {}", result.dump());
			return result;
		}

		std::string MainchainSubWallet::ProposalReviewDigest(const nlohmann::json &payload) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("payload: {}", payload.dump());

			CRCProposalReview proposalReview;
			try {
				proposalReview.FromJsonUnsigned(payload, CRCProposalReviewDefaultVersion);
			} catch (const std::exception &e) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "convert from json");
			}

			if (!proposalReview.IsValidUnsigned(CRCProposalReviewDefaultVersion)) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "invalid payload");
			}

			std::string digest = proposalReview.DigestUnsigned(CRCProposalReviewDefaultVersion).GetHex();

			ArgInfo("r => {}", digest);
			return digest;
		}

		nlohmann::json MainchainSubWallet::CreateProposalReviewTransaction(const nlohmann::json &payload,
																		   const std::string &memo) {
			WalletPtr wallet = _walletManager->GetWallet();
			ArgInfo("{} {}", wallet->GetWalletID(), GetFunName());
			ArgInfo("payload: {}", payload.dump());
			ArgInfo("memo: {}", memo);

			PayloadPtr p = PayloadPtr(new CRCProposalReview());
			try {
				p->FromJson(payload, CRCProposalReviewDefaultVersion);
			} catch (const std::exception &e) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "convert from json");
			}

			if (!p->IsValid(CRCProposalReviewDefaultVersion))
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "invalid payload");

			ErrorChecker::ThrowParamException(Error::InvalidArgument, "this interface not ready");

			OutputArray outputs;
			AddressPtr receiveAddr = wallet->GetReceiveAddress();
			outputs.push_back(OutputPtr(new TransactionOutput(0, *receiveAddr)));
			AddressPtr fromAddr(new Address(""));

			TransactionPtr tx = wallet->CreateTransaction(Transaction::crcProposalReview, p, fromAddr, outputs, memo);

			if (tx->GetOutputs().size() < 2)
				ErrorChecker::ThrowLogicException(Error::BalanceNotEnough, "balance not enough");

			tx->RemoveOutput(tx->GetOutputs().front());
			tx->FixIndex();

			nlohmann::json result;
			EncodeTx(result, tx);

			ArgInfo("r => {}", result.dump());
			return result;
		}

		nlohmann::json MainchainSubWallet::CreateVoteCRCProposalTransaction(const std::string &fromAddress,
		                                                                    const nlohmann::json &votes,
		                                                                    const std::string &memo,
		                                                                    const nlohmann::json &invalidCandidates) {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("fromAddr: {}", fromAddress);
			ArgInfo("votes: {}", votes.dump());
			ArgInfo("memo: {}", memo);

			ErrorChecker::CheckParam(!votes.is_object(), Error::Code::JsonFormatError, "votes is error json format");
			ErrorChecker::CheckJsonArray(invalidCandidates, 0, "invalidCandidates is error json format");
			BigInt bgStake = 0;

			VoteContent voteContent(VoteContent::CRCProposal);
			std::vector<CandidateVotes> candidates;
			bytes_t candidate;
			BigInt value;
			for (nlohmann::json::const_iterator it = votes.cbegin(); it != votes.cend(); ++it) {
				ErrorChecker::CheckParam(!it.value().is_string(), Error::InvalidArgument, "stake value should be big int string");

				candidate = it.key();
				ErrorChecker::CheckParam(candidate.size() != 32, Error::InvalidArgument, "invalid proposal hash");

				value.setDec(it.value().get<std::string>());
				ErrorChecker::CheckParam(value <= 0, Error::InvalidArgument, "stake value should larger than 0");

				voteContent.AddCandidate(CandidateVotes(candidate, value));
			}

			VoteContentArray dropedList;
			TransactionPtr tx = CreateVoteTx(voteContent, memo, false, dropedList);
			FilterVoteCandidates(tx, invalidCandidates);

			nlohmann::json result;
			EncodeTx(result, tx);

			std::vector<std::string> dropedTypes;
			for(VoteContentArray::iterator it = dropedList.begin(); it != dropedList.end(); ++it) {
				dropedTypes.push_back((*it).GetTypeString());
			}
			result["DropVotes"] = dropedTypes;

			ArgInfo("r => {}", result.dump());

			return result;
		}

		nlohmann::json MainchainSubWallet::CreateImpeachmentCRCTransaction(const std::string &fromAddress,
		                                                                   const nlohmann::json &votes,
		                                                                   const std::string &memo,
		                                                                   const nlohmann::json &invalidCandidates) {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("fromAddr: {}", fromAddress);
			ArgInfo("votes: {}", votes.dump());
			ArgInfo("memo: {}", memo);

			ErrorChecker::CheckParam(!votes.is_object(), Error::Code::JsonFormatError, "votes is error json format");
			ErrorChecker::CheckJsonArray(invalidCandidates, 0, "invalidCandidates is error json format");
			BigInt bgStake = 0;

			VoteContent voteContent(VoteContent::CRCImpeachment);
			std::vector<CandidateVotes> candidates;
			std::string key;
			bytes_t candidate;
			BigInt value;
			for (nlohmann::json::const_iterator it = votes.cbegin(); it != votes.cend(); ++it) {
				ErrorChecker::CheckParam(!it.value().is_string(), Error::InvalidArgument, "stake value should be big int string");
				ErrorChecker::CheckBigIntAmount(it.value().get<std::string>());

				key = it.key();
				Address didAddress(key);
				ErrorChecker::CheckParam(!didAddress.Valid(), Error::InvalidArgument, "invalid candidate did");
				candidate = didAddress.ProgramHash().bytes();

				value.setDec(it.value().get<std::string>());
				ErrorChecker::CheckParam(value <= 0, Error::InvalidArgument, "stake value should larger than 0");

				voteContent.AddCandidate(CandidateVotes(candidate, value));
			}
			VoteContentArray dropedList;
			TransactionPtr tx = CreateVoteTx(voteContent, memo, false, dropedList);
			FilterVoteCandidates(tx, invalidCandidates);

			nlohmann::json result;
			EncodeTx(result, tx);

			std::vector<std::string> dropedTypes;
			for(VoteContentArray::iterator it = dropedList.begin(); it != dropedList.end(); ++it) {
				dropedTypes.push_back((*it).GetTypeString());
			}
			result["DropVotes"] = dropedTypes;

			ArgInfo("r => {}", result.dump());

			return result;
		}

		std::string MainchainSubWallet::ProposalTrackingOwnerDigest(const nlohmann::json &payload) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("payload: {}", payload.dump());

			CRCProposalTracking proposalTracking;
			try {
				proposalTracking.FromJsonOwnerUnsigned(payload, CRCProposalTrackingDefaultVersion);
			} catch (const std::exception &e) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "convert from json");
			}

			if (!proposalTracking.IsValidOwnerUnsigned(CRCProposalTrackingDefaultVersion)) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "invalid payload");
			}
			std::string digest = proposalTracking.DigestOwnerUnsigned(CRCProposalTrackingDefaultVersion).GetHex();

			ArgInfo("r => {}", digest);
			return digest;
		}

		std::string MainchainSubWallet::ProposalTrackingNewOwnerDigest(const nlohmann::json &payload) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("payload: {}", payload.dump());

			CRCProposalTracking proposalTracking;
			try {
				proposalTracking.FromJsonNewOwnerUnsigned(payload, CRCProposalTrackingDefaultVersion);
			} catch (const std::exception &e) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "convert from json");
			}

			if (!proposalTracking.IsValidNewOwnerUnsigned(CRCProposalTrackingDefaultVersion)) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "invalid payload");
			}

			std::string digest = proposalTracking.DigestNewOwnerUnsigned(CRCProposalTrackingDefaultVersion).GetHex();

			ArgInfo("r => {}", digest);
			return digest;
		}

		std::string MainchainSubWallet::ProposalTrackingSecretaryDigest(const nlohmann::json &payload) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("payload: {}", payload.dump());

			CRCProposalTracking proposalTracking;
			try {
				proposalTracking.FromJsonSecretaryUnsigned(payload, CRCProposalTrackingDefaultVersion);
			} catch (const std::exception &e) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "convert from json");
			}

			if (!proposalTracking.IsValidSecretaryUnsigned(CRCProposalTrackingDefaultVersion)) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "invalid payload");
			}

			std::string digest = proposalTracking.DigestSecretaryUnsigned(CRCProposalTrackingDefaultVersion).GetHex();

			ArgInfo("r => {}", digest);
			return digest;
		}

		nlohmann::json
		MainchainSubWallet::CreateProposalTrackingTransaction(const nlohmann::json &payload, const std::string &memo) {
			WalletPtr wallet = _walletManager->GetWallet();
			ArgInfo("{} {}", wallet->GetWalletID(), GetFunName());
			ArgInfo("payload: {}", payload.dump());
			ArgInfo("memo: {}", memo);

			PayloadPtr p(new CRCProposalTracking());

			try {
				p->FromJson(payload, CRCProposalTrackingDefaultVersion);
			} catch (const std::exception &e) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "convert from json");
			}

			if (!p->IsValid(CRCProposalTrackingDefaultVersion)) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "invalid payload");
			}
			OutputArray outputs;
			AddressPtr receiveAddr = wallet->GetReceiveAddress();
			outputs.push_back(OutputPtr(new TransactionOutput(0, *receiveAddr)));
			AddressPtr fromAddr(new Address(""));

			TransactionPtr tx = wallet->CreateTransaction(Transaction::crcProposalTracking, p, fromAddr, outputs, memo);

			if (tx->GetOutputs().size() < 2)
				ErrorChecker::ThrowLogicException(Error::BalanceNotEnough, "balance not enough");

			tx->RemoveOutput(tx->GetOutputs().front());
			tx->FixIndex();

			nlohmann::json result;
			EncodeTx(result, tx);
			ArgInfo("r => {}", result.dump());

			return result;
		}

		std::string MainchainSubWallet::ProposalWithdrawDigest(const nlohmann::json &payload) const {
			ArgInfo("{} {}", _walletManager->GetWallet()->GetWalletID(), GetFunName());
			ArgInfo("payload: {}", payload.dump());

			CRCProposalWithdraw proposalWithdraw;
			try {
				proposalWithdraw.FromJsonUnsigned(payload, CRCProposalWithdrawVersion);
			} catch (const std::exception &e) {
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "convert from json");
			}

			if (!proposalWithdraw.IsValidUnsigned(CRCProposalWithdrawVersion))
				ErrorChecker::ThrowParamException(Error::InvalidArgument, "invalid payload");

			std::string digest = proposalWithdraw.DigestUnsigned(CRCProposalWithdrawVersion).GetHex();

			ArgInfo("r => {}", digest);
			return digest;
		}

		nlohmann::json MainchainSubWallet::CreateProposalWithdrawTransaction(const nlohmann::json &payload,
																			 const std::string &memo) {
			ErrorChecker::ThrowParamException(Error::InvalidArgument, "interface not ready");
			return nlohmann::json();
		}

	}
}
