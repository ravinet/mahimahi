AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = delayshell
delayshell_SOURCES = exception.hh ezio.cc ezio.hh delayshell.cc \
	ferry_queue.cc ferry_queue.hh \
	file_descriptor.hh tapdevice.cc tapdevice.hh timestamp.cc timestamp.hh \
	child_process.hh child_process.cc signalfd.hh signalfd.cc \
	ferry.hh ferry.cc system_runner.hh nat.hh
delayshell_LDADD = -lrt
