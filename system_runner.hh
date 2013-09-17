/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef SYSTEM_RUNNER
#define SYSTEM_RUNNER

#include <string>
#include <unistd.h>

#include "exception.hh"

void run( const std::string & command )
{
    ChildProcess command_process( [&] () -> int {
            if ( execl( "/bin/sh", "/bin/sh", "-c", command.c_str(), static_cast<char *>( nullptr ) ) < 0 ) {
                throw Exception( "execl" );
            }
            return EXIT_FAILURE;
        } );

    while ( !command_process.terminated() ) {
        command_process.wait();
    }

    if ( command_process.exit_status() != 0 ) {
        throw Exception( "`" + command + "'", "command returned "
                         + std::to_string( command_process.exit_status() ) );
    }
}

#endif /* SYSTEM_RUNNER */
