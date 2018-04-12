package rpc

import (
	"bytes"
	"encoding/hex"

	tx "github.com/elastos/Elastos.ELA.SPV/core/transaction"
)

func (server *Server) NotifyNewAddress(req Req) Resp {
	data, ok := req.Params["address"]
	if !ok {
		return FunctionError("Address parameter not exist")
	}
	switch data := data.(type) {
	case string:
		addr, err := hex.DecodeString(data)
		if err != nil {
			return FunctionError(err.Error())
		}
		err = server.handler.NotifyNewAddress(addr)
		if err != nil {
			return FunctionError(err.Error())
		}
		return Success("New address received")
	default:
		return FunctionError("Invalid data type")
	}
}

func (server *Server) SendTransaction(req Req) Resp {
	data, ok := req.Params["data"]
	if !ok {
		return FunctionError("Data parameter not exist")
	}
	switch data := data.(type) {
	case string:
		txBytes, err := hex.DecodeString(data)
		if err != nil {
			return FunctionError(err.Error())
		}
		var tx tx.Transaction
		err = tx.Deserialize(bytes.NewReader(txBytes))
		if err != nil {
			return FunctionError("Deserialize transaction failed")
		}
		err = server.handler.SendTransaction(tx)
		if err != nil {
			return FunctionError(err.Error())
		}
		return Success(tx.Hash().String())
	default:
		return FunctionError("Invalid data type")
	}
}
