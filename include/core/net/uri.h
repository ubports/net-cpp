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
#ifndef CORE_NET_URI_H_
#define CORE_NET_URI_H_

#include <core/net/visibility.h>

#include <boost/optional.hpp>

#include <cstdint>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace core
{
namespace net
{
class CORE_NET_DLL_PUBLIC Uri
{
public:
    enum class Tag
    {
        scheme,
        user_information,
        host_address,
        path_component,
        query,
        fragment
    };

    template<Tag tag>
    struct TaggedString
    {
        TaggedString(const std::string& s = std::string()) : value(s)
        {
        }

        TaggedString(const char& s) : value({s})
        {
        }

        TaggedString(const char* s) : value(s ? s : "")
        {
        }

        bool operator==(const TaggedString<tag>& rhs) const
        {
            return value == rhs.value;
        }

        operator const std::string&() const
        {
            return value;
        }

        operator std::string&()
        {
            return value;
        }

        friend std::ostream& operator<<(std::ostream& out, const TaggedString<tag>& string)
        {
            return out << string.value;
        }

        std::string value;
    };

    template<typename T>
    using Optional = boost::optional<T>;

    typedef TaggedString<Tag::scheme> Scheme;

    struct Authority
    {
        struct Host
        {
            typedef TaggedString<Tag::host_address> Address;

            Host(const Address& address = Address()) : address(address)
            {
            }

            Host& set(const Address& address)
            {
                this->address = address;
                return *this;
            }

            Host& set(std::uint16_t port)
            {
                this->port = port;
                return *this;
            }

            Address address;
            Optional<std::uint16_t> port = Optional<std::uint16_t>{};
        };

        typedef TaggedString<Tag::user_information> UserInformation;

        inline Authority(const Host& host = Host()) : host(host)
        {
        }

        inline Authority(const UserInformation& user_info, const Host& host)
            : user_info(user_info),
              host(host)
        {
        }

        inline Authority& set(const UserInformation& user_info)
        {
            this->user_info = user_info;
            return *this;
        }

        inline Authority& set(const Host& host)
        {
            this->host = host;
            return *this;
        }

        Optional<UserInformation> user_info = Optional<UserInformation>{};
        Host host;
    };

    struct Path
    {
        typedef TaggedString<Tag::path_component> Component;        

        Path() = default;

        Path(const std::string& component) : components(1, component)
        {
        }

        Path& set(const Component& component)
        {
            components = {component};
            return *this;
        }

        Path& add(const Component& component)
        {
            components.push_back(component);
            return *this;
        }

        std::vector<Component> components;
    };

    struct Hierarchical
    {
        Optional<Authority> authority;
        Optional<Path> path;
    };

    typedef TaggedString<Tag::query> Query;
    typedef TaggedString<Tag::fragment> Fragment;

    Uri& set(const Scheme& scheme)
    {
        this->scheme = scheme;
        return *this;
    }

    Uri& set(const Authority& a)
    {
        this->hierarchical.authority = a;
        return *this;
    }

    Uri& set(const Path& path)
    {
        this->hierarchical.path = path;
        return *this;
    }

    Uri& set(const Query& query)
    {
        this->query = query;
        return *this;
    }

    Uri& set(const Fragment& fragment)
    {
        this->fragment = fragment;
        return *this;
    }

    std::string as_string() const
    {
        std::stringstream ss;
        if (scheme)
            ss << scheme << "://";

        return std::string();
    }

    static Uri parse_from_string(const std::string& uri);

    Optional<Scheme> scheme;
    Hierarchical hierarchical;
    Optional<Query> query;
    Optional<Fragment> fragment;
};
}
}
#endif // CORE_NET_URI_H_
