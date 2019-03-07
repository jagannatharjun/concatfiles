# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("concatfiles")

# Include sub-projects.
add_subdirectory ("concatfiles")

set(GOOGLE_TEST_SRC_DIR "E:/Cpp/Sources/googletest-src")

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${GOOGLE_TEST_SRC_DIR}
                 ${CMAKE_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories("${gtest_SOURCE_DIR}/include")
endif()

add_executable(basic_tests tests/basic_tests.cpp)
target_link_libraries(basic_tests gtest gtest_main concatfiles)
add_test(basic_tests basic_tests)
