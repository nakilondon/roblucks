import paho.mqtt.client as mqtt
import json
import time
import serial
import json

def on_connect( client, userdata, flags, rc):
    print ("Connected with Code: " + str(rc) )
    client.subscribe("esc")
    client.subscribe("servo")
    client.subscribe("command")

def on_message(client, userdata, msg):
    msgDict = {}
    msgDict['topic'] = msg.topic
    msgDict['payload'] = json.loads(msg.payload.decode("utf-8"))
    jsonMsg = json.dumps(msgDict)
    print (str(jsonMsg))
    msgToSend = str(msg.topic) + ":"
    print(msgToSend)
    if msg.topic == "esc":
        msgToSend += msgDict['payload']['direction']
        if msgDict['payload']['direction'] != "stop":
            msgToSend += str(msgDict['payload']['speed'])
        msgToSend += ";"
    elif msg.topic == "servo":
        msgToSend += msgDict['payload']['direction']
        if msgDict['payload']['direction'] != "center":
            msgToSend += str(msgDict['payload']['percent'])
        msgToSend += ";"
    else:
        msgToSend += msgDict['payload']['command'] + ";"
    print(msgToSend)
    ArdunioSer.write(msgToSend.encode())
    
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect("localhost", 1883, 60)

ArdunioSer = serial.Serial('/dev/ttyACM0', 115200)
ArdunioSer.flushInput()

while True:
    while ArdunioSer.in_waiting:
        try:
            msgRead = (ArdunioSer.readline()).decode("utf-8")
        except Exception as e:
            print("characters from ardunio: ")
        
        #print("msgRead: " + str(msgRead))
        try:
            jsonMsg = json.loads(msgRead)
            topic = jsonMsg['topic']
            payload = jsonMsg['payload']
            client.publish(topic, str(json.dumps(payload)))
        except Exception as e:
            print("invalid json, from Ardunio: ")
    
    client.loop(timeout=1.0, max_packets=3)
        

