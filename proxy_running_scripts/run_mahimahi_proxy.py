from flask import Flask, request
from ConfigParser import ConfigParser
from argparse import ArgumentParser

import subprocess
import os

CONFIG = 'proxy_running_config'

PROXY_REPLAY_PATH = 'mm-proxyreplay'
NGHTTPX_PATH = 'nghttpx'
NGHTTPX_PORT = 'nghttpx_port'
NGHTTPX_KEY = 'nghttpx_key'
NGHTTPX_CERT = 'nghttpx_cert'
BASE_RECORD_DIR = 'base_record_dir'

MM_PROXYREPLAY = 'mm-proxyreplay'
NGHTTPX = 'nghttpx'
APACHE = 'apache'
PAGE = 'page'

CONFIG_FIELDS = [ PROXY_REPLAY_PATH, NGHTTPX_PATH, NGHTTPX_PORT, NGHTTPX_KEY, NGHTTPX_CERT, BASE_RECORD_DIR ]

app = Flask(__name__)

@app.route("/start_proxy")
def start_proxy():
    print proxy_config
    page = request.args[PAGE]
    path_to_recorded_page = os.path.join(proxy_config[BASE_RECORD_DIR], escape_page(page))
    # cmd: ./mm-proxyreplay ../../page_loads/apple.com/ ./nghttpx 10000 ../certs/nghttpx_key.pem ../certs/nghttpx_cert.pem
    command = '{0} {1} {2} {3} {4} {5}'.format(
                                           proxy_config[PROXY_REPLAY_PATH], \
                                           path_to_recorded_page, \
                                           proxy_config[NGHTTPX_PATH], \
                                           proxy_config[NGHTTPX_PORT], \
                                           proxy_config[NGHTTPX_KEY], \
                                           proxy_config[NGHTTPX_CERT])
    print command
    process = subprocess.Popen(command, shell=True)
    return 'Proxy Started'

@app.route("/stop_proxy")
def stop_proxy():
    processes = [ MM_PROXYREPLAY, NGHTTPX, APACHE ]
    for process in processes:
        command = 'pkill {0}'.format(process)
        subprocess.call(command, shell=True)
    return 'Proxy Stopped'

@app.route("/is_proxy_running")
def is_proxy_running():
    command = 'pgrep "mm-proxyreplay"'
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    stdout, stderr = process.communicate()
    return stdout

def get_proxy_config(config_filename):
    config = dict()
    config_parser = ConfigParser()
    config_parser.read(config_filename)
    for config_key in CONFIG_FIELDS:
        config[config_key] = config_parser.get(CONFIG, config_key)
    return config

HTTP_PREFIX = 'http://'
HTTPS_PREFIX = 'https://'
WWW_PREFIX = 'www.'
def escape_page(url):
    if url.endswith('/'):
        url = url[:len(url) - 1]
    if url.startswith(HTTPS_PREFIX):
        url = url[len(HTTPS_PREFIX):]
    elif url.startswith(HTTP_PREFIX):
        url = url[len(HTTP_PREFIX):]
    if url.startswith(WWW_PREFIX):
        url = url[len(WWW_PREFIX):]
    return url.replace('/', '_')

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('proxy_config')
    parser.add_argument('port', type=int)
    args = parser.parse_args()
    proxy_config = get_proxy_config(args.proxy_config)
    app.run(host='0.0.0.0', port=args.port, debug=True)
