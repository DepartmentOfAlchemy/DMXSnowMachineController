/*
  Triggerable DMX controller for T1600 Snow Masters Special Effects snow machine.

  Mode (x: 0-100): Overall operation of the snow machine - On, Off, or Momentary
    0-24: Off
    25-50: 5 Minute Cycle (Cycle Time & Duration active)
    51-74: 15 Minute Cycle (Cycle Time & Duration active)
    75-100: Always On
  Cycle Time (x+1: 0-100): Establishes the time it takes for the entire event
  Duration (x+2: 0-100): Length of time of snow output
  Flake Size (x+3: 0-100): 

  The minimum time of one cycle is 18 seconds, with a 10 second ON time,
  a 4 second SNEEZE, and a 4 second WAIT. (A SNEEZE is when the blower
  remains on without the pump and drives the sock, preventing postnasal drip.)

*/

#include <Bounce2.h>
#include <SparkFunDMX.h>

#define TRIGGER_BUTTON 0 // Local button to create a trigger

Bounce2::Button button = Bounce2::Button();
Bounce2::Button trigger = Bounce2::Button();

// Create DMX object
SparkFunDMX dmx;

HardwareSerial dmxSerial(1);

// Enable pin for DMX shield (Feather pinout)
uint8_t enPin = 43;

// DMX fixture specific
uint16_t numChannels = 4;
uint16_t address = 1;
#define DMX_CHANNEL_OFFSET_MODE        0
#define DMX_CHANNEL_OFFSET_CYCLE_TIME  1
#define DMX_CHANNEL_OFFSET_DURATION    2
#define DMX_CHANNEL_OFFSET_FLAKE_SIZE  3

#define MODE_OFF               0
#define MODE_5_MINUTE_CYCLE   25
#define MODE_15_MINUTE_CYCLE  51
#define MODE_ON               75

uint8_t mode = MODE_OFF;
uint8_t oldMode = MODE_OFF;
uint8_t pseudoMode = MODE_OFF;
uint8_t cycleTime = 0;
uint8_t duration = 0;
uint8_t flakeSize = 30;

#define DEFAULT_TRIGGER_DURATION 120000  // 2 minutes == 120000, 5 seconds = 5000 
unsigned long triggerStart = 0;
unsigned long triggerDuration = 0;
bool triggered = false;
unsigned long now = 0;

void setup() {
  button.attach(TRIGGER_BUTTON, INPUT_PULLUP);
  button.interval(25); // milliseconds
  button.setPressedState(LOW);

  trigger.attach(PIN_ISOLATED_INPUT, INPUT);
  trigger.interval(25); // milliseconds
  trigger.setPressedState(LOW);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);

  delay(3000);

  Serial.println(F("\nStarting DMX Snow Machine Triggered Controller"));
  Serial.println();

  // Begin DMX serial port
  dmxSerial.begin(DMX_BAUD, DMX_FORMAT);

  // Begin DMX driver
  dmx.begin(dmxSerial, enPin, numChannels);

  // Set communication direction, shich can be changed on the fly as needed
  dmx.setComDir(DMX_WRITE_DIR);

  Serial.println(F("DMX initialized!"));
}

void loop() {
  button.update();
  trigger.update();

  if (!triggered) {
    if ((trigger.pressed() || button.pressed())) {
      Serial.println(F("Scene triggered"));

      triggered = true;
      triggerStart = millis();
      triggerDuration = DEFAULT_TRIGGER_DURATION;
    }
  }

  if (triggered) {
    now = millis();

    if (now >= (triggerStart + triggerDuration)) {
      // triggered scene over
      mode = MODE_OFF;
      triggered = false;
    } else {
      mode = MODE_ON;
    }
  } else {
    mode = MODE_OFF;
  }

  dmx.writeByte(mode, address + DMX_CHANNEL_OFFSET_MODE);
  dmx.writeByte(cycleTime, address + DMX_CHANNEL_OFFSET_CYCLE_TIME);
  dmx.writeByte(duration, address + DMX_CHANNEL_OFFSET_DURATION);
  dmx.writeByte(flakeSize, address + DMX_CHANNEL_OFFSET_FLAKE_SIZE);

  dmx.update();

  if (mode == MODE_ON) {
    analogWrite(LED_BUILTIN, 255);
  } else {
    analogWrite(LED_BUILTIN, 0);
  }

  Serial.print("DMX: sent value to base address ");
  Serial.print(address);
  Serial.print(": Mode: ");
  Serial.print(mode);
  Serial.print(": Cycle Time: ");
  Serial.print(cycleTime);
  Serial.print(": Duration: ");
  Serial.print(duration);
  Serial.print(": Flake Size: ");
  Serial.println(flakeSize);
  Serial.print("Trigger duration: ");
  Serial.println(triggerDuration);
  Serial.print("triggerStart: ");
  Serial.print(triggerStart);
  Serial.print(", triggerDuration: ");
  Serial.println(triggerDuration);

  // Slow down communication for this example
  delay(100);
}
