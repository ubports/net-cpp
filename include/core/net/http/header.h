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
#ifndef CORE_NET_HTTP_HEADER_H_
#define CORE_NET_HTTP_HEADER_H_

#include <core/net/visibility.h>

#include <map>
#include <memory>
#include <set>
#include <string>

namespace core
{
namespace net
{
namespace http
{
/**
 * @brief The Header class encapsulates the headers of an HTTP request/response.
 */
class CORE_NET_DLL_PUBLIC Header
{
public:
    /**
     * @brief canonicalize_key returns the canonical form of the header key 'key'.
     *
     * The canonicalization converts the first
     * letter and any letter following a hyphen to upper case;
     * the rest are converted to lowercase.  For example, the
     * canonical key for "accept-encoding" is "Accept-Encoding".
     *
     * @param key The key to be canonicalized.
     */
    static std::string canonicalize_key(const std::string& key);

    virtual ~Header() = default;

    /**
     * @brief has checks if the header contains an entry for the given key with the given value.
     * @param key The key into the header map.
     * @param value The value to check for.
     */
    virtual bool has(const std::string& key, const std::string& value) const;

    /**
     * @brief has checks if the header contains any entry for the given key.
     * @param key The key into the header map.
     */
    virtual bool has(const std::string& key) const;

    /**
     * @brief add adds the given value for the given key to the header.
     */
    virtual void add(const std::string& key, const std::string& value);

    /**
     * @brief remove erases all values for the given key from the header.
     */
    virtual void remove(const std::string& key);

    /**
     * @brief remove erases the given value for the given key from the header.
     */
    virtual void remove(const std::string& key, const std::string& value);

    /**
     * @brief set assigns the given value to the entry with key 'key' and replaces any previous value.
     */
    virtual void set(const std::string& key, const std::string& value);

    /**
     * @brief enumerate iterates over the known fields and invokes the given enumerator for each of them.
     */
    virtual void enumerate(const std::function<void(const std::string&, const std::set<std::string>&)>& enumerator) const;

private:
    /// @cond
    std::map<std::string, std::set<std::string>> fields;
    /// @endcond
};
}
}
}

#endif // CORE_NET_HTTP_HEADER_H_
