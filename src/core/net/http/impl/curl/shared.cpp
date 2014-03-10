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

#include "shared.h"

#include <curl/curl.h>

namespace curl
{
namespace shared
{
static const curl_lock_data cookies = CURL_LOCK_DATA_COOKIE;
static const curl_lock_data dns = CURL_LOCK_DATA_DNS;
static const curl_lock_data ssl = CURL_LOCK_DATA_SSL_SESSION;

namespace option
{
static const CURLSHoption share = CURLSHOPT_SHARE;
}
}
}

namespace shared = ::curl::shared;

struct shared::Handle::Private
{
    Private() : handle(curl_share_init())
    {
        curl_share_setopt(handle, shared::option::share, shared::cookies);
        curl_share_setopt(handle, shared::option::share, shared::dns);
        curl_share_setopt(handle, shared::option::share, shared::ssl);
    }

    ~Private()
    {
        curl_share_cleanup(handle);
    }

    shared::Native handle;
};

shared::Handle::Handle() : d(new Private())
{
}

shared::Native shared::Handle::native() const
{
    return d->handle;
}
