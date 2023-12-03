/*
  DMX controller for T1600 Snow Masters Special Effects snow machine.

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

#include <SparkFunDMX.h>

// Create DMX object
SparkFunDMX dmx;

// Create serial port to be used for DMX interface. Exact implementation depends
// on platform, this example is for the ESP32
HardwareSerial dmxSerial(2);

// Enable pin for DMX  shield (Free pin on Thing plus or Feather pinout)
uint8_t enPin = 21;

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

uint8_t mode = MODE_5_MINUTE_CYCLE;
uint8_t cycleTime = 100;
uint8_t duration = 92;
uint8_t flakeSize = 30;

void setup() {
  Serial.begin(115200);
  Serial.println("T1600 Snow Masters Special Effects snow machine DMX controller");

    // Begin DMX serial port
  dmxSerial.begin(DMX_BAUD, DMX_FORMAT);

  // Begin DMX driver
  dmx.begin(dmxSerial, enPin, numChannels);

  // Set communication direction, which can be changed on the fly as needed
  dmx.setComDir(DMX_WRITE_DIR);

  Serial.println("DMX initialized!");
}

void loop() {
  // write current DMX values
  dmx.writeByte(mode, address + DMX_CHANNEL_OFFSET_MODE);
  dmx.writeByte(cycleTime, address + DMX_CHANNEL_OFFSET_CYCLE_TIME);
  dmx.writeByte(duration, address + DMX_CHANNEL_OFFSET_DURATION);
  dmx.writeByte(flakeSize, address + DMX_CHANNEL_OFFSET_FLAKE_SIZE);

  // Once all data has been written, update() must be called to actually send it
  dmx.update();

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

  // Slow down communication for this example
  delay(100);
}
