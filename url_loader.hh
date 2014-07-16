/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef URL_LOADER_HH
#define URL_LOADER_HH

#include <string>

/* class to load url with phantomjs and capture the HTTP traffic */

class URLLoader
{
public:
    URLLoader();
    int get_all_resources( const std::string & url );
};

#endif /* URL_LOADER_HH */
