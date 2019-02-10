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
global readCount
readCount = 0
hadEvent = True
sendStop = False
currentSpeed = 0
pygame.init()
pygame.joystick.init()
joystick = pygame.joystick.Joystick(0)
joystick.init()

ArdunioSer = serial.Serial('/dev/ttyACM0', 115200)
ArdunioSer.flushInput()

class PayLoad(object):
    def __init__(self, j):
        self.__dict__ = json.loads(j)

def ThreePointTurn(direction, direction2):
    SendArdunioMsg("servo:"+direction+"100;esc:rev" + str(currentSpeed) + ";")
    time.sleep(.9)
    SendArdunioMsg("servo:"+direction2+"100;esc:fwd" + str(currentSpeed) + ";")
    time.sleep(.9)
    SendArdunioMsg("servo:centre;esc:stop;")
    
def on_connect( client, userdata, flags, rc):
    print ("Connected with Code: " + str(rc) )
    client.subscribe("distance")

def on_message(client, userdata, msg):
    jsonMsg = json.loads(msg.payload.decode("utf-8"))
    print (str(jsonMsg))
    
def SendArdunioMsg(msgToSend):
    global readCount
    print(msgToSend)
    ArdunioSer.write(msgToSend.encode())
            
    while ArdunioSer.in_waiting:
        msgRead = (ArdunioSer.readline()).decode("utf-8")
        print("Message from ardunio: " + msgRead)
        readCount += 1
        if readCount > 4:
            #ardunioMsg = json.loads(msgRead);
            #print(ardunioMsg['topic']);
            ardunioMsg = PayLoad(str(msgRead))
            print("topic: " + ardunioMsg.topic)
            if ardunioMsg.topic == "distances":
                print("left: " + str(ardunioMsg.data['left']) + " right: " + str(ardunioMsg.data['right']))
            else:
                print("data: " + ardunioMsg.data)


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
try:
    print ('Press [ESC] to quit')
    # Loop indefinitely
    
    while True:
        msgToSend = ""
        # Get the currently pressed keys on the keyboard
        PygameHandler(pygame.event.get())
        if hadEvent:
            # Keys have changed, generate the command list based on keys
            hadEvent = False
            
            if sendStop:
                sendStop = False
                msgToSend = "esc:stop;"
                SendArdunioMsg(msgToSend)
        #else:        
        SendArdunioMsg("esc:fwd1;")
        print("hi")        
        # Wait for the interval period
        time.sleep(loopDelay)
    # Disable all drives
    MotorOff()
except KeyboardInterrupt:
    # CTRL+C exit, disable all drives
    MotorOff()
