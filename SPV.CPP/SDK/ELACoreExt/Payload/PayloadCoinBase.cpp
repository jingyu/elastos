// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstring>

#include "PayloadCoinBase.h"
#include "BRInt.h"

namespace Elastos {
	namespace ElaWallet {
		PayloadCoinBase::PayloadCoinBase() {

		}

		PayloadCoinBase::PayloadCoinBase(CMBlock &coinBaseData) {
			_coinBaseData = coinBaseData;
		}

		PayloadCoinBase::~PayloadCoinBase() {
		}

		CMBlock PayloadCoinBase::getData() const {
            return _coinBaseData;
        }

		void PayloadCoinBase::setCoinBaseData(const CMBlock &coinBaseData) {
			_coinBaseData = coinBaseData;
		}

		void PayloadCoinBase::Serialize(ByteStream &ostream) const {
			ostream.putVarUint(_coinBaseData.GetSize());
			if (_coinBaseData.GetSize() > 0) {
				ostream.putBytes(_coinBaseData, _coinBaseData.GetSize());
			}
		}

		bool PayloadCoinBase::Deserialize(ByteStream &istream) {
			uint64_t len = istream.getVarUint();

			if (0 < len) {
				CMBlock data((size_t)len);
				if (data) {
					istream.getBytes((uint8_t *) (void *) data, len);
					_coinBaseData.Resize(size_t(len));
					memcpy(_coinBaseData, data, len);
				}
			}

			return true;
		}

		nlohmann::json PayloadCoinBase::toJson() const {
			char *data = new char[_coinBaseData.GetSize()];
			memcpy(data, _coinBaseData, _coinBaseData.GetSize());
			std::string content(data, _coinBaseData.GetSize());

			nlohmann::json jsonData;
			jsonData["data"] = content;
			return jsonData;
		}

		void PayloadCoinBase::fromJson(const nlohmann::json &jsonData) {
			std::string content = jsonData["data"].get<std::string>();
			const char* data = content.c_str();
			_coinBaseData.Resize(content.size());
			memcpy(_coinBaseData, data, content.size());
		}
	}
}