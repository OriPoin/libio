cmake_minimum_required(VERSION 3.0.0)
project(lib VERSION 0.1.0)

add_library(lib libepoll.hpp libnet.hpp)
set_target_properties(lib PROPERTIES LINKER_LANGUAGE CXX)
