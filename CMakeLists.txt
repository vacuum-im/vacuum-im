cmake_minimum_required(VERSION 2.8)

project(vacuum-im)

find_package(Qt4 REQUIRED)

add_custom_target(updatets)

add_subdirectory(src)
