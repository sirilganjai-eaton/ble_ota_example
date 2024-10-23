# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Set the path to the protobuf-c library
#set(PROTOBUF_C_DIR $ENV{IDF_PATH}//components//protobuf-c//protobuf-c)

# Include the protobuf-c headers
#include_directories(${PROTOBUF_C_DIR}//protobuf-c/protobuf-c.h)

# Add the protobuf-c source file
#add_library(protobuf-c STATIC ${PROTOBUF_C_DIR}//protobuf-c//protobuf-c.c)



set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/bluetooth/nimble/common/nimble_peripheral_utils
                         $ENV{IDF_PATH}/components/protobuf-c)


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bleprph)
