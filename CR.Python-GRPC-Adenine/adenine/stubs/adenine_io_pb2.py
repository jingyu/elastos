# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: adenine_io.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='adenine_io.proto',
  package='adenineio',
  syntax='proto3',
  serialized_options=None,
  serialized_pb=_b('\n\x10\x61\x64\x65nine_io.proto\x12\tadenineio\")\n\x07Request\x12\x0f\n\x07\x61pi_key\x18\x01 \x01(\t\x12\r\n\x05input\x18\x02 \x01(\t\"B\n\x08Response\x12\x0e\n\x06output\x18\x01 \x01(\t\x12\x16\n\x0estatus_message\x18\x02 \x01(\t\x12\x0e\n\x06status\x18\x03 \x01(\x08\x32>\n\tAdenineIo\x12\x31\n\x04Sign\x12\x12.adenineio.Request\x1a\x13.adenineio.Response\"\x00\x62\x06proto3')
)




_REQUEST = _descriptor.Descriptor(
  name='Request',
  full_name='adenineio.Request',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='api_key', full_name='adenineio.Request.api_key', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='input', full_name='adenineio.Request.input', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=31,
  serialized_end=72,
)


_RESPONSE = _descriptor.Descriptor(
  name='Response',
  full_name='adenineio.Response',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='output', full_name='adenineio.Response.output', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='status_message', full_name='adenineio.Response.status_message', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='status', full_name='adenineio.Response.status', index=2,
      number=3, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=74,
  serialized_end=140,
)

DESCRIPTOR.message_types_by_name['Request'] = _REQUEST
DESCRIPTOR.message_types_by_name['Response'] = _RESPONSE
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

Request = _reflection.GeneratedProtocolMessageType('Request', (_message.Message,), {
  'DESCRIPTOR' : _REQUEST,
  '__module__' : 'adenine_io_pb2'
  # @@protoc_insertion_point(class_scope:adenineio.Request)
  })
_sym_db.RegisterMessage(Request)

Response = _reflection.GeneratedProtocolMessageType('Response', (_message.Message,), {
  'DESCRIPTOR' : _RESPONSE,
  '__module__' : 'adenine_io_pb2'
  # @@protoc_insertion_point(class_scope:adenineio.Response)
  })
_sym_db.RegisterMessage(Response)



_ADENINEIO = _descriptor.ServiceDescriptor(
  name='AdenineIo',
  full_name='adenineio.AdenineIo',
  file=DESCRIPTOR,
  index=0,
  serialized_options=None,
  serialized_start=142,
  serialized_end=204,
  methods=[
  _descriptor.MethodDescriptor(
    name='Sign',
    full_name='adenineio.AdenineIo.Sign',
    index=0,
    containing_service=None,
    input_type=_REQUEST,
    output_type=_RESPONSE,
    serialized_options=None,
  ),
])
_sym_db.RegisterServiceDescriptor(_ADENINEIO)

DESCRIPTOR.services_by_name['AdenineIo'] = _ADENINEIO

# @@protoc_insertion_point(module_scope)
