/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef URL_LOADER_HH
#define URL_LOADER_HH

#include <string>
#include <functional>

#include "file_descriptor.hh"

/* class to load url with phantomjs and capture the HTTP traffic */

class URLLoader
{
public:
    URLLoader();
    int get_all_resources( std::function<int( FileDescriptor & )> && child_procedure,
                           const int & veth_counter = 0,
                           const std::string & stdin_input = "" );
};

#endif /* URL_LOADER_HH */
