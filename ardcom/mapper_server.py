from flask import Flask, request, redirect, url_for,render_template, session
import os
import math
import time
# from pantiltcmd import BraduinoUSBCommunicator

app=Flask(__name__)

SECRET_KEY = "secret secret"

app.config.from_object(__name__)

radius = 400
connected = True

device = None

distances_dictionary = {} 
 
@app.route("/",methods=["GET"])
def home():	
	#give them the home page at a given radius
	return render_template("ui.html",radius=radius)

# @app.route("/move",methods=["POST"])
# def move():
# 	try:
# 		#unpack the position variables from the request
# 		x = int(request.form.get("x"))
# 		y = int(request.form.get("y"))
# 		if device:
# 			#send the position variables to the Braduino
# 			device.move_servos(x/float(radius), y/float(radius))
# 		else:
# 			#let the front end know something failed
# 			return "false"
# 	except Exception as e:
# 		raise e #debug
# 	return "true"

@app.route("/move",methods=["POST"])
def move():
	x = int(request.form.get("x"))
	y = int(request.form.get("y"))
	print x, y
	return "true"

@app.route("/scan",methods=["POST"])
def scan():
	r = radius
    for i in range(1,r,2):
	    for j in range(1,r):
	        pos1 = float(i)/r
	        pos2 = float(j)/r
	        device.move_servos(pos1,pos2)
	    for k in range(r,1,-1):
	        pos1 = float(i+1)/r
	        pos2 = float(k)/r
	        device.move_servos(pos1,pos2)



if __name__ == '__main__':
    try:
    	# device = BraduinoUSBCommunicator() #connect to the device
        port = int(os.environ.get('PORT', 5000))
        app.run(host='0.0.0.0', port=port, debug=True)
    except Exception as e:
        app.logger.debug("%s"%e)