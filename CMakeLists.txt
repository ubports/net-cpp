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

cmake_minimum_required(VERSION 2.8)

project(net-cpp)

find_package(Threads)

include(GNUInstallDirs)

    set(NET_CPP_VERSION_MAJOR 3)
    set(NET_CPP_VERSION_MINOR 0)
    set(NET_CPP_VERSION_PATCH 0)

if (NOT DEFINED NET_CPP_SOVERSION)
    set(NET_CPP_SOVERSION 4)
endif()
 
message(STATUS "${CMAKE_PROJECT_NAME} ${NET_CPP_VERSION_MAJOR}.${NET_CPP_VERSION_MINOR}.${NET_CPP_VERSION_PATCH}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -pedantic -Wextra -fvisibility=hidden")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror -Wall -fno-strict-aliasing -fvisibility=hidden -fvisibility-inlines-hidden -pedantic -Wextra")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)

include(CTest)

include_directories(
  include/
)

file(GLOB_RECURSE NET_CPP_INTERFACE_HEADERS include/*.h)

add_subdirectory(doc)
add_subdirectory(data)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(tests)
