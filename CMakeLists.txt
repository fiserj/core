# ------------------------------------------------------------------------------
# CMAKE SETUP
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

if(CMAKE_VERSION VERSION_LESS 3.21)
  get_property(PARENT_SOURCE_DIRECTORY DIRECTORY PROPERTY PARENT_DIRECTORY)

  if(NOT PARENT_SOURCE_DIRECTORY)
    set(PROJECT_IS_TOP_LEVEL true)
  endif()
endif()

project("Core")

# ------------------------------------------------------------------------------
# OPTIONS
# ------------------------------------------------------------------------------

option(CORE_BUILD_TESTS "Build core unit tests" ${PROJECT_IS_TOP_LEVEL})

option(CORE_CONFIG_NO_BOUNDS_CHECK "Turn off bounds checking" OFF)

option(CORE_CONFIG_THROW_EXCEPTION_ON_PANIC "Throw exception instead of abort on panic" OFF)

# ------------------------------------------------------------------------------
# LIBRARY
# ------------------------------------------------------------------------------

set(NAME core)

add_library(${NAME} STATIC
  core.cpp
  core.h
)

target_include_directories(${NAME} PUBLIC
  .
)

if(CORE_CONFIG_NO_BOUNDS_CHECK)
  target_compile_definitions(${NAME} PRIVATE
    CORE_CONFIG_NO_BOUNDS_CHECK=1
  )
endif()

if(CORE_CONFIG_THROW_EXCEPTION_ON_PANIC)
  target_compile_definitions(${NAME} PRIVATE
    CORE_CONFIG_THROW_EXCEPTION_ON_PANIC=1
  )
endif()

if(MSVC)
  target_compile_options(${NAME}
    PRIVATE
      /W4
    PUBLIC
      /wd4576
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
)

if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
  set_target_properties(${NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
  )
endif()

# ------------------------------------------------------------------------------
# UNIT TESTS
# ------------------------------------------------------------------------------

if(CORE_BUILD_TESTS)
  include(FetchContent)

  FetchContent_Declare(
    utest
    GIT_REPOSITORY https://github.com/sheredom/utest.h.git
    GIT_TAG        ab62788136a5635e04675322ece84b64d938bd76
  )

  FetchContent_MakeAvailable(utest)

  set(LIB_NAME "${NAME}")
  set(NAME "${NAME}_tests")

  add_executable(${NAME}
    core.cpp
    core.h
    tests.cpp
  )

  target_compile_definitions(${NAME} PRIVATE
    CORE_CONFIG_THROW_EXCEPTION_ON_PANIC=1
  )

  target_include_directories(${NAME} PRIVATE
    ${utest_SOURCE_DIR}
  )

  if(MSVC)
    target_compile_options(${NAME} PRIVATE
      /W4
      /wd4576
    )
  else()
    target_compile_options(${NAME} PRIVATE
      -Wall
      -Wextra
      -fsanitize=address
      -fno-omit-frame-pointer
    )

    target_link_options(${NAME} PRIVATE
      -fsanitize=address
    )
  endif()

  set_target_properties(${NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
    DEBUG_POSTFIX "_d"
  )

  add_custom_command(
    TARGET ${NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${NAME}> "${CMAKE_SOURCE_DIR}/bin/$<TARGET_FILE_NAME:${NAME}>"
  )
endif() # WITH_TESTS
