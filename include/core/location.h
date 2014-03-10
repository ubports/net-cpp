/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */
#ifndef CORE_LOCATION_H_
#define CORE_LOCATION_H_

#include <string>

namespace core
{
struct Location
{
    std::string print_with_what(const std::string& what) const;

    std::string file{};
    std::string function{};
    std::size_t line{};
};

Location from_here(const std::string& file, const std::string& function, std::size_t line);
}

#define CORE_FROM_HERE() core::from_here(__FILE__, __FUNCTION__, __LINE__)

#endif // CORE_LOCATION_H_
