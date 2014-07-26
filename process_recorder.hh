/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef PROCESS_RECORDER_HH
#define PROCESS_RECORDER_HH

#include <string>
#include <functional>

#include "file_descriptor.hh"

/* class that loads an arbitrary process and captures all HTTP traffic originating from it */

class ProcessRecorder
{
public:
    ProcessRecorder();
    int record_process( std::function<int( FileDescriptor & )> && child_procedure,
                        const int & veth_counter = 0,
                        const std::string & stdin_input = "" );
};

#endif /* PROCESS_RECORDER_HH */
