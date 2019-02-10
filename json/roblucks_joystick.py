#!/usr/bin/env python
# coding: Latin-1

# Load library functions we want
import time
import pygame
import serial

# Function to set all drives off
def MotorOff():
    print("MotorOff")

axisUpDown = 1                          # Joystick axis to read for up / down position
axisUpDownInverted = False              # Set this to True if up and down appear to be swapped
axisLeftRight = 3                       # Joystick axis to read for left / right position
axisLeftRightInverted = False           # Set this to True if left and right appear to be swapped
interval = 0.05                         # Time between keyboard updates in seconds, smaller responds faster but uses more processor time
stopButton = 0
increaseSpeedButton = 6
decreaseSpeedButton = 7
traceMessageButton = 12
armButton = 4
defaultFowardSpeed = 10
defaultReverseSpeed = 10
speedSteps = 5
maxSpeed = 80
threePointLeftButton = 3
threePointRightButton = 1

# Setup pygame and key states
global hadEvent
global moveUp
global moveDown
global moveLeft
global moveRight
global moveQuit
global leftRightValue
global sendStop
global newCommand
global icreaseSpeed
global decreaseSpeed
global currentSpeed
global traceCommand
global armCommand
global threePointLeft
global threePointRight
threePointLeft = False
threePointRight = False
hadEvent = True
moveUp = False
moveDown = False
moveLeft = False
moveRight = False
moveQuit = False
sendStop = False
increaseSpeed = False
decreaseSpeed = False
newCommand = False
traceMessage = True
traceCommand = False
armCommand = False
currentSpeed = 0
pygame.init()
pygame.joystick.init()
joystick = pygame.joystick.Joystick(1)
joystick.init()


ArdunioSer = serial.Serial('/dev/ttyACM0', 115200)
ArdunioSer.flushInput()

def ThreePointTurn(direction, direction2):
    global moveUp
    global moveDown
    global currentSpeed
    
    SendArdunioMsg("servo:"+direction+"100;")
    time.sleep(.2)
    SendArdunioMsg("esc:fwd" + str(currentSpeed) + ";")
    time.sleep(.9)
    SendArdunioMsg("servo:"+direction2+"100;")
    time.sleep(.2)
    SendArdunioMsg("esc:rev" + str(currentSpeed) + ";")
    time.sleep(.9)
    SendArdunioMsg("servo:centre;")
    if moveUp:
        SendArdunioMsg("esc:fwd" + str(currentSpeed) + ";")
    elif moveDown:
        SendArdunioMsg("esc:fwd" + str(currentSpeed) + ";")
    else:
        currentSpeed = 0
        SendArdunioMsg("esc:stop;")
    
def SendArdunioMsg(msgToSend):
    ArdunioSer.write(msgToSend.encode())
    print(msgToSend)
            
    while ArdunioSer.in_waiting:
        try:
            msgRead = (ArdunioSer.readline()).decode("utf-8")
            print("Message from ardunio: " + msgRead)
        except Exception as e:
            print("invalid msg, from Ardunio: ")
 


# Function to handle pygame events
def PygameHandler(events):
    # Variables accessible outside this function
    global hadEvent
    global moveUp
    global moveDown
    global moveLeft
    global moveRight
    global moveQuit
    global leftRightValue
    global sendStop
    global increaseSpeed
    global decreaseSpeed
    global traceMessage
    global traceCommand
    global armCommand
    global threePointRight
    global threePointLeft
    
    # Handle each event individually
    for event in events:
        
        if event.type == pygame.JOYBUTTONUP:
            print("JOYBUTTON " + str(event.button) + " UP")
        elif event.type == pygame.JOYBUTTONDOWN:
            if event.button == stopButton:
                hadEvent = True
                sendStop = True
                print("stopbut")
            elif event.button == increaseSpeedButton:
                hadEvent = True
                increaseSpeed = True
                print("increase speed")
            elif event.button == decreaseSpeedButton:
                hadEvent = True
                decreaseSpeed = True
            elif event.button == traceMessageButton:
                hadEvent = True
                traceMessage = not traceMessage
                traceCommand = True
            elif event.button == armButton:
                hadEvent = True
                armCommand = True
            elif event.button == threePointLeftButton:
                hadEvent = True
                threePointLeft = True
            elif event.button == threePointRightButton:
                hadEvent = True
                threePointRight = True
            
        elif event.type == pygame.QUIT:
            # User exit
            hadEvent = True
            moveQuit = True
        elif event.type == pygame.KEYDOWN:
            # A key has been pressed, see if it is one we want
            hadEvent = True
            if event.key == pygame.K_ESCAPE:
                moveQuit = True
        elif event.type == pygame.KEYUP:
            # A key has been released, see if it is one we want
            hadEvent = True
            if event.key == pygame.K_ESCAPE:
                moveQuit = False
        elif event.type == pygame.JOYAXISMOTION:
            # A joystick has been moved, read axis positions (-1 to +1)
            hadEvent = True
            upDown = joystick.get_axis(axisUpDown)
            leftRight = joystick.get_axis(axisLeftRight)
            # Invert any axes which are incorrect
            if axisUpDownInverted:
                upDown = -upDown
            if axisLeftRightInverted:
                leftRight = -leftRight
            # Determine Up / Down values
            if upDown < -0.1:
                moveUp = True
                moveDown = False
            elif upDown > 0.1:
                moveUp = False
                moveDown = True
            else:
                moveUp = False
                moveDown = False
            # Determine Left / Right values
            if leftRight < -0.1:
                moveLeft = True
                moveRight = False
                leftRightValue = int(abs(leftRight*100))
            elif leftRight > 0.1:
                moveLeft = False
                moveRight = True
                leftRightValue = int(abs(leftRight*100))
            else:
                moveLeft = False
                moveRight = False
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
            if moveQuit:
                break
               
            if moveUp:
                msgToSend = "esc:fwd"
            elif moveDown:
                msgToSend = "esc:rev"
            else:
                currentSpeed = 0
                msgToSend = "esc:stop;"
                
            if sendStop:
                sendStop = False
                msgToSend = "esc:stop;"
                
            if msgToSend != "esc:stop;":
                if currentSpeed == 0:
                    if moveUp:
                        currentSpeed = defaultFowardSpeed
                    else:
                        currentSpeed = defaultReverseSpeed
                    
                if increaseSpeed:
                    increaseSpeed = False
                    if currentSpeed + speedSteps <= maxSpeed:
                        currentSpeed += speedSteps
                    
                if decreaseSpeed:
                    decreaseSpeed = False
                    if currentSpeed > speedSteps:
                        currentSpeed -= speedSteps
                    
                msgToSend += str(currentSpeed) + ";"
                 
            if moveLeft:
                msgToSend += "servo:left" + str(leftRightValue) + ";"
            elif moveRight:
                msgToSend += "servo:right" + str(leftRightValue) + ";"
            else:
                msgToSend += "servo:center;"
                    
            if traceCommand:
                traceCommand = False
                if traceMessage:
                    msgToSend += "cmd:traceon;"
                else:
                    msgToSend += "cmd:traceoff;"
                    
            if armCommand:
                armCommand = False
                msgToSend += "cmd:arm;"
 
            if threePointLeft:
                threePointLeft = False
                ThreePointTurn("left","right")
            elif threePointRight:
                threePointRight = False
                ThreePointTurn("right","left")
            else:
                print(msgToSend)
                SendArdunioMsg(msgToSend)
    #        ArdunioSer.write(msgToSend.encode())
    #        
    #        while ArdunioSer.in_waiting:
    #            msgRead = ArdunioSer.readline()
    #            print("Message from ardunio: " + str(msgRead.decode()))
                
        # Wait for the interval period
        time.sleep(interval)
    # Disable all drives
    MotorOff()
except KeyboardInterrupt:
    # CTRL+C exit, disable all drives
    MotorOff()
