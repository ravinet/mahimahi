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
HTTP1_REPLAY_NO_PROXY_PATH = 'mm-http1-replay-no-proxy'
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
MM_DELAY_WITH_NAMESERVER = 'mm-delay-with-nameserver'
NGHTTPX = 'nghttpx'
APACHE = 'apache'
PAGE = 'page'
SQUID_PORT = 'squid_port'
SQUID = 'squid'
OPENVPN = 'openvpn'
OPENVPN_PORT = 'openvpn_port'
START_TCPDUMP = 'start_tcpdump'
DEPENDENCIES = 'dependencies'

TIME = 'time'
DELAY = 'delay'
HTTP_VERSION = 'http'
REPLAY_MODE = 'replay_mode'

CONFIG_FIELDS = [ BUILD_PREFIX, PROXY_REPLAY_PATH, NGHTTPX_PATH, NGHTTPX_PORT, \
                  NGHTTPX_KEY, NGHTTPX_CERT, BASE_RECORD_DIR, PHONE_RECORD_PATH, \
                  SQUID_PORT, BASE_RESULT_DIR, DELAYSHELL_WITH_PORT_FORWARDED, \
                  HTTP1_PROXY_REPLAY_PATH, HTTP1_REPLAY_NO_PROXY_PATH, OPENVPN_PORT, \
                  DEPENDENCY_DIRECTORY_PATH, START_TCPDUMP, SQUID ]

app = Flask(__name__)

@app.route("/start_squid_proxy")
def start_squid_proxy():
    # Start tcpdump, if necessary.
    if proxy_config[START_TCPDUMP] == 'True':
        start_tcpdump()

    squid_path = proxy_config[BUILD_PREFIX] +proxy_config[SQUID]
    print 'squid path: ' + squid_path
    command = '{0}'.format(squid_path)
    subprocess.call(command)
    return 'OK'


@app.route("/stop_squid_proxy")
def stop_squid_proxy():
    command = 'pkill squid'
    subprocess.call(command.split())

    # Start tcpdump, if necessary.
    if proxy_config[START_TCPDUMP] == 'True':
        stop_tcpdump('/home/ubuntu', 'regular_load')

    return 'OK'

proxy_process = None

@app.route("/start_proxy")
def start_proxy():
    print proxy_config
    request_time = request.args[TIME]
    page = request.args[PAGE]
    replay_mode = request.args[REPLAY_MODE]
    use_dependencies = request.args[DEPENDENCIES]
    escaped_page = escape_page(page)
    path_to_recorded_page = os.path.join(proxy_config[BASE_RESULT_DIR], request_time, escaped_page)
    # cmd:  ./mm-proxyreplay /home/ubuntu/long_running_page_load_done/1467058494.43/m.accuweather.com/ /home/ubuntu/build/bin/nghttpx 3000 /home/ubuntu/build/certs/reverse_proxy_key.pem /home/ubuntu/build/certs/reverse_proxy_cert.pem 1194 /home/ubuntu/all_dependencies/dependencies/m.accuweather.com/dependency_tree.txt
    replay_instance = proxy_config[BUILD_PREFIX] + proxy_config[PROXY_REPLAY_PATH]
    http_version = request.args.get(HTTP_VERSION)
    if http_version is not None:
        if http_version == '1':
            replay_instance = proxy_config[BUILD_PREFIX] + proxy_config[HTTP1_PROXY_REPLAY_PATH]
        elif http_version == '1_no_proxy':
            replay_instance = proxy_config[BUILD_PREFIX] + proxy_config[HTTP1_REPLAY_NO_PROXY_PATH]

    if use_dependencies == 'yes':
        dependency_filename = os.path.join(proxy_config[DEPENDENCY_DIRECTORY_PATH], escaped_page, 'dependency_tree.txt')
    else:
        dependency_filename = 'None'
    
    path_to_log = os.path.join(proxy_config[BUILD_PREFIX], 'logs', escaped_page)
    rm_existing_logs = 'rm -f {0}'.format(path_to_log)
    subprocess.call(rm_existing_logs, shell=True)
    path_to_log = os.path.join(proxy_config[BUILD_PREFIX], 'error-logs', escaped_page + '.log')
    rm_existing_logs = 'rm -f {0}'.format(path_to_log)
    subprocess.call(rm_existing_logs, shell=True)

    command = '{0} {1} {2} {3} {4} {5} {6} {7} {8} {9}'.format(
                                           replay_instance, \
                                           path_to_recorded_page, \
                                           proxy_config[BUILD_PREFIX] + proxy_config[NGHTTPX_PATH], \
                                           proxy_config[NGHTTPX_PORT], \
                                           proxy_config[BUILD_PREFIX] + proxy_config[NGHTTPX_KEY], \
                                           proxy_config[BUILD_PREFIX] + proxy_config[NGHTTPX_CERT], \
                                           proxy_config[OPENVPN_PORT], \
                                           replay_mode, \
                                           dependency_filename, \
                                           escaped_page)
    print command
    global proxy_process
    proxy_process = subprocess.Popen(command, shell=True)

    return 'Proxy Started'

@app.route("/stop_proxy")
def stop_proxy():
    processes = [ MM_PROXYREPLAY, MM_DELAY_WITH_NAMESERVER, NGHTTPX, OPENVPN ]
    # processes = [ MM_PROXYREPLAY ]
    # processes = [ OPENVPN ]
    #processes = [ MM_DELAY_WITH_NAMESERVER, OPENVPN ]
    for process in processes:
        command = 'sudo pkill -sigint {0}'.format(process)
        subprocess.Popen(command, shell=True)
    # global proxy_process
    # proxy_process.terminate()
    return 'Proxy Stopped'

def start_tcpdump():
    command = 'sudo tcpdump -i eth0 -w capture.pcap port 80 or port 443'
    subprocess.Popen(command.split())

def stop_tcpdump(pcap_directory, page_name):
    command = 'sudo pkill -sigint tcpdump'
    process = subprocess.Popen(command.split())
    process.wait()
    dst = os.path.join(pcap_directory, page_name + '.pcap')
    command = 'mv -f -v capture.pcap {0}'.format(dst)
    process = subprocess.Popen(command.split())
    process.wait()

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

    # Start tcpdump, if necessary.
    if proxy_config[START_TCPDUMP] == 'True':
        start_tcpdump()

    return 'Proxy Started'

@app.route("/stop_recording")
def stop_recording():
    processes = [ MM_PHONE_WEBRECORD, SQUID ]
    for process in processes:
        command = 'sudo pkill {0}'.format(process)
        print command
        subprocess.Popen(command.split())

    print proxy_config[START_TCPDUMP]
    if proxy_config[START_TCPDUMP] == 'True':
        print 'Stopping tcpdump'
        request_time = request.args[TIME]
        page = request.args[PAGE]
        page_name = escape_page(page)
        pcap_directory = os.path.join(proxy_config[BASE_RECORD_DIR], request_time, 'pcap')
        if not os.path.exists(pcap_directory):
            os.mkdir(pcap_directory)
        stop_tcpdump(pcap_directory, page_name)

    return 'Proxy Stopped'

@app.route("/is_record_proxy_running")
def is_record_proxy_running():
    process_names = [ 'squid' ]
    result = ''
    for process_name in process_names:
        result += check_process(process_name)

    return result.strip()

@app.route("/is_replay_proxy_running")
def is_replay_proxy_running():
    process_names = [ 'mm-proxyreplay', 'nghttpx', 'openvpn' ]
    http_version = request.args.get(HTTP_VERSION)
    if http_version is not None:
        if http_version == '1_no_proxy':
            process_names = [ 'openvpn' ]
        if http_version == '1':
            process_names = [ HTTP1_PROXY_REPLAY_PATH, 'openvpn' ]

    result = ''
    for process_name in process_names:
        result += check_process(process_name)
    return result.strip()

def check_process(process_name):
    command = 'ps aux | grep "{0}"'.format(process_name)
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    stdout, stderr = process.communicate()
    splitted_stdout = stdout.split('\n')
    print stdout

    result = '{0} NO\n'.format(process_name)
    for line in splitted_stdout:
        splitted_line = line.split()
        if len(splitted_line) >= 11:
            proc_name = splitted_line[10]
            if process_name in proc_name:
                result = '{0} YES\n'.format(process_name)
    return result

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
