#include <core/net/uri.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/request.h>
#include <core/net/http/response.h>

#include <gtest/gtest.h>

#include <json/json.h>

namespace http = core::net::http;
namespace json = Json;
namespace net = core::net;

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
    return "http://httpbin.org";
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
}
}

TEST(HttpClient, head_request_for_existing_resource_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host()) + httpbin::resources::get();

    // The client mostly acts as a factory for http requests.
    auto request = client->head(url);

    // We finally execute the query synchronously and story the response.
    auto response = request->execute();

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
}

TEST(HttpClient, get_request_for_existing_resource_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host()) + httpbin::resources::get();

    // The client mostly acts as a factory for http requests.
    auto request = client->get(url);

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute();

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // The url field of the payload should equal the original url we requested.
    EXPECT_EQ(url, root["url"].asString());
}

TEST(HttpClient, post_request_for_existing_resource_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host()) + httpbin::resources::post();

    std::string payload = "{ 'test': 'test' }";

    // The client mostly acts as a factory for http requests.
    auto request = client->post(url, payload, core::net::http::ContentType::json);

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute();

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // The url field of the payload should equal the original url we requested.
    EXPECT_EQ(payload, root["data"].asString());
}

TEST(HttpClient, put_request_for_existing_resource_succeeds)
{
    auto client = http::make_client();
    auto url = std::string(httpbin::host()) + httpbin::resources::put();

    const std::string value{"{ 'test': 'test' }"};
    std::stringstream payload(value);

    auto request = client->put(url, payload, value.size());

    json::Value root;
    json::Reader reader;

    auto response = request->execute();

    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ(payload.str(), root["data"].asString());
}
