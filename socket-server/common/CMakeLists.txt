add_library(console_support STATIC src/ConsoleExit.cpp)
target_include_directories(console_support PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(console_support PRIVATE project_options)

set(ZPD_PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../contracts/realtime")
add_library(protocol STATIC
    "${ZPD_PROTO_ROOT}/proto/echo.proto"
    "${ZPD_PROTO_ROOT}/proto/session.proto"
    "${ZPD_PROTO_ROOT}/proto/room.proto"
    "${ZPD_PROTO_ROOT}/proto/chat.proto"
    "${ZPD_PROTO_ROOT}/proto/position.proto"
    "${ZPD_PROTO_ROOT}/proto/game.proto"
    "${ZPD_PROTO_ROOT}/proto/matchmaking.proto"
)
target_compile_features(protocol PUBLIC cxx_std_20)
target_link_libraries(protocol PUBLIC protobuf::libprotobuf)
target_include_directories(protocol PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}"
)
protobuf_generate(TARGET protocol LANGUAGE cpp IMPORT_DIRS "${ZPD_PROTO_ROOT}")
