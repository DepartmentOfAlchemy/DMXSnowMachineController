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

  Menu based on oled menu from Upiir: https://github.com/upiir/arduino_oled_menu
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Adafruit_seesaw.h>
#include <Preferences.h>

// Include DMX library
#include <SparkFunDMX.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

uint8_t mode = MODE_OFF;
uint8_t oldMode = MODE_OFF;
uint8_t pseudoMode = MODE_OFF;
uint8_t cycleTime = 0;
uint8_t duration = 0;
uint8_t flakeSize = 0;

Adafruit_seesaw ss;

#define BUTTON_X        6
#define BUTTON_Y        2
#define BUTTON_A        5
#define BUTTON_B        1
#define BUTTON_SELECT   0
#define BUTTON_START   16
uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) | (1UL << BUTTON_START) | (1UL << BUTTON_A) | (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);
uint32_t buttons;

// --- bitmap resources ---

// 'scrollbar_background', 8x64px
const unsigned char bitmap_scrollbar_background [] PROGMEM = {
  0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 
  0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 
  0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 
  0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00
};

// 'item_sel_outline', 128x21px
const unsigned char bitmap_item_sel_outline [] PROGMEM = {
  0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 
  0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 
  0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 
  0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0
};

// --- Menu parameters ---

const int NUM_ITEMS = 5; // number of menu items
const int MAX_ITEM_LENGTH = 20; // maximum characters for the menu item name

char menu_items[NUM_ITEMS][MAX_ITEM_LENGTH] = { // array with menu item names
  {"Address"},
  {"Mode"},
  {"Cycle Time"},
  {"Duration"},
  {"Flake Size"}
};
// note - when changing the order of menu items above, make sure the other arrays
// referencing bitmaps also have the same order.

int item_selected = 0; // which item in the menu is selected

int item_sel_previous; // previous item - used in the menu screen to draw the item before the selected one
int item_sel_next; // next item - used in the menu screen to draw item after the selected one

#define PAGE_MENU        0
#define PAGE_ADDRESS     1
#define PAGE_MODE        2
#define PAGE_CYCLE_TIME  3
#define PAGE_DURATION    4
#define PAGE_FLAKE_SIZE  5

int currentPage = PAGE_MENU;

Preferences p;

#define FIVE_MINUTES_IN_MILLIS     300000
#define FIFTEEN_MINUTES_IN_MILLIS  900000

unsigned long cycleStart = 0;
unsigned long cycleDuration = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("T1600 Snow Masters Special Effects snow machine DMX controller");

  // Open the Preferences for storing the DMX controller values
  p.begin("dmx", false);
  address = p.getUShort("address", 1);
  mode = p.getUChar("mode", MODE_OFF);
  cycleTime = p.getUChar("cycleTime", 0);
  duration = p.getUChar("duration", 0);
  flakeSize = p.getUChar("flakeSize", 0);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  if (!ss.begin(0x50)) {
    Serial.println("ERROR! seesaw not found");
    while(1) delay(1);
  }
  Serial.println("seesaw started");
  uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
  if (version != 5743) {
    Serial.print("Wrong firmware loaded? ");
    Serial.println(version);
    while(1) delay(10);
  }
  Serial.println("Found Product 5743");

  ss.pinModeBulk(button_mask, INPUT_PULLUP);
  ss.setGPIOInterrupts(button_mask, 1);

  // Begin DMX serial port
  dmxSerial.begin(DMX_BAUD, DMX_FORMAT);

  // Begin DMX driver
  dmx.begin(dmxSerial, enPin, numChannels);

  // Set communication direction, which can be changed on the fly as needed
  dmx.setComDir(DMX_WRITE_DIR);

  Serial.println("DMX initialized!");
}

void loop() {
  buttons = ss.digitalReadBulk(button_mask);

  if (currentPage == PAGE_MENU) {
    // button management
    if (! (buttons & (1UL << BUTTON_X))) {
      // up
      item_selected = item_selected - 1; // select previous item
      if (item_selected < 0) { // if first item was selected, jump to last item
        item_selected = NUM_ITEMS - 1;
      }
    }
    if (! (buttons & (1UL << BUTTON_B))) {
      // down
      item_selected = item_selected + 1; // select next item
      if (item_selected >= NUM_ITEMS) { // last item was selected, jump to first menu item
        item_selected = 0;
      }
    }

    if (! (buttons & (1UL << BUTTON_A))) {
      // right
      if (item_selected == 0) {
        currentPage = PAGE_ADDRESS;
      } else if (item_selected == 1) {
        currentPage = PAGE_MODE;
      } else if (item_selected == 2) {
        currentPage = PAGE_CYCLE_TIME;
      } else if (item_selected == 3) {
        currentPage = PAGE_DURATION;
      } else if (item_selected == 4) {
        currentPage = PAGE_FLAKE_SIZE;
      }
    }
  } else if (currentPage == PAGE_ADDRESS) {
    if (! (buttons & (1UL << BUTTON_Y))) {
      // left
      currentPage = PAGE_MENU;
      p.putUShort("address", address);
    }
    if (! (buttons & (1UL << BUTTON_X))) {
      // up
      address++;
      if (address > 509) {address = 1;} // roll back to first available address
    }
    if (! (buttons & (1UL << BUTTON_B))) {
      // down
      address--;
      if (address < 1) {address = 509;} // roll forward to last available address
    }
  } else if (currentPage == PAGE_MODE) {
    if (! (buttons & (1UL << BUTTON_Y))) {
      // left
      currentPage = PAGE_MENU;
      p.putUChar("mode", mode);
    }
    if (! (buttons & (1UL << BUTTON_X))) {
      // up
      if (mode == MODE_OFF) {mode = MODE_5_MINUTE_CYCLE;}
      else if (mode == MODE_5_MINUTE_CYCLE) {mode = MODE_15_MINUTE_CYCLE;}
      else if (mode == MODE_15_MINUTE_CYCLE) {mode = MODE_ON;}
      else if (mode == MODE_ON) {mode = MODE_OFF;}
    }
    if (! (buttons & (1UL << BUTTON_B))) {
      // down
      if (mode == MODE_OFF) {mode = MODE_ON;}
      else if (mode == MODE_5_MINUTE_CYCLE) {mode = MODE_OFF;}
      else if (mode == MODE_15_MINUTE_CYCLE) {mode = MODE_5_MINUTE_CYCLE;}
      else if (mode == MODE_ON) {mode = MODE_15_MINUTE_CYCLE;}
    }
  } else if (currentPage == PAGE_CYCLE_TIME) {
    if (! (buttons & (1UL << BUTTON_Y))) {
      // left
      currentPage = PAGE_MENU;
      p.putUChar("cycleTime", cycleTime);
    }
    if (! (buttons & (1UL << BUTTON_X))) {
      // up
      cycleTime++;
      if (cycleTime > 100) {cycleTime = 0;} // roll back to first available address
    }
    if (! (buttons & (1UL << BUTTON_B))) {
      // down
      if (cycleTime == 0) {cycleTime = 100;} // roll forward to last available address
      else {cycleTime--;}
    }
  } else if (currentPage == PAGE_DURATION) {
    if (! (buttons & (1UL << BUTTON_Y))) {
      // left
      currentPage = PAGE_MENU;
      p.putUChar("duration", duration);
    }
    if (! (buttons & (1UL << BUTTON_X))) {
      // up
      duration++;
      if (duration > 100) {duration = 0;} // roll back to first available address
    }
    if (! (buttons & (1UL << BUTTON_B))) {
      // down
      if (duration == 0) {duration = 100;} // roll forward to last available address
      else {duration--;}
    }
  } else if (currentPage == PAGE_FLAKE_SIZE) {
    if (! (buttons & (1UL << BUTTON_Y))) {
      // left
      currentPage = PAGE_MENU;
      p.putUChar("flakeSize", flakeSize);
    }
    if (! (buttons & (1UL << BUTTON_X))) {
      // up
      flakeSize++;
      if (flakeSize > 100) {flakeSize = 0;} // roll back to first available address
    }
    if (! (buttons & (1UL << BUTTON_B))) {
      // down
      if (flakeSize == 0) {flakeSize = 100;} // roll forward to last available address
      else {flakeSize--;}
    }
  }

  // set correct values for the previous and next items
  item_sel_previous = item_selected - 1;
  if (item_sel_previous < 0) {item_sel_previous = NUM_ITEMS - 1;} // previous item would be below first = make it the last
  item_sel_next = item_selected + 1;
  if (item_sel_next >= NUM_ITEMS) {item_sel_next = 0;} // next item would be after last = make it the first

  // draw display
  if (currentPage == PAGE_MENU) {
    display.clearDisplay(); // Clear display buffer

    // selected item background
    display.drawBitmap(0, 22, bitmap_item_sel_outline, 128, 21, 1);

    // draw previous item as icon + label
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(25, 15);
    display.print(menu_items[item_sel_previous]);
    // TODO: draw logo

    // draw selected item as icon + label in bold font
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(25, 15+20+2);
    display.print(menu_items[item_selected]);
    // TODO: draw logo

    // draw next item as icon + label
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(25, 15+20+20+2+2);
    display.print(menu_items[item_sel_next]);
    // TODO: draw logo

    // draw scrollbar background
    display.drawBitmap(128-8, 0, bitmap_scrollbar_background, 8, 64, 1);

    // draw scrollbar handle
    display.drawRect(125, 64/NUM_ITEMS, 3, 64/NUM_ITEMS, 1);

    display.display();
  } else if (currentPage == PAGE_ADDRESS) {
    display.clearDisplay(); // Clear display buffer

    // draw previous item as icon + label
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(5, 15);
    display.print(F("Address:"));

    display.setFont(&FreeSans9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 15+20+2);
    display.print(address);

    display.display();
  } else if (currentPage == PAGE_MODE) {
    display.clearDisplay(); // Clear display buffer

    // draw previous item as icon + label
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(5, 15);
    display.print(F("Mode:"));

    display.setFont(&FreeSans9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 15+20+2);
    if (mode == MODE_OFF) {display.print(F("Off"));}
    else if (mode == MODE_5_MINUTE_CYCLE) {display.print(F("5 Minute"));}
    else if (mode == MODE_15_MINUTE_CYCLE) {display.print(F("15 Minute"));}
    else if (mode == MODE_ON) {display.print(F("On"));}

    display.display();
  } else if (currentPage == PAGE_CYCLE_TIME) {
    display.clearDisplay(); // Clear display buffer

    // draw previous item as icon + label
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(5, 15);
    display.print(F("Cycle Time:"));

    display.setFont(&FreeSans9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 15+20+2);
    display.print(cycleTime);

    display.display();
  } else if (currentPage == PAGE_DURATION) {
    display.clearDisplay(); // Clear display buffer

    // draw previous item as icon + label
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(5, 15);
    display.print(F("Duration:"));

    display.setFont(&FreeSans9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 15+20+2);
    display.print(duration);

    display.display();
  } else if (currentPage == PAGE_FLAKE_SIZE) {
    display.clearDisplay(); // Clear display buffer

    // draw previous item as icon + label
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(5, 15);
    display.print(F("Flake Size:"));

    display.setFont(&FreeSans9pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 15+20+2);
    display.print(flakeSize);

    display.display();
  }

  // did mode change?
  if (mode != oldMode) {
    if ((mode == MODE_5_MINUTE_CYCLE) || (mode == MODE_15_MINUTE_CYCLE)) {
      // new cycle mode
      cycleStart = 0;
      cycleDuration = 0;
    }
    oldMode = mode;
  }

  // convert 5 and 15 minute modes to explicit ON and OFF
  if ((mode == MODE_5_MINUTE_CYCLE) || (mode == MODE_15_MINUTE_CYCLE)) {
    unsigned long now = millis();
    if (now >= (cycleStart + cycleDuration)) {
      // new cycle
      cycleStart = now;
      if (mode == MODE_5_MINUTE_CYCLE) {
        cycleDuration = FIVE_MINUTES_IN_MILLIS;
      } else if (mode == MODE_15_MINUTE_CYCLE) {
        cycleDuration = FIFTEEN_MINUTES_IN_MILLIS;
      }

      pseudoMode = MODE_ON;
    } else if (now >= (cycleStart + (duration * 1000))) {
      pseudoMode = MODE_OFF;
    }
  } else {
    pseudoMode = mode;
  }

  // write current DMX values
  dmx.writeByte(pseudoMode, address + DMX_CHANNEL_OFFSET_MODE);
  dmx.writeByte(cycleTime, address + DMX_CHANNEL_OFFSET_CYCLE_TIME);
  dmx.writeByte(duration, address + DMX_CHANNEL_OFFSET_DURATION);
  dmx.writeByte(flakeSize, address + DMX_CHANNEL_OFFSET_FLAKE_SIZE);

  // Once all data has been written, update() must be called to actually send it
  dmx.update();

  Serial.print("DMX: sent value to base address ");
  Serial.print(address);
  Serial.print(": Mode: ");
  Serial.print(mode);
  Serial.print(": Pseudo Mode: ");
  Serial.print(pseudoMode);
  Serial.print(": Cycle Time: ");
  Serial.print(cycleTime);
  Serial.print(": Duration: ");
  Serial.print(duration);
  Serial.print(": Flake Size: ");
  Serial.println(flakeSize);
  Serial.print("Current page: ");
  Serial.println(currentPage);
  Serial.print("cycleStart: ");
  Serial.print(cycleStart);
  Serial.print(", cycleDuration: ");
  Serial.println(cycleDuration);

  // Slow down communication for this example
  delay(100);
}