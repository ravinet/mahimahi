/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef PROCESS_RECORDER_HH
#define PROCESS_RECORDER_HH

#include <string>
#include <functional>

#include "file_descriptor.hh"
#include "socket.hh"

/* class that loads an arbitrary process and captures all HTTP traffic originating from it */

template <class TargetType>
class ProcessRecorder
{
public:
    ProcessRecorder() {}

    template <typename... Targs>
    int record_process( std::function<int( FileDescriptor & )> && child_procedure,
                        Socket && socket_output,
                        const int & veth_counter, 
                        bool record,
                        const std::string & stdin_input,
                        Targs... Fargs );
};

#endif /* PROCESS_RECORDER_HH */
