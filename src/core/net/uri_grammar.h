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

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_auto.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace core
{
namespace net
{
template<typename Iterator>
struct UriGrammar : public boost::spirit::qi::grammar<Iterator>
{
    struct Components
    {
        boost::iterator_range<Iterator> scheme;
        boost::iterator_range<Iterator> userinfo;
        boost::iterator_range<Iterator> host;
        boost::iterator_range<Iterator> port;
        boost::iterator_range<Iterator> path;
        boost::iterator_range<Iterator> query;
        boost::iterator_range<Iterator> fragment;
    };

    UriGrammar() : UriGrammar::base_type(uri)
    {
        using namespace boost::spirit::qi;

        sub_delims %= char_("!$&\\()*+,;=");
        pct_encoded %= '%' >> xdigit >> xdigit;
        unreserved %= alnum | char_("-._~");
        pchar = unreserved | pct_encoded | sub_delims | char_(":@");
        scheme = alpha >> *(alnum | char_("+-."));

        userinfo = *(unreserved | pct_encoded | sub_delims | ':');
        ip_literal = '[' >> (ipv6address | ipvfuture) >> ']';
        ipvfuture = 'v' >> +xdigit >> '.' >> +(unreserved | sub_delims | ':');
        ipv6address = repeat(6)[h16 >> ':'] >> ls32
            | "::" >> repeat(5)[h16 >> ':'] >> ls32
            | -(h16) >> "::" >> repeat(4)[h16 >> ':'] >> ls32
            | -(repeat(0, 1)[h16 >> ':'] >> h16) >> "::" >> repeat(3)[h16 >> ':'] >> ls32
            | -(repeat(0, 2)[h16 >> ':'] >> h16) >> "::" >> repeat(2)[h16 >> ':'] >> ls32
            | -(repeat(0, 3)[h16 >> ':'] >> h16) >> "::" >> h16 >> ':'  >> ls32
            | -(repeat(0, 4)[h16 >> ':'] >> h16) >> "::" >> ls32
            | -(repeat(0, 5)[h16 >> ':'] >> h16) >> "::" >> h16
            | -(repeat(0, 6)[h16 >> ':'] >> h16) >> "::";
        h16 = repeat(1, 4)[xdigit];
        ls32 = h16 >> ':' >> h16 | ipv4address;
        ipv4address = dec_octet >> '.' >> dec_octet >> '.' >> dec_octet >> '.' >> dec_octet;
        dec_octet = "25" >> char_("0-5")
            | '2' >> char_("0-4") >> digit
            | '1' >> repeat(2)[digit]
            | char_("1-9") >> digit
            | digit;
        reg_name %=  *(unreserved | pct_encoded | sub_delims);
        host %= ip_literal
            | ipv4address
            | reg_name;
        port = *digit;
        authority = -(raw[userinfo]>> '@')
            >> raw[host]
            >> -(':' >> raw[port]);
        path_abempty = *('/' >> *pchar);
        path_absolute = '/' >> -(+pchar >> *('/' >> *pchar));
        path_rootless = +pchar >> *('/' >> *pchar);
        path_empty = eps;
        hier_part = "//" >> authority >> raw[path_abempty]
            | raw[(path_absolute | path_rootless | path_empty)];
        query = *(pchar | char_("/?"));
        fragment = *(pchar | char_("/?"));
        segment_nz_nc = +(unreserved
            | pct_encoded
            | sub_delims
            | '@');
        path_noscheme = segment_nz_nc >> *('/' >> *pchar);
        path_empty = eps;
        relative_part = "//" >> authority >> raw[path_abempty]
            | raw[(path_absolute | path_noscheme | path_empty)];
        relative_ref = relative_part
            >> -('?' >> raw[query])
            >> -('#' >> raw[fragment]);
        uri = raw[scheme]
            >> ':' >> hier_part
            >> -('?' >> raw[query])
            >> -('#' >> raw[fragment]);
        uri_reference = uri
            | raw[eps] >> relative_ref;
        absolute_uri = raw[scheme]
            >> ':' >> hier_part
            >> -('?' >> raw[query]);
    }

    boost::spirit::qi::rule<Iterator> sub_delims;
    boost::spirit::qi::rule<Iterator> pct_encoded;
    boost::spirit::qi::rule<Iterator> unreserved;
    boost::spirit::qi::rule<Iterator> pchar;
    boost::spirit::qi::rule<Iterator> scheme;
    boost::spirit::qi::rule<Iterator> authority;
    boost::spirit::qi::rule<Iterator> ip_literal;
    boost::spirit::qi::rule<Iterator> ipv6address;
    boost::spirit::qi::rule<Iterator> userinfo;
    boost::spirit::qi::rule<Iterator> host;
    boost::spirit::qi::rule<Iterator> port;
    boost::spirit::qi::rule<Iterator> reg_name;
    boost::spirit::qi::rule<Iterator> ipv4address;
    boost::spirit::qi::rule<Iterator> dec_octet;
    boost::spirit::qi::rule<Iterator> h16;
    boost::spirit::qi::rule<Iterator> ls32;
    boost::spirit::qi::rule<Iterator> ipvfuture;
    boost::spirit::qi::rule<Iterator> path_abempty;
    boost::spirit::qi::rule<Iterator> path_absolute;
    boost::spirit::qi::rule<Iterator> hier_part;
    boost::spirit::qi::rule<Iterator> path_rootless;
    boost::spirit::qi::rule<Iterator> query;
    boost::spirit::qi::rule<Iterator> fragment;
    boost::spirit::qi::rule<Iterator> relative_ref;
    boost::spirit::qi::rule<Iterator> relative_part;
    boost::spirit::qi::rule<Iterator> path_noscheme;
    boost::spirit::qi::rule<Iterator> path_empty;
    boost::spirit::qi::rule<Iterator> segment_nz_nc;
    boost::spirit::qi::rule<Iterator> uri_reference;
    boost::spirit::qi::rule<Iterator> uri;
    boost::spirit::qi::rule<Iterator> absolute_uri;
};
}
}
#endif // CORE_NET_URI_GRAMMAR_H_
