from flask import Flask, request, g
from ConfigParser import ConfigParser
from argparse import ArgumentParser

import subprocess
import os
import signal
import time

CONFIG = 'proxy_running_config'

BUILD_PREFIX = 'build-prefix'
PROXY_REPLAY_PATH = 'mm-proxyreplay'
HTTP1_PROXY_REPLAY_PATH = 'mm-http1-proxyreplay'
PHONE_RECORD_PATH = 'mm-phone-webrecord'
DELAYSHELL_WITH_PORT_FORWARDED = 'mm-delayshell-with-port-forwarded'
NGHTTPX_PATH = 'nghttpx'
NGHTTPX_PORT = 'nghttpx_port'
NGHTTPX_KEY = 'nghttpx_key'
NGHTTPX_CERT = 'nghttpx_cert'
BASE_RECORD_DIR = 'base_record_dir'
BASE_RESULT_DIR = 'base_result_dir'
DEPENDENCY_DIRECTORY_PATH = 'dependency_directory_path'

MM_PROXYREPLAY = 'mm-proxyreplay'
MM_PHONE_WEBRECORD = 'mm-phone-webrecord'
MM_DELAYSHELL_WITH_PORT_FORWARDED = 'mm-delayshell-port-forwarded'
NGHTTPX = 'nghttpx'
APACHE = 'apache'
PAGE = 'page'
SQUID_PORT = 'squid_port'
SQUID = 'squid'
OPENVPN_PORT = 'openvpn_port'

TIME = 'time'
DELAY = 'delay'
HTTP_VERSION = 'http'

CONFIG_FIELDS = [ BUILD_PREFIX, PROXY_REPLAY_PATH, NGHTTPX_PATH, NGHTTPX_PORT, \
                  NGHTTPX_KEY, NGHTTPX_CERT, BASE_RECORD_DIR, PHONE_RECORD_PATH, \
                  SQUID_PORT, BASE_RESULT_DIR, DELAYSHELL_WITH_PORT_FORWARDED, \
                  HTTP1_PROXY_REPLAY_PATH, OPENVPN_PORT, DEPENDENCY_DIRECTORY_PATH ]

app = Flask(__name__)

@app.route("/start_proxy")
def start_proxy():
    print proxy_config
    page = request.args[PAGE]
    request_time = request.args[TIME]
    escaped_page = escape_page(page)
    path_to_recorded_page = os.path.join(proxy_config[BASE_RESULT_DIR], request_time, escaped_page)
    # cmd: ./mm-proxyreplay ../../page_loads/apple.com/ ./nghttpx 10000 ../certs/nghttpx_key.pem ../certs/nghttpx_cert.pem
    # cmd:  ./mm-proxyreplay /home/ubuntu/long_running_page_load_done/1467058494.43/m.accuweather.com/ /home/ubuntu/build/bin/nghttpx 3000 /home/ubuntu/build/certs/reverse_proxy_key.pem /home/ubuntu/build/certs/reverse_proxy_cert.pem 1194 /home/ubuntu/all_dependencies/dependencies/m.accuweather.com/dependency_tree.txt
    dependency_filename = os.path.join(proxy_config[DEPENDENCY_DIRECTORY_PATH], escaped_page, 'dependency_tree.txt')
    command = '{0} {1} {2} {3} {4} {5} {6} {7}'.format(
                                           proxy_config[BUILD_PREFIX] + proxy_config[PROXY_REPLAY_PATH], \
                                           path_to_recorded_page, \
                                           proxy_config[BUILD_PREFIX] + proxy_config[NGHTTPX_PATH], \
                                           proxy_config[NGHTTPX_PORT], \
                                           proxy_config[BUILD_PREFIX] + proxy_config[NGHTTPX_KEY], \
                                           proxy_config[BUILD_PREFIX] + proxy_config[NGHTTPX_CERT], \
                                           proxy_config[OPENVPN_PORT], \
                                           dependency_filename)
    print command
    process = subprocess.Popen(command, shell=True)
    return 'Proxy Started'

@app.route("/stop_proxy")
def stop_proxy():
    processes = [ MM_PROXYREPLAY, NGHTTPX, SQUID ]
    for process in processes:
        command = 'pkill {0}'.format(process)
        subprocess.Popen(command, shell=True)
    return 'Proxy Stopped'

@app.route("/start_delay_replay_proxy")
def start_delay_replay_proxy():
    print proxy_config
    page = request.args[PAGE]
    request_time = request.args[TIME]
    delay = request.args[DELAY]
    http_version = request.args.get(HTTP_VERSION)
    if http_version is None:
        http_version = 2
    else:
        http_version = int(http_version)
    
    if http_version == 1:
        path_to_recorded_page = os.path.join(proxy_config[BASE_RESULT_DIR], request_time, escape_page(page))
        # cmd: ./mm-proxyreplay ../../page_loads/apple.com/ ./nghttpx 10000 ../certs/nghttpx_key.pem ../certs/nghttpx_cert.pem
        command = '{0} {1} {2} {3} {4}'.format(
                                               proxy_config[BUILD_PREFIX] + proxy_config[DELAYSHELL_WITH_PORT_FORWARDED], \
                                               delay, \
                                               proxy_config[SQUID_PORT], \
                                               proxy_config[BUILD_PREFIX] + proxy_config[HTTP1_PROXY_REPLAY_PATH], \
                                               path_to_recorded_page)
    elif http_version == 2:
        path_to_recorded_page = os.path.join(proxy_config[BASE_RESULT_DIR], request_time, escape_page(page))
        # cmd: ./mm-proxyreplay ../../page_loads/apple.com/ ./nghttpx 10000 ../certs/nghttpx_key.pem ../certs/nghttpx_cert.pem
        command = '{0} {1} {2} {3} {4} {5} {6} {7} {8}'.format(
                                               proxy_config[BUILD_PREFIX] + proxy_config[DELAYSHELL_WITH_PORT_FORWARDED], \
                                               delay, \
                                               proxy_config[NGHTTPX_PORT], \
                                               proxy_config[BUILD_PREFIX] + proxy_config[PROXY_REPLAY_PATH], \
                                               path_to_recorded_page, \
                                               proxy_config[BUILD_PREFIX] + proxy_config[NGHTTPX_PATH], \
                                               proxy_config[NGHTTPX_PORT], \
                                               proxy_config[BUILD_PREFIX] + proxy_config[NGHTTPX_KEY], \
                                               proxy_config[BUILD_PREFIX] + proxy_config[NGHTTPX_CERT])
        print command

    g._running_delay_proxy_process = subprocess.Popen(command, shell=True, preexec_fn=os.setsid)
    return 'Proxy Started'

@app.route("/stop_delay_replay_proxy")
def stop_delay_replay_proxy():
    command = 'pkill {0}'.format(MM_DELAYSHELL_WITH_PORT_FORWARDED)
    subprocess.Popen(command, shell=True)
    stop_proxy()
    return 'Proxy Stopped'

@app.route("/start_recording")
def start_recording():
    print 'start recording'
    page = request.args[PAGE]
    request_time = request.args[TIME]
    mkdir_cmd = 'mkdir -p {0}'.format(os.path.join(proxy_config[BASE_RECORD_DIR], request_time))
    subprocess.call(mkdir_cmd, shell=True)
    record_path = os.path.join(proxy_config[BASE_RECORD_DIR], request_time, escape_page(page))
    if os.path.exists(record_path):
        rm_cmd = 'rm -r {0}'.format(record_path)
        subprocess.call(rm_cmd, shell=True)
    command = '{0} {1} {2}'.format(
                            proxy_config[BUILD_PREFIX] + proxy_config[PHONE_RECORD_PATH], \
                            os.path.join(proxy_config[BASE_RECORD_DIR], request_time, escape_page(page)), \
                            proxy_config[SQUID_PORT])
    process = subprocess.Popen(command, shell=True)
    return 'Proxy Started'

@app.route("/stop_recording")
def stop_recording():
    processes = [ MM_PHONE_WEBRECORD, SQUID ]
    for process in processes:
        command = 'pkill {0}'.format(process)
        print command
        subprocess.Popen(command, shell=True)
    return 'Proxy Stopped'

@app.route("/is_recording")
def is_recording():
    command = 'pgrep "mm-phone-webrecord"'
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    stdout, stderr = process.communicate()
    return stdout

@app.route("/is_proxy_running")
def is_proxy_running():
    result = ''
    command = 'pgrep "mm-proxyreplay"'
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    stdout, stderr = process.communicate()
    splitted_stdout = stdout.split('\n')
    if len(splitted_stdout) > 1:
        result += 'mm-proxyreplay YES\n'
    elif len(splitted_stdout) == 1 and len(splitted_stdout[0]) > 0:
        result += 'mm-proxyreplay YES\n'
    else:
        result += 'mm-proxyreplay NO\n'

    command = 'pgrep "squid"'
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    stdout, stderr = process.communicate()
    splitted_stdout = stdout.split('\n')
    if len(splitted_stdout) > 1:
        result += 'squid YES\n'
    elif len(splitted_stdout) == 1 and len(splitted_stdout[0]) > 0:
        result += 'squid YES\n'
    else:
        result += 'squid NO\n'

    command = 'pgrep "nghttpx"'
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    stdout, stderr = process.communicate()
    splitted_stdout = stdout.split('\n')
    if len(splitted_stdout) > 1:
        result += 'nghttpx YES\n'
    elif len(splitted_stdout) == 1 and len(splitted_stdout[0]) > 0:
        result += 'nghttpx YES\n'
    else:
        result += 'nghttpx NO\n'

    command = 'pgrep "openvpn"'
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    stdout, stderr = process.communicate()
    print 'openvpn: ' + stdout
    splitted_stdout = stdout.split('\n')
    if len(splitted_stdout) > 1:
        result += 'openvpn YES\n'
    elif len(splitted_stdout) == 1 and len(splitted_stdout[0]) > 0:
        result += 'openvpn YES\n'
    else:
        result += 'openvpn NO\n'

    return result.strip()

@app.route("/done")
def done():
    command = 'mv {0}/* {1}'.format(proxy_config[BASE_RECORD_DIR], proxy_config[BASE_RESULT_DIR])
    process = subprocess.Popen(command, shell=True)
    print 'DONE'
    return 'OK', 200

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
