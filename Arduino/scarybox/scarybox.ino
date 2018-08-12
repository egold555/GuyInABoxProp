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

Bounce inputTrigger;

const int SMOKE_ON_TIME = 10 * 1000;  // SMOKE ON time after start
const int SMOKE_OFF_TIME = 30 * 1000;  // SMOKE OFF time

long int lastTimeSmokeOn;
long int lastTimeSmokeOff;
bool smokeOn;

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
  digitalWrite(PIN_RELAY_LID, LOW);
  digitalWrite(PIN_RELAY_SMOKE, LOW);
  delay(300);


  // Seems to take about 450ms for the audio to actually start playing.
  // So this synchronizes the audio with the man going up.
  playAudio(AUDIO_SCREAM);
  delay(450);

  digitalWrite(PIN_RELAY_CYLINDER_MAN, LOW);
  digitalWrite(PIN_RELAY_REDLIGHT, LOW);
  delay(1000);

  delay(2000);
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

  playAudio(AUDIO_BOOT);
  
}


void loop() {
  // put your main code here, to run repeatedly:
  inputTrigger.update();

  if (inputTrigger.fell()) {
    // The button is pressed.
    runAnimation();
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
