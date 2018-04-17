package message

import (
	"bytes"
	"crypto/sha256"
	"encoding/binary"

	chain "github.com/elastos/Elastos.ELA/blockchain"
	"github.com/elastos/Elastos.ELA/config"
	"github.com/elastos/Elastos.ELA/log"
	. "github.com/elastos/Elastos.ELA/net/protocol"

	"github.com/elastos/Elastos.ELA.Utility/common/serialize"
)

type ping struct {
	Header
	height uint64
}

func NewPingMsg() ([]byte, error) {
	var msg ping
	msg.Header.Magic = config.Parameters.Magic
	copy(msg.Header.CMD[0:7], "ping")
	msg.height = uint64(chain.DefaultLedger.Store.GetHeight())
	tmpBuffer := bytes.NewBuffer([]byte{})
	serialize.WriteUint64(tmpBuffer, msg.height)
	b := new(bytes.Buffer)
	err := binary.Write(b, binary.LittleEndian, tmpBuffer.Bytes())
	if err != nil {
		log.Error("Binary Write failed at new Msg")
		return nil, err
	}
	s := sha256.Sum256(b.Bytes())
	s2 := s[:]
	s = sha256.Sum256(s2)
	buf := bytes.NewBuffer(s[:4])
	binary.Read(buf, binary.LittleEndian, &(msg.Header.Checksum))
	msg.Header.Length = uint32(len(b.Bytes()))

	m, err := msg.Serialize()
	if err != nil {
		log.Error("Error Convert net message ", err.Error())
		return nil, err
	}
	return m, nil
}

func (msg ping) Handle(node Noder) error {
	node.SetHeight(msg.height)
	buf, err := NewPongMsg()
	if err != nil {
		log.Error("failed build a new ping message")
	} else {
		go node.Tx(buf)
	}
	return err
}

func (msg ping) Serialize() ([]byte, error) {
	hdrBuf, err := msg.Header.Serialize()
	if err != nil {
		return nil, err
	}
	buf := bytes.NewBuffer(hdrBuf)
	err = serialize.WriteUint64(buf, msg.height)
	if err != nil {
		return nil, err
	}
	return buf.Bytes(), err

}

func (msg *ping) Deserialize(p []byte) error {
	buf := bytes.NewBuffer(p)
	err := binary.Read(buf, binary.LittleEndian, &(msg.Header))
	if err != nil {
		return err
	}

	msg.height, err = serialize.ReadUint64(buf)
	return err
}
