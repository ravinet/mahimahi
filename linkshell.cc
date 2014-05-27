/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "link_queue.hh"
#include "packetshell.cc"
#include "temp_file.hh"

using namespace std;

/* return hostname by iterating through headers */
string get_host( HTTP_Record::reqrespair & current_record )
{
    for ( int i = 0; i < current_record.req().headers_size(); i++ ) {
        HTTPHeader current_header( current_record.req().headers(i) );
        if ( current_header.key() == "Host" ) {
            return current_header.value().substr( 0, current_header.value().find( "\r\n" ) );
        }
    }
    throw Exception( "replayshell", "No Host Header in Recorded File" );
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 3 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " uplink downlink" );
        }

        const std::string uplink_filename = argv[ 1 ];
        const std::string downlink_filename = argv[ 2 ];

        /* check if user-specified storage folder exists */
        string directory = argv[3];
        /* make sure directory ends with '/' so we can prepend directory to file name for storage */
        if ( directory.back() != '/' ) {
            directory.append( "/" );
        }

        if ( not check_folder_existence( directory ) ) { /* folder does not exist */
            throw Exception( "replayshell", "folder with recorded content does not exist" );
        }

        TempFile dnsmasq_hosts( "hosts" );

        vector< string > files;
        list_files( directory, files );

        for ( unsigned int i = 0; i < files.size(); i++ ) {
            string bulk_file = directory + "bulkreply.proto";
            if ( files[i] != bulk_file ) {
            FileDescriptor response( SystemCall( "open", open( files[i].c_str(), O_RDONLY ) ) );
            HTTP_Record::reqrespair current_record;
            current_record.ParseFromFileDescriptor( response.num() );
            Address current_addr( current_record.ip(), current_record.port() );

            /* add entry to dnsmasq host mapping file */
            string entry_host = get_host( current_record );
            dnsmasq_hosts.write( current_addr.ip() + " " +entry_host + "\n" );
        }
        }

        PacketShell<LinkQueue> link_shell_app( "link" );

        link_shell_app.start_uplink( "[link, up=" + uplink_filename + ", down=" + downlink_filename + "] ",
                                     user_environment, dnsmasq_hosts.name(), uplink_filename );
        link_shell_app.start_downlink( downlink_filename );
        return link_shell_app.wait_for_exit();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
