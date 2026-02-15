cmake_minimum_required(VERSION 3.13)
project(MinimalGrpcClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖（Arch Linux 已验证）
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(absl REQUIRED)

# Proto 文件名
set(PROTO_FILE grpcdata.proto)

# 输出目录
set(PROTO_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_GENERATED_DIR})

# ---------- 1. 生成 protobuf 消息代码（.pb.cc）----------
protobuf_generate(
    LANGUAGE cpp
    PROTOS ${PROTO_FILE}
    OUT_VAR PROTO_SRCS
    PROTOC_OUT_DIR ${PROTO_GENERATED_DIR}
)

# ---------- 2. 生成 gRPC 服务代码（.grpc.pb.cc）----------
protobuf_generate(
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc   # ⚠️ 关键！必须显式指定扩展名
    PROTOS ${PROTO_FILE}
    OUT_VAR GRPC_SRCS
    PROTOC_OUT_DIR ${PROTO_GENERATED_DIR}
    PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
)

# ---------- 3. 可执行文件 ----------
add_executable(minimal_client
    client.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_include_directories(minimal_client PRIVATE ${PROTO_GENERATED_DIR})

# ---------- 4. 链接库 ----------
target_link_libraries(minimal_client
    gRPC::grpc++
    protobuf::libprotobuf
    absl::log_internal_check_op   # 必须，否则 DSO missing
    absl::strings
    absl::synchronization
    absl::time
    absl::cord
)