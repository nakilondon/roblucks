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

String  readValue;
String  perviousReadValue;


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
   if (readInput()) {
      processMessage(readValue);
   }
}

void processMessage(String requestMessage) {
  
   int commandStart = 0;
   int commandEnd = requestMessage.indexOf(";");

   if (commandEnd == -1) {
      outputTrace( "Invalid message: " + requestMessage);
   }
   
   while (commandEnd != -1 ) {
      if (requestMessage.substring(commandStart, commandStart+4) == "esc:") {
         processEscRequest(requestMessage.substring(commandStart+4,commandEnd));
      } else if (requestMessage.substring(commandStart, commandStart+6) == "servo:") {
         processServoRequest(requestMessage.substring(commandStart+6, commandEnd));
      } else if (requestMessage.substring(commandStart, commandStart+4) == "cmd:") {
         processCommandRequest(requestMessage.substring(commandStart+4, commandEnd));
      } else {
         outputTrace( "Invalid message: " + requestMessage);
      }
      
      commandStart = commandEnd + 1;
      commandEnd = requestMessage.indexOf(";", commandStart);
   }
  
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

void processEscRequest(String requestMessage) {
   int requestedSpeed;
   
   if (requestMessage.startsWith("fwd")) {
      outputTrace("Forward requested");
      
      requestedSpeed = requestMessage.substring(3).toInt();
      if (requestedSpeed == 0) {
         return;
      }
      
      if (CurrentDirection == reverse){
         stopEsc();
      }
      
      controlEsc(map(requestedSpeed, 0, 100, FORWARD_MIN, FORWARD_MAX) );
      CurrentDirection = forward;
      
   } else if (requestMessage.startsWith("rev")) {
     outputTrace("Reverse requested");
     
      requestedSpeed = requestMessage.substring(3).toInt();
      if (requestedSpeed == 0) {
         return;
      }
      
      if (CurrentDirection == forward){
         stopEsc();
      }
      controlEsc(map(requestedSpeed, 0, 100, REVERSE_MIN, REVERSE_MAX) );
      CurrentDirection = reverse;
      
   } else if (requestMessage.startsWith("stop")) {
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

void processServoRequest(String requestMessage) {
   int requestedDirection;
   
   if (requestMessage.startsWith("right")) {
      outputTrace("Right requested");
      
      requestedDirection = requestMessage.substring(5).toInt();
      if (requestedDirection == 0) {
         return;
      }
  
      controlServo(map(requestedDirection, 0, 100, CENTER, RIGHT) );
      
   } else if (requestMessage.startsWith("left")) {
     outputTrace("left requested");
     
      requestedDirection = requestMessage.substring(4).toInt();
      if (requestedDirection == 0) {
         return;
      }
  
      controlServo(map(requestedDirection, 0, 100, CENTER, LEFT) );
      
   } else if (requestMessage.startsWith("c")) {
     outputTrace("Center requested");
     controlServo(CENTER);
     
   } else {
     outputTrace("Invalid servo request");
   }
}

void processCommandRequest(String command) {
   if (command=="traceon") {
      DebugMode = true;
      outputTrace("Debug Mode on");
   } else if (command == "traceoff") {
      outputTrace("Debug mode off");
      DebugMode = false;
   } else if (command == "arm") {
      sendArmMsgToEsc();
   } else {
      outputTrace( "Invalid command: " + command);
   }
}

boolean readInput() {
   if (Serial.available()) {
     digitalWrite(LED_BUILTIN, HIGH); // on the onboard LED, for visual indicator  
     readValue = Serial.readString();
   }

   readValue.trim();
   readValue.toLowerCase();

   if (readValue != perviousReadValue) {
      perviousReadValue = readValue;
      outputTrace("Message read: " + readValue);
      return true;
   }
   
   return false;

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
