/** -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 78 -*-
 *
 * uri_grammar
 *
 * Copyright 2010  Braden McDaniel
 *
 * Distributed under the Boost Software License, Version 1.0.
 *
 * See accompanying file COPYING or copy at
 * http://www.boost.org/LICENSE_1_0.txt
 */
#ifndef CORE_NET_URI_GRAMMAR_H_
#define CORE_NET_URI_GRAMMAR_H_

#include <core/net/uri.h>

#include <boost/range/iterator_range.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_auto.hpp>
#include <boost/spirit/include/qi_expect.hpp>

#include <boost/spirit/include/phoenix.hpp>

#include <boost/fusion/adapted/struct/adapt_struct.hpp>

BOOST_FUSION_ADAPT_STRUCT
(
        core::net::Uri::Authority::Host,
        (core::net::Uri::Authority::Host::Address, address)
        (core::net::Uri::Optional<std::uint16_t>, port)
        )

BOOST_FUSION_ADAPT_STRUCT
(
        core::net::Uri::Authority,
        (core::net::Uri::Optional<core::net::Uri::Authority::UserInformation>, user_info)
        (core::net::Uri::Authority::Host, host)
        )

BOOST_FUSION_ADAPT_STRUCT
(
        core::net::Uri::Path,
        (std::vector<core::net::Uri::Path::Component>, components)
        )

BOOST_FUSION_ADAPT_STRUCT
(
        core::net::Uri::Hierarchical,
        (core::net::Uri::Optional<core::net::Uri::Authority>, authority)
        (core::net::Uri::Optional<core::net::Uri::Path>, path)
        )

BOOST_FUSION_ADAPT_STRUCT
(
        core::net::Uri,
        (core::net::Uri::Optional<core::net::Uri::Scheme>, scheme)
        (core::net::Uri::Hierarchical, hierarchical)
        (core::net::Uri::Optional<core::net::Uri::Query>, query)
        (core::net::Uri::Optional<core::net::Uri::Fragment>, fragment)
        )

namespace boost
{
namespace spirit
{
namespace traits {
template<>
struct container_value<core::net::Uri::Path, void> {
    typedef core::net::Uri::Path::Component type;
};

template<core::net::Uri::Tag tag>
struct container_value<core::net::Uri::TaggedString<tag>, void> {
    typedef std::string::value_type type;
};

template<>
struct push_back_container<core::net::Uri::Path, core::net::Uri::Path::Component, void> {
    static bool call(core::net::Uri::Path& c,
                     const core::net::Uri::Path::Component& val) {
        c.components.push_back(val);
        return true;
    }
};

template<core::net::Uri::Tag tag>
struct push_back_container<core::net::Uri::TaggedString<tag>, std::string::value_type, void> {
    static bool call(core::net::Uri::TaggedString<tag>& c,
                     const std::string& val) {
        c.value.push_back(c);
        return true;
    }
};
}
}
}

namespace core
{
namespace net
{
template<typename Iterator>
struct UriGrammar : public boost::spirit::qi::grammar<Iterator, Uri()>
{
    UriGrammar() : UriGrammar::base_type(start)
    {
        // gen-delims = ":" / "/" / "?" / "#" / "[" / "]" / "@"
        gen_delims %= boost::spirit::qi::char_(":/?#[]@");
        gen_delims.name("gen_delims");
        // sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
        sub_delims %= boost::spirit::qi::char_("!$&'()*+,;=");
        sub_delims.name("sub_delims");
        // reserved = gen-delims / sub-delims
        reserved %= gen_delims | sub_delims;
        reserved.name("reserved");
        // unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
        unreserved %= boost::spirit::qi::alnum | boost::spirit::qi::char_("-._~");
        unreserved.name("unreserved");
        // pct-encoded = "%" HEXDIG HEXDIG
        pct_encoded %= boost::spirit::qi::char_("%") >> boost::spirit::qi::repeat(2)[boost::spirit::qi::xdigit];
        pct_encoded.name("pct_encoded");
        // pchar = unreserved / pct-encoded / sub-delims / ":" / "@"
        pchar %= boost::spirit::qi::raw[unreserved | pct_encoded | sub_delims | boost::spirit::qi::char_(":@")];
        pchar.name("pchar");
        // segment = *pchar
        segment %= boost::spirit::qi::as_string[*pchar];
        segment.name("segment");
        // segment-nz = 1*pchar
        segment_nz %= boost::spirit::qi::as_string[+pchar];
        segment_nz.name("segment_nz");
        // segment-nz-nc = 1*( unreserved / pct-encoded / sub-delims / "@" )
        segment_nz_nc %= boost::spirit::qi::as_string[+(unreserved | pct_encoded | sub_delims | boost::spirit::qi::char_("@"))];
        segment_nz_nc.name("segment_nz_nc");
        // path-abempty  = *( "/" segment )
        path_abempty %= *(boost::spirit::qi::lit("/") >> segment);
        path_abempty.name("path_abempty");
        // path-absolute = "/" [ segment-nz *( "/" segment ) ]
        path_absolute %= boost::spirit::qi::lit("/") >>  -(segment_nz >> *(boost::spirit::qi::lit("/") >> segment));
        path_absolute.name("path_absolute");
        // path-rootless = segment-nz *( "/" segment )
        path_rootless %= segment_nz >> *(boost::spirit::qi::lit("/") >> segment);
        path_rootless.name("path_rootless");
        // path-empty = 0<pchar>
        path_empty %= boost::spirit::qi::eps;
        path_empty.name("path_empty");
        // scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
        scheme %= boost::spirit::qi::as_string[boost::spirit::qi::alpha >> *(boost::spirit::qi::alnum | boost::spirit::qi::char_("+.-"))];
        scheme.name("scheme");
        // user_info = *( unreserved / pct-encoded / sub-delims / ":" )
        user_info %= *(unreserved | pct_encoded | sub_delims | boost::spirit::qi::char_(":"));
        user_info.name("user_info");

        ip_literal %= boost::spirit::qi::lit('[') >> (ipv6address | ipvfuture) >> ']';
        ip_literal.name("ip_literal");
        ipvfuture %= boost::spirit::qi::lit('v') >> +boost::spirit::qi::xdigit >> '.' >> +( unreserved | sub_delims | ':');
        ipvfuture.name("ipvfuture");
        ipv6address %= boost::spirit::qi::raw[boost::spirit::qi::repeat(6)[h16 >> ':'] >> ls32 |
                "::" >> boost::spirit::qi::repeat(5)[h16 >> ':'] >> ls32 |
                - boost::spirit::qi::raw[h16] >> "::" >> boost::spirit::qi::repeat(4)[h16 >> ':'] >> ls32 |
                - boost::spirit::qi::raw[h16] >> "::" >> boost::spirit::qi::repeat(3)[h16 >> ':'] >> ls32 |
                - boost::spirit::qi::raw[h16] >> "::" >> boost::spirit::qi::repeat(2)[h16 >> ':'] >> ls32 |
                - boost::spirit::qi::raw[h16] >> "::" >> h16 >> ':'  >> ls32 |
                - boost::spirit::qi::raw[h16] >> "::" >> ls32 |
                - boost::spirit::qi::raw[h16] >> "::" >> h16 |
                - boost::spirit::qi::raw[h16] >> "::" |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(1)[(h16 >> ':')] >> h16] >> "::" >> boost::spirit::qi::repeat(3)[h16 >> ':'] >> ls32 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(1)[(h16 >> ':')] >> h16] >> "::" >> boost::spirit::qi::repeat(2)[h16 >> ':'] >> ls32 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(1)[(h16 >> ':')] >> h16] >> "::" >> h16 >> ':'  >> ls32 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(1)[(h16 >> ':')] >> h16] >> "::" >> ls32 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(1)[(h16 >> ':')] >> h16] >> "::" >> h16 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(1)[(h16 >> ':')] >> h16] >> "::" |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(2)[(h16 >> ':')] >> h16] >> "::" >> boost::spirit::qi::repeat(2)[h16 >> ':'] >> ls32 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(2)[(h16 >> ':')] >> h16] >> "::" >> h16 >> ':'  >> ls32 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(2)[(h16 >> ':')] >> h16] >> "::" >> ls32 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(2)[(h16 >> ':')] >> h16] >> "::" >> h16 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(2)[(h16 >> ':')] >> h16] >> "::" |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(3)[(h16 >> ':')] >> h16] >> "::" >> h16 >> ':'  >> ls32 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(3)[(h16 >> ':')] >> h16] >> "::" >> ls32 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(3)[(h16 >> ':')] >> h16] >> "::" >> h16 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(3)[(h16 >> ':')] >> h16] >> "::" |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(4)[(h16 >> ':')] >> h16] >> "::" >> ls32 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(4)[(h16 >> ':')] >> h16] >> "::" >> h16 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(4)[(h16 >> ':')] >> h16] >> "::" |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(5)[(h16 >> ':')] >> h16] >> "::" >> h16 |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(5)[(h16 >> ':')] >> h16] >> "::" |
                - boost::spirit::qi::raw[boost::spirit::qi::repeat(6)[(h16 >> ':')] >> h16] >> "::"];
        ipv6address.name("ipv6address");
        // ls32 = ( h16 ":" h16 ) / IPv4address
        ls32 %= (h16 >> ':' >> h16) | ipv4address;
        ls32.name("ls32");
        // h16 = 1*4HEXDIG
        h16 %= boost::spirit::qi::repeat(1, 4)[boost::spirit::qi::xdigit];
        h16.name("h16");
        // dec-octet = DIGIT / %x31-39 DIGIT / "1" 2DIGIT / "2" %x30-34 DIGIT / "25" %x30-35
        dec_octet %= boost::spirit::qi::raw[boost::spirit::qi::uint_parser<std::uint8_t, 10, 1, 3>()];
        dec_octet.name("dec_octet");
        // IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
        ipv4address %= dec_octet > boost::spirit::qi::repeat(3)[boost::spirit::ascii::char_(".") >> dec_octet];
        ipv4address.name("ipv4address");
        // reg-name = *( unreserved / pct-encoded / sub-delims )
        reg_name %= *(unreserved | pct_encoded | sub_delims);
        reg_name.name("reg_name");
        // TODO, host = IP-literal / IPv4address / reg-name
        authority %= (("//" >> user_info >> '@') | "//") >> host >> -(port);
        authority.name("authority");
        host %= boost::spirit::qi::as_string[ip_literal | ipv4address | reg_name];
        host.name("host");
        port %= boost::spirit::qi::ushort_ | boost::spirit::qi::eps;
        port.name("port");
        // query = *( pchar / "/" / "?" )
        query %= *(pchar | boost::spirit::qi::char_("/?"));
        query.name("query");
        // fragment = *( pchar / "/" / "?" )
        fragment %= *(pchar | boost::spirit::qi::char_("/?"));
        fragment.name("fragment");
        // hier-part = "//" authority path-abempty / path-absolute / path-rootless / path-empty
        // authority = [ userinfo "@" ] host [ ":" port ]
        hier_part %= (authority >> path_abempty) |
                (boost::spirit::qi::attr(core::net::Uri::Optional<core::net::Uri::Authority>()) >>
                 (path_absolute | path_rootless | path_empty));
        hier_part.name("hier_part");
        start %= (scheme > ':') >> hier_part >> -('?' >> query) >> -('#' >> fragment);
        start.name("start");

        enable_debugging();
    }

    void enable_debugging()
    {
        debug(gen_delims);
        debug(sub_delims);
        debug(reserved);
        debug(unreserved);
        debug(pct_encoded);
        debug(pchar);
        debug(segment);
        debug(segment_nz);
        debug(segment_nz_nc);
        debug(path_abempty);
        debug(path_absolute);
        debug(path_rootless);
        debug(path_empty);
        debug(digit);
        debug(authority);
        debug(dec_octet);
        debug(ipv4address);
        debug(reg_name);
        debug(ipv6address);
        debug(ipvfuture);
        debug(ip_literal);
        debug(h16);
        debug(ls32);
        debug(host);
        debug(port);
        debug(scheme);
        debug(user_info);
        debug(query);
        debug(fragment);
        debug(hier_part);

        // actual uri parser
        debug(start);
    }

    boost::spirit::qi::rule<Iterator, std::string::value_type()> gen_delims;
    boost::spirit::qi::rule<Iterator, std::string::value_type()> sub_delims;
    boost::spirit::qi::rule<Iterator, std::string::value_type()> reserved;
    boost::spirit::qi::rule<Iterator, std::string::value_type()> unreserved;
    boost::spirit::qi::rule<Iterator, std::string()> pct_encoded;
    boost::spirit::qi::rule<Iterator, std::string()> pchar;
    boost::spirit::qi::rule<Iterator, core::net::Uri::Path::Component()> segment;
    boost::spirit::qi::rule<Iterator, core::net::Uri::Path::Component()> segment_nz;
    boost::spirit::qi::rule<Iterator, core::net::Uri::Path::Component()> segment_nz_nc;
    boost::spirit::qi::rule<Iterator, core::net::Uri::Path()> path_abempty;
    boost::spirit::qi::rule<Iterator, core::net::Uri::Path()> path_absolute;
    boost::spirit::qi::rule<Iterator, core::net::Uri::Path()> path_rootless;
    boost::spirit::qi::rule<Iterator, core::net::Uri::Path()> path_empty;
    boost::spirit::qi::rule<Iterator, char()> digit;
    boost::spirit::qi::rule<Iterator, std::string()> dec_octet;
    boost::spirit::qi::rule<Iterator, std::string()> ipv4address;
    boost::spirit::qi::rule<Iterator, std::string()> reg_name;
    boost::spirit::qi::rule<Iterator, std::string()> ipv6address;
    boost::spirit::qi::rule<Iterator, std::string()> ipvfuture;
    boost::spirit::qi::rule<Iterator, std::string()> ip_literal;
    boost::spirit::qi::rule<Iterator, std::string()> h16;
    boost::spirit::qi::rule<Iterator, std::string()> ls32;
    boost::spirit::qi::rule<Iterator, core::net::Uri::Authority()> authority;
    boost::spirit::qi::rule<Iterator, core::net::Uri::Authority::Host()> host;
    boost::spirit::qi::rule<Iterator, std::uint16_t()> port;
    boost::spirit::qi::rule<Iterator, core::net::Uri::Scheme()> scheme;
    boost::spirit::qi::rule<Iterator, core::net::Uri::Authority::UserInformation()> user_info;
    boost::spirit::qi::rule<Iterator, core::net::Uri::Query()> query;
    boost::spirit::qi::rule<Iterator, core::net::Uri::Fragment()> fragment;
    boost::spirit::qi::rule<Iterator, Uri::Hierarchical()> hier_part;

    // actual uri parser
    boost::spirit::qi::rule<Iterator, Uri()> start;
};
}
}
#endif // CORE_NET_URI_GRAMMAR_H_
