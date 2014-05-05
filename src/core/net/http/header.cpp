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
    auto it = fields.find(canonicalize_key(key));

    if (it == fields.end())
        return false;

    return it->second.count(value) > 0;
}

bool http::Header::has(const std::string& key) const
{
    return fields.count(canonicalize_key(key)) > 0;
}

void http::Header::add(const std::string& key, const std::string& value)
{
    fields[canonicalize_key(key)].insert(value);
}

void http::Header::remove(const std::string& key)
{
    fields.erase(canonicalize_key(key));
}

void http::Header::remove(const std::string& key, const std::string& value)
{
    auto it = fields.find(canonicalize_key(key));

    if (it != fields.end())
    {
        it->second.erase(value);
    }
}

void http::Header::set(const std::string& key, const std::string& value)
{
    fields[canonicalize_key(key)] = std::set<std::string>{value};
}

std::string http::Header::canonicalize_key(const std::string& key)
{
    std::string result{key};

    bool should_be_capitalized{true};

    for (auto it = result.begin(), itE = result.end(); it != itE; ++it)
    {
        *it = should_be_capitalized ? std::toupper(*it) : std::tolower(*it);
        should_be_capitalized = *it == '-';
    }

    return result;
}

void http::Header::enumerate(const std::function<void(const std::string&, const std::set<std::string>&)>& enumerator) const
{
    for (const auto& pair : fields)
        enumerator(pair.first, pair.second);
}
