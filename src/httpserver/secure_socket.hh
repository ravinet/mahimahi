/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* run: sudo apt-get install libssl-dev */

#ifndef SECURE_SOCKET_HH
#define SECURE_SOCKET_HH

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "socket.hh"

enum SSL_MODE { CLIENT, SERVER };

class SecureSocket : public TCPSocket
{
    friend class SSLContext;

private:
    struct SSL_deleter { void operator()( SSL * x ) const { SSL_free( x ); } };
    typedef std::unique_ptr<SSL, SSL_deleter> SSL_handle;
    SSL_handle ssl_;

    SecureSocket( TCPSocket && sock, SSL * ssl );

public:
    void connect( void );
    void accept( void );

    std::string read( void );
    void write( const std::string & message );

    /* if an SNI servername has been received, assigns it to servername and returns true;
     * returns false otherwise */
    bool get_sni_servername( std::string & servername );
    /* make the socket send the specified servername when connecting to a server */
    void set_sni_servername_sent( const std::string & servername );
};

class SSLContext
{
private:
    struct CTX_deleter { void operator()( SSL_CTX * x ) const { SSL_CTX_free( x ); } };
    typedef std::unique_ptr<SSL_CTX, CTX_deleter> CTX_handle;
    CTX_handle ctx_;

public:
    SSLContext( const SSL_MODE type );

    SecureSocket new_secure_socket( TCPSocket && sock );
};

#endif
