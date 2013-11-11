import serial
ser = serial.Serial('/dev/ttyUSB0',19200,timeout=0)
print ser.portstr
s = ser.readline()
print ser.inWaiting()
while True:
	if ser.inWaiting() > 3:
		s = ser.readline(4)
		print s