AM_CPPFLAGS = $(CXX11_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

noinst_LIBRARIES = libutil.a

libutil_a_SOURCES = exception.hh ezio.cc ezio.hh                               \
        file_descriptor.hh file_descriptor.cc netdevice.cc netdevice.hh        \
	timestamp.cc timestamp.hh                                              \
        child_process.hh child_process.cc signalfd.hh signalfd.cc              \
        socket.cc socket.hh address.cc address.hh                              \
        system_runner.hh system_runner.cc nat.hh nat.cc                        \
        util.hh util.cc dns_proxy.hh dns_proxy.cc                              \
        interfaces.hh interfaces.cc                                            \
        poller.hh poller.cc bytestream_queue.hh bytestream_queue.cc            \
        event_loop.hh event_loop.cc                                            \
        temp_file.hh temp_file.cc dns_server.hh dns_server.cc                  \
        socketpair.hh socketpair.cc autosocket.hh
