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

#include <core/net/uri.h>

#include "uri_grammar.h"

core::net::Uri core::net::Uri::parse_from_string(const std::string& uri)
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    core::net::UriGrammar<std::string::const_iterator> grammar;

    auto begin = uri.begin();
    auto end = uri.end();

    if (!qi::phrase_parse(
                uri.begin(),
                uri.end(),
                grammar,
                ascii::space))
    {
        throw std::runtime_error("Problem parsing, remaining string: " + std::string(begin, end));
    }

    return core::net::Uri();
}
