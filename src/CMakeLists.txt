set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

set(SOURCE
  main.cpp
  network_interface/tcp_socket.cpp
  updater.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE})

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/external/json/include
  ${CMAKE_SOURCE_DIR}/external/json/include/lib
)

target_link_libraries(${PROJECT_NAME}
  SWUpdate::Client
  ${Boost_LIBRARIES}
  pthread            # usually pulled transitively, but harmless to add
)
