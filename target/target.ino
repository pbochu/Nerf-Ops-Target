// Nerf Ops Pro Alphapoint target
//
// Simple behaviour: the target uses a RGB led to indicate its own team color.
// If shot by a legitimate opponent, just flashes, if shot by someone else, shoots back
// (as a legitimate opponent of the attacker).
// 
// Notes: 3 separate LEDs are a possible option to indicate color, but the device I use integrates current
// limiting resistors and the inputs can be directly driven by the Arduino
// An inexpensive VS1838B was used as an IR sensor
// The reach obtained with my only IR LED is still below 1 meter, even with a current driver.
// Tests with various wavelength and widespread are ongoing...
//
// This project uses the IRremote library Version 2.0.1 June, 2015, Copyright 2009 Ken Shirriff
// For details on this library, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.html


#include <IRremote.h>

#define TAG_LENGTH  35
#define RED_TAG     0x0800
#define BLUE_TAG    0x0840
#define PURPLE_TAG  0x0880

#define IR_SENSOR_PIN   2           // data from IR sensor (Sony SBX1620-52 or VS1838B)
#define BLUE_LED_PIN    4           // blue LED
#define RED_LED_PIN     5           // red LED
#define GREEN_LED_PIN   6           // yes, green LED

#define NONE            0
#define BLUE            1
#define RED             2
#define PURPLE          3

IRsend irsend; // LED emitter on pin 3

uint8_t opponent[4];
uint16_t IRTag[4][TAG_LENGTH];

uint8_t shooter = NONE;
bool shot = false;
bool shooting = false;

// shoot at a player as one of his legitimate opponent
void shoot(uint8_t color) {
  uint8_t shooterColor = opponent[color];
  shooting = true;  // disable decoding in the interrupt vector to avoid decoding my own shot!
  irsend.sendRaw(IRTag[shooterColor], 35, 38);  // 35 int length tag, 38 KHz
  delay(250);
  shooting = false;
  // green flash. Was first white, but I decided that green was more recognizable
  // digitalWrite(BLUE_LED_PIN, HIGH);
  // digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);
  delay(250);
  // digitalWrite(RED_LED_PIN, LOW);
  // digitalWrite(BLUE_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
}

// the target got shot by a legitimate attacker
void gotShot(uint8_t victim) {
  // just flash the victim color several times to acknowledge the shooting
  uint8_t redState = ((victim == RED) || (victim == PURPLE)) ? HIGH : LOW;
  uint8_t blueState = ((victim == BLUE) || (victim == PURPLE)) ? HIGH : LOW;
  for (uint8_t i = 0; i < 5; i++) {
    digitalWrite(BLUE_LED_PIN, blueState);
    digitalWrite(RED_LED_PIN, redState);
    delay(40);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
    delay(60);
  }
}

// Interrupt raised on pin 2 change
void intVect() {
  static uint8_t previousState = HIGH;
  static uint8_t idx = 0;
  static uint16_t tag;
  static uint32_t previousTime = micros();
  uint8_t currentState = digitalRead(IR_SENSOR_PIN);

  // treated only if the IR sensor state changed
  if (!shooting && (currentState != previousState)) {
    uint32_t currentTime = micros();
    uint16_t duration = uint16_t((currentTime - previousTime + 500) / 1000);
    if (idx == 0) {
      if ((currentState == HIGH) && (duration == 3)) {
        idx++;
      } else {
        // idx = 0;
      }
    } else if (idx == 1) {
      if ((currentState == LOW) && (duration == 6)) {
        idx++;
      } else {
        idx = 0;
      }
    } else if (idx == 2) {
      if ((currentState == HIGH) && (duration == 3)) {
        idx++;
        // header recognized, now read tag
      } else {
        idx = 0;
      }
    } else {
      // tag is 16 bits long
      // one bit corresponds to a 2ms negative pulse followed by
      // - a 1ms positive pulse for 0
      // - a 2ms positive pulse for 1
      if (currentState == LOW) {
        // idx is odd
        if (duration == 2) {
          idx++;
        } else {
          idx = 0;
        }
      } else {
        // idx is even
        if (duration == 1) {
          tag *= 2;
          idx++;
        } else if (duration == 2) {
          tag = tag * 2 + 1;
          idx++;
        } else {
          idx = 0;
        }
        if (idx == 35) {
          // tag complete
          // the last 2 bits change when the blaster is connected to the app: ignored
          if ((tag & 0xFFFC) == PURPLE_TAG) {
            shooter = PURPLE;
            shot = true;
          } else if ((tag & 0xFFFC) == RED_TAG) {
            shooter = RED;
            shot = true;
          } else if ((tag & 0xFFFC) == BLUE_TAG) {
            shooter = BLUE;
            shot = true;
          }
          idx = 0;
        }
      }
    }
    previousTime = currentTime;
    previousState = currentState;
  }
}

// build the tag array from its 16 bit int representation
void buildTag(uint8_t color, uint16_t tag) {
  // header
  IRTag[color][0] = 3000;
  IRTag[color][1] = 6000;
  IRTag[color][2] = 3000;
  // 16 bits
  for (uint8_t b = 0; b < 16; b++) {
    IRTag[color][2 * b + 3] = 2000;
    IRTag[color][2 * b + 4] = (tag & 0x8000) ? 2000 : 1000;
    tag = tag << 1;
  }
}

// the target 
void loop() {
  uint8_t i = 0;
  uint8_t target = NONE;
  bool s = false;
  uint8_t sr = NONE;
  
  // random delay: currently none
  // delay(1000 * random(10, 20));
  target = random(4);      
  // activate target
  if ((target == RED) || (target == PURPLE)) {
    digitalWrite(RED_LED_PIN, HIGH);
  }
  if ((target == BLUE) || (target == PURPLE)) {
    digitalWrite(BLUE_LED_PIN, HIGH);
  }
  while (i++ < 200) {
    s = shot;
    sr = shooter;
    shot = false;
    delay(10);
    if (s) {
      // got shot
      if (sr == opponent[target]) {
        // by an opponent
        gotShot(target);
      } else {
        // by someone else: I shoot back!
        shoot(sr);
      }
      break;
    }
  }
  // deactivate target
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, LOW);
}

void setup() {
  // RED vs BLUE, PURPLE vs PURPLE
  opponent[NONE] = NONE;
  opponent[RED] = BLUE;
  opponent[BLUE] = RED;
  opponent[PURPLE] = PURPLE;

  // INPUT or INPUT_PULLUP mode, depending on the device used
  pinMode(IR_SENSOR_PIN, INPUT);

  // OUTPUTS (LEDS)
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // build tag arrays
  buildTag(RED, RED_TAG);
  buildTag(BLUE, BLUE_TAG);
  buildTag(PURPLE, PURPLE_TAG);

  // Attach an interrupt to the ISR vector
  // interrupt raised when pin 2 changes state
  attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), intVect, CHANGE);
}
