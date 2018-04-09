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

execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/httpbin.tar.bz2
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/httpbin.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/httpbin.h
    @ONLY
)

find_package(PkgConfig)
find_package(Threads)

pkg_check_modules(JSON_CPP jsoncpp)
pkg_check_modules(PROCESS_CPP process-cpp)

find_package(GMock)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}

    ${GMOCK_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIR}
    ${JSON_CPP_INCLUDE_DIRS}
    ${PROCESS_CPP_INCLUDE_DIRS}
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
  http_streaming_client_test
  http_streaming_client_test.cpp
)

add_executable(
  http_client_load_test
  http_client_load_test.cpp
)

target_link_libraries(
    header_test

    net-cpp

    ${GMOCK_LIBRARIES}
    ${JSON_CPP_LDFLAGS}
    ${PROCESS_CPP_LDFLAGS}
)

target_link_libraries(
    http_client_test

    net-cpp

    ${GMOCK_LIBRARIES}
    ${JSON_CPP_LDFLAGS}
    ${PROCESS_CPP_LDFLAGS}
)

target_link_libraries(
    http_streaming_client_test

    net-cpp

    ${GMOCK_LIBRARIES}
    ${JSON_CPP_LDFLAGS}
    ${PROCESS_CPP_LDFLAGS}
)

target_link_libraries(
    http_client_load_test

    net-cpp

    ${GMOCK_LIBRARIES}
    ${JSON_CPP_LDFLAGS}
    ${PROCESS_CPP_LDFLAGS}
)

add_test(header_test ${CMAKE_CURRENT_BINARY_DIR}/header_test)
add_test(http_client_test ${CMAKE_CURRENT_BINARY_DIR}/http_client_test)
add_test(http_streaming_client_test ${CMAKE_CURRENT_BINARY_DIR}/http_streaming_client_test)
add_test(http_client_load_test ${CMAKE_CURRENT_BINARY_DIR}/http_client_load_test)
