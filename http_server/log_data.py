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

import time
import csv
import os.path
import config

URL_LOG_EXT = ".csv"
CSV_HEADER = ["IP", "TIMESTAMP", "DATA", "UNITS"]

def open_csv_file_write(path):
    if os.path.exists(os.path.dirname(path)) == False:
        os.makedirs(os.path.dirname(path))
    if os.path.exists(path) == False:
        open_mode = "w"
    else:
        open_mode = "a"
    f = open(path, open_mode)
    c = csv.writer(f)
    if open_mode is "w":
        c.writerow(CSV_HEADER)
    return f

def close_csv_file(f):
    f.close()

def open_csv_file_read(path):
    return open(path, "r")
    
def write_measurement_to_csv(f, host, data, units):
    sys_time = time.time()
    c = csv.writer(f)
    c.writerow([host,sys_time,data,units])

def get_devices():
    return os.listdir(config.DATA_DIR)

def get_device_resources(device):
    resources = []
    device_path = config.DATA_DIR + "/" + device
    if os.path.isdir(device_path) == False:
        return None
    unfiltered =  os.listdir(device_path)
    for u in unfiltered:
        if u.startswith('.'):
            continue
        if os.path.isdir(u) == True:
            continue
        h,t = os.path.split(u)
        r,e = os.path.splitext(t)
        resources.append(r)
    return resources

def get_device_resource_lists(device, resource):
    path = config.DATA_DIR + "/" + device + "/" + resource + URL_LOG_EXT
    f = open_csv_file_read(path)
    c = csv.reader(f)
    time_list = []
    data_list = []
    unit_list = []
    for row in c:
        time_list.append(row[1])
        data_list.append(row[2])
        unit_list.append(row[3])
    f.close()
    return (time_list, data_list, unit_list)


