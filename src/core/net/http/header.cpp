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

#include <core/net/http/header.h>

#include <cctype>

namespace http = core::net::http;

bool http::Header::has(const std::string& key, const std::string& value) const
{
    auto it = fields.find(key);

    if (it == fields.end())
        return false;

    return it->second.count(value) > 0;
}

bool http::Header::has(const std::string& key) const
{
    return fields.count(key) > 0;
}

void http::Header::add(const std::string& key, const std::string& value)
{
    fields[key].insert(value);
}

void http::Header::remove(const std::string& key)
{
    fields.erase(key);
}

void http::Header::remove(const std::string& key, const std::string& value)
{
    auto it = fields.find(key);

    if (it != fields.end())
    {
        it->second.erase(value);
    }
}

void http::Header::set(const std::string& key, const std::string& value)
{
    fields[key] = std::set<std::string>{value};
}

std::string http::Header::canonicalize_key(const std::string& key)
{
    std::string result{key};

    auto it = result.begin();
    auto itE = result.end();

    bool next_should_be_capitalized{false};

    for (; it != itE; ++it)
    {
        if (it == result.begin() or next_should_be_capitalized)
            *it = std::toupper(*it);
        else if (*it == '-')
        {
            next_should_be_capitalized = true;
            continue;
        } else
        {
            *it = std::tolower(*it);
        }

        next_should_be_capitalized = false;
    }

    return result;
}
