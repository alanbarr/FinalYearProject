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
import graph_data
import time
import os
import config

HTTP_SERVER_RUNNING = False
HTTP_SERVER_THREAD = None

class Handler(BaseHTTPRequestHandler):
    def get_file(self, path):
        return log_data.open_csv_file_write(path)

    def close_file(self, f):
        log_data.close_csv_file(f)

#http://stackoverflow.com/questions/2617615/slow-python-http-server-on-localhost
    def address_string(self):
        host, port = self.client_address[:2]
        return host

# TODO what if path is bad, no leading forward slash etc XXX
    def path_to_local(self):
        if self.path == "/":
            raise InvalidURL("Invalid Resource:", self.path)
        return log_data.URL_LOG_DIR + self.path + log_data.URL_LOG_EXT
 
    def path_to_device_resource(self):
        path, ext = os.path.splitext(self.path)
        if path.count("/") != 2:
            raise InvalidURL("Invalid Resource:", path)
        split_path = path.split("/")
        return (split_path[1], split_path[2])

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

    def do_POST(self):
        path = self.path_to_local()
        f = self.get_file(path)
        self.end_headers()
        self.log_data(f)
        close_file(f)
        self.send_response(OK, "OK")

    def do_GET(self):
        if (self.path == "/"):  
            f=open(config.HTTP_ROOT_FILE_PATH, "r")
            reply=f.read()
            f.close()

            reply = reply + "\nResources:\n"
            devices = log_data.get_devices()
            for d in devices:
                reply = reply + "\n" + d + "/"
                resources = log_data.get_device_resources(d)
                for r in resources:
                    reply = reply + "\n" + "     " + r

            self.send_response(OK, "OK")
            self.send_header("Content-type","text/plain")
            self.send_header("Content-length", len(reply))
            self.end_headers()
            self.wfile.write(reply.encode(encoding="ASCII"))
        elif self.path.endswith(".graph"):
            dev,res = self.path_to_device_resource()
            png = graph_data.open_png_graph_device_resource(dev,res)
            self.send_response(OK, "OK")
            self.send_header("Content-type","image/png")
            self.send_header("Content-length", len(png))
            self.end_headers()
            self.wfile.write(png)
        elif os.path.isfile(config.DATA_DIR + self.path):
            print("attempting to get" + self.path)
            f=open(config.DATA_DIR + self.path, "rb")
            self.send_response(OK, "OK")
            self.send_header("Content-type", "text/plain")
            f.seek(0, os.SEEK_END)
            self.send_header("Content-length",f.tell())
            f.seek(0, os.SEEK_SET)
            self.end_headers()
            self.wfile.write(f.read())
            f.close()
            

def http_server_thread():
    global HTTP_SERVER_RUNNING
    HttpHandler = Handler
    HttpHandler.protocol_version = "HTTP/1.1"
    HttpHandler.timeout = 10
    HttpServer = HTTPServer((config.SERVER_HOST, config.SERVER_PORT), HttpHandler)
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


