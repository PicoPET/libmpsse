from struct import *

def delta_ts_from_parts(low, high):
    return low + (high & 0xf) * 256
 
f = open ('trace.bin', 'rb')
frames = []

frame = f.read(1536)
while frame != b'':
    frames.append(frame)
    frame = f.read(1536)

print ('Read %df frames. Now on to decoding...' % len(frames))

duration = 0
TS_start = 0xffffffff # 1 in 4Gi runs chance to hit this mark...
TS = 0

def processFrame(frame):
    global duration, TS_start, TS
    (TS, TS_at_Tx) = unpack ('II', frame[0:8])
    print ("TS = 0x%x, TS_at_Tx = 0x%x" % (TS, TS_at_Tx))

    if TS != 0xffffffff & TS != 0x0:
        if TS_start == 0xffffffff:
           TS_start = TS

        for i in range(8, 1536, 6):
            if i <= 1530:
                (delta_ts_low, chan_plus_delta_ts, current, voltage) = unpack ('BBHH', frame[i:i+6])
                if delta_ts_low == 0 and chan_plus_delta_ts == 0:
                    continue
                chan = chan_plus_delta_ts >> 4
                delta_ts = delta_ts_from_parts(delta_ts_low, chan_plus_delta_ts) 
                fduration += delta_ts
                print ((TS + delta_ts, chan, current, voltage))
                TS += delta_ts

        print ('==> Frame duration %d' % fduration)
        duration += fduration

for frame in frames:
    processFrame(frame)

print ('Duration %d' % (TS - TS_start))
