/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "http_memory_store.hh"
#include "http_record.pb.h"
#include "file_descriptor.hh"
#include "http_header.hh"
#include "int32.hh"
#include "file_descriptor.hh"
#include "google/protobuf/io/gzip_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "http_message.hh"
#include "webp/encode.h"
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "webp/decode.h"

using namespace std;
//using namespace cv;

HTTPMemoryStore::HTTPMemoryStore()
    : mutex_(),
      requests(),
      responses()
{}

void HTTPMemoryStore::save( const HTTPResponse & response, const Address & server_address __attribute__ ( ( unused ) ) )
{
    unique_lock<mutex> ul( mutex_ );

    /* Add the current request to requests BulkMessage and current response to responses BulkMessage */
    requests.add_msg()->CopyFrom( response.request().toprotobuf() );
    if ( response.has_header("Content-Type") ) {
        if ( HTTPMessage::equivalent_strings(response.get_header_value("Content-Type"), "image/jpeg") || HTTPMessage::equivalent_strings(response.get_header_value("Content-Type"), "image/png") ) {
            /* create new protobuf after converting image to webP format (also must change content length) */
            MahimahiProtobufs::HTTPMessage converted;
            MahimahiProtobufs::HTTPMessage original = response.toprotobuf();

            /* convert body to webp and get new content length */
            cv::Mat img = cv::imread(original.body().c_str() );
            uint8_t* output;
            //auto datasize = WebPEncodeRGB((uint8_t*)(uchar *)img->imageData, img->width, img->height, img->widthStep, 0.9, &output);
            auto datasize = WebPEncodeRGB(img.data, img.size().width, img.size().height, img.step, 0.9, &output);

            //string o( (char *)output, (int)datasize );
            converted.set_first_line( original.first_line() );
            converted.set_body( reinterpret_cast<char*>(output) );
            /* copy all headers except Content-Type (change it to webP) */
            for ( int i = 0; i < original.header_size(); i++ ) {
                HTTPHeader current_header( original.header(i) );
                if ( HTTPMessage::equivalent_strings( current_header.key(), "Content-Type" ) ) {
                    /* this is the value we want to change */
                    string new_vals = current_header.key() + ": " + "image/webp,*/*;q=0.8";
                    HTTPHeader new_header( new_vals );
                    converted.add_header()->CopyFrom( new_header.toprotobuf() );
                } else if ( HTTPMessage::equivalent_strings( current_header.key(), "Content-Length" ) ) {
                    /* this is the value we want to change */
                    string new_vals = current_header.key() + ": " + to_string(datasize);
                    HTTPHeader new_header( new_vals );
                    converted.add_header()->CopyFrom( new_header.toprotobuf() );
                } else {
                    converted.add_header()->CopyFrom( current_header.toprotobuf() );
                }
            }
            responses.add_msg()->CopyFrom( converted );
        } else {
            responses.add_msg()->CopyFrom( response.toprotobuf() );
        }
    } else {
        responses.add_msg()->CopyFrom( response.toprotobuf() );
    }
}

void HTTPMemoryStore::serialize_to_socket( Socket && client )
{
    /* bulk response format:
       length-value pair for requests protobuf,
       for each response:
         length-value pair for each response
     */
    unique_lock<mutex> ul( mutex_ );

    /* make sure there is a 1 to 1 matching of requests and responses */
    assert( requests.msg_size() == responses.msg_size() );

    if ( responses.msg_size() == 0 ) {
        const string error_message =
         "HTTP/1.1 404 Not Found" + CRLF +
         "Content-Type: text/plain" + CRLF + CRLF;
        client.write( static_cast<string>( Integer32( error_message.size() ) ) + error_message );
        return;
    }

    /* Serialize all requests alone to a length-value pair for transmission */
    string all_requests;
    {
        google::protobuf::io::GzipOutputStream::Options options;
        options.format = google::protobuf::io::GzipOutputStream::GZIP;
        options.compression_level = 9;
        google::protobuf::io::StringOutputStream compressedStream( &all_requests );
        google::protobuf::io::GzipOutputStream compressingStream( &compressedStream, options );
        requests.SerializeToZeroCopyStream( &compressingStream );
    }
    all_requests = static_cast<string>( Integer32( all_requests.size() ) ) + all_requests;

    /* Now make a set of length-value pairs, one for each response */
    string all_responses;
    for ( int i = 0; i < responses.msg_size(); i++ ) {
        /* add response string size and response string */
        string current_response;
        {
            google::protobuf::io::GzipOutputStream::Options options;
            options.format = google::protobuf::io::GzipOutputStream::GZIP;
            options.compression_level = 9;
            google::protobuf::io::StringOutputStream compressedStream( &current_response );
            google::protobuf::io::GzipOutputStream compressingStream( &compressedStream, options );
            responses.msg(i).SerializeToZeroCopyStream( &compressingStream );
        }
        all_responses = all_responses + static_cast<string>( Integer32( current_response.size() ) ) + current_response;
    }

    client.write( all_requests + all_responses );
}
