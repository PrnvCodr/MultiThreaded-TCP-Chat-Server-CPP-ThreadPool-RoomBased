cmake_minimum_required(VERSION 3.15)
project(ChatServer VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7+
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Server sources
set(SERVER_SOURCES
    server.cpp
    sockutil.cpp
    thread_pool.cpp
    iocp_server.cpp
    connection_manager.cpp
    chat_room.cpp
    message_store.cpp
)

# Client sources
set(CLIENT_SOURCES
    client.cpp
    sockutil.cpp
)

# Server executable
add_executable(server ${SERVER_SOURCES})
target_link_libraries(server ws2_32 mswsock)

# Client executable
add_executable(client ${CLIENT_SOURCES})
target_link_libraries(client ws2_32)

# Set output directories
set_target_properties(server client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install targets
install(TARGETS server client DESTINATION bin)
