AM_CXXFLAGS = $(PICKY_CXXFLAGS)

common_source = exception.hh ezio.cc ezio.hh                                   \
	file_descriptor.hh netdevice.cc netdevice.hh timestamp.cc timestamp.hh \
	child_process.hh child_process.cc signalfd.hh signalfd.cc              \
	socket.cc socket.hh address.cc address.hh                              \
	system_runner.hh system_runner.cc nat.hh nat.cc                        \
	socket_type.hh util.hh util.cc dns_proxy.hh dns_proxy.cc               \
	get_address.hh get_address.cc                                          \
	poller.hh poller.cc bytestream_queue.hh bytestream_queue.cc

bin_PROGRAMS = delayshell
delayshell_SOURCES = $(common_source) delayshell.cc ferry.hh ferry.cc \
	ferry_queue.hh ferry_queue.cc
delayshell_LDADD = -lrt
delayshell_LDFLAGS = -pthread

bin_PROGRAMS += recordshell
recordshell_SOURCES = $(common_source) recordshell.cc \
	http_proxy.hh http_proxy.cc
recordshell_LDADD = -lrt
recordshell_LDFLAGS = -pthread

install-exec-hook:
	chown root $(DESTDIR)$(bindir)/delayshell
	chmod u+s $(DESTDIR)$(bindir)/delayshell
	chown root $(DESTDIR)$(bindir)/recordshell
	chmod u+s $(DESTDIR)$(bindir)/recordshell
