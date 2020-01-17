cmake_minimum_required(VERSION 3.15)
project(dsvpn_reimpl)

set(CMAKE_CXX_STANDARD 17)

include_directories(/usr/local/include)
include_directories(/usr/local/Cellar/boost/1.71.0/include)

link_libraries(/usr/local/lib/libyaml-cpp.a)
link_libraries(/usr/local/Cellar/boost/1.71.0/lib/libboost_coroutine-mt.a)
link_libraries(/usr/local/Cellar/boost/1.71.0/lib/libboost_context-mt.a)
link_libraries(/usr/local/Cellar/boost/1.71.0/lib/libboost_thread-mt.a)
add_executable(dsvpn_reimpl main.cpp tun/TunDevice.cpp tun/TunDevice.h route/Router.cpp route/Router.h utils/Shell.cpp utils/Shell.h utils/Trim.h tun/TunDeviceImpl.h tun/TunDeviceImplOSX.cpp tun/TunDeviceImplLinux.cpp route/RouterImpl.h state/Context.cpp state/Context.h utils/Singleton.h utils/YamlHelper.cpp utils/YamlHelper.h state/Constant.h route/RouteImplOSX.cpp utils/Resolver.h)