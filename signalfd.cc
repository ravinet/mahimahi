/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <csignal>
#include <cstring>

#include "signalfd.hh"

using namespace std;

/* add given signals to the mask */
SignalMask::SignalMask( const initializer_list< int > signals )
    : mask_()
{
    if ( sigemptyset( &mask_ ) < 0 ) {
        throw Exception( "sigemptyset" );
    }

    for ( const auto signal : signals ) {
        if ( sigaddset( &mask_, signal ) < 0 ) {
            throw Exception( "sigaddset" );
        }
    }
}

/* block these signals from interrupting our process */
/* (because we'll use a signalfd instead to read them */
void SignalMask::block( void ) const
{
    if ( sigprocmask( SIG_BLOCK, &mask(), NULL ) < 0 ) {
        throw Exception( "sigprocmask" );
    }
}

SignalFD::SignalFD( const SignalMask & signals )
    : fd_( signalfd( -1, &signals.mask(), 0 ), "signalfd" )
{
}

/* read one signal */
signalfd_siginfo SignalFD::read_signal( void ) const
{
    signalfd_siginfo delivered_signal;

    string delivered_signal_str = fd().read( sizeof( signalfd_siginfo ) );

    if ( delivered_signal_str.size() != sizeof( signalfd_siginfo ) ) {
        throw Exception( "signalfd read size mismatch" );
    }

    memcpy( &delivered_signal, delivered_signal_str.data(), sizeof( signalfd_siginfo ) );

    return delivered_signal;
}
