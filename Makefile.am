AM_CPPFLAGS = -I./protobufs
AM_CXXFLAGS = $(PICKY_CXXFLAGS)
SUBDIRS = protobufs
common_source = exception.hh ezio.cc ezio.hh                                   \
	file_descriptor.hh netdevice.cc netdevice.hh timestamp.cc timestamp.hh \
	child_process.hh child_process.cc signalfd.hh signalfd.cc              \
	socket.cc socket.hh address.cc address.hh                              \
	system_runner.hh system_runner.cc nat.hh nat.cc                        \
	socket_type.hh util.hh util.cc dns_proxy.hh dns_proxy.cc               \
	get_address.hh get_address.cc                                          \
	poller.hh poller.cc bytestream_queue.hh bytestream_queue.cc            \
	read_write_interface.hh

bin_PROGRAMS = delayshell
delayshell_SOURCES = $(common_source) delayshell.cc ferry.hh ferry.cc \
	delay_queue.hh delay_queue.cc packetshell.cc packetshell.hh make_pipe.hh
delayshell_LDADD = -lrt
delayshell_LDFLAGS = -pthread

bin_PROGRAMS += recordshell
recordshell_SOURCES = $(common_source) recordshell.cc \
	http_proxy.hh http_proxy.cc \
	http_request_parser.hh \
	http_request.hh http_request.cc \
	http_response.hh http_response.cc \
	http_header.hh http_header.cc \
	http_response_parser.hh http_response_parser.cc \
	tokenize.hh mime_type.hh mime_type.cc \
	body_parser.hh \
	chunked_parser.hh chunked_parser.cc \
	http_message.hh http_message.cc \
	http_message_sequence.hh \
	secure_socket.hh secure_socket.cc \
	certificate.hh
recordshell_LDADD = -lrt
recordshell_LDADD += protobufs/libhttprecordprotos.a -lm $(protobuf_LIBS)
recordshell_LDADD += -lssl -lcrypto -ldl
recordshell_LDFLAGS = -pthread

install-exec-hook:
	chown root $(DESTDIR)$(bindir)/delayshell
	chmod u+s $(DESTDIR)$(bindir)/delayshell
	chown root $(DESTDIR)$(bindir)/recordshell
	chmod u+s $(DESTDIR)$(bindir)/recordshell
