#!/usr/bin/env python3
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
import server
import cmd
import log_data
import graph_data

class Cli(cmd.Cmd):
    intro = "Sensor Logging and Control."
    prompt = "(SLC) "

    def do_server(self,str):
        """HTTP server control. Arguments:
        'start' - start the server
        'stop'  - stop ther server"""
        if str == "start":
            server.http_server_start()
        elif str == "stop":
            server.http_server_stop()
        else:
            print("Incorrect argument: ", str)

    def do_exit(self,str):
        """Exit from the program."""
        server.http_server_stop()
        return True

    def emptyline(self):
        pass
    
    def do_list(self,str):
        """List devices or resources. Arguments:
           'devices' - Lists all devices
           'resources <device>' - List all resources under device"""
        s = str.split()
        if len(s) == 1 and s[0] == "devices":
            devices = log_data.get_devices()
            for d in devices:
                print(d)
        elif len(s) == 2 and s[0] == "resources":
            resources = log_data.get_device_resources(s[1])
            for r in resources:
                print(r)
        else:
            print("Incorrect argument: ", str)
        
    def do_graph(self,str):
        """Graph a resource. Argumments:
           <device> - graph all resources known for device
           <device resource> - graph particular resource from device"""
        s = str.split()
        if len(s) == 1:
            graph_data.show_graph_device(s[0])
        elif len(s) == 2:
            graph_data.show_graph_device_resource(s[0], s[1])
        else:
            print("Incorrect argument: ", str)

if __name__ == "__main__":
    cli = Cli()
    cli.cmdloop()

