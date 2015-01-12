import os
import sys

# takes a chunked HTTP response and outputs new file without chunks

# chunked HTTP format: number of bytes in chunk (hex) followed by CRLF and then chunk followed by CRLF (crlf not included in chunk size)
# the last chunk has size 0 (followed by CRLF) and then final CRLF
# could be followed by the trailer which are headers which could be added to original headers- for now, we print the trailers

chunked_file = sys.argv[1]
new_file = sys.argv[2]
chunked = open(chunked_file, 'r')
new = open(new_file, 'a')

body = ""
for line in chunked:
    body += line

# body has entire chunked response
chunk_size = -1

new_body = ""


#test = "himynameispaul"
#new = test[0:6]
#test = test[6:]
#print new
#print test

# iterate through chunks until we hit the last chunk
crlf_loc = body.find('\r\n')
chunk_size = int( body[:crlf_loc], 16 )
body = body[crlf_loc+2:]
while( chunk_size != 0 ):
    # add chunk content to new body and remove from old body
    new_body += body[0:chunk_size]
    body = body[chunk_size:]

    # remove CRLF trailing chunk
    body = body[2:]

    # get chunk size
    crlf_loc = body.find('\r\n')
    chunk_size = int( body[:crlf_loc], 16 )
    body = body[crlf_loc+2:]

# on the last chunk
body = body[crlf_loc+2:]

if ( len(body) != 0 ):
    print "Trailers present!"

new.write(new_body)
