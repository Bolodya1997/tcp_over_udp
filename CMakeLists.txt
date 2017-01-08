cmake_minimum_required(VERSION 3.3)
project(tcp_over_udp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
link_libraries(pthread)
include_directories(/home/bolodya/lib/)

set(LIB_SOURCE "/home/bolodya/lib")
set(LIB_FILES ${LIB_SOURCE}/net/socket.cpp
              ${LIB_SOURCE}/net/datagram_socket.cpp
              ${LIB_SOURCE}/net/server_socket.cpp
              ${LIB_SOURCE}/poll/pollable.cpp
              ${LIB_SOURCE}/poll/poller.cpp)

set(D_TCP_FILES d_tcp/d_server_socket.cpp d_tcp/d_socket.cpp d_tcp/d_socket.h)
set(SYSTEM_FILES system/system.cpp
                 system/system_socket.cpp
                 system/unix_socket.h
                 system/system_exception.h
                 system/protocol.h)
set(D_TCP_REALISATION_FILES system/d_tcp_realisation/virtual_socket.cpp
                            system/d_tcp_realisation/connect_exception.h
                            system/d_tcp_realisation/connector.cpp
                            system/d_tcp_realisation/messenger.cpp)
set(D_TCP ${LIB_FILES}
          ${D_TCP_FILES}
          ${SYSTEM_FILES}
          ${D_TCP_REALISATION_FILES})

set(TEST_FILES main.cpp)

set(FILE_TRANSFER_FILES file_transfer/metainfo.cc
                        file_transfer/fd_wrap.h)

add_executable(server ${D_TCP}
                      ${FILE_TRANSFER_FILES}
                      file_transfer/server.cc)
add_executable(client ${D_TCP}
                      ${FILE_TRANSFER_FILES}
                      file_transfer/client.cc)