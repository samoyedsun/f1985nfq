cmake_minimum_required(VERSION 3.10)

project(nfqclient2 VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)

message("SOURCE DIR : ${PROJECT_SOURCE_DIR}")
message("BINARY DIR : ${PROJECT_BINARY_DIR}")

add_definitions("-Wall -g -fPIC")

include_directories(${PROJECT_SOURCE_DIR}/../../boost_1_78_0)
link_directories(${PROJECT_SOURCE_DIR}/../../boost_1_78_0/stage/lib)

add_executable(nfqclient2 main.cpp)

target_link_libraries(nfqclient2 PUBLIC pthread boost_thread)
