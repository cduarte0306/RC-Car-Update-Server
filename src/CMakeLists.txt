set(CMAKE_CXX_STANDARD 11)

# Add debug flags
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

set(SOURCE  main.cpp
            network_interface/tcp_socket.hpp
            network_interface/tcp_socket.cpp
            updater
            )

add_executable(${PROJECT_NAME} ${SOURCE})

# Add this near the top
target_include_directories(rc-car-updater PRIVATE
    ${CMAKE_SOURCE_DIR}/external/json/include
    ${CMAKE_SOURCE_DIR}/external/json/include/lib
)