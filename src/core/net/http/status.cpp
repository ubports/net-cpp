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

#include <core/net/http/status.h>

#include <map>
#include <ostream>
#include <string>

namespace core
{
namespace net
{
namespace http
{
std::ostream& operator<<(std::ostream& out, Status status)
{
    static const std::map<Status, std::string> lut =
    {
        {Status::continue_, "continue_(100)"},
        {Status::switching_protocols, "switching_protocols(101)"},
        {Status::ok, "ok(200)"},
        {Status::created, "created(201)"},
        {Status::accepted, "accepted(202)"},
        {Status::non_authorative_info, "non_authorative_info(203)"},
        {Status::no_content, "no_content(204)"},
        {Status::reset_content, "reset_content(205)"},
        {Status::partial_content, "partial_content(206)"},
        {Status::multiple_choices, "multiple_choices(300)"},
        {Status::moved_permanently, "moved_permanently(301)"},
        {Status::found, "found(302)"},
        {Status::see_other, "see_other(303)"},
        {Status::not_modified, "not_modified(304)"},
        {Status::use_proxy, "use_proxy(305)"},
        {Status::temporary_redirect, "temporary_redirect(307)"},
        {Status::bad_request, "bad_request(400)"},
        {Status::unauthorized, "unauthorized(401)"},
        {Status::payment_required, "payment_required(402)"},
        {Status::forbidden, "forbidden(403)"},
        {Status::not_found, "not_found(404)"},
        {Status::method_not_allowed, "method_not_allowed(405)"},
        {Status::not_acceptable, "not_acceptable(406)"},
        {Status::proxy_auth_required, "proxy_auth_required(407)"},
        {Status::request_timeout, "request_timeout(408)"},
        {Status::conflict, "conflict(409)"},
        {Status::gone, "gone(410)"},
        {Status::length_required, "length_required(411)"},
        {Status::precondition_failed, "precondition_failed(412)"},
        {Status::request_entity_too_large, "request_entity_too_large(413)"},
        {Status::request_uri_too_long, "request_uri_too_long(414)"},
        {Status::unsupported_media_type, "unsupported_media_type(415)"},
        {Status::requested_range_not_satisfiable, "requested_range_not_satisfiable(416)"},
        {Status::expectation_failed, "expectation_failed(417)"},
        {Status::teapot, "teapot(418)"},
        {Status::internal_server_error, "internal_server_error(500)"},
        {Status::not_implemented, "not_implemented(501)"},
        {Status::bad_gateway, "bad_gateway(502)"},
        {Status::service_unavailable, "service_unavailable(503)"},
        {Status::gateway_timeout, "gateway_timeout(504)"},
        {Status::http_version_not_supported, "http_version_not_supported(505)"}
    };

    return out << lut.at(status);
}
}
}
}
