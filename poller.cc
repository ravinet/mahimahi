/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "poller.hh"
#include "util.hh"

using namespace std;

void Poller::add_action( Poller::Action action )
{
    pollfds_.push_back( { action.fd.num(), action.direction, 0 } );
    callbacks_.push_back( action.callback );
}

Poller::Result Poller::poll( const int & timeout_ms )
{
    const int poll_return = ::poll( &pollfds_[ 0 ], pollfds_.size(), timeout_ms );

    if ( poll_return < 0 ) {
        throw Exception( "poll" );
    } else if ( poll_return == 0 ) {
        return Result::Type::Timeout;
    }

    for ( unsigned int i = 0; i < pollfds_.size(); i++ ) {
        if ( pollfds_[ i ].revents & (POLLERR | POLLHUP | POLLNVAL) ) {
            throw Exception( "poll fd error" );
        }

        if ( pollfds_[ i ].revents & pollfds_[ i ].events ) {
            /* we only want to call callback if revents includes
               the event we asked for */
            auto result = callbacks_[ i ]();
            if ( result.result == Action::Result::Type::Exit ) {
                return Result( Result::Type::Exit, result.exit_status );
            }
        }
    }

    return Result::Type::Success;
}
