/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: device_feature.proto */

#ifndef PROTOBUF_C_device_5ffeature_2eproto__INCLUDED
#define PROTOBUF_C_device_5ffeature_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1000000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003003 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _DeviceFeature DeviceFeature;


/* --- enums --- */


/* --- messages --- */

struct  _DeviceFeature
{
  ProtobufCMessage base;
  char *device_type;
  protobuf_c_boolean has_voltage_rating;
  int32_t voltage_rating;
  protobuf_c_boolean has_current_rating;
  int32_t current_rating;
  protobuf_c_boolean has_wifi_support;
  protobuf_c_boolean wifi_support;
  protobuf_c_boolean has_ble_support;
  protobuf_c_boolean ble_support;
};
#define DEVICE__FEATURE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&device__feature__descriptor) \
    , NULL, 0, 0, 0, 0, 0, 0, 0, 0 }


/* DeviceFeature methods */
void   device__feature__init
                     (DeviceFeature         *message);
size_t device__feature__get_packed_size
                     (const DeviceFeature   *message);
size_t device__feature__pack
                     (const DeviceFeature   *message,
                      uint8_t             *out);
size_t device__feature__pack_to_buffer
                     (const DeviceFeature   *message,
                      ProtobufCBuffer     *buffer);
DeviceFeature *
       device__feature__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   device__feature__free_unpacked
                     (DeviceFeature *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*DeviceFeature_Closure)
                 (const DeviceFeature *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor device__feature__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_device_5ffeature_2eproto__INCLUDED */
