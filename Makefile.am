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
	read_write_interface.hh                                                \
        cell_queue.hh cell_queue.cc                                            \
        delay_queue.hh delay_queue.cc

packet_shell_source = ferry.hh ferry.cc packetshell.cc packetshell.hh make_pipe.hh

bin_PROGRAMS = mpshell
mpshell_SOURCES = $(common_source) $(packet_shell_source) mpshell.cc \
	rate_delay_queue.hh rate_delay_queue.cc
mpshell_LDADD = -lrt
mpshell_LDFLAGS = -pthread

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

bin_PROGRAMS += replayshell
replayshell_SOURCES = $(common_source) replayshell.cc \
	temp_file.hh temp_file.cc \
	web_server.hh web_server.cc \
	apache_configuration.hh \
	http_header.cc http_header.hh
replayshell_LDADD = -lrt
replayshell_LDADD += protobufs/libhttprecordprotos.a -lm $(protobuf_LIBS)
replayshell_LDFLAGS = -pthread

bin_PROGRAMS += nph-replayserver.cgi
nph_replayserver_cgi_SOURCES = $(common_source) replayserver.cc \
	http_header.cc http_header.hh \
	http_message.cc http_message.hh
nph_replayserver_cgi_LDADD = -lrt
nph_replayserver_cgi_LDADD += protobufs/libhttprecordprotos.a -lm $(protobuf_LIBS)
nph_replayserver_cgi_LDFLAGS = -pthread

install-exec-hook:
	chown root $(DESTDIR)$(bindir)/recordshell
	chmod u+s $(DESTDIR)$(bindir)/recordshell
	chown root $(DESTDIR)$(bindir)/replayshell
	chmod u+s $(DESTDIR)$(bindir)/replayshell
	chown root $(DESTDIR)$(bindir)/mpshell
	chmod u+s $(DESTDIR)$(bindir)/mpshell
