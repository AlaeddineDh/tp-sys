cmake_minimum_required(VERSION 3.5.1)
project(tp)

set(CMAKE_C_STANDARD 99)

set(PROD prod.c )
add_executable(prod ${PROD})

set(CONS cons.c)
add_executable(cons ${CONS})
