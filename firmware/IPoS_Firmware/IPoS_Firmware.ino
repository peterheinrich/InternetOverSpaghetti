#include "TimerOne.h"  

#define SPAGHETTI_RX  5
#define SPAGHETTI_TX  4
#define LED_RED       6
#define LED_GREEN     7
#define CTS           3
#define RTS           2

#define BUFFSIZE      32
#define __DEBUG__

// Global variables need 'volatile' so that they can be altered within the ISR routine
volatile int tick = 0;

volatile char tx_buff[BUFFSIZE];
volatile char tx_buff_last = 0;
volatile char tx_byte = 0;
volatile char tx_state = 0;
volatile char tx_bit = 0;
volatile char tx_blank = 0;

volatile char rx_byte = 0;
volatile char rx_state = 0;
volatile char rx_bit = 0;
volatile char rx_tick_offset = 0;

/* ********************************************************************** */
/* *** BOARD SETUP                                                    *** */
/* ********************************************************************** */
void setup() {

  // Setup board specific pins
  pinMode(SPAGHETTI_TX, OUTPUT);
  pinMode(SPAGHETTI_RX, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(CTS, INPUT);
  pinMode(RTS, OUTPUT);

  // Simple 9600 8N1
  Serial.begin(9600); 

  // Assert RTS to accept incomming bytes
  digitalWrite(RTS,LOW);
  
  // Initialize 2.5ms timer interupt and register isr
  Timer1.initialize(2500);
  Timer1.attachInterrupt(isr_timer);

}


/* ********************************************************************** */
/* *** MAIN 1KHZ INTERRUPT ROUTINE                                    *** */
/* ********************************************************************** */
void isr_timer() {
  int i;

  /* ********************************************************************** */
  /* *** TRANSMIT                                                       *** */
  /* ********************************************************************** */

  // Check if buffer is full. If buffer is full it is better to lose bytes than to crash ...
  if (Serial.available() > 0 && tx_buff_last < BUFFSIZE - 1) {
    tx_buff[tx_buff_last] = Serial.read() & 0xFF;
    tx_buff_last++;
  } 

  // Some implementation take several bytes until they react on RTS deassertion. Thus, we accept only 4 bytes before deasserting RTS.
  if(tx_buff_last > 3) {
      digitalWrite(RTS,HIGH);
  }
  else {
      digitalWrite(RTS,LOW);
  }

  // A single bit is exactly 25ms long. Thus, we switch states every 10 timer ticks.
  if(tick % 10 == 0) {
    switch(tx_state) {
      case 0:
       // If buffer is nonempty, remove and transmit first byte from buffer
        if(tx_buff_last > 0) {
          tx_byte = tx_buff[0];
          // move all items to the front - yes, dumb but simple! No need for a ringbuffer whatsoever!
          for(i = 1; i < tx_buff_last; i++) {
            tx_buff[i-1] = tx_buff[i];
          }
          tx_buff_last --;
          tx_state = 1;
          tx_bit = 0;
        }
        break;
      case 1:
        // Write the start bit
        digitalWrite(SPAGHETTI_TX,HIGH);
        tx_state = 2;
        break;
      case 2:
        // Write individual bits
        digitalWrite(SPAGHETTI_TX,(tx_byte >> tx_bit) & 0x01);
        tx_bit++;
        if(tx_bit > 7) {
          tx_blank = 0;
          tx_state = 3;
          tx_bit = 0;
        }
        break;
      case 3:
        // Insert a two cycle blank
        digitalWrite(SPAGHETTI_TX,LOW);
        if(tx_blank > 1) {
          tx_state = 0;
        }
        tx_blank ++;
        break;
    }   
  }

  /* ********************************************************************** */
  /* *** RECEIVE                                                        *** */
  /* ********************************************************************** */
  
  // We scan for the start bit on any tick (400Hz)
  if(rx_state == 0) {
    if(digitalRead(SPAGHETTI_RX)) {

#ifdef __DEBUG__
      digitalWrite(LED_RED,HIGH);
      delay(1);
      digitalWrite(LED_RED,LOW);
#endif

      rx_state = 1;
      rx_byte = 0x0;
      rx_bit = 0;
      // We store the offset to the 25ms bit cycle and add some margin to scan for the next bits roughly in the middle of their heigh period.
      rx_tick_offset = ((tick + 6) % 10);  
    }
  }
  
  if(rx_state != 0 && tick % 10 == rx_tick_offset) {
    switch(rx_state) {
      case 1:
        // Waiting for the start bit to clear (dummy state)
        rx_state = 2;
        break;
      case 2:
      
#ifdef __DEBUG__  
        digitalWrite(LED_GREEN,HIGH);
        delay(1);
        digitalWrite(LED_GREEN,LOW);
#endif
        // Reading all the bits
        rx_byte = rx_byte | (digitalRead(SPAGHETTI_RX) << rx_bit);       
        rx_bit ++;
        
        if(rx_bit > 7) {
          Serial.write(rx_byte);
          rx_state = 3;
        }
        break;
      case 3:
        // Waiting for last transferred bit to clear (dummy state)
        rx_state = 0;
        rx_bit = 0;
        break;
    }   
  }

  /* ********************************************************************** */
  /* *** HOUSE KEEPING                                                  *** */
  /* ********************************************************************** */

  // Increment tick counter and roll over
  tick ++;
  if(tick >= 1000) tick = 0;
}

// Nothing left to be done. Just an empty loop ...
void loop() {
    
}
