#include "Bounce2.h"
#include "AudioPlayer.h"
#include <SoftwareSerial.h>

// Uncomment this line to use the ultrasonic sensor.
#define USE_ULTRASONIC

//#define PIN_RELAY_REDLIGHT 7
#define PIN_RELAY_SMOKE 6
#define PIN_RELAY_LID 4
#define PIN_RELAY_CYLINDER_MAN 5

#define PIN_TRIGGER_BUTTON 8

#define PIN_AUDIO_RX 9
#define PIN_AUDIO_TX 10
#define AUDIO_BOOT 1
#define AUDIO_SCREAM 2
#define AUDIO_CALIBRATIONFAIL 3
#define AUDIO_TRUMP_1 4
#define AUDIO_TRUMP_2 5
#define AUDIO_TRUMP_3 6
#define AUDIO_TRUMP_4 7
#define AUDIO_TRUMP_5 8
#define AUDIO_TRUMP_6 9

//Blue Wire
#define LIGHT_TX 2

//Orange Wire
#define LIGHT_RX 3

// Defines which audio files are used for the screen/trump sound.
const int NUMBER_OF_SCREAMS = 1 /* 6 */;
int screamFiles[] = { AUDIO_SCREAM /*, AUDIO_TRUMP_1, AUDIO_TRUMP_2, AUDIO_TRUMP_3, AUDIO_TRUMP_4, AUDIO_TRUMP_5, AUDIO_TRUMP_6 */  };
unsigned long screamLength[] = { 3000 /*, 2000, 2500, 3300, 3650, 2850, 7300 */};
unsigned long beforeScreamDelay = 0 /* 1000 */;
unsigned long afterScreamDelay = 0 /* 2500 */;


// Pins for the ultrasonic sensor:
const int trigPin = 11;
const int echoPin = 12;
int ultrasonicThreshold = -1;

// We need to add a timeout for the longes range that the Sensor can sense, so that
// it does not get stuck waiting for an echo that never comes.
const long timeout = 400 * 29 * 2; //Timeout in microseconds for a max range of 400 cm

Bounce inputTrigger;

const long SMOKE_ON_TIME = /*10*/10 * 1000L;  // SMOKE ON time after start
const long SMOKE_OFF_TIME = /* 60 */ 60 * 1000L;  // SMOKE OFF time

#ifdef USE_ULTRASONIC
const int RESET_DELAY_SECONDS = 10; // time before box will trigger again.
#else
const int RESET_DELAY_SECONDS = 2; // time before box will trigger again.
#endif

long lastTimeSmokeOn;
long lastTimeSmokeOff;
bool smokeOn;

long nextTriggerTimePossible = 0;

int currentScreamNumber = 0;

SoftwareSerial lightSerial(LIGHT_RX, LIGHT_TX);

int measureDistanceInCm()
{

  // The sensor is triggered by a HIGH signal for 10 microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse and
  // to reset the sensor if it didn't register an echo from the last ping:

  digitalWrite(trigPin, LOW);
  delayMicroseconds(10); //This delay lets the capacitors empty and resets the sensor
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // The echo pin is used to read the signal from the Sensor: A HIGH pulse
  // whose duration is the time (in microseconds) from the sending of the ping
  // to the reception of its echo off of an object.

  long duration = pulseIn(echoPin, HIGH, timeout);


  // The speed of sound is 343 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the object we
  // take half of the distance travelled.
  return (int) (duration / (29 * 2));
}

// Calibrate the distance, return true if successful, false if not.
// If success, sets threshold variable. Delta is the difference allowed in the distance
// (should be around 20 cm or so).
bool setupAndCalibrateUltrasonic(int delta)
{
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  int minDist = 8000;
  for (int i = 0; i < 6; ++i) {
    int dist = measureDistanceInCm();
    Serial.print("Calibrate: ");
    Serial.println(dist);
    if (dist > 0 && dist < minDist) {
      minDist = dist;
    }
    delay(150);
  }
  if (minDist >= 1000 || minDist <= delta) {
    Serial.println("Calibration failed.");
    return false;
  }

  for (int i = 0; i < 15; ++i) {
    int dist = measureDistanceInCm();
    Serial.print("Check: ");
    Serial.println(dist);
    if (dist > 0) {
      if (dist > minDist + delta || dist <= minDist - delta) {
        Serial.println("Check failed.");
        return false;
      }
    }
    delay(150);
  }

  ultrasonicThreshold = minDist - delta;
  return true;
}

// Returns true if the ultrasonic sensor is triggered.
bool isUltrasonicTriggered()
{
  int dist = measureDistanceInCm();
  Serial.println(dist);
  if (dist > 0 && dist < ultrasonicThreshold) {
    return true;
  }
  else {
    return false;
  }
}

bool isFlickering = false;  // true means light is currently flicker red/white
bool isFlickerRed = false;  // true means light is currently in red part of red/white flicker
const int millisFlicker = 60; // milliseconds of each flicker
long lastFlickerStart; // time last flicker started.

// Begin flickering the light
void startFlicker()
{
  isFlickering = true;
  isFlickerRed = true;
  lastFlickerStart = millis();
  changeColor(255, 0, 0);
}

// Stop flickering the light
void endFlicker()
{
  isFlickering = false;
  changeColor(0, 0, 0);
}

// Update the flickering.
void updateFlicker()
{
  if (isFlickering) {
    long currentMillis = millis();
    if (currentMillis > lastFlickerStart + millisFlicker) {
      lastFlickerStart = currentMillis;
      isFlickerRed = !isFlickerRed;
      if (isFlickerRed) {
        changeColor(255, 0, 0);
      }
      else {
        changeColor(255, 200, 160);
      }
    }
  }
}

// Like delay, but runs background tasks also.
// Always use this instead of delay.
void delayAndRunBg(int millisToDelay)
{
  long endMillis = millis() + millisToDelay;
  while (millis() < endMillis) {
    updateFlicker();
    // add other background tasks here.
    delay(1);
  }
}




// This is the animation that runs when the button is pressed.
// Used delay statements to get the timing right.
// LOW for a relay means ON, HIGH is OFF
void runAnimation()
{
  // Rotate throught the scream files.
  int audioFileNumber = screamFiles[currentScreamNumber];
  long audioDelay = screamLength[currentScreamNumber];
  ++currentScreamNumber;
  if (currentScreamNumber >= NUMBER_OF_SCREAMS) {
    currentScreamNumber = 0;
  }

  digitalWrite(PIN_RELAY_LID, LOW);
  digitalWrite(PIN_RELAY_SMOKE, LOW);
  delayAndRunBg(700);


  // Seems to take about 50ms for the audio to actually start playing.
  // So this synchronizes the audio with the man going up.

  delayAndRunBg(50);

  digitalWrite(PIN_RELAY_CYLINDER_MAN, LOW);
  //digitalWrite(PIN_RELAY_REDLIGHT, LOW);
  startFlicker();

  delayAndRunBg(beforeScreamDelay);
  playAudio(audioFileNumber);

  delayAndRunBg(audioDelay);
  delayAndRunBg(afterScreamDelay);

  //flicker(2000, PIN_RELAY_REDLIGHT, 20, 80, 40, 120);

  digitalWrite(PIN_RELAY_CYLINDER_MAN, HIGH);


  delayAndRunBg(800);
  digitalWrite(PIN_RELAY_LID, HIGH);
  delayAndRunBg(2000);
  //digitalWrite(PIN_RELAY_REDLIGHT, HIGH);
  endFlicker();

  if (!smokeOn) {
    // turn smoke off, unless it should stay on.
    digitalWrite(PIN_RELAY_SMOKE, HIGH);
  }
}

void changeColor(int red, int green, int blue) {
  lightSerial.print(String(red) + String(F(",")) + String(green) + String(F(",")) + String(blue) + String(F("\n")));
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  setupAudio(PIN_AUDIO_RX, PIN_AUDIO_TX);
  lightSerial.begin(9600);


  pinMode(PIN_TRIGGER_BUTTON, INPUT_PULLUP);
  //pinMode(PIN_RELAY_REDLIGHT, OUTPUT);
  pinMode(PIN_RELAY_SMOKE, OUTPUT);
  pinMode(PIN_RELAY_LID, OUTPUT);
  pinMode(PIN_RELAY_CYLINDER_MAN, OUTPUT);

  // Turn relays OFF (HIGH)
  //digitalWrite(PIN_RELAY_REDLIGHT, HIGH);
  digitalWrite(PIN_RELAY_SMOKE, LOW);
  digitalWrite(PIN_RELAY_LID, HIGH);
  digitalWrite(PIN_RELAY_CYLINDER_MAN, HIGH);

  inputTrigger.attach(PIN_TRIGGER_BUTTON);

  randomSeed(analogRead(0) + 37 * analogRead(1));

  // start with smoke on
  digitalWrite(PIN_RELAY_SMOKE, LOW);
  lastTimeSmokeOn = millis();
  lastTimeSmokeOff = lastTimeSmokeOn + SMOKE_ON_TIME;
  smokeOn = true;

#ifdef USE_ULTRASONIC
  bool success = setupAndCalibrateUltrasonic(15);
#else
  bool success = true;
#endif

  if (!success) {
    playAudio(AUDIO_CALIBRATIONFAIL);
  }
  else {
    playAudio(AUDIO_BOOT);

  }
  changeColor(255, 0, 0);
  //lightSerial.print("255,0,0\n");
  delayAndRunBg(500);
  changeColor(0, 255, 0);
  //lightSerial.print("0,255,0\n");
  delayAndRunBg(500);
  changeColor(0, 0, 255);
  //lightSerial.print("0,0,255\n");
  delayAndRunBg(500);
  changeColor(0, 0, 0);
  //lightSerial.print("0,0,0\n");
}


void loop() {
  bool triggered;

  // put your main code here, to run repeatedly:
  inputTrigger.update();

  delayAndRunBg(100);  // delay for ultrasonic.

#ifdef USE_ULTRASONIC
  triggered = isUltrasonicTriggered();
#else
  triggered = inputTrigger.fell();
#endif

  if (triggered) {
    // The button is pressed or the ultrasonic is triggered.
    Serial.println("Triggering!");
    if (millis() >= nextTriggerTimePossible) {
      runAnimation();
      nextTriggerTimePossible = millis() + 1000 * RESET_DELAY_SECONDS;
    }
    else {
      Serial.println("Trigger suppressed due to timeout");
    }
  }

  long currentTime = millis();
  if (smokeOn && currentTime > lastTimeSmokeOn + SMOKE_ON_TIME) {
    // turn smoke off.
    Serial.println("Smoke on");
    digitalWrite(PIN_RELAY_SMOKE, HIGH);
    smokeOn = false;
    lastTimeSmokeOff = currentTime;
  }
  if (!smokeOn && currentTime > lastTimeSmokeOff + SMOKE_OFF_TIME) {
    // turn smoke on.
    Serial.println("Smoke off");
    digitalWrite(PIN_RELAY_SMOKE, LOW);
    smokeOn = true;
    lastTimeSmokeOn = currentTime;
  }
}
