from struct import *
import time
from datetime import date, datetime, timedelta

# Slave frame size: 12 KiB, starting with two 32-bit microsecond timestamps:
# - TS of first ADC ISR for that buffer (whether with full sample or not)
# - TS of submission of buffer for DMA (NSS was pulled down).
slave_frame_size = 1536 * 8

vref = 2.9625  # reference "3.000" of the STM in Volt
resistor = 0.500 #  the installed resistor
gain = 50

# Formula for actual current: rawI * vref / 4096.0 / gain / resistor
def rawToCurrent (rawCurrent):
    return float (rawCurrent) * vref / gain / resistor / 4096.0

# Formula for actual voltage: rawV * vref / 4096.0 * 2
def rawToVoltage (rawVoltage):
    return float (rawVoltage) * vref / 4096.0 * 2

f = open ('trace.bin', 'rb')

(seconds, useconds) = unpack ('qq', f.read(16))

print ('[HOST] Start of log: ' + str (datetime.fromtimestamp (seconds + 0.000001 * useconds)))

# The 32 bytes extra are for two struct timeval in a 64-bit machine.
# Reduce to 16 on a 32-bit machine (typically ARMs: Raspberry Pi, Odroid-C1, etc.).
frames = []
frame = f.read(slave_frame_size + 32)
while frame != b'':
    frames.append(frame)
    frame = f.read(slave_frame_size + 32)

print ('Read %df frames. Now on to decoding...' % len(frames))

duration = 0
got_start_TS = False
TS_start = 0xffffffff # 1 in 4Gi runs chance to miss the actual value...
TS = 0

def processFrame(frame):
    global duration, TS_start, TS, got_start_TS

    fduration = 0

    (seconds_start, useconds_start, seconds_end, useconds_end) = unpack ('qqqq', frame[0:32])

    print ('[HOST] Transaction: ' + ('%15.6f' % (seconds_start + 0.000001 * useconds_start)), end='')
    print (' .. ' + ('%15.6f' % (seconds_end + 0.000001 * useconds_end)))

    print ('[HOST] Transaction: ' + ('0x%08x:0x%08x' % (seconds_start, useconds_start)), end='')
    print (' .. ' + ('0x%08x:0x%08x' % (seconds_end, useconds_end)))

    # Handle the (currently unexplained) case where a frame is preceded by 6 bytes at 0x0.
    (foo, bar) = unpack ('IH', frame[32:38])
    if foo == 0 and bar == 0:
        frame_start = 38
    else:
        frame_start = 32

    print ('[SLAVE] foo = 0x%x, bar = 0x%x' % (foo, bar))

    (TS, TS_at_Tx) = unpack ('II', frame[frame_start:frame_start+8])

    # On empty frames (no SPI data on MISO) both timestamps are 0xffffffff.
    # If either differs from 0xfffffffff, it's a valid frame.
    if TS != 0xffffffff or TS_at_Tx != 0xffffffff:
        print ("[SLAVE] Frame start at TS = %d (0x%x), submitted at TS = %d (0x%x)" % (TS, TS, TS_at_Tx, TS_at_Tx))

        if not got_start_TS:
           TS_start = TS
           got_start_TS = True

        if TS > TS_at_Tx:
            print ("[SLAVE] *** frame partially overwritten after being submitted for Tx")

        for i in range(frame_start + 8, slave_frame_size + 32, 6):
            if i <= slave_frame_size + 26:
                (delta_ts, chan_plus_current, chan_plus_voltage) = unpack ('HHH', frame[i:i+6])
                channel = (chan_plus_current & 0xf000) >> 12
                current = rawToCurrent (chan_plus_current & 0xfff)
                voltage = rawToVoltage (chan_plus_voltage & 0xfff)
                # If a sample is all-zeroes, do not print it (it's part of end-of-buffer space.)
                if delta_ts == 0 and channel == 0 and current == 0 and voltage == 0:
                    break

                fduration += delta_ts
                print ('%.6f, %1d, %8.6f, %8.6f, 0x%03x, 0x%03x' % ((TS + delta_ts) / 1000000, channel, current, voltage, chan_plus_current & 0xfff, chan_plus_voltage & 0xfff))
                TS += delta_ts

        print ('[SLAVE] Frame timespan: %d microseconds' % fduration)
        duration += fduration

for frame in frames:
    processFrame(frame)

print ('Duration %d' % (TS - TS_start))
