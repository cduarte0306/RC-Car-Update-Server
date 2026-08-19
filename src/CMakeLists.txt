set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

set(LIB_SOURCE
  lib/MessageLib.cpp
)

set(SOURCE
  main.cpp
  network_interface/TcpServer.cpp
  network_interface/TcpServer.hpp
  network_interface/Proxy.hpp
  network_interface/Proxy.cpp
  updater.cpp
  utils/logger.hpp
  utils/logger.cpp
)

include(FetchContent)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.2
)

FetchContent_MakeAvailable(json)



add_executable(${PROJECT_NAME} ${SOURCE})

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src/
)

find_package(nlohmann_json REQUIRED)
target_link_libraries(${PROJECT_NAME}
  SWUpdate::Client
  nlohmann_json::nlohmann_json
  systemd
  ${Boost_LIBRARIES}
  pthread            # usually pulled transitively, but harmless to add
)
