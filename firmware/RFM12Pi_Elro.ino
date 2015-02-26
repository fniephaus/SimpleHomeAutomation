/// @dir RF12demo
/// Configure some values in EEPROM for easy config of the RF12 later on.
// 2009-05-06 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// this version adds flash memory support, 2009-11-19

#include <JeeLib.h>
#include <util/crc16.h>
#include <util/parity.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>


#define SERIAL_BAUD 9600

#define DATAFLASH 1 // check for presence of DataFlash memory on JeeLink
#define FLASH_MBIT  16  // support for various dataflash sizes: 4/8/16 Mbit

#define LED_PIN   9 // activity LED, comment out to disable


#define COLLECT 0x20 // collect mode, i.e. pass incoming without sending acks

// static unsigned long now () {
//   // FIXME 49-day overflow
//   return millis() / 1000;
// }

static void activityLed (byte on) {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, on);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// RF12 configuration setup code

// typedef struct {
//   byte nodeId;
//   byte group;
//   char msg[RF12_EEPROM_SIZE-4];
//   word crc;
// } RF12Config;

// static RF12Config config;

static char cmd;
static byte value, stack[RF12_MAXDATA+4], top, sendLen, dest, quiet;
static byte testbuf[RF12_MAXDATA], testCounter, useHex;

// static void showNibble (byte nibble) {
//   char c = '0' + (nibble & 0x0F);
//   if (c > '9')
//     c += 7;
//   Serial.print(c);
// }

// static void showByte (byte value) {
//   if (useHex) {
//     showNibble(value >> 4);
//     showNibble(value);
//   } else
//     Serial.print((int) value);
// }

// static void addCh (char* msg, char c) {
//   byte n = strlen(msg);
//   msg[n] = c;
// }

// static void addInt (char* msg, word v) {
//   if (v >= 10)
//     addInt(msg, v / 10);
//   addCh(msg, '0' + v % 10);
// }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// OOK transmit code

// Turn transmitter on or off, but also apply asymmetric correction and account
// for 25 us SPI overhead to end up with the proper on-the-air pulse widths.
// With thanks to JGJ Veken for his help in getting these values right.
static void ookPulse(int on, int off) {
  rf12_onOff(1);
  delayMicroseconds(on + 150);
  rf12_onOff(0);
  delayMicroseconds(off - 200);
}

// static void fs20sendBits(word data, byte bits) {
//   if (bits == 8) {
//     ++bits;
//     data = (data << 1) | parity_even_bit(data);
//   }
//   for (word mask = bit(bits-1); mask != 0; mask >>= 1) {
//     int width = data & mask ? 600 : 400;
//     ookPulse(width, width);
//   }
// }

// static void fs20cmd(word house, byte addr, byte cmd) {
//   byte sum = 6 + (house >> 8) + house + addr + cmd;
//   for (byte i = 0; i < 3; ++i) {
//     fs20sendBits(1, 13);
//     fs20sendBits(house >> 8, 8);
//     fs20sendBits(house, 8);
//     fs20sendBits(addr, 8);
//     fs20sendBits(cmd, 8);
//     fs20sendBits(sum, 8);
//     fs20sendBits(0, 1);
//     delay(10);
//   }
// }

// static void kakuSend(char addr, byte device, byte on) {
//   int cmd = 0x600 | ((device - 1) << 4) | ((addr - 1) & 0xF);
//   if (on)
//     cmd |= 0x800;
//   for (byte i = 0; i < 4; ++i) {
//     for (byte bit = 0; bit < 12; ++bit) {
//       ookPulse(375, 1125);
//       int on = bitRead(cmd, bit) ? 1125 : 375;
//       ookPulse(on, 1500 - on);
//     }
//     ookPulse(375, 375);
//     delay(11); // approximate
//   }
// }

#define ELROSHORT (375)
#define ELROLONG (3*ELROSHORT)

static void elroSend(word address){
  signed short i,j;
  //sent pulse 4 times to make sure
  for(j = 0; j < 8; j++) {
    for(i = 11; i >= 0; i--){
      if((address >> i) & 0x01) {
        //Bit set, send code for low
        ookPulse(ELROSHORT, ELROLONG);
        ookPulse(ELROSHORT, ELROLONG);
      } else {
        //Bit not set, send code for float
        ookPulse(ELROSHORT, ELROLONG);
        ookPulse(ELROLONG, ELROSHORT);
      }
    }
    ookPulse(ELROSHORT, (31 * ELROSHORT));
  }
}

// This is just a helper function to transform the remote controll keys to addresses
// adddressCode is the 5 bit code set via the dip switches inside the remote, the upper
// position is a 1, lower is 0;
// deviceNumber is 1 for button A, 2 for B, etc

void elroSwitch(char addressCode, byte deviceNumber, byte on) {
  uint16_t cmd = 0;
  cmd |= ((addressCode & 0x1F) << 7);
  cmd |= (1 << (7-deviceNumber));
  cmd |= on ? 2 : 1;
  elroSend(cmd);
  //Serial.print(cmd);
}


const char helpText[] PROGMEM =
  "\n"
  "Remote control commands:" "\n"
  "  <addr>,<dev>,<on> e              - ELRO command (433 MHz)" "\n"
;

static void showString (PGM_P s) {
  for (;;) {
    char c = pgm_read_byte(s++);
    if (c == 0)
      break;
    if (c == '\n')
      Serial.print('\r');
    Serial.print(c);
  }
}

static void handleInput (char c) {
  if ('0' <= c && c <= '9')
    value = 10 * value + c - '0';
  else if (c == ',') {
    if (top < sizeof stack)
      stack[top++] = value;
    value = 0;
  } else if ('a' <= c && c <='z') {
    Serial.print("> ");
    for (byte i = 0; i < top; ++i) {
      Serial.print((int) stack[i]);
      Serial.print(',');
    }
    Serial.print((int) value);
    Serial.println(c);
    switch (c) {
      default:
        showString(helpText);
        break;
      case 'e': // set node id
        activityLed(1);
        elroSwitch(stack[0], stack[1], value);
        activityLed(0);
        break;
    }
    value = top = 0;
    memset(stack, 0, sizeof stack);
  } else
    showString(helpText);
}

void setup() {
  Serial.begin(9600);
  Serial.println("\nelrodemo");
  activityLed(1);

  rf12_initialize(0, RF12_433MHZ);
  activityLed(0);
}


void loop() {
  if (Serial.available()){
    handleInput(Serial.read());
  }
}
