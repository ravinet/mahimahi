AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = delayshell
delayshell_SOURCES = exception.hh ezio.cc ezio.hh delayshell.cc \
	ferry_queue.cc ferry_queue.hh \
	file_descriptor.hh tundevice.cc tundevice.hh timestamp.cc timestamp.hh \
	child_process.hh child_process.cc signalfd.hh signalfd.cc \
	socket.cc socket.hh address.cc address.hh \
	ferry.hh ferry.cc system_runner.hh nat.hh
delayshell_LDADD = -lrt
bin_PROGRAMS += dnsoutsideproxy
dnsoutsideproxy_SOURCES = address.hh address.cc socket.cc socket.hh ezio.cc \
		ezio.hh dnsoutsideproxy.cc timestamp.cc exception.hh file_descriptor.hh \
		timestamp.hh
dnsoutsideproxy_LDADD = -lrt
bin_PROGRAMS += dnsinsideproxy
dnsinsideproxy_SOURCES = address.hh address.cc socket.cc socket.hh ezio.cc \
                ezio.hh dnsinsideproxy.cc timestamp.cc exception.hh file_descriptor.hh \
                timestamp.hh
dnsinsideproxy_LDADD = -lrt
