#!/usr/bin/env python
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import BaseHTTPServer
#import SimpleHTTPServer
import SocketServer
import base64
import urllib
import sys
import subprocess
import time

def parse_header(x):
    pieces = x.split(":")
    head = pieces[0]
    val = pieces[1]
    for i in range(2, len(pieces)):
        val = val + ":" + pieces[i]
    if val[0] == " ":
        val = val[1:]
    return [head, val]

#could use ForkingMixIn to spawn another process rather than another thread (or ThreadingMixIn)
class ThreadingSimpleServer(SocketServer.ForkingMixIn,
                            BaseHTTPServer.HTTPServer):
    pass

class Request_Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    def do_GET(self):
        command = "findmatch " + dir_to_use + " '" + self.requestline + "'"
        proc = subprocess.Popen([command], stdout=subprocess.PIPE, shell=True)
        (out,err) = proc.communicate()
        self.wfile.write(out)
        # check if server wants to close connection, if yes- close it
        y = out.split("\r\n")
        for x in range(1, len(y)-1):
            if ( y[x] == '' ):
                break
            res = parse_header(y[x])
            if ( res[0].lower() == "connection" ):
                if ( res[1].lower() == "close" ):
                    self.close_connection = 1
                #elif ( res[1].lower() == "keep-alive" ):
                #    self.close_connection = 0
        return

    def do_POST(self):
        command = "findmatch " + dir_to_use + " '" + self.requestline + "'"
        proc = subprocess.Popen([command], stdout=subprocess.PIPE, shell=True)
        (out,err) = proc.communicate()
        self.wfile.write(out)
        # check if server wants to close connection, if yes- close it
        y = out.split("\r\n")
        for x in range(1, len(y)-1):
            if ( y[x] == '' ):
                break
            res = parse_header(y[x])
            if ( res[0].lower() == "connection" ):
                if ( res[1].lower() == "close" ):
                    self.close_connection = 1
        return


def run(ip, port, server_class=HTTPServer, handler_class=Request_Handler):
    server_address = (ip, port)
    httpd = ThreadingSimpleServer(server_address, handler_class)
    #httpd = server_class(server_address, handler_class)
    print 'Listening on port ' + str(port)
    httpd.serve_forever()

if __name__ == "__main__":
    from sys import argv
    if len(argv) == 4:
        global dir_to_use
        dir_to_use = argv[3]
        run(ip=argv[1], port=int(argv[2]))
    else:
        print "ERROR"
