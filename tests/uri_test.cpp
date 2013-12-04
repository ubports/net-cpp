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

#include <gtest/gtest.h>

TEST(Uri, default_constructed_uri_yields_empty_string)
{
    core::net::Uri uri;
    EXPECT_EQ(std::string(), uri.as_string());
}

TEST(Uri, parsing_valid_url_from_string_works)
{
    auto uri = core::net::Uri::parse_from_string("http://www.google.com");
}

TEST(Uri, parsing_an_invalid_url_from_string)
{
    EXPECT_ANY_THROW(core::net::Uri::parse_from_string("http//www.google.com"));
}

TEST(Uri, setting_host_yields_correct_string)
{
    using namespace core::net;

    static const std::string google{"www.google.com"};

    auto uri = Uri().set(Uri::Authority{Uri::Authority::Host{google}});
    EXPECT_EQ(google, uri.as_string());
}
