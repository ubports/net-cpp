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
/**
 * @brief Models a uniform resource identifier as described in http://tools.ietf.org/html/rfc3986#page-23.
 */
class CORE_NET_DLL_PUBLIC Uri
{
public:
    struct Errors
    {
        Errors() = delete;

        struct MalformedUri : public std::runtime_error
        {
            MalformedUri() : std::runtime_error("Malformed Uri")
            {
            }

            // TODO(tvoss): Add more context to the exception in terms of what actually failed during parsing.
        };
    };

    /** @brief Enumerates string-based components of a Uri. */
    enum class Tag
    {
        scheme,
        user_information,
        host_address,
        path_component,
        query,
        fragment
    };

    /** @brief A TaggedString is a string with additional semantics as implied by tag. */
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

    /**
     * @brief Scheme wraps the scheme part of a Uri.
     *
     * Each URI begins with a scheme name that refers to a specification for
     * assigning identifiers within that scheme.  As such, the URI syntax is
     * a federated and extensible naming system wherein each scheme's
     * specification may further restrict the syntax and semantics of
     * identifiers using that scheme.
     */
    typedef TaggedString<Tag::scheme> Scheme;

    /**
     * @brief The Authority struct encapsulates information about an entity owning a namespace.
     *
     * Many URI schemes include a hierarchical element for a naming
     * authority so that governance of the name space defined by the
     * remainder of the URI is delegated to that authority (which may, in
     * turn, delegate it further).  The generic syntax provides a common
     * means for distinguishing an authority based on a registered name or
     * server address, along with optional port and user information.
     */
    struct Authority
    {
        /**
         * The host subcomponent of authority is identified by an IP literal
         * encapsulated within square brackets, an IPv4 address in dotted-
         * decimal form, or a registered name.
         */
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

        /** @brief UserInformation encapsulates username and optionally access credentials.
         *
         * The userinfo subcomponent may consist of a user name and, optionally,
         * scheme-specific information about how to gain authorization to access
         * the resource.
         */
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

    /**
     * The path component contains data, usually organized in hierarchical
     * form, that, along with data in the non-hierarchical query component
     * (Section 3.4), serves to identify a resource within the scope of the
     * URI's scheme and naming authority (if any).
     */
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

    /** @brief Bundles information required to describe access to a specific resource. */
    struct Hierarchical
    {
        Optional<Authority> authority;
        Optional<Path> path;
    };

    /** @brief The optional query portion of a Uri.
     *
     * The query component contains non-hierarchical data that, along with
     * data in the path component (Section 3.3), serves to identify a
     * resource within the scope of the URI's scheme and naming authority
     * (if any).
     */
    typedef TaggedString<Tag::query> Query;

    /** @brief The optional fragment portion of a Uri.
     *
     * The fragment identifier component of a URI allows indirect
     * identification of a secondary resource by reference to a primary
     * resource and additional identifying information.  The identified
     * secondary resource may be some portion or subset of the primary
     * resource, some view on representations of the primary resource, or
     * some other resource defined or described by those representations.
     */
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

    /**
     * @brief Parses a Uri from the given string.
     * @throw Errors::MalformedUri in case of errors.
     * @param uri The uri in string representation.
     * @return An instance of Uri if parsing succeeds.
     */
    static Uri from_string(const std::string& uri);

    Optional<Scheme> scheme;
    Hierarchical hierarchical;
    Optional<Query> query;
    Optional<Fragment> fragment;
};
}
}
#endif // CORE_NET_URI_H_
