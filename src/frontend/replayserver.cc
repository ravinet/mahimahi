/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <vector>
#include <limits>
#include <sstream>
#include <map>
#include <regex>
#include <set>

#include "util.hh"
#include "http_record.pb.h"
#include "http_header.hh"
#include "exception.hh"
#include "http_request.hh"
#include "http_response.hh"
#include "file_descriptor.hh"

using namespace std;

string safe_getenv( const string & key )
{
    const char * const value = getenv( key.c_str() );
    if ( not value ) {
        throw runtime_error( "missing environment variable: " + key );
    }
    return value;
}

/* does the actual HTTP header match this stored request? */
bool header_match( const string & env_var_name,
                   const string & header_name,
                   const HTTPRequest & saved_request )
{
    const char * const env_value = getenv( env_var_name.c_str() );

    /* case 1: neither header exists (OK) */
    if ( (not env_value) and (not saved_request.has_header( header_name )) ) {
        return true;
    }

    /* case 2: headers both exist (OK if values match) */
    if ( env_value and saved_request.has_header( header_name ) ) {
        return saved_request.get_header_value( header_name ) == string( env_value );
    }

    /* case 3: one exists but the other doesn't (failure) */
    return false;
}

string strip_query( const string & request_line )
{
    const auto index = request_line.find( "?" );
    if ( index == string::npos ) {
        return request_line;
    } else {
        return request_line.substr( 0, index );
    }
}

string extract_url_from_request_line( const string & request_line ) {
  vector< string > elems;
  stringstream ss(request_line);
  string item;
  while (getline(ss, item, ' ')) {
    elems.push_back(item);
  }
  return elems[1];
}

string remove_trailing_slash( const string & url ) {
  if (url.length() <= 0) {
    return url;
  }

  string retval = url;
  while (retval[retval.length() - 1] == '/' ) {
    retval = retval.substr(0, retval.length() - 1);
  }
  return retval;
}

string strip_hostname( const string & url, const string & path ) {
  string http = "http://";
  string https = "https://";
  if ( (url.find( http ) == 0 && path.find( http ) == 0) ||
       (url.find( https ) == 0 && path.find( https ) == 0 )) {
    return url;
  }

  string retval = url;
  if ( url.find( http ) == 0 ) {
    retval = url.substr( http.length(), url.length() );
  } else if ( url.find( https ) == 0 ) {
    retval = url.substr( https.length(), url.length() );
  }

  string www = "www.";
  if ( retval.find( www ) == 0 ) {
    retval = retval.substr( www.length(), retval.length() );
  }

  const auto index = retval.find( "/" );
  retval = retval.substr( index, retval.length() );

  return retval;
}

string extract_hostname( const string & url ) {
  string http = "http://";
  string https = "https://";

  string retval = url;
  if ( url.find( http ) == 0 ) {
    retval = url.substr( http.length(), url.length() );
  } else if ( url.find( https ) == 0 ) {
    retval = url.substr( https.length(), url.length() );
  }

  const auto index = retval.find( "/" );
  retval = retval.substr( 0, index );
  return retval;
}

string strip_www( const string & url ) {
  string retval = url;

  string www = "www.";
  if ( retval.find( www ) == 0 ) {
    retval = retval.substr( www.length(), retval.length() );
  }
  return retval;
}

unsigned int match_url( const string & saved_request_url, 
                        const string & request_url ) {
    /* must match first line up to "?" at least */
    // ofstream myfile;
    // myfile.open("match_url.txt", ios::app);
    // if (request_url.find("5o3Rb.5pjsDBCdfFtzlQ8w") != string::npos) {
    //   myfile << "striping url" << endl;
    // }
    if ( strip_query(request_url) != strip_query(saved_request_url) ) {
      // if (request_url.find("5o3Rb.5pjsDBCdfFtzlQ8w") != string::npos && saved_request_url.find("5o3Rb.5pjsDBCdfFtzlQ8w") != string::npos) {
      //   myfile << "returned from strip query mismatch: " << request_url << " saved: " << saved_request_url << endl;
      // }
      return 0;
    }

    // if (request_url.find("5o3Rb.5pjsDBCdfFtzlQ8w") != string::npos) {
    //   myfile << "Matched " << request_url << " to " << saved_request_url << endl;
    // }
    /* success! return size of common prefix */
    const auto max_match = min( request_url.size(), saved_request_url.size() );
    for ( unsigned int i = 0; i < max_match; i++ ) {
        if ( request_url.at( i ) != saved_request_url.at( i ) ) {
            // if (request_url.find("5o3Rb.5pjsDBCdfFtzlQ8w") != string::npos) {
            //   myfile << "score: " << i << endl;
            // }
            return i;
        }
    }
    // if (request_url.find("5o3Rb.5pjsDBCdfFtzlQ8w") != string::npos) {
    //   myfile << "score: " << max_match << endl;
    // }
    // myfile.close();
    return max_match;
}

/* compare request_line and certain headers of incoming request and stored request */
unsigned int match_score( const MahimahiProtobufs::RequestResponse & saved_record,
                          const string & request_line,
                          const bool is_https )
{
    HTTPRequest saved_request( saved_record.request() );
    
    // ofstream myfile;
    // myfile.open("match_score.txt", ios::app);
    // if (request_line.find("5o3Rb.5pjsDBCdfFtzlQ8w") != string::npos) {
    //   myfile << " Before match score: Request Line: " << request_line << "Saved request: " << saved_request.first_line() << endl;
    // }
    
    /* match HTTP/HTTPS */
    if ( is_https and (saved_record.scheme() != MahimahiProtobufs::RequestResponse_Scheme_HTTPS) ) {
        // if (request_line.find("5o3Rb.5pjsDBCdfFtzlQ8w") != string::npos) {
        //   myfile << "\tFailed HTTPS" << endl;
        // }
        return 0;
    }

    if ( (not is_https) and (saved_record.scheme() != MahimahiProtobufs::RequestResponse_Scheme_HTTP) ) {
        return 0;
    }

    /* match host header */
    if ( not header_match( "HTTP_HOST", "Host", saved_request ) ) {
        // if (request_line.find("5o3Rb.5pjsDBCdfFtzlQ8w") != string::npos) {
        //   myfile << "\tFailed Host" << endl;
        // }
        return 0;
    }

    /* match user agent */
    // if ( not header_match( "HTTP_USER_AGENT", "User-Agent", saved_request ) ) {
    //     return 0;
    // }

    string request_url = strip_hostname(extract_url_from_request_line(request_line), extract_url_from_request_line(saved_request.first_line()));
    string saved_request_url = extract_url_from_request_line(saved_request.first_line());
    // myfile.close();
    return match_url(saved_request_url, request_url);
}

void split(const string &s, char delim, vector<string> &elems) {
  stringstream ss(s);
  string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

vector<string> split(const string &s, char delim) {
  vector<string> elems;
  split(s, delim, elems);
  return elems;
}

string infer_resource_type(const string & resource_type) {
  if (resource_type == "Image") {
    return ";as=image";
  } else if (resource_type == "Stylesheet") {
    return ";as=style";
  } else if (resource_type == "Script") {
    return ";as=script";
  // } else if (resource_type == "Document") {
  //   return ";as=document";
  } else if (resource_type == "Font") {
    return ";as=font;crossorigin";
  } else if (resource_type == "XHR" || resource_type == "DEFAULT") {
    return "";
  }
  return "";
}

void populate_push_configurations( const string & dependency_file, 
                                   const string & request_url, 
                                   HTTPResponse & response, 
                                   const string & current_loading_page ) {
  map< string, vector< string >> dependencies_map;
  map< string, string > dependency_type_map;
  map< string, string > dependency_priority_map;
  ifstream infile(dependency_file);
  string line;
  if (infile.is_open()) {
    while (getline(infile, line)) {
      vector< string > splitted_line = split(line, ' ');
      string parent = remove_trailing_slash(splitted_line[0]);
      if (dependencies_map.find(parent) == dependencies_map.end()) {
        dependencies_map[parent] = { };
      }
      string child = splitted_line[2];
      string resource_type = splitted_line[4];
      string resource_priority = splitted_line[5];
      dependencies_map[parent].push_back(child);
      dependency_type_map[child] = resource_type;
      dependency_priority_map[child] = resource_priority;
    }
    infile.close();
  }

  if ( !dependencies_map.empty() ) {
    // Write the dependencies to the configuration file.
    string removed_slash_request_url = remove_trailing_slash(request_url);
    vector< string > link_resources;
    vector< string > unimportant_resources;
    if (dependencies_map.find(removed_slash_request_url) != dependencies_map.end()) {
      auto key = removed_slash_request_url;
      auto values = dependencies_map[key];
      for (auto list_it = values.begin(); list_it != values.end(); ++list_it) {
        // Push all dependencies for the location.
        string dependency_filename = *list_it;
        // if (dependency_type_map[dependency_filename] == "Document" ||
        //     dependency_type_map[dependency_filename] == "Script" ||
        //     dependency_type_map[dependency_filename] == "Stylesheet") {
        if ((dependency_priority_map[dependency_filename] == "VeryHigh" ||
            dependency_priority_map[dependency_filename] == "High" ||
            dependency_priority_map[dependency_filename] == "Medium") &&
            (dependency_type_map[dependency_filename] == "Document" ||
            dependency_type_map[dependency_filename] == "Script" ||
            dependency_type_map[dependency_filename] == "Stylesheet")) {
          string dependency_type = dependency_type_map[dependency_filename];
          string link_resource_string = "<" + dependency_filename + ">;rel=preload"
            + infer_resource_type(dependency_type_map[dependency_filename]);

          // Add push or nopush directive based on the hostname of the URL.
          string request_hostname = strip_www( extract_hostname( dependency_filename ));
          if ( request_hostname != current_loading_page ) {
            link_resource_string += ";nopush";
          }

          link_resources.push_back(link_resource_string);
        } else {
          string unimportant_resource_string = dependency_filename + ";" + 
                                               dependency_type_map[dependency_filename];
          unimportant_resources.push_back(unimportant_resource_string);
        }
      }
    }

    if (link_resources.size() > 0) {
      string link_string_value = "";
      for (auto it = link_resources.begin(); it != link_resources.end(); ++it) {
        link_string_value += *it + ", ";
      }
      string link_string = "Link: " + link_string_value.substr(0, link_string_value.size() - 2);
      response.add_header_after_parsing(link_string);
    }
    if (unimportant_resources.size() > 0) {
      string unimportant_resource_value = "";
      for (auto it = unimportant_resources.begin(); it != unimportant_resources.end(); ++it) {
        unimportant_resource_value += *it + "|";
      }
      string x_systemname_unimportant_resource_string = "x-systemname-unimportant: " + unimportant_resource_value.substr(0, unimportant_resource_value.size() - 1);
      response.add_header_after_parsing(x_systemname_unimportant_resource_string);
    }
  }
}

int check_redirect( MahimahiProtobufs::RequestResponse saved_record, int previous_score ) {
    HTTPRequest saved_request( saved_record.request() );
    HTTPResponse saved_response( saved_record.response() );

    if ( previous_score == 0 )  {
      return previous_score;
    }

    /* check response code */
    string response_first_line = saved_response.first_line();
    vector< string > splitted_response_first_line = split(response_first_line, ' ');
    string status_code = splitted_response_first_line[1];
    if ( status_code != "301" && status_code != "302" ) {
      // Not redirection.
      return previous_score;
    }

    /* Check the response with the redirected location. */
    string request_first_line = saved_request.first_line();
    vector< string > splitted_request_first_line = split(request_first_line, ' ');
    string request_url = splitted_request_first_line[1]; // This shouldn't contain the HOST
    if ( !saved_response.has_header("Location"))
      return previous_score;

    string redirected_location = saved_response.get_header_value("Location");
    string redirected_location_host = extract_hostname(redirected_location);
    string path = strip_hostname( redirected_location, request_url );

    if ( path == request_url && redirected_location_host == saved_request.get_header_value("Host") ) {
      return 0;
    }
    return previous_score;
}

int main( void )
{
    try {
        assert_not_root();

        const string working_directory = safe_getenv( "MAHIMAHI_CHDIR" );
        const string recording_directory = safe_getenv( "MAHIMAHI_RECORD_PATH" );
        const string current_loading_page = safe_getenv( "LOADING_PAGE" );
        const string dependency_filename = safe_getenv( "DEPENDENCY_FILE" );
        const string path = safe_getenv( "REQUEST_URI" );
        const string request_line = safe_getenv( "REQUEST_METHOD" )
            + " " + path
            + " " + safe_getenv( "SERVER_PROTOCOL" );
        const bool is_https = getenv( "HTTPS" );

        SystemCall( "chdir", chdir( working_directory.c_str() ) );

        const vector< string > files = list_directory_contents( recording_directory );

        unsigned int best_score = 0;
        MahimahiProtobufs::RequestResponse best_match;

        for ( const auto & filename : files ) {
            FileDescriptor fd( SystemCall( "open", open( filename.c_str(), O_RDONLY ) ) );
            MahimahiProtobufs::RequestResponse current_record;
            if ( not current_record.ParseFromFileDescriptor( fd.fd_num() ) ) {
                throw runtime_error( filename + ": invalid HTTP request/response" );
            }

            unsigned int score = match_score( current_record, request_line, is_https );
            if ( score > best_score ) {
                best_match = current_record;
                best_score = score;
            }
        }

        best_score = check_redirect(best_match, best_score);

        if ( best_score > 0 ) { /* give client the best match */
            HTTPRequest request( best_match.request() );
            HTTPResponse response( best_match.response() );

            /* Remove all cache-related headers. */
            vector< string > headers = { "Cache-control", 
                                         "Expires",
                                         "Last-modified",
                                         "Date",
                                         "Age",
                                         "Etag" };
            for ( auto it = headers.begin(); it != headers.end(); ++it ) {
                response.remove_header( *it );
            }

            /* Add the cache-control header and set to 3600. */
            response.add_header_after_parsing( "Cache-Control: max-age=60" );
            // response.add_header_after_parsing( "Cache-Control: no-cache, no-store, must-revalidate max-age=0" );

            if (dependency_filename != "None") {
              string scheme = is_https ? "https://" : "http://";
              string request_url = scheme + request.get_header_value("Host") + path;
              populate_push_configurations(dependency_filename,
                                           request_url,
                                           response,
                                           current_loading_page);
            }

            if (!response.has_header("Access-Control-Allow-Origin")) {
              response.add_header_after_parsing( "Access-Control-Allow-Origin: *" );
            }

            cout << response.str();
            return EXIT_SUCCESS;
        } else {                /* no acceptable matches for request */
            // string response_body = "replayserver: could not find a match for " + request_line;
            string response_body = "replayserver: could not find a match.";
            cout << "HTTP/1.1 404 Not Found" << CRLF;
            cout << "Content-Type: text/plain" << CRLF;
            cout << "Content-Length: " << response_body.length() << CRLF;
            cout << "Cache-Control: max-age=60" << CRLF << CRLF;
            cout << response_body << CRLF;
            return EXIT_FAILURE;
        }
    } catch ( const exception & e ) {
        cout << "HTTP/1.1 500 Internal Server Error" << CRLF;
        cout << "Content-Type: text/plain" << CRLF << CRLF;
        cout << "mahimahi mm-webreplay received an exception:" << CRLF << CRLF;
        print_exception( e, cout );
        return EXIT_FAILURE;
    }
}
