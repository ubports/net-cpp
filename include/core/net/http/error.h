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
#ifndef CORE_NET_HTTP_ERROR_H_
#define CORE_NET_HTTP_ERROR_H_

#include <core/net/error.h>

namespace core
{
namespace net
{
namespace http
{
class Error : public core::net::Error
{
public:
    explicit Error(const std::string& what, const Location& loc);
    virtual ~Error() = default;
};
}
}
}
#endif // CORE_NET_HTTP_ERROR_H_
