#include <Servo.h>
#include <NewPing.h>
#include <Timer.h>
#include <ArduinoJson.h>

#define LED_BUILTIN           13
#define ESC_PIN               10 
#define SERVO_PIN              9
#define RIGHT_TRIGGER_PIN     12  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define RIGHT_ECHO_PIN        11  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define LEFT_TRIGGER_PIN       7  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define LEFT_ECHO_PIN          6  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define BACK_TRIGGER_PIN       4  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define BACK_ECHO_PIN          5  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define FRONT_TRIGGER_PIN      8  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define FRONT_ECHO_PIN         3  // Arduino pin tied to echo pin on the ultrasonic sensor.

#define FORWARD_MAX         1680
#define FORWARD_MIN         1550
#define REVERSE_MAX         1320
#define REVERSE_MIN         1430
#define THROTTLE_BAKE       1500
#define THROTTLE_NETURAL    1500

#define CENTER              1600
#define LEFT                1870
#define RIGHT               1330

#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing rightSonar(RIGHT_TRIGGER_PIN, RIGHT_ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing leftSonar(LEFT_TRIGGER_PIN, LEFT_ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing backSonar(BACK_TRIGGER_PIN, BACK_ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing frontSonar(FRONT_TRIGGER_PIN, FRONT_ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

boolean startReading  = false;        // default to not reading 
boolean DebugMode = true;
boolean DoNotSendToESC = false;
boolean DoNotSendToServo = false;

Servo   ESControl;
Servo   ServoControl;

enum Direction{
   forward,
   reverse,
   halt,
   none
};

Timer t;

Direction CurrentDirection = none;

void setup() {
   Serial.begin(115200);
   Serial.setTimeout(50);
   outputTrace("Started Setup........");

   sendArmMsgToEsc();
   digitalWrite(LED_BUILTIN, HIGH);
   delay(1000);
   digitalWrite(LED_BUILTIN, LOW);

   ServoControl.attach(SERVO_PIN);
   controlServo(CENTER);
 //  t.every(50, outputDistances, 0);
}

void loop() {
   t.update();
   readInput();
}

void sendArmMsgToEsc() {
   ESControl.detach();
   delay(2000);
   ESControl.attach(ESC_PIN);
   controlEsc(THROTTLE_NETURAL);
   outputTrace("Arming........");   // just some display message 
   delay(2000);
   outputTrace("Arming........After delay");  
   controlEsc(THROTTLE_NETURAL);
}

void processEscRequest(JsonObject &requestMessage) {
   int requestedSpeed;

   
   if (requestMessage["direction"] == "fwd") {
      outputTrace("Forward requested");
      
      requestedSpeed = requestMessage.get<int>("speed");
      if (requestedSpeed == 0) {
         return;
      }
      
      if (CurrentDirection == reverse){
         stopEsc();
      }
      
      controlEsc(map(requestedSpeed, 0, 100, FORWARD_MIN, FORWARD_MAX) );
      CurrentDirection = forward;
      
   } else if (requestMessage["direction"] == "rev") {
     outputTrace("Reverse requested");
     
      requestedSpeed = requestMessage.get<int>("speed");
      if (requestedSpeed == 0) {
         return;
      }
      
      if (CurrentDirection == forward){
         stopEsc();
      }
      controlEsc(map(requestedSpeed, 0, 100, REVERSE_MIN, REVERSE_MAX) );
      CurrentDirection = reverse;
      
   } else if (requestMessage["direction"] == "stop" ) {
     outputTrace("Stop requested");
     stopEsc();
     
   } else {
     outputTrace("Invalid esc request");
   }
}

void stopEsc() {
  outputTrace("Stopping");
  controlEsc(THROTTLE_BAKE);
  delay(100);
}

void processServoRequest(JsonObject &requestMessage) {
   int requestedDirection;
   outputTrace("InServo");

   if (requestMessage["direction"] == "right") {
      outputTrace("Right requested");
      
      requestedDirection = requestMessage.get<int>("percent");
      if (requestedDirection == 0) {
         return;
      }
  
      controlServo(map(requestedDirection, 0, 100, CENTER, RIGHT) );
      
   } else if (requestMessage["direction"] =="left") {
      outputTrace("left requested");
     
      requestedDirection = requestMessage.get<int>("percent");

      if (requestedDirection == 0) {
         return;
      }
  
      controlServo(map(requestedDirection, 0, 100, CENTER, LEFT) );
      
   } else if (requestMessage["direction"] == "center") {
     outputTrace("Center requested");
     controlServo(CENTER);
     
   } else {
     outputTrace("Invalid servo request");
   }
}

void processCommandRequest(JsonObject &command) {
   if (command["command"] == "traceon") {
      DebugMode = true;
      outputTrace("Debug Mode on");
   } else if (command["command"] == "traceoff") {
      outputTrace("Debug mode off");
      DebugMode = false;
   } else if (command["command"] == "arm") {
      sendArmMsgToEsc();
   } else {
      outputTrace( "Invalid command: ");
   }
}

void readInput() {
   int i = 0;
   StaticJsonBuffer<250> jsonBuffer;
   char msgRead[100] = {0};
   
   while(Serial.available() && i < sizeof(msgRead) && msgRead[i] != '\n') {
      msgRead[i++] = Serial.read();
      if (!Serial.available() && msgRead[i] != '\n' ) 
         delay(50);
   }
   
   if (i) {
      JsonObject &jsonRqst = jsonBuffer.parseObject(msgRead);
      if (!jsonRqst.success()) {
         outputTrace( "Invalid Json; " + String(msgRead));
         return;
      }

      String topic = jsonRqst["topic"];
      JsonObject &payload = jsonRqst["payload"];
  
      if (topic == "servo") {
         processServoRequest(payload);
      } else if (topic == "esc") {
         processEscRequest(payload);
      } else if (topic == "cmd") {
         processCommandRequest(payload);
      } else {
         outputTrace( "Invalid topic, " + String(topic));
      }
   }
}   

void outputTrace(String msg) {
   if (DebugMode) {
      unsigned long time;
      StaticJsonBuffer<200> jsonBuffer;
      JsonObject& jsonMessage = jsonBuffer.createObject();
      jsonMessage["topic"] = "trace";
      time = millis();
      jsonMessage["time"] = time;      
      jsonMessage["payload"] = msg;
      jsonMessage.printTo(Serial);
      Serial.println();
   }  
}

void controlEsc(int requestedSpeed) {
   outputTrace("Requested speed: " + String(requestedSpeed));
  
   if (requestedSpeed > FORWARD_MAX || requestedSpeed < REVERSE_MAX){
      outputTrace("Invalid speed");
      return;
   }
   
   if (!DoNotSendToESC) {
      outputTrace("Sending to ESC");
      ESControl.writeMicroseconds(requestedSpeed); 
   }
}


void controlServo(int requestedDirection) {
   outputTrace("Control servo");
  
   if (requestedDirection > LEFT || requestedDirection < RIGHT){
      outputTrace("Invalid servo direction");
      return;
   }
   
   if (!DoNotSendToServo) {
      outputTrace("Sending to Servo " + String(requestedDirection));
      ServoControl.writeMicroseconds(requestedDirection); 
   }
}

void outputDistances(void* context)
{
  unsigned long time;
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& jsonMessage = jsonBuffer.createObject();
  jsonMessage["topic"] = "distances";
  time = millis();
  jsonMessage["time"] = time; 
  JsonObject& payload = jsonMessage.createNestedObject("payload");
  payload["front"] = frontSonar.ping_cm();
  payload["back"]  = backSonar.ping_cm();
  payload["left"]  = leftSonar.ping_cm();
  payload["right"] = rightSonar.ping_cm();
  jsonMessage.printTo(Serial);
  Serial.println();
}
