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

namespace http = core::net::http;

std::ostream& http::operator<<(std::ostream& out, http::Status status)
{
    static const std::map<Status, std::string> lut =
    {
        {http::Status::continue_, "continue_(100)"},
        {http::Status::switching_protocols, "switching_protocols(101)"},
        {http::Status::ok, "ok(200)"},
        {http::Status::created, "created(201)"},
        {http::Status::accepted, "accepted(202)"},
        {http::Status::non_authorative_info, "non_authorative_info(203)"},
        {http::Status::no_content, "no_content(204)"},
        {http::Status::reset_content, "reset_content(205)"},
        {http::Status::partial_content, "partial_content(206)"},
        {http::Status::multiple_choices, "multiple_choices(300)"},
        {http::Status::moved_permanently, "moved_permanently(301)"},
        {http::Status::found, "found(302)"},
        {http::Status::see_other, "see_other(303)"},
        {http::Status::not_modified, "not_modified(304)"},
        {http::Status::use_proxy, "use_proxy(305)"},
        {http::Status::temporary_redirect, "temporary_redirect(307)"},
        {http::Status::bad_request, "bad_request(400)"},
        {http::Status::unauthorized, "unauthorized(401)"},
        {http::Status::payment_required, "payment_required(402)"},
        {http::Status::forbidden, "forbidden(403)"},
        {http::Status::not_found, "not_found(404)"},
        {http::Status::method_not_allowed, "method_not_allowed(405)"},
        {http::Status::not_acceptable, "not_acceptable(406)"},
        {http::Status::proxy_auth_required, "proxy_auth_required(407)"},
        {http::Status::request_timeout, "request_timeout(408)"},
        {http::Status::conflict, "conflict(409)"},
        {http::Status::gone, "gone(410)"},
        {http::Status::length_required, "length_required(411)"},
        {http::Status::precondition_failed, "precondition_failed(412)"},
        {http::Status::request_entity_too_large, "request_entity_too_large(413)"},
        {http::Status::request_uri_too_long, "request_uri_too_long(414)"},
        {http::Status::unsupported_media_type, "unsupported_media_type(415)"},
        {http::Status::requested_range_not_satisfiable, "requested_range_not_satisfiable(416)"},
        {http::Status::expectation_failed, "expectation_failed(417)"},
        {http::Status::teapot, "teapot(418)"},
        {http::Status::internal_server_error, "internal_server_error(500)"},
        {http::Status::not_implemented, "not_implemented(501)"},
        {http::Status::bad_gateway, "bad_gateway(502)"},
        {http::Status::service_unavailable, "service_unavailable(503)"},
        {http::Status::gateway_timeout, "gateway_timeout(504)"},
        {http::Status::http_version_not_supported, "http_version_not_supported(505)"}
    };

    return out << lut.at(status);
}
