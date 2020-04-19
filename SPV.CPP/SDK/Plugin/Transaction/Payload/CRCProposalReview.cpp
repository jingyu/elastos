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
#include "CRCProposalReview.h"
#include <Common/Log.h>
#include <Common/hash.h>

namespace Elastos {
	namespace ElaWallet {

#define JsonKeyProposalHash "ProposalHash"
#define JsonKeyOpinion "Opinion"
#define JsonKeyOpinionHash "OpinionHash"
#define JsonKeyCRCommitteeDID "CRCommitteeDID"
#define JsonKeySignature "Signature"

		CRCProposalReview::CRCProposalReview() {

		}

		CRCProposalReview::~CRCProposalReview() {

		}

		void CRCProposalReview::SetProposalHash(const uint256 &hash) {
			_proposalHash = hash;
		}

		const uint256 &CRCProposalReview::GetProposalHash() const {
			return _proposalHash;
		}

		void CRCProposalReview::SetOpinion(Opinion opinion) {
			_opinion = opinion;
		}

		CRCProposalReview::Opinion CRCProposalReview::GetOpinion() const {
			return _opinion;
		}

		void CRCProposalReview::SetOpinionHash(const uint256 &hash) {
			_opinionHash = hash;
		}

		const uint256 &CRCProposalReview::GetOpinionHash() const {
			return _opinionHash;
		}

		void CRCProposalReview::SetCRCommitteeDID(const Address &crDID) {
			_crCommitteeDID = crDID;
		}

		const Address &CRCProposalReview::GetCRCommitteeDID() const {
			return _crCommitteeDID;
		}

		void CRCProposalReview::SetSignature(const bytes_t &signature) {
			_signature = signature;
		}

		const bytes_t &CRCProposalReview::GetSignature() const {
			return _signature;
		}

		const uint256 &CRCProposalReview::DigestUnsigned(uint8_t version) const {
			if (_digest == 0) {
				ByteStream stream;
				SerializeUnsigned(stream, version);
				_digest = sha256(stream.GetBytes());
			}
			return _digest;
		}

		size_t CRCProposalReview::EstimateSize(uint8_t version) const {
			ByteStream stream;
			size_t size = 0;

			size += _proposalHash.size();
			size += sizeof(uint8_t);
			size += _opinionHash.size();
			size += _crCommitteeDID.ProgramHash().size();
			size += stream.WriteVarUint(_signature.size());
			size += _signature.size();

			return size;
		}

		void CRCProposalReview::SerializeUnsigned(ByteStream &ostream, uint8_t version) const {
			ostream.WriteBytes(_proposalHash);
			ostream.WriteUint8(_opinion);
			ostream.WriteBytes(_opinionHash);
			ostream.WriteBytes(_crCommitteeDID.ProgramHash());
		}

		bool CRCProposalReview::DeserializeUnsigned(const ByteStream &istream, uint8_t version) {
			if (!istream.ReadBytes(_proposalHash)) {
				SPVLOG_ERROR("deserialize proposal hash");
				return false;
			}

			uint8_t opinion = 0;
			if (!istream.ReadUint8(opinion)) {
				SPVLOG_ERROR("deserialize opinion");
				return false;
			}
			_opinion = Opinion(opinion);

			if (!istream.ReadBytes(_opinionHash)) {
				SPVLOG_ERROR("deesrialize opinion hash");
				return false;
			}

			uint168 programHash;
			if (!istream.ReadBytes(programHash)) {
				SPVLOG_ERROR("deserialize did");
				return false;
			}
			_crCommitteeDID = Address(programHash);

			return true;
		}

		void CRCProposalReview::Serialize(ByteStream &ostream, uint8_t version) const {
			SerializeUnsigned(ostream, version);
			ostream.WriteVarBytes(_signature);
		}

		bool CRCProposalReview::Deserialize(const ByteStream &istream, uint8_t version) {
			if (!DeserializeUnsigned(istream, version)) {
				SPVLOG_ERROR("proposal review deserialize unsigned");
				return false;
			}

			if (!istream.ReadVarBytes(_signature)) {
				SPVLOG_ERROR("proposal review deserialize signature");
				return false;
			}

			return true;
		}

		nlohmann::json CRCProposalReview::ToJsonUnsigned(uint8_t version) const {
			nlohmann::json j;
			j[JsonKeyProposalHash] = _proposalHash.GetHex();
			j[JsonKeyOpinion] = _opinion;
			j[JsonKeyOpinionHash] = _opinionHash.GetHex();
			j[JsonKeyCRCommitteeDID] = _crCommitteeDID.String();
			return j;
		}

		void CRCProposalReview::FromJsonUnsigned(const nlohmann::json &j, uint8_t version) {
			_proposalHash.SetHex(j[JsonKeyProposalHash].get<std::string>());
			_opinion = Opinion(j[JsonKeyOpinion].get<uint8_t>());
			_opinionHash.SetHex(j[JsonKeyOpinionHash].get<std::string>());
			_crCommitteeDID = Address(j[JsonKeyCRCommitteeDID].get<std::string>());
		}

		nlohmann::json CRCProposalReview::ToJson(uint8_t version) const {
			nlohmann::json j = ToJsonUnsigned(version);
			j[JsonKeySignature] = _signature.getHex();
			return j;
		}

		void CRCProposalReview::FromJson(const nlohmann::json &j, uint8_t version) {
			FromJsonUnsigned(j, version);
			_signature.setHex(j[JsonKeySignature].get<std::string>());
		}

		bool CRCProposalReview::IsValidUnsigned(uint8_t version) const {
			if (_opinion >= Opinion::unknownOpinion) {
				SPVLOG_ERROR("invalid opinion: {}", _opinion);
				return false;
			}

			if (!_crCommitteeDID.Valid()) {
				SPVLOG_ERROR("invalid committee did");
				return false;
			}

			return true;
		}

		bool CRCProposalReview::IsValid(uint8_t version) const {
			if (IsValidUnsigned(version))
				return false;

			if (_signature.empty()) {
				SPVLOG_ERROR("signature is empty");
				return false;
			}

			return true;
		}

		IPayload &CRCProposalReview::operator=(const IPayload &payload) {
			try {
				const CRCProposalReview &review = dynamic_cast<const CRCProposalReview &>(payload);
				operator=(review);
			} catch (const std::bad_cast &e) {
				SPVLOG_ERROR("payload is not instance of CRCProposalReview");
			}
			return *this;
		}

		CRCProposalReview &CRCProposalReview::operator=(const CRCProposalReview &payload) {
			_proposalHash = payload._proposalHash;
			_opinion = payload._opinion;
			_opinionHash = payload._opinionHash;
			_crCommitteeDID = payload._crCommitteeDID;
			_signature = payload._signature;
			return *this;
		}

	}
}