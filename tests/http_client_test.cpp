#include <core/net/uri.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/request.h>
#include <core/net/http/response.h>

#include <gtest/gtest.h>

#include <json/json.h>

#include <future>

namespace http = core::net::http;
namespace json = Json;
namespace net = core::net;

namespace
{
auto default_progress_reporter = [](const http::Request::Progress& progress)
{
    if (progress.download.current > 0. && progress.download.total > 0.)
        std::cout << "Download progress: " << progress.download.current / progress.download.total << std::endl;
    if (progress.upload.current > 0. && progress.upload.total > 0.)
        std::cout << "Upload progress: " << progress.upload.current / progress.upload.total << std::endl;

    return http::Request::Progress::Next::continue_operation;
};
}

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

TEST(HttpClient, head_request_for_existing_resource_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host()) + httpbin::resources::get();

    // The client mostly acts as a factory for http requests.
    auto request = client->head(http::Request::Configuration::from_uri_as_string(url));

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter);

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
    auto request = client->get(http::Request::Configuration::from_uri_as_string(url));

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter);

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // The url field of the payload should equal the original url we requested.
    EXPECT_EQ(url, root["url"].asString());
}

TEST(HttpClient, get_request_for_existing_resource_guarded_by_basic_auth_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host()) + httpbin::resources::basic_auth();

    // The client mostly acts as a factory for http requests.
    auto configuration = http::Request::Configuration::from_uri_as_string(url);
    configuration.authentication_handler.for_http = [](const std::string&)
    {
        return http::Request::Credentials{"user", "passwd"};
    };
    auto request = client->get(configuration);

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter);

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // We expect authentication to work.
    EXPECT_TRUE(root["authenticated"].asBool());
    // With the correct user id
    EXPECT_EQ("user", root["user"].asString());
}

TEST(HttpClient, get_request_for_existing_resource_guarded_by_digest_auth_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host()) + httpbin::resources::digest_auth();

    // The client mostly acts as a factory for http requests.
    auto configuration = http::Request::Configuration::from_uri_as_string(url);
    configuration.authentication_handler.for_http = [](const std::string&)
    {
        return http::Request::Credentials{"user", "passwd"};
    };
    auto request = client->get(configuration);

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter);

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // We expect authentication to work.
    EXPECT_TRUE(root["authenticated"].asBool());
    // With the correct user id
    EXPECT_EQ("user", root["user"].asString());
}

TEST(HttpClient, async_get_request_for_existing_resource_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Execute the client
    std::thread worker{[client]() { client->run(); }};

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host()) + httpbin::resources::get();

    // The client mostly acts as a factory for http requests.
    auto request = client->get(http::Request::Configuration::from_uri_as_string(url));

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    std::promise<core::net::http::Response> promise;
    auto future = promise.get_future();

    // We finally execute the query asynchronously.
    request->async_execute(
                default_progress_reporter,
                [&](const core::net::http::Response& response)
                {
                    promise.set_value(response);
                    client->stop();
                },
                [&]()
                {
                    promise.set_exception(
                                std::make_exception_ptr(
                                    std::runtime_error("Error accessing remote resource")));
                    client->stop();
                });

    // And wait here for the response to arrive.
    auto response = future.get();

    // We shut down our worker thread
    if (worker.joinable())
        worker.join();

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // The url field of the payload should equal the original url we requested.
    EXPECT_EQ(url, root["url"].asString());
}

TEST(HttpClient, async_get_request_for_existing_resource_guarded_by_basic_authentication_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Execute the client
    std::thread worker{[client]() { client->run(); }};

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host()) + httpbin::resources::basic_auth();

    // The client mostly acts as a factory for http requests.
    auto configuration = http::Request::Configuration::from_uri_as_string(url);

    configuration.authentication_handler.for_http = [](const std::string&)
    {
        return http::Request::Credentials{"user", "passwd"};
    };

    auto request = client->get(configuration);

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    std::promise<core::net::http::Response> promise;
    auto future = promise.get_future();

    // We finally execute the query asynchronously.
    request->async_execute(
                default_progress_reporter,
                [&](const core::net::http::Response& response)
                {
                    promise.set_value(response);
                    client->stop();
                },
                [&]()
                {
                    promise.set_exception(
                                std::make_exception_ptr(
                                    std::runtime_error("Error accessing remote resource")));
                    client->stop();
                });

    // And wait here for the response to arrive.
    auto response = future.get();

    // We shut down our worker thread
    if (worker.joinable())
        worker.join();

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // We expect authentication to work.
    EXPECT_TRUE(root["authenticated"].asBool());
    // With the correct user id
    EXPECT_EQ("user", root["user"].asString());
}

TEST(HttpClient, post_request_for_existing_resource_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host()) + httpbin::resources::post();

    std::string payload = "{ 'test': 'test' }";

    // The client mostly acts as a factory for http requests.
    auto request = client->post(http::Request::Configuration::from_uri_as_string(url),
                                payload,
                                core::net::http::ContentType::json);

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter);

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

    auto request = client->put(http::Request::Configuration::from_uri_as_string(url),
                               payload,
                               value.size());

    json::Value root;
    json::Reader reader;

    auto response = request->execute(default_progress_reporter);

    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ(payload.str(), root["data"].asString());
}

namespace com
{
namespace mozilla
{
namespace services
{
namespace location
{
//const char* host() { return "https://location.services.mozilla.com"; }
const char* host() { return "http://54.80.82.190:7001/"; }

namespace resources
{
namespace v1
{
const char* search() { return "/v1/search"; }
const char* submit() { return "/v1/submit"; }
}
}
}
}
}
}

// See https://mozilla-ichnaea.readthedocs.org/en/latest/api/search.html
// for API and endpoint documentation.
TEST(HttpClient, search_for_location_on_mozillas_location_service_succeeds)
{
    json::FastWriter writer;
    json::Value search;
    json::Value cell;
    cell["radio"] = "umts";
    cell["mcc"] = 123;
    cell["mnc"] = 123;
    cell["lac"] = 12345;
    cell["cid"] = 12345;
    cell["signal"] = -61;
    cell["asu"] = 26;
    json::Value wifi1, wifi2;
    wifi1["key"] = "01:23:45:67:89:ab";
    wifi1["channel"] = 11;
    wifi1["frequency"] = 2412;
    wifi1["signal"] = -50;
    wifi2["key"] = "01:23:45:67:cd:ef";

    search["radio"] = json::Value("gsm");
    search["cell"].append(cell);
    search["wifi"].append(wifi1);
    search["wifi"].append(wifi2);

    auto client = http::make_client();
    auto url =
            std::string(com::mozilla::services::location::host()) +
            com::mozilla::services::location::resources::v1::search();
    auto request = client->post(http::Request::Configuration::from_uri_as_string(url),
                                writer.write(search),
                                http::ContentType::json);

    auto response = request->execute(default_progress_reporter);

    json::Reader reader;
    json::Value result;

    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, result));

    // We cannot be sure that the server has got information for the given
    // cell and wifi ids. For that, we disable the test.
    // EXPECT_EQ("ok", result["status"].asString());
    // EXPECT_DOUBLE_EQ(-22.7539192, result["lat"].asDouble());
    // EXPECT_DOUBLE_EQ(-43.4371081, result["lon"].asDouble());
}

// See https://mozilla-ichnaea.readthedocs.org/en/latest/api/submit.html
// for API and endpoint documentation.
TEST(HttpClient, submit_of_location_on_mozillas_location_service_succeeds)
{
    json::Value submit;
    json::Value cell;
    cell["radio"] = "umts";
    cell["mcc"] = 123;
    cell["mnc"] = 123;
    cell["lac"] = 12345;
    cell["cid"] = 12345;
    cell["signal"] = -60;
    json::Value wifi1, wifi2;
    wifi1["key"] = "01:23:45:67:89:ab";
    wifi1["channel"] = 11;
    wifi1["frequency"] = 2412;
    wifi1["signal"] = -50;
    wifi2["key"] = "01:23:45:67:cd:ef";

    json::Value item;
    item["lat"] = -22.7539192;
    item["lon"] = -43.4371081;
    item["time"] = "2012-03-15T11:12:13.456Z";
    item["accuracy"] = 10;
    item["altitude"] = 100;
    item["altitude_accuracy"] = 1;
    item["radio"] = "gsm";
    item["cell"].append(cell);
    item["wifi"].append(wifi1);
    item["wifi"].append(wifi2);

    submit["items"].append(item);

    json::FastWriter writer;
    auto client = http::make_client();
    auto url =
            std::string(com::mozilla::services::location::host()) +
            com::mozilla::services::location::resources::v1::submit();
    auto request = client->post(http::Request::Configuration::from_uri_as_string(url),
                                writer.write(submit),
                                http::ContentType::json);
    auto response = request->execute(default_progress_reporter);

    EXPECT_EQ(http::Status::no_content,
              response.status);
}
