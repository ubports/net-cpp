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

#include <core/location.h>

#include <sstream>

std::string core::Location::print_with_what(const std::string& what) const
{
    std::stringstream ss;

    ss << file << "@" << line << " - " << function << ": " << what;
    return ss.str();
}

core::Location core::from_here(const std::string& file, const std::string& function, std::size_t line)
{
    core::Location result;

    result.file = file;
    result.function = function;
    result.line = line;

    return result;
}
