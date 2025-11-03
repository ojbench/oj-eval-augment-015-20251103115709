cmake_minimum_required(VERSION 3.10)
project(problem_015_code CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimize for speed and smaller binary
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(code main.cpp)

# Static linking to reduce runtime dependencies (where possible)
# May be ignored on some systems; harmless if not supported
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(code PRIVATE -O3 -pipe -s -DNDEBUG)
endif()

