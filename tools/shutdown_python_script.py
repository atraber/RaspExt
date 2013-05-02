import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BCM)
GPIO.setup(7, GPIO.IN)
var=0
x=0
while (var==0) :
    if GPIO.input(7):
        x=int(x)+1
        time.sleep(0.25)
        print ("pressed")
    if (x%2==0):
        time.sleep(0.1)
    else:
        raise SystemExit