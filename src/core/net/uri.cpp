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

core::net::Uri core::net::Uri::from_string(const std::string& uri)
{
    namespace ascii = boost::spirit::ascii;

    core::net::UriGrammar<std::string::const_iterator> grammar;

    auto begin = uri.begin();
    auto end = uri.end();

    try
    {
        core::net::Uri uri;
        if (!boost::spirit::qi::phrase_parse(
                    begin,
                    end,
                    grammar,
                    ascii::space,
                    uri
                    ))
        {
            throw core::net::Uri::Errors::MalformedUri{};
        }

        return uri;
    } catch(const boost::spirit::qi::expectation_failure<std::string::const_iterator>& f)
    {
        std::stringstream ss;
        ss << "An expectation of the parser has been violated:";
        ss << "\t" << f.what_;
        throw core::net::Uri::Errors::MalformedUri{};
    }

    // Satisfying gcc's requirements here.
    return core::net::Uri();
}
