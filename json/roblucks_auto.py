#!/usr/bin/env python
# coding: Latin-1

# Load library functions we want
import paho.mqtt.client as mqtt
import time
import pygame
import serial
import json

# Function to set all drives off
def MotorOff():
    print("MotorOff")

loopDelay = 0.01
stopButton = 0
defaultFowardSpeed = 20
defaultReverseSpeed = 20
speedSteps = 5
maxSpeed = 80

# Setup pygame and key states
global hadEvent
global sendStop
global newCommand
hadEvent = True
sendStop = False
currentSpeed = 0
pygame.init()
pygame.joystick.init()
joystick = pygame.joystick.Joystick(0)
joystick.init()



class PayLoad(object):
    def __init__(self, j):
        self.__dict__ = json.loads(j)

def ThreePointTurn(direction, direction2):
#    SendArdunioMsg("servo:"+direction+"100;esc:rev" + str(currentSpeed) + ";")
    time.sleep(.9)
#    SendArdunioMsg("servo:"+direction2+"100;esc:fwd" + str(currentSpeed) + ";")
    time.sleep(.9)
#    SendArdunioMsg("servo:centre;esc:stop;")
    
def on_connect( client, userdata, flags, rc):
    print ("Connected with Code: " + str(rc) )
    client.subscribe("distances")

def on_message(client, userdata, msg):
    msgDecoded = str(msg.payload.decode("utf-8","ignore"))
    try:
        msgDict = json.loads(msgDecoded)
    except ValueError as e:
        print("exception: " +str(e) + " data: " + str(msgDecoded))
        
    if msg.topic == "distances":
        ProcessDistances( msgDict)
    else:
        print("Unkown topic: " + msg.topic)
 #   print ("left: " + str(msgDict['left']) + " right: " + str(msgDict['right']))

def ProcessDistances(distances):
    left = distances['left']
    right = distances['right']
    front = distances['front']
    back = distances['back']
    
    width = left + right
    if abs(left-right) > width*.2:
        if left > right:
            Adjust("right", "left")
        else:
            Adjust("left", "right")
        
def Adjust(direction1, direction2):
    payload = {}
    payload['direction'] = direction1
    payload['percent'] = 10
    print(json.dumps(payload))
    client.publish("servo",json.dumps(payload))
    delay(0.1)
    payload.pop('percent', None)
    client.publish("servo", json.dumps(payload))
    delay(0.3)
    payload['direction'] = direction2
    payload['percent'] = 10
    client.publish("servo",json.dumps(payload))

    
    
# Function to handle pygame events
def PygameHandler(events):
    # Variables accessible outside this function
    global hadEvent

    global sendStop
    
    # Handle each event individually
    for event in events:
        
        if event.type == pygame.JOYBUTTONUP:
            print("JOYBUTTON " + str(event.button) + " UP")
        elif event.type == pygame.JOYBUTTONDOWN:
            if event.button == stopButton:
                hadEvent = True
                sendStop = True
        elif event.type == pygame.QUIT:
            # User exit
            hadEvent = True
            moveQuit = True
        elif event.type == pygame.KEYDOWN:
            # A key has been pressed, see if it is one we want
            hadEvent = True
            if event.key == pygame.K_ESCAPE:
                moveQuit = True

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect("localhost", 1883, 60)


try:
    print ('Press [ESC] to quit')
    # Loop indefinitely
    
    while True:
        client.loop(timeout=1.0, max_packets=1)
        msgToSend = ""
        # Get the currently pressed keys on the keyboard
        PygameHandler(pygame.event.get())
        if hadEvent:
            # Keys have changed, generate the command list based on keys
            hadEvent = False
            
            if sendStop:
                sendStop = False
#                msgToSend = "esc:stop;"
#                SendArdunioMsg(msgToSend)
       # else:
          #  print("fwd")
            SendArdunioMsg("esc:fwd1;")
                
        # Wait for the interval period
        time.sleep(loopDelay)
    # Disable all drives
    MotorOff()
except KeyboardInterrupt:
    # CTRL+C exit, disable all drives
    MotorOff()
