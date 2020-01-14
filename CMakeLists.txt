cmake_minimum_required(VERSION 3.15)
project(dsvpn_reimpl)

set(CMAKE_CXX_STANDARD 17)

add_executable(dsvpn_reimpl main.cpp tun/TunDevice.cpp tun/TunDevice.h route/Router.cpp route/Router.h utils/Shell.cpp utils/Shell.h utils/Trim.h tun/TunDeviceImpl.h tun/TunDeviceImplOSX.cpp tun/TunDeviceImplLinux.cpp route/RouterImpl.h)