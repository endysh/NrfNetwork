/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include <avr/sleep.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"


#define BYTE unsigned char

#define PACKET_SIZE 32

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 
RF24 radio(9,10);

const BYTE ledTx = 7;
const BYTE ledRx = 8;
const BYTE btnOn = 3;

// Radio pipe addresses for the 2 nodes to communicate.
//const uint64_t pipes[2] = { 0xE7E7E7E700LL, 0xE7E7E7E700LL };
const uint64_t pipes[2] = { 0x0003E7E700LL, 0x0003E7E700LL };

BYTE dataToSend = 'z';
BYTE attempts = 0;

void openPipes()
{
    uint64_t txPipe = pipes[0];
    uint64_t rxPipe = pipes[1];

  // open two pipes
    radio.openWritingPipe(txPipe);
    radio.openReadingPipe(1, rxPipe);
   
    printf("listen on %#1x%lx\r\n", ((char*)&rxPipe)[4] & 0x000ff, rxPipe);
    printf("write to  %#1x%lx\r\n", ((char*)&txPipe)[4] & 0x000ff, txPipe);
    printf("\r\n");
}

void doButtonOn()
{
    if(0 == digitalRead(btnOn)) {
        dataToSend = 'T';
    }
    attempts = 0;
}

void setup(void)
{
  //set buttons pin to input
  pinMode(btnOn, INPUT);     
  //set internal pullup resistors
  digitalWrite(btnOn, HIGH);
  
  pinMode(ledTx, OUTPUT);     
  pinMode(ledRx, OUTPUT);     
  
  //flash transmission LED
  digitalWrite(ledTx, HIGH);
  digitalWrite(ledRx, HIGH);
  delay(500);               // wait for a second
  digitalWrite(ledTx, LOW);
  digitalWrite(ledRx, LOW);
  delay(100);               // wait for a second
  digitalWrite(ledTx, HIGH);
  digitalWrite(ledRx, HIGH);
  delay(100);               // wait for a second
  digitalWrite(ledTx, LOW);
  digitalWrite(ledRx, LOW);
  delay(100);               // wait for a second
  
  
  //
  // Print preamble
  //
  Serial.begin(57600);
  printf_begin();
  printf("\n\rRF24/examples/GettingStarted/\n\r");

  //
  // Setup and configure rf radio
  //
  radio.begin();

  // set output level
  radio.setPALevel(RF24_PA_LOW);
  
  // optionally, increase the delay between retries & # of retries
  //  radio.setRetries(15, 5);

  radio.setAutoAck(false);
  
  // set CRC to 8 bit
  radio.setCRCLength(RF24_CRC_8);
  
  // set data rate to 250 Kbps
  radio.setDataRate(RF24_250KBPS);
  
  // set channel 15
  radio.setChannel(15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(PACKET_SIZE);

  //
  // Open pipes to other nodes for communication
  //
  openPipes();
  
  //
  // Dump the configuration of the rf unit for debugging
  //
  radio.printDetails();

  printf("setup done\r\n");
}

void sendData(const BYTE a_data)
{
  dataToSend = a_data;
  attempts = 0;
}


BYTE skt_send(const BYTE* a_data, const BYTE a_length)
{
  BYTE Result = 0;
  
  // w/o this it will not send data ... 
  radio.startListening();
  delay(10);
  
  // stop listening so we can talk.
  radio.stopListening();
  delay(10);

  // Take the time, and send it.  This will block until complete
  printf("Now sending %d, length=%d\r\n", a_data[0], a_length);
  delay(10);
  bool ok = radio.write(a_data, a_length);
  
  Result = (ok) ? a_length : 0;
  return Result;
}


BYTE skt_recv(BYTE* a_buf, const BYTE a_length)
{
  BYTE Result = 0;
  
  radio.startListening();
  delay(20);
  
  BYTE done = 0;
  BYTE attempts = 0;
  
  while(attempts++ < 30) {
    if (radio.available()) {
      while (!done) {
          // Fetch the payload, and see if this was the last one.
          done = radio.read(a_buf, 1);
          printf("Got payload %d, length=%d\r\n", a_buf[0], a_length);
          printf("done=%d, attempts=%d ...\r\n", done, attempts);
      }
      break;
          
    } else {
      delay(20);
    }
  }
  
  if(attempts >= 30) {
    printf("failed to receive data\r\n");
  }
  
  return done;
}



///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
void loop(void)
{

  if (dataToSend != 'z')  {
    printf("\r\n");
    printf("dataToSend = %c\r\n", dataToSend);
    
    BYTE buf[PACKET_SIZE];
    buf[0] = dataToSend;

    //send data    
    digitalWrite(ledTx, HIGH);
    BYTE ret = skt_send(buf, sizeof(buf));
    digitalWrite(ledTx, LOW);
    
    //receive ack
    buf[0] = 0;
    ret = skt_recv(buf, sizeof(buf));
    digitalWrite(ledRx, HIGH);
    delay(50);
    digitalWrite(ledRx, LOW);
    
    if(ret == 1) {
      printf("OK\r\n");
      
    } else {
      printf("send failed.\r\n");
      digitalWrite(ledTx, HIGH);
      delay(50);
      digitalWrite(ledTx, LOW);
      delay(30);
      digitalWrite(ledTx, HIGH);
      delay(50);
      digitalWrite(ledTx, LOW);
      delay(30);
      digitalWrite(ledTx, HIGH);
      delay(50);
      digitalWrite(ledTx, LOW);
    }
    
    dataToSend = 'z';
    delay(200);

  } else {
    if(LOW == digitalRead(btnOn)) {
      sendData('t');
    }
  }
}

