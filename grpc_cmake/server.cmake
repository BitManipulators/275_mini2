cmake_minimum_required(VERSION 3.8)



# Proto file
get_filename_component(cm_proto "./proto/collision.proto" ABSOLUTE)
get_filename_component(cm_proto_path "${cm_proto}" PATH)


# Generated sources
set(cm_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/collision.pb.cc")
set(cm_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/collision.pb.h")
set(cm_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/collision.grpc.pb.cc")
set(cm_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/collision.grpc.pb.h")
add_custom_command(
      OUTPUT "${cm_proto_srcs}" "${cm_proto_hdrs}" "${cm_grpc_srcs}" "${cm_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${cm_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${cm_proto}"
      DEPENDS "${cm_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# hw_grpc_proto
add_library(cm_grpc_proto
  ${cm_grpc_srcs}
  ${cm_grpc_hdrs}
  ${cm_proto_srcs}
  ${cm_proto_hdrs})
target_link_libraries(cm_grpc_proto
  absl::check
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# Targets greeter_[async_](client|server)
foreach(_target
  server client
  )
  add_executable(${_target} "${_target}.cpp")
  target_link_libraries(${_target}
    cm_grpc_proto
    absl::check
    absl::flags
    absl::flags_parse
    absl::log
    collision_manager
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
endforeach()