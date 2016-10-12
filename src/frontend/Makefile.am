AM_CPPFLAGS = -I../protobufs -I$(srcdir)/../util -I$(srcdir)/../packet -I$(srcdir)/../graphing -I$(srcdir)/../http -I$(srcdir)/../httpserver $(XCBPRESENT_CFLAGS) $(XCB_CFLAGS) $(PANGOCAIRO_CFLAGS) $(CXX11_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = mm-delay
mm_delay_SOURCES = delayshell.cc delay_queue.hh delay_queue.cc
mm_delay_LDADD = -lrt ../util/libutil.a ../packet/libpacket.a
mm_delay_LDFLAGS = -pthread

bin_PROGRAMS += mm-loss
mm_loss_SOURCES = lossshell.cc loss_queue.hh loss_queue.cc
mm_loss_LDADD = -lrt ../util/libutil.a ../packet/libpacket.a
mm_loss_LDFLAGS = -pthread

bin_PROGRAMS += mm-onoff
mm_onoff_SOURCES = onoffshell.cc loss_queue.hh loss_queue.cc
mm_onoff_LDADD = -lrt ../util/libutil.a ../packet/libpacket.a
mm_onoff_LDFLAGS = -pthread

bin_PROGRAMS += mm-link
mm_link_SOURCES = linkshell.cc link_queue.hh link_queue.cc
mm_link_LDADD = -lrt ../util/libutil.a ../packet/libpacket.a ../graphing/libgraph.a $(XCBPRESENT_LIBS) $(XCB_LIBS) $(PANGOCAIRO_LIBS)
mm_link_LDFLAGS = -pthread

bin_PROGRAMS += mm-meter
mm_meter_SOURCES = meter.cc meter_queue.hh meter_queue.cc
mm_meter_LDADD = -lrt ../util/libutil.a ../packet/libpacket.a ../graphing/libgraph.a $(XCBPRESENT_LIBS) $(XCB_LIBS) $(PANGOCAIRO_LIBS)
mm_meter_LDFLAGS = -pthread

bin_PROGRAMS += mm-webrecord
mm_webrecord_SOURCES = recordshell.cc
mm_webrecord_LDADD = -lrt ../httpserver/libhttpserver.a ../http/libhttp.a ../protobufs/libhttprecordprotos.a ../util/libutil.a $(protobuf_LIBS) $(libcrypto_LIBS) $(libssl_LIBS)
mm_webrecord_LDFLAGS = -pthread

bin_PROGRAMS += mm-webreplay
mm_webreplay_SOURCES = replayshell.cc web_server.hh web_server.cc
mm_webreplay_LDADD = -lrt ../util/libutil.a ../http/libhttp.a ../protobufs/libhttprecordprotos.a $(protobuf_LIBS)
mm_webreplay_LDFLAGS = -pthread

bin_PROGRAMS += mm-replayserver
mm_replayserver_SOURCES = replayserver.cc
mm_replayserver_LDADD = -lrt ../util/libutil.a ../http/libhttp.a ../protobufs/libhttprecordprotos.a $(protobuf_LIBS)
mm_replayserver_LDFLAGS = -pthread

lib_LTLIBRARIES = libmod_deepcgi.la
libmod_deepcgi_la_SOURCES = mod_deepcgi.c replayserver_filename.cc
libmod_deepcgi_la_CFLAGS = -I@APACHE2_INCLUDE@ $(libapr1_CFLAGS)
libmod_deepcgi_la_CPPFLAGS = # empty

install-exec-hook:
	chown root $(DESTDIR)$(bindir)/mm-delay
	chmod u+s $(DESTDIR)$(bindir)/mm-delay
	chown root $(DESTDIR)$(bindir)/mm-loss
	chmod u+s $(DESTDIR)$(bindir)/mm-loss
	chown root $(DESTDIR)$(bindir)/mm-onoff
	chmod u+s $(DESTDIR)$(bindir)/mm-onoff
	chown root $(DESTDIR)$(bindir)/mm-link
	chmod u+s $(DESTDIR)$(bindir)/mm-link
	chown root $(DESTDIR)$(bindir)/mm-meter
	chmod u+s $(DESTDIR)$(bindir)/mm-meter
	chown root $(DESTDIR)$(bindir)/mm-webrecord
	chmod u+s $(DESTDIR)$(bindir)/mm-webrecord
	chown root $(DESTDIR)$(bindir)/mm-webreplay
	chmod u+s $(DESTDIR)$(bindir)/mm-webreplay
