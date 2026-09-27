add_library(packet_client STATIC
    src/ClientApplication.cpp
    src/ConsoleInput.cpp
    src/EchoRequestBuilder.cpp
    src/PacketClient.cpp
    src/PacketClientCommands.cpp
    src/PacketClientReceiver.cpp
)
target_include_directories(packet_client PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(packet_client PUBLIC project_options protocol ws2_32)

add_executable(zpd-client src/main.cpp)
target_link_libraries(zpd-client PRIVATE packet_client console_support)

if(BUILD_TESTING)
    find_program(POWERSHELL_EXECUTABLE NAMES pwsh powershell REQUIRED)
    add_test(NAME packet-client-integration
        COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/packet_client_tests.ps1"
            -BinDirectory "$<TARGET_FILE_DIR:zpd-client>")
    set_tests_properties(packet-client-integration PROPERTIES TIMEOUT 60)
endif()
