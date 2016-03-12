from struct import *
import time
from datetime import date, datetime, timedelta

def delta_ts_from_parts(low, high):
    return low + (high & 0xf) * 256
 
f = open ('trace.bin', 'rb')

(seconds, useconds) = unpack ('qq', f.read(16))

print ('[HOST] Start of log: ' + str (datetime.fromtimestamp (seconds + 0.000001 * useconds)))

frames = []
frame = f.read(1536+32)
while frame != b'':
    frames.append(frame)
    frame = f.read(1536+32)

print ('Read %df frames. Now on to decoding...' % len(frames))

duration = 0
TS_start = 0xffffffff # 1 in 4Gi runs chance to hit this mark...
TS = 0

def processFrame(frame):
    global duration, TS_start, TS

    fduration = 0

    (seconds, useconds) = unpack ('qq', frame[0:16])
    print ('[HOST] Transaction: ' + str (datetime.fromtimestamp (seconds + 0.000001 * useconds)), end='')

    (seconds, useconds) = unpack ('qq', frame[16:32])
    print (' .. ' + str (datetime.fromtimestamp (seconds + 0.000001 * useconds)))

    (TS, TS_at_Tx) = unpack ('II', frame[32:40])
    print ("[SLAVE] Frame start at TS = %d (0x%x), submitted at TS = %d (0x%x)" % (TS, TS, TS_at_Tx, TS_at_Tx))

    if TS != 0xffffffff and TS != 0x0:
        if TS_start == 0xffffffff:
           TS_start = TS

        if TS > TS_at_Tx:
            print ("[SLAVE] *** frame partially overwritten after being submitted for Tx")

        for i in range(40, 1568, 6):
            if i <= 1562:
                (delta_ts_low, chan_plus_delta_ts, current, voltage) = unpack ('BBHH', frame[i:i+6])
                if delta_ts_low == 0 and chan_plus_delta_ts == 0:
                    continue
                chan = chan_plus_delta_ts >> 4
                delta_ts = delta_ts_from_parts(delta_ts_low, chan_plus_delta_ts) 
                fduration += delta_ts
                print ((TS + delta_ts, chan, current, voltage))
                TS += delta_ts

        print ('[SLAVE] Frame timespan: %d microseconds' % fduration)
        duration += fduration

for frame in frames:
    processFrame(frame)

print ('Duration %d' % (TS - TS_start))
