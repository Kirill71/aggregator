cmake_minimum_required(VERSION 3.29)
project(aggregator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

set(SRC
    aggregator.h
    request.h
    aggregator.cpp
    main.cpp
)

add_executable(aggregator ${SRC})
