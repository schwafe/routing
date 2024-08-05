cmake_minimum_required(VERSION 3.5.0)
project(routing VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_executable(${PROJECT_NAME}.app src/main.cpp src/constants.hpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)