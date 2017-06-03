cmake_minimum_required(VERSION 3.7)
project(siktacka)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3")

include_directories(".")

set(SERVER_SOURCE_FILES server/main.cpp common/network/HostAddress.cpp common/network/HostAddress.hpp common/utils.hpp common/utils.cpp common/utils_impl.hpp common/network/Socket.cpp common/network/Socket.hpp common/network/UdpSocket.cpp common/network/UdpSocket.hpp common/network/TcpSocket.cpp common/network/TcpSocket.hpp)
add_executable(siktacka-server ${SERVER_SOURCE_FILES})

set(CLIENT_SOURCE_FILES client/main.cpp common/network/HostAddress.cpp common/network/HostAddress.hpp client/Client.cpp client/Client.hpp common/utils.hpp common/utils.cpp common/utils_impl.hpp common/network/Socket.cpp common/network/Socket.hpp common/network/UdpSocket.cpp common/network/UdpSocket.hpp common/network/TcpSocket.cpp common/network/TcpSocket.hpp)
add_executable(siktacka-client ${CLIENT_SOURCE_FILES})

add_custom_target(siktacka)
add_dependencies(siktacka siktacka-server siktacka-client)
