#include "Bounce2.h"
#include "AudioPlayer.h"

#define PIN_RELAY_REDLIGHT 7
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

// Defines which audio files are used for the screen/trump sound.
const int NUMBER_OF_SCREAMS = /*1*/ 6;
int screamFiles[] = { /*AUDIO_SCREAM,*/ AUDIO_TRUMP_1, AUDIO_TRUMP_2, AUDIO_TRUMP_3, AUDIO_TRUMP_4, AUDIO_TRUMP_5, AUDIO_TRUMP_6  };
unsigned long screamLength[] = { /*3000,*/ 2000, 2500, 3300, 3650, 2850, 7300 };
unsigned long beforeScreamDelay = /*0*/ 1000;
unsigned long afterScreamDelay = /*0*/ 2500;


// Pins for the ultrasonic sensor:
const int trigPin = 11;
const int echoPin = 12;
int ultrasonicThreshold = -1;

// We need to add a timeout for the longes range that the Sensor can sense, so that
// it does not get stuck waiting for an echo that never comes.
const long timeout = 400 * 29 * 2; //Timeout in microseconds for a max range of 400 cm

Bounce inputTrigger;

const int SMOKE_ON_TIME = 10 * 1000;  // SMOKE ON time after start
const int SMOKE_OFF_TIME = /* 30 */ 60 * 1000;  // SMOKE OFF time

const int RESET_DELAY_SECONDS = 10; // time before box will trigger again.

long int lastTimeSmokeOn;
long int lastTimeSmokeOff;
bool smokeOn;

long nextTriggerTimePossible = 0;

int currentScreamNumber = 0;

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

void flicker(long millisToRun, int pin, int offMin, int offMax, int onMin, int onMax)
{
  long startMillis = millis();
  while (millis() < startMillis + millisToRun) {
    digitalWrite(PIN_RELAY_REDLIGHT, LOW);
    delay(random(offMin, offMax));
    digitalWrite(PIN_RELAY_REDLIGHT, HIGH);
    delay(random(onMin, onMax));
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
  delay(700);


  // Seems to take about 50ms for the audio to actually start playing.
  // So this synchronizes the audio with the man going up.

  delay(50);

  digitalWrite(PIN_RELAY_CYLINDER_MAN, LOW);
  digitalWrite(PIN_RELAY_REDLIGHT, LOW);
  
  delay(beforeScreamDelay);
  playAudio(audioFileNumber);
  
  delay(audioDelay);
  delay(afterScreamDelay);
  
  //flicker(2000, PIN_RELAY_REDLIGHT, 20, 80, 40, 120);

  digitalWrite(PIN_RELAY_CYLINDER_MAN, HIGH);


  delay(500);
  digitalWrite(PIN_RELAY_LID, HIGH);
  delay(2000);
  digitalWrite(PIN_RELAY_REDLIGHT, HIGH);

  if (!smokeOn) {
    // turn smoke off, unless it should stay on.
    digitalWrite(PIN_RELAY_SMOKE, HIGH);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  setupAudio(PIN_AUDIO_RX, PIN_AUDIO_TX);

  pinMode(PIN_TRIGGER_BUTTON, INPUT_PULLUP);
  pinMode(PIN_RELAY_REDLIGHT, OUTPUT);
  pinMode(PIN_RELAY_SMOKE, OUTPUT);
  pinMode(PIN_RELAY_LID, OUTPUT);
  pinMode(PIN_RELAY_CYLINDER_MAN, OUTPUT);

  // Turn relays OFF (HIGH)
  digitalWrite(PIN_RELAY_REDLIGHT, HIGH);
  digitalWrite(PIN_RELAY_SMOKE, HIGH);
  digitalWrite(PIN_RELAY_LID, HIGH);
  digitalWrite(PIN_RELAY_CYLINDER_MAN, HIGH);

  inputTrigger.attach(PIN_TRIGGER_BUTTON);

  randomSeed(analogRead(0) + 37 * analogRead(1));

  // start with smoke on
  digitalWrite(PIN_RELAY_SMOKE, LOW);
  lastTimeSmokeOn = millis();
  lastTimeSmokeOff = lastTimeSmokeOn + SMOKE_ON_TIME;
  smokeOn = true;

  bool success = setupAndCalibrateUltrasonic(15);
  if (!success) {
    playAudio(AUDIO_CALIBRATIONFAIL);
  }
  else {
    playAudio(AUDIO_BOOT);
  }
}


void loop() {
  // put your main code here, to run repeatedly:
  inputTrigger.update();

  delay(100);  // delay for ultrasonic.

  if (/*inputTrigger.fell() || */ isUltrasonicTriggered()) {
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
    digitalWrite(PIN_RELAY_SMOKE, HIGH);
    smokeOn = false;
    lastTimeSmokeOff = currentTime;
  }
  if (!smokeOn && currentTime > lastTimeSmokeOff + SMOKE_OFF_TIME) {
    // turn smoke on.
    digitalWrite(PIN_RELAY_SMOKE, LOW);
    smokeOn = true;
    lastTimeSmokeOn = currentTime;
  }
}
