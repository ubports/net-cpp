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

#ifndef HTTPBIN_H_
#define HTTPBIN_H_

/**
 *
 * Testing an HTTP Library can become difficult sometimes. Postbin is fantastic
 * for testing POST requests, but not much else. This exists to cover all kinds
 * of HTTP scenarios. Additional endpoints are being considered (e.g.
 * /deflate).
 *
 * All endpoint responses are JSON-encoded.
 *
 */
namespace httpbin
{
const char* host()
{
    return "http://127.0.0.1:5000";
}
namespace resources
{
/** A non-existing resource */
const char* does_not_exist()
{
    return "/does_not_exist";
}
/** Returns Origin IP. */
const char* ip()
{
    return "/ip";
}
/** Returns user-agent. */
const char* user_agent()
{
    return "/user-agent";
}
/** Returns header dict. */
const char* headers()
{
    return "/headers";
}
/** Returns GET data. */
const char* get()
{
    return "/get";
}
/** Returns POST data. */
const char* post()
{
    return "/post";
}
/** Returns PUT data. */
const char* put()
{
    return "/put";
}
/** Challenges basic authentication. */
const char* basic_auth()
{
    return "/basic-auth/user/passwd";
}
/** Challenges digest authentication. */
const char* digest_auth()
{
    return "/digest-auth/auth/user/passwd";
}
}
}

#endif // HTTPBIN_H_
