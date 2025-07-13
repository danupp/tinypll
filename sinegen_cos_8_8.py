import math

for A in range(0,256):
    val_c = 127+127*math.cos((float(A)/256)*2*math.pi)
    print("%d, " % (round(val_c)), end = '')
