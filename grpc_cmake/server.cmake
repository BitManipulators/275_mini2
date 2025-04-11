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

#------------------------------------------------------------

find_package(Python REQUIRED COMPONENTS Interpreter)

# Function to check if a Python package is installed
function(check_python_package package_name result_var)
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import ${package_name}"
        RESULT_VARIABLE exit_code
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(exit_code EQUAL 0)
        set(${result_var} TRUE PARENT_SCOPE)
    else()
        set(${result_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Check for grpcio and grpcio-tools
check_python_package(grpc GRPCIO_FOUND)
check_python_package(grpc_tools GRPCIO_TOOLS_FOUND)

if(GRPCIO_FOUND)
    message(STATUS "Found grpcio")
else()
    message(WARNING "grpcio not found")
endif()

if(GRPCIO_TOOLS_FOUND)
    message(STATUS "Found grpcio-tools")
else()
    message(WARNING "grpcio-tools not found")
endif()

if(NOT GRPCIO_FOUND OR NOT GRPCIO_TOOLS_FOUND)
    message(FATAL_ERROR "Required Python packages not found. Please install grpcio and grpcio-tools.")
endif()

#------------------------------------------------------------

# Define the output Python files in the build directory
set(GENERATED_PYTHON_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/collision_pb2.py"
    "${CMAKE_CURRENT_BINARY_DIR}/collision_pb2_grpc.py"
)

# Add a custom command to generate Python gRPC files using python3
add_custom_command(
    OUTPUT ${GENERATED_PYTHON_FILES}
    COMMAND ${CMAKE_COMMAND} -E env python3 -m grpc_tools.protoc
            -I ${cm_proto_path}
            --python_out=${CMAKE_CURRENT_BINARY_DIR}
            --grpc_python_out=${CMAKE_CURRENT_BINARY_DIR}
            ${cm_proto}
    DEPENDS ${cm_proto}
)

# Create a custom target that depends on the generated files
add_custom_target(generate_python_grpc ALL
    DEPENDS ${GENERATED_PYTHON_FILES}
)

#------------------------------------------------------------

# Copy the Python client script to the binary directory.
configure_file(${CMAKE_SOURCE_DIR}/client.py ${CMAKE_CURRENT_BINARY_DIR}/client.py COPYONLY)

# Mark the copied script as executable (only works on UNIX-like systems).
if(UNIX)
  execute_process(
    COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/client.py
  )
endif()

# Create a custom target for the Python client.
add_custom_target(python_client ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/client.py
  COMMENT "Running Python client"
)

# Optionally, if you want both executables to be in a unified list, you can list them:
set(CLIENT_TARGETS python_client)

add_library(
    collision_proto_converters
    collision_proto_converter.cpp
    query_proto_converter.cpp
)
target_link_libraries(
    collision_proto_converters
    cm_grpc_proto
)

add_library(
    collision_query_service_impl
    collision_query_service_impl.cpp
)
target_link_libraries(
    collision_query_service_impl
    collision_proto_converters
    cm_grpc_proto
)

add_library(
    shared_memory_manager
    shared_memory/bakery_mutex.cpp
    shared_memory/shared_memory.cpp
    shared_memory/shared_memory_manager.cpp
)

# Targets greeter_[async_](client|server)
foreach(_target
  async_server async_shm_server sync_server client
  )
  add_executable(${_target} "${_target}.cpp")
  target_link_libraries(${_target}
    yaml_parser
    absl::check
    absl::flags
    absl::flags_parse
    absl::log
    collision_proto_converters
    collision_query_service_impl
    collision_manager
    shared_memory_manager
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
endforeach()
