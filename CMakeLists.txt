cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Must remove -Fno-rtti from /esp-idf/tools/cmake/idf_functions.cmake


add_definitions(-DREFKEEN_VER_KDREAMS)
add_definitions(-DREFKEEN_VER_KDREAMS_ANYEGA_ALL)
add_definitions(-DGRMODE=EGAGR)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(cgenius)

