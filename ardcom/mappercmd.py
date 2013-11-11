import usb.core
import time
import serial
import math
import numpy as np
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt
import random


class BraduinoUSBCommunicator:

    def __init__(self):
        print "Your UART cable better be plugged into the USB 2.0 port, or else kittens will cry"
        self.HELLO = 0
        self.SET_VALS = 1
        self.GET_VALS = 2
        self.PRINT_VALS = 3
        self.dev = usb.core.find(idVendor = 0x6666, idProduct = 0x0003)
        if self.dev is None:
            raise ValueError('no USB device found matching idVendor = 0x6666 and idProduct = 0x0003')
        self.dev.set_configuration()

    def close(self):
        self.dev = None

    def hello(self):
        try:
            self.dev.ctrl_transfer(0x40, self.HELLO)
        except usb.core.USBError:
            print "Could not send HELLO vendor request."

    def set_vals(self, val1, val2):
        try:
            self.dev.ctrl_transfer(0x40, self.SET_VALS, int(val1), int(val2))
        except usb.core.USBError as e:
            print e
            print "Could not send SET_VALS vendor request."
            raise e

    def get_vals(self):
        try:
            ret = self.dev.ctrl_transfer(0xC0, self.GET_VALS, 0, 0, 4)
        except usb.core.USBError:
            print "Could not send GET_VALS vendor request."
        else:
            return [int(ret[0])+int(ret[1])*256, int(ret[2])+int(ret[3])*256]

    def print_vals(self):
        try:
            self.dev.ctrl_transfer(0x40, self.PRINT_VALS)
        except usb.core.USBError:
            print "Could not send PRINT_VALS vendor request."

    def move_servos(self,pos1,pos2):
        #do some data validation, then send the servo positions to the PIC
        if pos1 < 0 or pos1 > 1:
            raise ValueError,"pos1 must be between 0 and 1"
        elif pos2 < 0 or pos2 > 1:
            raise ValueError,"pos2 must be between 0 and 1"
        else:
            val1 = int(pos1 * (2**16-1))
            val2 = int(pos2 * (2**16-1))
            print "val1 = %d and val2=%d" % (val1,val2)
            print "in binary, val1 = %s and val2 = %s" % (bin(val1)[1:],bin(val2)[1:])
            self.set_vals(val1,val2)
            time.sleep(0.1) #wait long enough so we get a position back

class Plotter:

    def __init__(self,port,baudrate):
        self.setup_serial(port,baudrate)

    def setup_serial(self, port, baudrate):
        """Opens the specified serial port, and adds an IO watch to monitor it"""
        # Open a serial port, specifying a 10ms timeout for reads
        self.serial = serial.Serial(port=port, baudrate=baudrate,
                                    timeout=SERIAL_READ_TIMEOUT)
        # Discard anything old sitting around in the serial port buffer.
        self.serial.flushInput()

    def continuousPlot(self,refreshRate):
        self.distances = []
        self.fig = plt.figure()
        gobject.timeout_add(refreshRate,self.updateGraph)
        plt.show()

    # def read_from_serial(self):
    # if self.serial.inWaiting() > 0:
    #     s = self.serial.readline()
    #     try:
    #         t = float(s)
    #     except:
    #         return False
    #     return t
    # return False

    def updateGraph(self):
        if self.serial.inWaiting() and self.serial.inWaiting() > 0:
            
            #Read the data, and clear the output buffer so there's no delay
            new_distance = float(self.serial.readline().strip())
            if len(str(new_distance)) > 0:
                
                self.serial.flushInput()
            
                print new_distance, self.serial.inWaiting()
                # If we got new data then append it to the list of
                # temperatures and trim to 750 points
                if new_distance and new_distance != '':
                    self.distances.append(float(new_distance))
                    self.distances = self.distances[-50:]
                
                self.fig.clear()
                self.fig.suptitle("Live Brightness")
                a = self.fig.add_subplot(111)
                a.grid(True)
                l, = a.plot(self.distances)
                self.fig.canvas.draw_idle()

        return True


def distance(x1,y1,x2,y2):
    return math.sqrt((x2-x1)**2 + (y2-y1)**2)

def generate_servo_positions(resolution):
    resolution = float(resolution)
    num_points = 180.0/resolution
    omega, phi = 0, 0 #positions in degrees
    omegas, phis = [], []
    for i in xrange(1,int(num_points)+2):
        omegas.append(omega)
        phis.append(phi)
        omega += resolution
        phi += resolution
    all_locations = []
    omega,phi = 0, 0
    for i in xrange(0,len(omegas)):
        omega = omegas[i]
        for j in xrange(0,len(phis)):
            phi = phis[j]
            location = (omega,phi)
            all_locations.append(location)
        phis.reverse()
    return all_locations

def read_from_serial(ser):
    if ser.inWaiting() > 6:
        s = ser.readline(8)
        try:
            t = float(s)
        except:
            return False
        return t
    return False

def omega_to_theta(omega):
    return math.radians(omega-90.0)
    # return [math.radians(position[0]-90) for position in positions]

def phi_to_r(phi):
    return math.sin(math.radians(phi-90.0))
    # return [math.cos(math.radians(position[1])) for position in positions]

def plot(measurements,res):
    theta = []
    r = []
    z = []
    for measurement in measurements:
        theta.append(omega_to_theta(measurement[0]))
        r.append(phi_to_r(measurement[1]))
        z.append(measurement[2])
    area = 1000/res
    colors = z
    ax = plt.subplot(111, polar=True)
    c = plt.scatter(theta, r, c=colors, s=area, cmap=plt.cm.hsv)
    c.set_alpha(0.50)
    plt.colorbar()
    plt.show()

if __name__ == '__main__':
    ser = serial.Serial('/dev/ttyUSB0',19200,timeout=0)
    h = BraduinoUSBCommunicator()  
    res = 5
    positions = generate_servo_positions(res)
    measurements = [] 
    for position in positions:
        h.move_servos(position[0]/180.0,position[1]/180.0)
        t = read_from_serial(ser)
        if t:
            measurements.append((position[0],position[1],t))
        else:
            measurements.append((position[0],position[1],1153.0))
    plot(measurements,res)
