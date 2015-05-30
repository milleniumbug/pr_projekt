cmake_minimum_required(VERSION 3.0.0)
project(BombermanSerwer)
file(GLOB Bomberman_sources
    "*.hpp"
    "*.cpp"
)
add_executable(BombermanSerwer ${Bomberman_sources})

# only version 3.1.0
# set_property(TARGET BombermanSerwer PROPERTY CXX_STANDARD 14)
# set_property(TARGET BombermanSerwer PROPERTY CXX_STANDARD_REQUIRED ON)
# set_property(TARGET BombermanSerwer PROPERTY CXX_EXTENSIONS OFF)

# for older versions. Note - not compatible with every compiler
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")