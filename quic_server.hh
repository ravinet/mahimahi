/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef QUIC_SERVER_HH
#define QUIC_SERVER_HH

#include <string>

#include "address.hh"
#include "child_process.hh"
#include "exception.hh"

class QuicServer
{
private:
    ChildProcess quic_server_process_;

public:
    QuicServer( const Address & addr, const std::string & record_folder, const std::string & user );
    ~QuicServer();

    QuicServer( QuicServer && other );
};

#endif
