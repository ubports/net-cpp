# Copyright © 2013 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Thomas Voss <thomas.voss@canonical.com>

find_package(PkgConfig)
find_package(Threads)

pkg_check_modules(JSON_CPP jsoncpp)

# Build with system gmock and embedded gtest
set (GMOCK_INCLUDE_DIR "/usr/include/gmock/include" CACHE PATH "gmock source include directory")
set (GMOCK_SOURCE_DIR "/usr/src/gmock" CACHE PATH "gmock source directory")
set (GTEST_INCLUDE_DIR "${GMOCK_SOURCE_DIR}/gtest/include" CACHE PATH "gtest source include directory")
set (GMOCK_BOTH_LIBRARIES gmock gmock_main)
add_subdirectory(${GMOCK_SOURCE_DIR} "${CMAKE_CURRENT_BINARY_DIR}/gmock")

include_directories(
    ${GMOCK_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIR}
    ${JSON_CPP_INCLUDE_DIRS}
)

add_executable(
  header_test
  header_test.cpp
)

add_executable(
  http_client_test
  http_client_test.cpp
)

add_executable(
  http_client_load_test
  http_client_load_test.cpp
)

add_executable(
  uri_test
  uri_test.cpp
)

target_link_libraries(
    header_test

    net-cpp

    ${GMOCK_BOTH_LIBRARIES}
    ${JSON_CPP_LIBRARIES}
)

target_link_libraries(
    http_client_test

    net-cpp

    ${GMOCK_BOTH_LIBRARIES}
    ${JSON_CPP_LIBRARIES}
)

target_link_libraries(
    http_client_load_test

    net-cpp

    ${GMOCK_BOTH_LIBRARIES}
    ${JSON_CPP_LIBRARIES}
)

target_link_libraries(
    uri_test

    net-cpp

    ${GMOCK_BOTH_LIBRARIES}
)

add_test(header_test ${CMAKE_CURRENT_BINARY_DIR}/header_test)
add_test(http_client_test ${CMAKE_CURRENT_BINARY_DIR}/http_client_test)
add_test(http_client_load_test ${CMAKE_CURRENT_BINARY_DIR}/http_client_test)
add_test(uri_test ${CMAKE_CURRENT_BINARY_DIR}/uri_test)
