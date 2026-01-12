cmake_minimum_required(VERSION 3.10)

# Project name
project(NetworkExamSystem C)

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Server executable
add_executable(server
    src/server.c
    src/deamon_init.c
    src/server_utils.c
    src/tlv.c
    src/quiz.c
    src/multicast_discovery.c
    src/sock_options.c
)

# Find cJSON library
find_library(CJSON_LIB cjson)
if(NOT CJSON_LIB)
    message(FATAL_ERROR "cJSON library not found. Install with: sudo apt-get install libcjson-dev")
endif()

# Link pthread and cJSON libraries for server
target_link_libraries(server pthread ${CJSON_LIB})

# Client executable
add_executable(client
    src/client.c
    src/client_utils.c
    src/menu.c
    src/tlv.c
    src/multicast_discovery.c
    src/sock_options.c
)

# Link pthread library for client
target_link_libraries(client pthread)

# Install targets
install(TARGETS server client
    RUNTIME DESTINATION bin
)

# --- Symlink executables to project root after build ---
add_custom_command(TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        $<TARGET_FILE:server>
        ${CMAKE_SOURCE_DIR}/server
    COMMENT "Symlinking server to project root"
)
add_custom_command(TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        $<TARGET_FILE:client>
        ${CMAKE_SOURCE_DIR}/client
    COMMENT "Symlinking client to project root"
)
