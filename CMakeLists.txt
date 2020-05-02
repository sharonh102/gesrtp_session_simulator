cmake_minimum_required(VERSION 3.6)
project(gesrtp_generator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set (  Boost_INCLUDE_DIRS /usr/local/include/boost/ )

find_package(Boost REQUIRED COMPONENTS thread system)

include_directories (${Boost_INCLUDE_DIRS})

file(GLOB SRCS_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

if(Boost_FOUND)
    message(OK)
endif()

#client:
add_executable(hmi_client gesrtp_hmi_client_side.cpp)
target_link_libraries(hmi_client ${Boost_LIBRARIES})

add_executable(plc_server gesrtp_plc_server_side.cpp)
target_link_libraries(plc_server ${Boost_LIBRARIES})
