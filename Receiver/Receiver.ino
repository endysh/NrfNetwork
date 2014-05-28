/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

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

int ledRX = 7;
int ledTX = 7;
int ledOn = 8;
int relay = 5;

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
//const uint64_t pipes[2] = { 0xE7E7E7E700LL, 0xE7E7E7E700LL };
const uint64_t pipes[2] = { 0x0003E7E700LL, 0x0003E7E700LL };


char dataToSend = 'A';
bool bRelayOn = false;


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
  printf("skt_send(), Now sending %d, length=%d\r\n", a_data[0], a_length);
  delay(10);
  bool ok = radio.write(a_data, a_length);
  
  Result = (ok) ? a_length : 0;
  return Result;
}


BYTE skt_recv(BYTE* a_buf, const BYTE a_length)
{
//  radio.startListening();
  delay(20);
  
  BYTE done = 0;
  BYTE attempts = 0;
  
  while (!done) {
//    if (radio.available()) {
      // Fetch the payload, and see if this was the last one.
      done = radio.read(a_buf, a_length);
      printf("skt_recv(), got payload %d, length=%d\r\n", a_buf[0], a_length);
      printf("skt_recv(), done=%d, attempts=%d ...\r\n", done, attempts);
  
/*      
    } else {
      if(attempts++ > 30) {
        printf("skt_recv(), failed to receive data\r\n");
        break;
      }
      delay(20);
    }
*/    
  }
  
  return done;
}


void relayOn()
{
    digitalWrite(ledOn, HIGH);
    if(!bRelayOn) {
      digitalWrite(relay, HIGH);
//      delay(300);
    }
    bRelayOn = true;
}

void relayOff()
{
    digitalWrite(relay, LOW);
    digitalWrite(ledOn, LOW);
    bRelayOn = false;
}

void relayToggle()
{
    if(bRelayOn){
        relayOff();
    } else {
        relayOn();
    }
}


void setReceivingRole()
{
    uint64_t txPipe = pipes[1];
    uint64_t rxPipe = pipes[0];
  
    printf("\r\n*** Receiving mode ***\r\n");
  
  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  // Open 'our' pipe for writing
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)
    radio.openWritingPipe(txPipe);
    radio.openReadingPipe(1, rxPipe);
   
    printf("listen on %#1x%lx\r\n", ((char*)&rxPipe)[4] & 0x000ff, rxPipe);
    printf("write to  %#1x%lx\r\n", ((char*)&txPipe)[4] & 0x000ff, txPipe);
    printf("\r\n");
}


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

void setup(void)
{
  
  pinMode(ledRX, OUTPUT);     
  pinMode(ledTX, OUTPUT);     
  pinMode(ledOn, OUTPUT);     
  pinMode(relay, OUTPUT);     
  
  digitalWrite(ledRX, HIGH);
  digitalWrite(ledTX, HIGH);
  digitalWrite(ledOn, HIGH);
  delay(1000);               // wait for a second
  digitalWrite(ledRX, LOW);
  digitalWrite(ledTX, LOW);
  digitalWrite(ledOn, LOW);
  
  relayOff();

  //
  // Print preamble
  //
  Serial.begin(57600);
  printf_begin();
  printf("\r\nwireless relay receiver v1.0\r\n");

  //
  // Setup and configure rf radio
  //
  radio.begin();
  
  // set output level
  radio.setPALevel(RF24_PA_LOW);

  // optionally, increase the delay between retries & # of retries
//  radio.setRetries(15,15);
//  radio.setAutoAck(true);
  radio.setAutoAck(false);
  
  // set CRC to 8 bit
  radio.setCRCLength(RF24_CRC_8);
  
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(PACKET_SIZE);

  //
  // Open pipes to other nodes for communication
  //
  openPipes();

  //
  // Start listening
  //
  radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //
  radio.printDetails();

  printf("initialization complete\r\n");
  
}



///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
void loop(void)
{
  //
  // Ping out role.  Repeatedly send the current time
  //
  delay(50);     
  BYTE data = 0;

  // if there is data ready
  if (radio.available()) {
    
    printf("%d\r\n", radio.available());
    
    // Dump the payloads until we've gotten everything
    BYTE buf[PACKET_SIZE] = {0};
    bool done = false;
    bool reply = true;
    
    printf("\r\n");
    
    // Fetch the payload, and see if this was the last one.
    done = skt_recv(buf, sizeof(buf));
    data = buf[0];
    // Spew it
    printf("Got payload %d, length=%d\r\n", data, done);
    
    if(data == 'o') { // turn relay ON
      relayOn();
      
    } else if(data == 'f') { // turn relay OFF
      relayOff();
      
    } else if((data == 't') ||(data == 'T')) { // toggle relay
      relayToggle();
      reply = (data == 't');
      
    } else if(data == 's') { // report status
      data = bRelayOn ? 'o' : 'f';
    }
    
    digitalWrite(ledRX, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(50);
    digitalWrite(ledRX, LOW);   
    
    
    data = 'T';
    reply = true;
    
    if(reply) {
      // Send the byte back.
      BYTE buf[PACKET_SIZE] = {0};
      buf[0] = data;
  
      printf("Sending response...%c(%d)\r\n", data, data);
      bool ok = skt_send(buf, sizeof(buf));
      
      if(ok) {
        printf("OK\r\n");
        
      } else {
        printf("FAILED\r\n");
        digitalWrite(ledTX, HIGH);
        delay(50);
        digitalWrite(ledTX, LOW);   
        delay(100);
        
        digitalWrite(ledTX, HIGH);
        delay(50);
        digitalWrite(ledTX, LOW);   
        delay(100);
        
        digitalWrite(ledTX, HIGH);
        delay(50);
        digitalWrite(ledTX, LOW);   
      }
      
      digitalWrite(ledTX, HIGH);
      delay(50);
      digitalWrite(ledTX, LOW);   
    }

    // Now, resume listening so we catch the next packets.
    radio.startListening();
  }

  //
  // Change roles
  //
  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    if ( c == 'D' ) {
        // Become the primary receiver (pong back)
        radio.printDetails();
        
    } else if ( c == 'T' ) {
        relayToggle();
        
    } else if ( c == 'O' ) {
        relayOn();
        
    } else if ( c == 'F' ) {
        relayOff();
    }
  }
  
}

