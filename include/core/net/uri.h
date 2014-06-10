/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Pete Woods <pete.woods@canonical.com>
 */

#ifndef CORE_NET_URI_H_
#define CORE_NET_URI_H_

#include <string>
#include <vector>

#include <core/net/visibility.h>

namespace core
{
namespace net
{

struct Uri
{
    typedef std::string Base;

    typedef std::vector<std::string> Endpoint;

    typedef std::vector<std::pair<std::string, std::string>> Parameters;

    Base base;

    Endpoint endpoint;

    Parameters parameters;
};

/**
 * @brief Build a URI from its components
 */
CORE_NET_DLL_PUBLIC
Uri make_uri (const Uri::Base& base, const Uri::Endpoint& endpoint = Uri::Endpoint(),
           const Uri::Parameters& parameters = Uri::Parameters());

}
}

#endif // CORE_NET_URI_H_
