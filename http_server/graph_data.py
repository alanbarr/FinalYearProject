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

import matplotlib.pyplot as pyplot
import log_data
import io

def get_graph_device_resource(device, resource):
    ts,da,un = log_data.get_device_resource_lists(device, resource)
    ts.pop(0)
    da.pop(0)
    un.pop(0)
    if(len(set(un)) != 1):
        raise Exception("Units not all the same")
    units = un[0]
    fig = pyplot.figure()
    axis = fig.add_subplot(1,1,1)
    axis.scatter(ts, da)
    fig.suptitle("Graph of " + resource + " from " +  device)
    axis.set_xlabel("Time (s)")
    axis.set_ylabel(resource + " (" + units + ")")
    return fig

def get_graph_device(device):
    resources = log_data.get_device_resources(device)
    fig = pyplot.figure()
    fig.suptitle("Graph of " + device)
    ctr = 1
    axis = []
    for resource in resources:
        if ctr == 1:
            axis1 = fig.add_subplot(len(resources), 1, ctr)
            axis = axis1
        else:
            axis = fig.add_subplot(len(resources), 1, ctr, sharex=axis1)
        ts,da,un = log_data.get_device_resource_lists(device, resource)
        ts.pop(0)
        da.pop(0)
        un.pop(0)
        if(len(set(un)) != 1):
            raise Exception("Units not all the same")
        units = un[0]
        axis.scatter(ts, da)
        axis.set_xlabel("Time (s)")
        axis.xaxis.set_tick_params(labelsize=10)
        axis.set_ylabel(resource + " (" + units + ")", fontsize=10)
        axis.yaxis.set_tick_params(labelsize=10)
        axis.set_title(resource, fontsize=10)
        ctr += 1
    return fig


def show_graph_device_resource(device, resource):
    get_graph_device_resource(device, resource).show()

def show_graph_device(device):
    get_graph_device(device).show()
    
def open_png_graph_device_resource(device,resource):
    plot = get_graph_device_resource(device, resource)
    buf = io.BytesIO()
    plot.savefig(buf, format = 'png')
    buf.seek(0)
    png = buf.read()
    buf.close()
    return png

def open_png_graph_device(device):
    plot = get_graph_device(device).show()
    buf = io.BytesIO()
    plot.savefig(buf, format = 'png')
    buf.seek(0)
    png = buf.read()
    buf.close()
    return buf




