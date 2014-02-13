/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "secure_socket.hh"
#include "certificate.hh"

#include <fstream>
#include <string>
#include <iostream>
#include <cassert>

using namespace std;

SecureSocket::SecureSocket( Socket && sock, SSL_MODE type )
    : underlying_socket( move( sock ) ),
      ctx(),
      ssl_connection(),
      mode( type )
{
    /* create client/server supporting TLS version 1 */
    if ( mode == CLIENT ) { /* client */
        ctx = SSL_CTX_new( TLSv1_client_method() );
    } else { /* server */
        ctx = SSL_CTX_new( TLSv1_server_method() );

        if ( SSL_CTX_use_certificate_ASN1( ctx, 678, certificate ) < 1 ) {
            throw Exception( "SSL_CTX_use_certificate_ASN1", ERR_error_string( ERR_get_error(), nullptr ) );
        }

        if ( SSL_CTX_use_RSAPrivateKey_ASN1( ctx, private_key, 1191 ) < 1 ) {
            throw Exception( "SSL_CTX_use_RSAPrivateKey_ASN1", ERR_error_string( ERR_get_error(), nullptr ) );
        }

        /* check consistency of private key with loaded certificate */
        if ( SSL_CTX_check_private_key( ctx ) < 1 ) {
            throw Exception( "SSL_CTX_check_private_key", ERR_error_string( ERR_get_error(), nullptr ) );
        }
    }
    if ( ctx == NULL ) {
        throw Exception( "SSL_CTX_new", ERR_error_string( ERR_get_error(), nullptr ) );
    }

    if ( (ssl_connection = SSL_new( ctx ) ) == NULL ) {
        throw Exception( "SSL_new", ERR_error_string( ERR_get_error(), nullptr ) );
    }

    if ( SSL_set_fd( ssl_connection, underlying_socket.fd().num() ) < 1 ) {
        throw Exception( "SSL_set_fd", ERR_error_string( ERR_get_error(), nullptr ) );
    }

    /* enable read/write to return only after handshake/renegotiation and successful completion */
    SSL_set_mode( ssl_connection, SSL_MODE_AUTO_RETRY );

    handshake();
}

void SecureSocket::handshake( void )
{
    if ( mode == CLIENT ) { /* client-initiate handshake */
        if ( SSL_connect( ssl_connection ) < 1 ) {
            throw Exception( "SSL_connect", ERR_error_string( ERR_get_error(), nullptr ) );
        }
    } else { /* server-finish handshake */
        if ( SSL_accept( ssl_connection ) < 1 ) {
            throw Exception( "SSL_accept", ERR_error_string( ERR_get_error(), nullptr ) );
        }
    }
}

void SecureSocket::check_server_certificate( void )
{
    X509 *server_certificate;

    server_certificate = SSL_get_peer_certificate( ssl_connection );

    if ( SSL_get_verify_result( ssl_connection ) == X509_V_OK ) { /* verification succeeded of no certificate presented */
        X509_free( server_certificate );
    } else { /* verification failed */
        throw Exception( "SSL_get_verify_result", ERR_error_string( SSL_get_verify_result( ssl_connection ), nullptr ) );
    }
}

string SecureSocket::read( void )
{
    /* SSL record max size is 16kB */
    const size_t SSL_max_record_length = 16384;

    char buffer[ SSL_max_record_length ];

    ssize_t bytes_read = SSL_read( ssl_connection, &buffer, SSL_max_record_length );

    /* Make sure that we really are reading from the underlying fd */
    assert( 0 == SSL_pending( ssl_connection ) );

    if ( bytes_read == 0 ) {
        int error_return = SSL_get_error( ssl_connection, bytes_read );
        if ( SSL_ERROR_ZERO_RETURN == error_return ) { /* Clean SSL close */
            underlying_socket.fd().set_eof();
        } else if ( SSL_ERROR_SYSCALL == error_return ) { /* Underlying TCP connection close */
            /* Verify error queue is empty so we can conclude it is EOF */
            assert( ERR_get_error() == 0 );
            underlying_socket.fd().set_eof();
        }
        return string(); /* EOF */
    } else if ( bytes_read < 0 ) {
        throw Exception( "SSL_read", ERR_error_string( SSL_get_error( ssl_connection, bytes_read ), nullptr ) );
    } else {
        /* success */
        return string( buffer, bytes_read );
    }
}

void SecureSocket::write(const string & message )
{
    /* SSL_write returns with success if complete contents of message are written */
    ssize_t bytes_written = SSL_write( ssl_connection, message.c_str(), message.length() );

    if ( bytes_written < 0 ) {
        throw Exception( "SSL_write", ERR_error_string( SSL_get_error( ssl_connection, bytes_written ), nullptr ) );
    }
}
