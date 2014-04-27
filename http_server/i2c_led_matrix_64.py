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

# GPIOA - Columns
# GPIOB - Rows

from threading import Thread, Lock
import time
import random
import quick2wire.i2c as i2c
from quick2wire.parts.mcp23017 import MCP23017
from quick2wire.parts.mcp23x17 import IODIRA, IODIRB, GPIOA, GPIOB

MATRIX_THD = None
MATRIX_THD_RUNNING = False
MATRIX_THD_LOCK = None
MATRIX_PATTERN = None 
CHIP = None

CHIP_ADDR = 0x20

def sleep_ms(t):
    time.sleep(t/1000)

def get_byte_row_contents(pattern, row):
    base = row*8
    str_byte = pattern[base:base+8]
    index = 0
    byte = 0
    for bit in str_byte:
        if bit == "1":
            byte += 0x1<<index
        index += 1
    return byte

def get_byte_row_id(row):
    return 0xFF & ~(0x1<<row)

def matrix_set_pattern(pattern):
    global MATRIX_THD_LOCK
    global MATRIX_PATTERN
    MATRIX_THD_LOCK.acquire()
    MATRIX_PATTERN = pattern
    MATRIX_THD_LOCK.release()

def update_by_row():
    global MATRIX_PATTERN
    global CHIP
    for row in range(8):
        CHIP.registers.write_register(GPIOA, 0x00)
        CHIP.registers.write_register(GPIOB, get_byte_row_id(row))
        CHIP.registers.write_register(GPIOA, get_byte_row_contents(MATRIX_PATTERN,row))
        sleep_ms(10)

def update_1d():
    global MATRIX_PATTERN
    global CHIP
    CHIP.registers.write_register(GPIOA, 0x00)
    CHIP.registers.write_register(GPIOB, 0x00)
    CHIP.registers.write_register(GPIOA, 0x0F)
    sleep_ms(5)

def matrix_worker_thread():
    global MATRIX_THD_RUNNING
    while(MATRIX_THD_RUNNING is True):
        #update_by_row()
        update_1d()

def matrix_start():
    global MATRIX_THD
    global MATRIX_THD_RUNNING
    global MATRIX_PATTERN
    global CHIP
    global MATRIX_THD_LOCK
    bus = i2c.I2CMaster()
    CHIP = MCP23017(bus, CHIP_ADDR)
    CHIP.reset()
    CHIP.registers.write_register(IODIRA, 0x00)
    CHIP.registers.write_register(IODIRB, 0x00)
    CHIP.registers.write_register(GPIOA, 0x00)
    CHIP.registers.write_register(GPIOB, 0x00)
    MATRIX_PATTERN = 0x00
    MATRIX_THD_LOCK = Lock()
    MATRIX_THD_RUNNING = True
    MATRIX_THD = Thread(target = matrix_worker_thread)
    MATRIX_THD.start()

def matrix_stop():
    global MATRIX_THD
    global MATRIX_THD_RUNNING
    if MATRIX_THD_RUNNING is True:
        print("Stopping display...")
        MATRIX_THD_RUNNING = False
        MATRIX_THD.join()
        CHIP.registers.write_register(GPIOA, 0x00)
        CHIP.registers.write_register(GPIOB, 0x00)
        print("Stopped display.")
        MATRIX_THD = None

def random_pattern(normalised_64):
    global MATRIX_PATTERN
    global MATRIX_THD_LOCK
    random.seed()
    pop = random.sample(range(64), normalised_64)
    list = ["0"] * 64
    for p in pop:
        list[p] = "1"
    MATRIX_THD_LOCK.acquire()
    MATRIX_PATTERN = "".join(list) 
    MATRIX_THD_LOCK.release()


matrix_start()

MATRIX_PATTERN = "10000000" + \
                 "01000000" + \
                 "00100000" + \
                 "00010000" + \
                 "00001000" + \
                 "00000100" + \
                 "00000010" + \
                 "00000001"

for i in range(100):
    random_pattern(32)
    time.sleep(1)
matrix_stop()


