from struct import *
import time
from datetime import date, datetime, timedelta

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

frames = []
frame = f.read(1536*8+32)
while frame != b'':
    frames.append(frame)
    frame = f.read(1536*8+32)

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

    # Skip SIX 0x0 values (one sample...) spuriously received ahead of the actual STM32F4 frame.
    (TS, TS_at_Tx) = unpack ('II', frame[38:46])

    # On empty frames (no SPI data on MISO) both timestamps are 0xffffffff.
    # If either differs from 0xfffffffff, it's a valid frame.
    if TS != 0xffffffff or TS_at_Tx != 0xffffffff:
        print ("[SLAVE] Frame start at TS = %d (0x%x), submitted at TS = %d (0x%x)" % (TS, TS, TS_at_Tx, TS_at_Tx))

        if not got_start_TS:
           TS_start = TS
           got_start_TS = True

        if TS > TS_at_Tx:
            print ("[SLAVE] *** frame partially overwritten after being submitted for Tx")

        for i in range(46, 1536*8 + 32, 6):
            if i <= 1536*8 + 26:
                (delta_ts, chan_plus_current, chan_plus_voltage) = unpack ('HHH', frame[i:i+6])
                channel = (chan_plus_current & 0xf000) >> 12
                current = rawToCurrent (chan_plus_current & 0xfff)
                voltage = rawToVoltage (chan_plus_voltage & 0xfff)
                # If a sample is all-zeroes, do not print it (it's part of end-of-buffer space.)
                if delta_ts == 0 and channel == 0 and current == 0 and voltage == 0:
                    break

                fduration += delta_ts
                print ('%.6f, %1d, %8.6f, %8.6f' % ((TS + delta_ts) / 1000000, channel, current, voltage))
                TS += delta_ts

        print ('[SLAVE] Frame timespan: %d microseconds' % fduration)
        duration += fduration

for frame in frames:
    processFrame(frame)

print ('Duration %d' % (TS - TS_start))
