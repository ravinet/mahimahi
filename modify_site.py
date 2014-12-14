import os
import sys
import subprocess

recorded_folder = sys.argv[1]

# list files in specified directory
files = os.listdir(recorded_folder);

for filename in files:
    print filename
    command = "protototext " + recorded_folder + "/" + filename + " " + recorded_folder + "/tempfile"
    proc = subprocess.Popen([command], stdout=subprocess.PIPE, shell=True)
    (out, err) = proc.communicate()
    return_code = proc.returncode
    out = out.strip("\n")
    print out
    # need to still handle if response is chunked and gzipped (we can't just run gzip on it)!
    if ( out == "gzipped" ):
        os.system("gzip -d -c " + recorded_folder + "/tempfile > " + recorded_folder + "/plaintext")
        # file "plaintext" in recorded_folder has plain text version of recorded response. for now we delete
        # it but we want to run our rewriter on that and then make it a new file in a new 'recorded folder' (using texttoproto)
        # and then change the content length accordingly
        os.system("rm " + recorded_folder + "/plaintext")
    os.system("rm " + recorded_folder + "/tempfile")
