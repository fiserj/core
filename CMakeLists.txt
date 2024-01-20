cmake_minimum_required(VERSION 3.16)

project("Core")

set(NAME core)

add_executable(${NAME}
  core.cpp
  core.h
  test.cpp
)

if(MSVC)
  target_compile_options(${NAME} PRIVATE
    /W4
  )
else()
  target_compile_options(${NAME} PRIVATE
    -Wall
    -Wextra
  )
endif()

set_target_properties(${NAME} PROPERTIES
  CXX_STANDARD 20
  CXX_EXTENSIONS OFF
  CXX_STANDARD_REQUIRED ON
  DEBUG_POSTFIX "_d"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
  set_target_properties(${NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
  )
endif()
