#include <Arduino.h>
#include "wr.h" 

typedef unsigned long us_t;
typedef unsigned long ns_t;

us_t reset_time = 50;

ns_t t1h_time = 800; //3 //12.8
ns_t t1l_time = 450; //2 //7.2
ns_t t0h_time = 400; //2 //6.4 
ns_t t0l_time = 850; //3 //13.6
ns_t ttl_time = 450; // 7.2
ns_t tolerance_time = 150;

int dout = 2;

void setup() {
  //Serial.begin(115200);
  pinMode(dout, OUTPUT);
}

//#pragma GCC optimize "O1"
//250 ns
//__attribute__((always_inline))
inline __attribute__((always_inline)) void bit_write(volatile byte *port, const byte &b, volatile byte state) {
  //NOP; NOP; NOP; NOP;
  bit_write<1>(port, b);
  //PORTE |= 16;
   //NOP; NOP; if 2 instructions
   //NOP; 
  if (!state) {
    bit_write<0>(port, b);
    //PORTE &= ~16;
    NOP; NOP; NOP; NOP; NOP; NOP; //NOP;
    //NOP;
    
    //NOP; // 1 rjmp
  } else {    
    NOP; NOP; NOP; NOP; NOP; NOP;// NOP;
    //PORTE &= ~16;
    bit_write<0>(port, b);
    //NOP;// NOP;
  }
  bit_write<0>(port, b);
}

#define _POST NOP//; NOP; NOP; NOP; NOP; NOP

//__attribute__((always_inline))
inline __attribute__((always_inline)) void byte_write(volatile byte *port, const byte &dout, volatile byte b) {
  //_POST; _POST;
  bit_write(port, dout, b & 128); _POST;  // 2 instructions per call overhead
  bit_write(port, dout, b & 64); _POST;
  bit_write(port, dout, b & 32); _POST;
  bit_write(port, dout, b & 16); _POST;
  bit_write(port, dout, b & 8); _POST;
  bit_write(port, dout, b & 4); _POST; 
  bit_write(port, dout, b & 2); _POST;
  bit_write(port, dout, b & 1); _POST;
}

inline void reset_write() {
  bit_write<0>(&PORTE, 4);
  delayMicroseconds(50);
}

inline __attribute__((always_inline)) void rgb_write(volatile byte *port, const byte &dout, volatile byte r, volatile byte g, volatile byte b) {
  cli();
  byte_write(port, dout, g);
  byte_write(port, dout, r);
  byte_write(port, dout, b);
  sei();
}

inline void bit_write_test() {
  const int test_iter = 100;
  us_t start = micros();

  for (int i = 0; i < test_iter; i++) {
    rgb_write(&PORTE, 4, 64, 255, i);
  }

  us_t total = micros() - start;

  float time_per_write = float(total) / (float(test_iter) * 24.0f);

  Serial.print("total_time:");
  Serial.print(total);
  Serial.print(",time_per_write:");
  Serial.println(time_per_write);
}

//#pragma GCC optimize "Os"

inline void led_write(volatile byte r, volatile byte g, volatile byte b) {
  rgb_write(&PORTE, 4, r, g, b);
  rgb_write(&PORTE, 4, g, b, r);
  rgb_write(&PORTE, 4, b, r, g);
  reset_write();
}

inline void rgb_write(volatile byte r, volatile byte g, volatile byte b) {
  rgb_write(&PORTE, 4, r, g, b);
}

int value = 0;

void loop() {
  int range = 2160;
  value = (value + 1) % range;
  
  float pi = 3.14159f;
  float bf = ((float(value) / float(range) * 2.0f) - 1.0f) * pi;
  
  float r30  = pi / 6.0f;
  float r60  = r30 * 2;
  float r90  = r30 * 3;
  float r120 = r30 * 4;
  float r180 = r30 * 6;
  float r240 = r30 * 8;
  
  float range2 = 127.5f;
  float offset = range2;
  
  int color    = sin(bf)        * range2 + offset;
  int color60  = sin(bf + r60)  * range2 + offset;
  int color90  = sin(bf + r90)  * range2 + offset;
  int color120 = sin(bf * 2.0f + r120) * range2 + offset;
  int color180 = sin(bf + r180) * range2 + offset;
  int color240 = sin(bf * 4.0f + r240) * range2 + offset;
  byte d = 8;
  
  volatile byte zero = 0;
  led_write(color, color120, color240);

  delay(d);
}
