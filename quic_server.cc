/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <iostream>

#include "config.h"
#include "quic_server.hh"

using namespace std;

QuicServer::QuicServer( const Address & addr, const string & record_folder, const string & )
    : quic_server_process_( [&] () {
        const string ip_flag = "--ip=" + addr.ip();
        const string port_flag = "--port=" + to_string(addr.port());
        const string record_folder_flag = "--record_folder=" + record_folder;
        const string replay_server_flag = "--replay_server=" + string(REPLAYSERVER);

        cout << "quic_server started: " << ip_flag << " " << port_flag << endl;
        SystemCall( "execl", execl( QUIC_SERVER, QUIC_SERVER, ip_flag.c_str(), port_flag.c_str(),
                                    record_folder_flag.c_str(), replay_server_flag.c_str(),
                                    static_cast<char *>( nullptr ) ) );
        return EXIT_FAILURE;
    } )
{
}

QuicServer::~QuicServer()
{
}

QuicServer::QuicServer( QuicServer && other )
    : quic_server_process_( move( other.quic_server_process_ ) )
{
}
