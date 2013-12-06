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
    auto uri = core::net::Uri::from_string("http://www.google.com");
}

TEST(Uri, parsing_a_url_without_scheme_throws)
{
    EXPECT_ANY_THROW(auto uri = core::net::Uri::from_string("www.google.com"));
}

TEST(Uri, parsing_a_scheme_without_colon_throws)
{
    EXPECT_ANY_THROW(core::net::Uri::from_string("http//www.google.com"));
}

TEST(Uri, parsing_a_raw_ipv4_address_works)
{
    auto uri = core::net::Uri::from_string("http://192.168.0.1");
}

TEST(Uri, parsing_a_path_works)
{
    auto uri = core::net::Uri::from_string("http://192.168.0.1/this/is/a/path");
    EXPECT_TRUE(uri.hierarchical.path);
    EXPECT_EQ(core::net::Uri::Path::Component{"this"}, uri.hierarchical.path.get().components[0]);
    EXPECT_EQ(core::net::Uri::Path::Component{"is"}, uri.hierarchical.path.get().components[1]);
    EXPECT_EQ(core::net::Uri::Path::Component{"a"}, uri.hierarchical.path.get().components[2]);
    EXPECT_EQ(core::net::Uri::Path::Component{"path"}, uri.hierarchical.path.get().components[3]);
}

TEST(Uri, DISABLED_setting_host_yields_correct_string)
{
    using namespace core::net;

    static const std::string google{"www.google.com"};

    auto uri = Uri().set(Uri::Authority{Uri::Authority::Host{google}});
    EXPECT_EQ(google, uri.as_string());
}
