/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include "secure_socket.hh"
#include "exception.hh"

#include <fstream>
#include <string>
#include <iostream>
#include <cassert>

using namespace std;

Secure_Socket::Secure_Socket( Socket && sock, SSL_MODE type, const string & cert )
    : underlying_socket( move( sock ) ),
      ctx(),
      ssl_connection(),
      certificate( cert ),
      mode( type )
{
    /* initialize context object for client/server TLS version 1 */
    if ( mode == CLIENT ) { /* client */
        ctx = SSL_CTX_new( TLSv1_client_method() );
        /* set locations for ctx where CA certificates are located */
        if ( SSL_CTX_load_verify_locations( ctx, certificate.c_str(), NULL ) < 1 ) {
            throw Exception( "SSL_CTX_load_verify_locations", ERR_error_string( ERR_get_error(), nullptr ) );
        }
    } else { /* server */
        ctx = SSL_CTX_new( TLSv1_server_method() );
        /* loads first certificate stored in certificate file (PEM format) into ssl ctx object */
        if ( SSL_CTX_use_certificate_file( ctx, certificate.c_str(), SSL_FILETYPE_PEM ) < 1 ) {
            throw Exception( "SSL_CTX_use_certificate_file", ERR_error_string( ERR_get_error(), nullptr ) );
        }

        /* load first private key from file with key to ssl ctx object */
        if ( SSL_CTX_use_PrivateKey_file( ctx, certificate.c_str(), SSL_FILETYPE_PEM ) < 1 ) {
            throw Exception( "SSL_CTX_use_PrivateKey_file", ERR_error_string( ERR_get_error(), nullptr ) );
        }

        /* checks consistency of private key with loaded certificate */
        if ( SSL_CTX_check_private_key( ctx ) < 1 ) {
            throw Exception( "SSL_CTX_check_private_key", ERR_error_string( ERR_get_error(), nullptr ) );
        }
    }
    if ( ctx == NULL ) {
        throw Exception( "SSL_CTX_new", ERR_error_string( ERR_get_error(), nullptr ) );
    }

    /* create new SSL structure */
    if ( (ssl_connection = SSL_new( ctx ) ) == NULL ) {
        throw Exception( "SSL_new", ERR_error_string( ERR_get_error(), nullptr ) );
    }

    /* connect SSL structure with file descriptor for socket connected to new client */
    if ( SSL_set_fd( ssl_connection, underlying_socket.fd().num() ) < 1 ) {
        throw Exception( "SSL_set_fd", ERR_error_string( ERR_get_error(), nullptr ) );
    }
    SSL_set_mode( ssl_connection, SSL_MODE_AUTO_RETRY );
    handshake();
}

void Secure_Socket::handshake( void )
{
    if ( mode == CLIENT ) { /* client-initiate handshake */
        if ( SSL_connect( ssl_connection ) < 1 ) {
            throw Exception( "SSL_connect", ERR_error_string( ERR_get_error(), nullptr ) );
        } else { /* SSL handshake complete */
            /* check server certificates */
            check_server_certificate();
        }
    } else { /* server-finish handshake */
        if ( SSL_accept( ssl_connection ) < 1 ) {
            throw Exception( "SSL_accept", ERR_error_string( ERR_get_error(), nullptr ) );
        }
    }
}

void Secure_Socket::check_server_certificate( void ) //const string expected_host )
{
    X509 *server_certificate;
    //char peer_common_name[ 256 ];

    /* get certificate presented by server (peer). if none, then NULL */
    server_certificate = SSL_get_peer_certificate( ssl_connection );

    /* if certificate is presented, certificate verification is done during the SSL handshake with SSL_connect() and we use SSL_get_verify_result() to get the verification result */
    if ( server_certificate != NULL ) { /* server presented certificate */
        cout << "Certificate presented by server" << endl;
        if ( SSL_get_verify_result( ssl_connection ) == X509_V_OK ) { /* certificate verified, check if server name matches expected name */
            /*X509_NAME_get_text_by_NID( X509_get_subject_name( server_certificate ), NID_commonName, peer_common_name, 256 );
            if( strcasecmp( peer_common_name, expected_host.c_str() ) != 0 ) {
                cout << "Common name doesn't match expected host name: " << peer_common_name << endl;
            }*/
            cout << "Successfully verified server" << endl;
            X509_free( server_certificate );
        } else { /* verify result did not return X509_V_OK */
            cout << "Could not successfully verify server certificate. Error: " << SSL_get_verify_result( ssl_connection ) << endl;
        }
    } else { /* server did not present certificate */
        cout << "No certificates" << endl;
    }
}

string Secure_Socket::read( void )
{
    /* SSL record max size is 16kB */
    const size_t SSL_max_record_length = 16384;

    char buffer[ SSL_max_record_length ];

    ssize_t bytes_read = SSL_read( ssl_connection, &buffer, SSL_max_record_length );

    /* Make sure that we really are reading from the underlying fd */
    assert( 0 == SSL_pending( ssl_connection ) );

    if ( bytes_read == 0 ) {
        return string(); /* EOF */
    } else if ( bytes_read < 0 ) {
        throw Exception( "SSL_read", ERR_error_string( SSL_get_error( ssl_connection, bytes_read ), nullptr ) );
    } else {
        /* success */
        return string( buffer, bytes_read );
    }
}

void Secure_Socket::write(const string & message )
{
    string to_be_written = message;
    size_t total_written = 0;
    while ( total_written < message.size() ) { /* write until we have written entire message or connection closed/has errors */
        int amount_written;
        if ( ( amount_written = SSL_write( ssl_connection, to_be_written.c_str(), to_be_written.length() ) ) > 0 ) { /* we wrote some data...wrote successful */
            total_written = total_written + amount_written;
            to_be_written.replace( 0, amount_written, string() );
        } else if ( amount_written < 0 ) { /* write unsuccessful...check error (should not be WANT_READ/WANT_WRITE because we set SSL_MODE_AUTO_RETRY) */
            assert( SSL_get_error( ssl_connection, amount_written ) != SSL_ERROR_WANT_READ );
            assert( SSL_get_error( ssl_connection, amount_written ) != SSL_ERROR_WANT_WRITE );
            throw Exception( "SSL_write", ERR_error_string( SSL_get_error( ssl_connection, amount_written ), nullptr ) );
        } else { /* write unsuccessful either due shutdown (either clean or incomplete shutdown) */
            return;
        }
    }
}
