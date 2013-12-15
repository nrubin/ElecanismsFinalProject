import usb.core
import time
class USBComm:

    def __init__(self):
        # print "Your UART cable better be plugged into the USB 2.0 port, or else kittens will cry"
        self.GOTO_POS = 0
        self.SET_0 = 1
        self.GET_POS = 2
        self.dev = usb.core.find(idVendor = 0x6666, idProduct = 0x0003)
        if self.dev is None:
            raise ValueError('no USB device found matching idVendor = 0x6666 and idProduct = 0x0003')
        self.dev.set_configuration()

    def close(self):
        self.dev = None

    def goto_pos(self, pos):
        try:
            self.dev.ctrl_transfer(0x40, self.GOTO_POS, int(pos), 0)
        except usb.core.USBError as e:
            print e
            print "Could not send GOTO_POS vendor request."
            raise e

    def set_zero(self):
        try:
            self.dev.ctrl_transfer(0x40, self.SET_0)
        except usb.core.USBError as e:
            print e
            print "Could not send SET_0 vendor request."
            raise e

    def get_pos(self):
        try:
            ret = self.dev.ctrl_transfer(0xC0, self.GET_POS, 0, 0, 4)
        except usb.core.USBError:
            print "Could not send GET_POS vendor request."
        else:
            return [int(ret[0])+int(ret[1])*256, int(ret[2])+int(ret[3])*256]

    def values(self):
        positions = self.get_pos();
        print "current_pos = %s, desired_pos = %s" % (positions[0], positions[1])
        if positions[0] == positions[1]:
            return True

if __name__ == '__main__':
    u = USBComm()
    u.set_zero()
    # direction = int(raw_input("direction: 1 or 0 \n"))
    
    while True:
        pos = int(raw_input("pos: 0 - 65536 \n"))
        u.goto_pos(pos)
        while not u.values():
            u.values()
            time.sleep(0.5)
    # while True:
    #     v = u.get_vals()[1]
    #     if v > stack[0]:
    #         stack = [v] + stack
    #         print v
