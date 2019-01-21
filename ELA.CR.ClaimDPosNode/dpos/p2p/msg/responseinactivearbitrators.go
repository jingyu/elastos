package msg

import (
	"io"

	"github.com/elastos/Elastos.ELA/common"
	"github.com/elastos/Elastos.ELA/core/types"
	"github.com/elastos/Elastos.ELA/crypto"
)

const ResponseInactiveArbitratorsLength = 32 + 33 + 64

type ResponseInactiveArbitrators struct {
	Tx     types.Transaction
	Signer []byte
	Sign   []byte
}

func (i *ResponseInactiveArbitrators) CMD() string {
	return CmdResponseInactiveArbitrators
}

func (i *ResponseInactiveArbitrators) MaxLength() uint32 {
	return ResponseInactiveArbitratorsLength
}

func (i *ResponseInactiveArbitrators) Serialize(w io.Writer) error {
	if err := i.SerializeUnsigned(w); err != nil {
		return err
	}

	if err := common.WriteVarBytes(w, i.Sign); err != nil {
		return err
	}
	return nil
}

func (i *ResponseInactiveArbitrators) SerializeUnsigned(w io.Writer) error {
	if err := i.Tx.Serialize(w); err != nil {
		return err
	}

	if err := common.WriteVarBytes(w, i.Signer); err != nil {
		return err
	}
	return nil
}

func (i *ResponseInactiveArbitrators) Deserialize(r io.Reader) (err error) {
	if err = i.DeserializeUnsigned(r); err != nil {
		return err
	}

	if i.Sign, err = common.ReadVarBytes(r, crypto.SignatureLength, "sign data"); err != nil {
		return err
	}
	return err
}

func (i *ResponseInactiveArbitrators) DeserializeUnsigned(r io.Reader) (err error) {
	if err = i.Tx.Deserialize(r); err != nil {
		return err
	}

	if i.Signer, err = common.ReadVarBytes(r, crypto.NegativeBigLength, "public key"); err != nil {
		return err
	}
	return err
}
