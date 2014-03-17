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

#include <core/net/http/request.h>

#include <map>
#include <ostream>
#include <string>

namespace http = core::net::http;

http::Request::Errors::AlreadyActive::AlreadyActive(const core::Location& loc)
    : http::Error("Request is already active.", loc)
{
}

const http::Request::ProgressHandler& http::Request::Handler::on_progress() const
{
    return progress_handler;
}

http::Request::Handler& http::Request::Handler::on_progress(const http::Request::ProgressHandler& handler)
{
    progress_handler = handler;
    return *this;
}

const http::Request::ResponseHandler& http::Request::Handler::on_response() const
{
    return response_handler;
}

http::Request::Handler& http::Request::Handler::on_response(const http::Request::ResponseHandler& handler)
{
    response_handler = handler;
    return *this;
}

const http::Request::ErrorHandler& http::Request::Handler::on_error() const
{
    return error_handler;
}

http::Request::Handler& http::Request::Handler::on_error(const http::Request::ErrorHandler& handler)
{
    error_handler = handler;
    return *this;
}
