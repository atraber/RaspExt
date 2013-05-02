import RPi.GPIO as GPIO
import time
import os

GPIO.setmode(GPIO.BCM)
GPIO.setup(7, GPIO.IN)
x=0
while True:
	if GPIO.input(7):
		os.system("sudo shutdown -h now")
		print ("pressed")
		break
	else:
		time.sleep(0.1)
