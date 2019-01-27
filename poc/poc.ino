// Proof of concept: Nerf Ops Pro Alphapoint target
// Decodes the relevant tag received from the IR receiver and
// prints out the sender color on the serial link
  
#define BUFFER_LENGTH 16
#define IRPIN 2           // data from SBX 1620-52
#define LED   13
#define NO_TEAM   0
#define RED_TEAM  1
#define BLUE_TEAM 2

uint32_t currentTime = 0;
uint8_t currentState = HIGH;
uint8_t previousState = HIGH;
uint8_t idx = 0;
uint16_t tag;

void reset() {
  idx = 0;
  previousState = HIGH;
  digitalWrite(LED, LOW);
}

void gotShot(uint8_t team) {
  if (team == RED_TEAM) {
    Serial.println("got shot by a RED player");
    Serial.println();
  } else if (team == BLUE_TEAM) {
    Serial.println("got shot by a BLUE player");
    Serial.println();
  } else {
    Serial.println("got shot by a PURPLE player");
    Serial.println();
  }
}

void loop()  {
  static uint32_t previousTime = micros();
  currentState = digitalRead(IRPIN);
  if (currentState != previousState) {
    currentTime = micros();
    uint8_t duration = uint8_t((currentTime - previousTime + 500) / 1000);
    if (idx == 0) {
      if ((currentState == HIGH) && (duration == 3)) {
        idx++;
      } else {
        reset();
      }
    } else if (idx == 1) {
      if ((currentState == LOW) && (duration == 6)) {
        idx++;
      } else {
        reset();
      }
    } else if (idx == 2) {
      if ((currentState == HIGH) && (duration == 3)) {
        idx++;
        // header recognized, now read tag
      } else {
        reset();
      }
    } else {
      // tag is 16 bits long
      // one bit corresponds to a 2ms negative pulse followed by
      // - a 1ms positive pulse for 0
      // - a 2ms positive pulse for 1
      digitalWrite(LED, HIGH);
      if (currentState == LOW) {
        if (duration != 2) {
          reset();
        }
      } else {
        if (duration == 1) {
          tag *= 2;
          idx++;
        } else if (duration == 2) {
          tag = tag * 2 + 1;
          idx++;
        } else {
          reset();
        }
        if (idx == 19) {
          // the last 2 bits change when the blaster is connected to the app: ignored
          if ((tag & 0xFFFC) == 0x0880) {
            gotShot(NO_TEAM);
          } else if ((tag & 0xFFFC) == 0x0800) {
            gotShot(RED_TEAM);
          } else if ((tag & 0xFFFC) == 0x0840) {
            gotShot(BLUE_TEAM);
          }
          reset();
        }
      }
    }
    previousTime = currentTime;
    previousState = currentState;
  }
}

void setup() {
  pinMode(IRPIN, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  reset();
}
