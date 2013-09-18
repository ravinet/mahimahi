AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = hello
hello_SOURCES = hello.cc

bin_PROGRAMS += unsharenet
unsharenet_SOURCES = unsharenet.cc

bin_PROGRAMS += forktest
forktest_SOURCES = forktest.cc

bin_PROGRAMS += simple-ferry
simple_ferry_SOURCES = exception.hh ezio.cc ezio.hh simple-ferry.cc ferry_queue.cc ferry_queue.hh \
		file_descriptor.hh tapdevice.cc tapdevice.hh timestamp.cc timestamp.hh
simple_ferry_LDADD = -lrt

bin_PROGRAMS += delayshell
delayshell_SOURCES = exception.hh ezio.cc ezio.hh delayshell.cc \
	ferry_queue.cc ferry_queue.hh \
	file_descriptor.hh tapdevice.cc tapdevice.hh timestamp.cc timestamp.hh \
	child_process.hh child_process.cc signalfd.hh signalfd.cc \
	ferry.hh ferry.cc system_runner.hh nat.hh
delayshell_LDADD = -lrt
