AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = delayshell
delayshell_SOURCES = exception.hh ezio.cc ezio.hh delayshell.cc \
	ferry_queue.cc ferry_queue.hh \
	file_descriptor.hh tundevice.cc tundevice.hh timestamp.cc timestamp.hh \
	child_process.hh child_process.cc signalfd.hh signalfd.cc \
	socket.cc socket.hh address.cc address.hh \
	ferry.hh ferry.cc system_runner.hh system_runner.cc nat.hh \
	socket_type.hh util.hh util.cc dns_proxy.hh dns_proxy.cc
delayshell_LDADD = -lrt
install-exec-hook:
	chown root $(bindir)/delayshell
	chmod u+s $(bindir)/delayshell
