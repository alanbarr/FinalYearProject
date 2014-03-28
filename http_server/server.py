################################################################################
# Copyright (c) 2014, Alan Barr
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
################################################################################

from http.server import BaseHTTPRequestHandler, HTTPServer
from http.client import InvalidURL, OK
from threading import Thread
import log_data

SERVER_HOST = "10.0.0.1"
SERVER_PORT = 9000



HTTP_SERVER_RUNNING = False
HTTP_SERVER_THREAD = None

class Handler(BaseHTTPRequestHandler):
    def get_file(self, path):
        return log_data.open_csv_file_write(path)

# TODO what if path is bad, no leading forward slash etc XXX
    def path_to_local(self):
        if self.path == "/":
            raise InvalidURL("Invalid Resource:", self.path)
        return log_data.URL_LOG_DIR + self.path + log_data.URL_LOG_EXT

    def log_data(self, log_file):
        host,port = self.client_address
        body_len = int(self.headers.get('content-length'))
        body = self.rfile.read(body_len).decode().split()
        if len(body) > 0:
            data = body[0]
        if len(body) is 2:
            units = body[1]
        else:
            units = "UNITS"
        log_data.write_measurement_to_csv(log_file, host, data, units)
        log_file.close()

    def do_POST(self):
        print("In do POST for:", self.path)
        path = self.path_to_local()
        f = self.get_file(path)
        self.log_data(f)
        self.send_response(OK, "OK")

    def do_GET(self):
        print("In do GET for:", self.path)

def http_server_thread():
    global HTTP_SERVER_RUNNING
    HttpServer = HTTPServer((SERVER_HOST, SERVER_PORT), Handler)
    HttpServer.version = "HTTP/1.0"
    HttpServer.timeout = 2
    while(HTTP_SERVER_RUNNING is True):
        HttpServer.handle_request()

def http_server_start():
    global HTTP_SERVER_RUNNING
    global HTTP_SERVER_THREAD
    print("Starting server...")
    if HTTP_SERVER_RUNNING is True:
        print("Already running!")
        return
    HTTP_SERVER_RUNNING = True
    HTTP_SERVER_THREAD = Thread(target = http_server_thread)
    HTTP_SERVER_THREAD.start()

def http_server_stop():
    global HTTP_SERVER_RUNNING
    global HTTP_SERVER_THREAD
    if HTTP_SERVER_THREAD is not None:
        print("Stopping server...")
        HTTP_SERVER_RUNNING = False
        HTTP_SERVER_THREAD.join()
        HTTP_SERVER_THREAD = None


