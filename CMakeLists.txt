cmake_minimum_required(VERSION 3.12)
project(dsvpn_reimpl)

add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
set(CMAKE_CXX_STANDARD 17)

include_directories(/usr/local/include)
include_directories(/usr/local/Cellar/boost/1.71.0/include)

link_libraries(/usr/local/lib/libyaml-cpp.a)
link_libraries(/usr/local/lib/libspdlog.a)
link_libraries(/usr/local/lib/libsodium.a)
link_libraries(/usr/local/lib/libboost_coroutine.a)
link_libraries(/usr/local/lib/libboost_context.a)
link_libraries(/usr/local/lib/libboost_thread.a)
link_libraries(pthread)

add_executable(dsvpn_reimpl main.cpp tun/TunDevice.cpp tun/TunDevice.h route/Router.cpp route/Router.h utils/Shell.cpp utils/Shell.h utils/Trim.h tun/TunDeviceImpl.h tun/TunDeviceImplOSX.cpp tun/TunDeviceImplLinux.cpp route/RouterImpl.h state/Context.cpp state/Context.h utils/Singleton.h utils/YamlHelper.cpp utils/YamlHelper.h state/Constant.h route/RouteImplOSX.cpp utils/Resolver.h utils/IOWorker.cpp utils/IOWorker.h route/RouteImplLinux.cpp state/ContextHelper.h Client.h connection/Protocol.cpp connection/Protocol.h Server.h connection/AES-256-GCM.h connection/ProtocolHeader.h connection/IConnection.h connection/UDPConnection.cpp connection/UDPConnection.h connection/TCPConnection.cpp connection/TCPConnection.h misc/ip.h utils/RandomNumberGenerator.h utils/NetworkCheck.h Client.cpp utils/SingleApp.cpp utils/SingleApp.h)