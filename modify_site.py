import os
import sys
import subprocess

# recorded folder to be copied and rewritten
recorded_folder = sys.argv[1]

# folder to store rewritten protobufs
os.system( "cp -r " + recorded_folder + " rewritten" )

files = os.listdir("rewritten")

# read in the window handler to add to top-level html
proxy_inline = ""
with open("inline.html") as handler:
    for line in handler:
        proxy_inline += line

wrote_top_html = 0
for filename in files:
    print filename

    # convert response in protobuf to text (ungzip if necessary)
    command = "protototext rewritten/" + filename + " rewritten/tempfile"
    proc = subprocess.Popen([command], stdout=subprocess.PIPE, shell=True)
    (out, err) = proc.communicate()
    return_code = proc.returncode
    out = out.strip("\n")
    print out
    # need to still handle if response is chunked and gzipped (we can't just run gzip on it)!
    if ( ("html" in out) or ("javascript" in out) ): # html or javascript file, so rewrite
        if ( "chunked" in out ): # response chunked so we must unchunk
            os.system( "python unchunk.py rewritten/tempfile rewritten/tempfile1" )
            os.system( "mv rewritten/tempfile1 rewritten/tempfile" )
            # remove transfer-encoding chunked header from original file since we are unchunking
            os.system( "removeheader rewritten/" + filename + " Transfer-Encoding" )
        if ( "not" in out ): # html or javascript but not gzipped
            if ( "javascript" in out ):
                os.system('nodejs rewrite.js rewritten/tempfile rewritten/retempfile')
                os.system('mv rewritten/retempfile rewritten/tempfile')
            # REWRITE TEMPFILE HERE

            if ( ("index" in out) and ("html" in out) ): # top-level html file so add the handler in-line script
               if ( wrote_top_html ): # already thought we wrote top html
                   print "MULTIPLE FILES SEEM TO BE TOP LEVEL HTML (check protototext)"
               os.system('cp inline.html rewritten/prependtempfile')
               os.system('cat rewritten/tempfile >> rewritten/prependtempfile')
               os.system('mv rewritten/prependtempfile rewritten/tempfile')
               wrote_top_html = 1

            # get new length of response
            size = os.path.getsize('rewritten/tempfile')

            # convert modified file back to protobuf
            os.system( "texttoproto rewritten/tempfile rewritten/" + filename )

            # add new content length header
            os.system( "changeheader rewritten/" + filename + " Content-Length " + str(size) )
        else: # gzipped
            os.system("gzip -d -c rewritten/tempfile > rewritten/plaintext")
            if ( "javascript" in out ):
                os.system('nodejs rewrite.js rewritten/plaintext rewritten/retempfile')
                os.system('mv rewritten/retempfile rewritten/plaintext')
            # REWRITE PLAINTEXT HERE

            if ( ("index" in out) and ("html" in out) ): # top-level html file so add the handler in-line script
                if ( wrote_top_html ): # already thought we wrote top html
                    print "MULTIPLE FILES SEEM TO BE TOP LEVEL HTML (check protototext)"
                os.system('cp inline.html rewritten/prependtempfile')
                os.system('cat rewritten/plaintext >> rewritten/prependtempfile')
                os.system('mv rewritten/prependtempfile rewritten/plaintext')
                wrote_top_html = 1

            # after modifying plaintext, gzip it again (gzipped file is 'finalfile')
            os.system( "gzip -c rewritten/plaintext > rewritten/finalfile" )

            # get new length of response
            size = os.path.getsize('rewritten/finalfile')

            # convert modified file back to protobuf
            os.system( "texttoproto rewritten/finalfile rewritten/" + filename )

            # add new content length header to the newly modified protobuf (name is filename)
            os.system( "changeheader rewritten/" + filename + " Content-Length " + str(size) )

            # delete temp files
            os.system("rm rewritten/plaintext")
            os.system("rm rewritten/finalfile")
    # delete original tempfile
    os.system("rm rewritten/tempfile")

if ( not wrote_top_html ): # we never wrote a top level HTML file
    print "NEVER WROTE TOP LEVEL HTML FILE WITH PROXY HANDLERS!!!"
