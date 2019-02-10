#include <ArduinoJson.h>

// ---------------------------------------------------------------------------
// Example NewPing library sketch that does a ping about 20 times per second.
// ---------------------------------------------------------------------------

#include <NewPing.h>

#define RIGHT_TRIGGER_PIN  12  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define RIGHT_ECHO_PIN     11  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define LEFT_TRIGGER_PIN  7// Arduino pin tied to trigger pin on the ultrasonic sensor.
#define LEFT_ECHO_PIN     6  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define BACK_TRIGGER_PIN  4  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define BACK_ECHO_PIN     5  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define FRONT_TRIGGER_PIN  8  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define FRONT_ECHO_PIN     3  // Arduino pin tied to echo pin on the ultrasonic sensor.

#define MAX_DISTANCE 300 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing rightSonar(RIGHT_TRIGGER_PIN, RIGHT_ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing leftSonar(LEFT_TRIGGER_PIN, LEFT_ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing backSonar(BACK_TRIGGER_PIN, BACK_ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing frontSonar(FRONT_TRIGGER_PIN, FRONT_ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

unsigned long time;

void setup() {
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  while (!Serial) continue;
}


void outputDistances()
{
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& jsonMessage = jsonBuffer.createObject();
  jsonMessage["topic"] = "distances";
  time = millis();
  jsonMessage["time"] = time; 
  JsonObject& data = jsonMessage.createNestedObject("data");
  data["front"] = frontSonar.ping_cm();
  data["back"]  = backSonar.ping_cm();
  data["left"]  = leftSonar.ping_cm();
  data["right"] = rightSonar.ping_cm();
  jsonMessage.printTo(Serial);
  Serial.println();
}

void loop() {
  delay(1000);
  outputDistances();
}
