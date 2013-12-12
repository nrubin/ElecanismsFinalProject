import usb_comm
import time as t
def main():

	# Initialize usb
	husb = usb_comm.usb_comm()
	MID_L = 32768;
	MID_R = 32832;
	
	while(1):
		
		[current_val, emf_val, fb_val, enc_count_val] = husb.get_vals()
		if emf_val > MID_R:
			print [current_val, 1, fb_val, enc_count_val]
		elif emf_val < MID_L:
			print [current_val, -1, fb_val, enc_count_val]
		else:
			print [current_val, 0, fb_val, enc_count_val]
		#t.sleep(0.5)
		#print husb.get_vals()

if __name__ == '__main__':
	main()
