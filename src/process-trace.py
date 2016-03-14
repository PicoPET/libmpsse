from struct import *
import time
from datetime import date, datetime, timedelta

def delta_ts_from_parts(low, high):
    return low + (high & 0xf) * 256
 
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

    (seconds, useconds) = unpack ('qq', frame[0:16])
    print ('[HOST] Transaction: ' + ('%15.6f' % (seconds + 0.000001 * useconds)), end='')

    (seconds, useconds) = unpack ('qq', frame[16:32])
    print (' .. ' + ('%15.6f' % (seconds + 0.000001 * useconds)))

    (TS, TS_at_Tx) = unpack ('II', frame[32:40])

    # On empty frames (no SPI data on MISO) both timestamps are 0xffffffff.
    # If either differs from 0xfffffffff, it's a valid frame.
    if TS != 0xffffffff or TS_at_Tx != 0xffffffff:
        print ("[SLAVE] Frame start at TS = %d (0x%x), submitted at TS = %d (0x%x)" % (TS, TS, TS_at_Tx, TS_at_Tx))

        if not got_start_TS:
           TS_start = TS
           got_start_TS = True

        if TS > TS_at_Tx:
            print ("[SLAVE] *** frame partially overwritten after being submitted for Tx")

        for i in range(40, 1536*8 + 32, 6):
            if i <= 1536*8 + 26:
                (delta_ts, chan_plus_current, chan_plus_voltage) = unpack ('HHH', frame[i:i+6])
                channel = (chan_plus_current & 0xf000) >> 12
                current = chan_plus_current & 0xfff
                voltage = chan_plus_voltage & 0xfff
                fduration += delta_ts
                print ('%10d, %1d, %4d, %4d' % (TS + delta_ts, channel, current, voltage))
                TS += delta_ts

        print ('[SLAVE] Frame timespan: %d microseconds' % fduration)
        duration += fduration

for frame in frames:
    processFrame(frame)

print ('Duration %d' % (TS - TS_start))
