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
        username,
        password,
        host_address,
        path_component,
        query_key,
        query_value,
        fragment
    };

    template<Tag tag>
    struct TaggedString
    {
        TaggedString(const std::string& s) : value(s)
        {
        }

        TaggedString(const char* s) : value(s ? s : "")
        {
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
    class Optional
    {
    private:
        T* value_;
    public:
        Optional() : value_(nullptr)
        {
        }

        Optional(const Optional<T>& rhs) : value_(rhs ? new T(rhs.get()) : nullptr)
        {
        }

        Optional(const T& value) : value_(new T(value))
        {
        }

        ~Optional()
        {
            delete value_;
        }

        Optional<T>& operator=(const Optional<T>& rhs)
        {
            if (rhs)
                return *this = *rhs;

            reset();
            return *this;
        }

        Optional<T>& operator=(const T& rhs)
        {
            set(rhs);
            return *this;
        }

        void set(const T& value)
        {
            if (!value_)
            {
                value_ = new T(value);
                return;
            }

            *value_ = value;
        }

        const T& get() const
        {
            if (!value_)
                throw std::runtime_error("Optional is empty, cannot get value.");

            return *value_;
        }

        void reset()
        {
            delete value_;
            value_ = nullptr;
        }

        const T& operator*() const
        {
            return get();
        }

        operator bool() const
        {
            return value_ != nullptr;
        }

        friend std::ostream& operator<<(std::ostream& out, const Optional<T>& value)
        {
            if (value)
                out << *value;

            return out;
        }
    };

    typedef TaggedString<Tag::scheme> Scheme;

    struct Authority
    {
        struct Host
        {
            typedef TaggedString<Tag::host_address> Address;

            Host(const Address& address) : address(address)
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

        struct Credentials
        {
            typedef TaggedString<Tag::username> Username;
            typedef TaggedString<Tag::password> Password;

            Credentials& set(const Username& username)
            {
                this->username = username;
                return *this;
            }

            Credentials& set(const Password& password)
            {
                this->password = password;
                return *this;
            }

            Username username;
            Password password;
        };

        inline Authority(const Host& host) : host(host)
        {
        }

        inline Authority(const Credentials& credentials, const Host& host)
            : credentials(credentials),
              host(host)
        {
        }

        inline Authority& set(const Credentials& credentials)
        {
            this->credentials = credentials;
            return *this;
        }

        inline Authority& set(const Host& host)
        {
            this->host = host;
            return *this;
        }

        Optional<Credentials> credentials = Optional<Credentials>{};
        Host host;
    };

    struct Path
    {
        typedef TaggedString<Tag::path_component> Component;

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

    struct Query
    {
        typedef TaggedString<Tag::query_key> Key;
        typedef TaggedString<Tag::query_value> Value;

        Query& set(const Key& key)
        {
            this->key = key;
            return *this;
        }

        Query& set(const Value& value)
        {
            this->value = value;
            return *this;
        }

        Key key;
        Value value;
    };

    typedef TaggedString<Tag::fragment> Fragment;

    Uri& set(const Scheme& scheme)
    {
        this->scheme = scheme;
        return *this;
    }

    Uri& set(const Authority& a)
    {
        this->authority = a;
        return *this;
    }

    Uri& set(const Path& path)
    {
        this->path = path;
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
    Optional<Authority> authority;
    Optional<Path> path;
    Optional<Query> query;
    Optional<Fragment> fragment;
};
}
}
#endif // CORE_NET_URI_H_
