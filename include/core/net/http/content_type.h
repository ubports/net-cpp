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
#ifndef CORE_NET_HTTP_CONTENT_TYPE_H_
#define CORE_NET_HTTP_CONTENT_TYPE_H_

#include <core/net/visibility.h>

namespace core
{
namespace net
{
namespace http
{
class CORE_NET_DLL_PUBLIC ContentType
{
public:
    inline ContentType(const std::string& value) : value(value)
    {
    }

    const std::string& as_string() const
    {
        return value;
    }

private:
    std::string value;

public:
    static const ContentType x_www_form_urlencoded;
    static const ContentType json;
    static const ContentType xml;
    static const ContentType html;
};

const ContentType ContentType::json{"application/json"};
}
}
}

#endif // CORE_NET_HTTP_CONTENT_TYPE_H_
