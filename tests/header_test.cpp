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

#include <gtest/gtest.h>

namespace http = core::net::http;

TEST(Header, canonicalizing_empty_string_does_not_throw)
{
    std::string key{};

    EXPECT_TRUE(key.empty());

    auto result = http::Header::canonicalize_key(key);

    EXPECT_TRUE(result.empty());
}

TEST(Header, canonicalizing_a_valid_key_works)
{
    std::string key{"accept-encoding"};

    auto result = http::Header::canonicalize_key(key);

    EXPECT_EQ("Accept-Encoding", result);
}

TEST(Header, canonicalizing_is_idempotent)
{
    std::string key{"Accept-Encoding"};

    auto result = http::Header::canonicalize_key(key);

    EXPECT_EQ("Accept-Encoding", result);
}

TEST(Header, canonicalizing_corrects_random_capitalization)
{
    std::string key{"aCcEpT-eNcOdInG"};

    auto result = http::Header::canonicalize_key(key);

    EXPECT_EQ("Accept-Encoding", result);
}

TEST(Header, adding_values_works_correctly)
{
    http::Header header;
    header.add("Accept-Encoding", "utf8");
    EXPECT_TRUE(header.has("Accept-Encoding"));
    EXPECT_TRUE(header.has("Accept-Encoding", "utf8"));
    header.add("Accept-Encoding", "utf16");
    EXPECT_TRUE(header.has("Accept-Encoding", "utf8"));
    EXPECT_TRUE(header.has("Accept-Encoding", "utf16"));
}

TEST(Header, removing_values_works_correctly)
{
    http::Header header;
    header.add("Accept-Encoding", "utf8");
    EXPECT_TRUE(header.has("Accept-Encoding"));
    EXPECT_TRUE(header.has("Accept-Encoding", "utf8"));
    header.remove("Accept-Encoding", "utf8");
    EXPECT_TRUE(header.has("Accept-Encoding"));
    EXPECT_FALSE(header.has("Accept-Encoding", "utf8"));
    header.remove("Accept-Encoding");
    EXPECT_FALSE(header.has("Accept-Encoding"));
}

TEST(Header, setting_values_works_correctly)
{
    http::Header header;
    header.add("Accept-Encoding", "utf8");

    EXPECT_TRUE(header.has("Accept-Encoding"));
    EXPECT_TRUE(header.has("Accept-Encoding", "utf8"));

    header.set("Accept-Encoding", "utf16");
    EXPECT_TRUE(header.has("Accept-Encoding"));
    EXPECT_FALSE(header.has("Accept-Encoding", "utf8"));
    EXPECT_TRUE(header.has("Accept-Encoding", "utf16"));
}
