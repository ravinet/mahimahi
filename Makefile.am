AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = hello
hello_SOURCES = hello.cc

bin_PROGRAMS += unsharenet
unsharenet_SOURCES = unsharenet.cc

bin_PROGRAMS += forktest
forktest_SOURCES = forktest.cc

bin_PROGRAMS += ferry
ferry_SOURCES = exception.hh ezio.cc ezio.hh ferry.cc ferry_queue.cc ferry_queue.hh \
	file_descriptor.hh tapdevice.cc tapdevice.hh timestamp.cc timestamp.hh
ferry_LDADD = -lrt

bin_PROGRAMS += forked-ferry
forked_ferry_SOURCES = exception.hh ezio.cc ezio.hh forked-ferry.cc \
	ferry_queue.cc ferry_queue.hh \
	file_descriptor.hh tapdevice.cc tapdevice.hh timestamp.cc timestamp.hh \
	child_process.hh child_process.cc signalfd.hh signalfd.cc \
	terminal_saver.hh
forked_ferry_LDADD = -lrt
