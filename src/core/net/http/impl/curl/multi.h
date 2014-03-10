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
#ifndef CORE_NET_HTTP_IMPL_CURL_MULTI_H_
#define CORE_NET_HTTP_IMPL_CURL_MULTI_H_

#include "easy.h"

namespace curl
{
namespace multi
{
typedef CURLM* Native;

class Handle
{
public:
    Handle();

    void run();
    void stop();

    void add(curl::easy::Handle easy);
    void remove(curl::easy::Handle easy);

    curl::easy::Handle easy_handle_from_native(CURL* native);

    Native native() const;

private:
    struct Private;
    std::shared_ptr<Private> d;
};
}
}
#endif // CORE_NET_HTTP_IMPL_CURL_MULTI_H_
