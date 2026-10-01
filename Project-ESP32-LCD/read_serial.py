import serial
import time

try:
    ser = serial.Serial('COM3', 115200, timeout=0.5)
    t0 = time.time()
    while time.time() - t0 < 3.0:
        line = ser.readline()
        if line:
            print(line.decode('latin1', errors='ignore'), end='')
    ser.close()
except Exception as e:
    print("Error:", e)
