/***
***  Basic read / send firmware sketch
***  https://github.com/etx/CANBus-Triple
***/


#include <SPI.h>
#include <CANBus.h>
#include <Message.h>
#include <QueueArray.h>
#include <EEPROM.h>

#include "WheelButton.h"
#include "ChannelSwap.h"
#include "MazdaLED.h"
#include "SerialCommand.h"


#define BOOT_LED 7
// comment the debug line out to enable debug
//#define DEBUG

#define CAN1INT 0
#define CAN1SELECT 0
#define CAN1RESET 4

#define CAN2INT 1
#define CAN2SELECT 1
#define CAN2RESET 12

#define CAN3SELECT 5
#define CAN3RESET 11

CANBus CANBus1(CAN1SELECT, CAN1RESET, 1, "Bus 1");
CANBus CANBus2(CAN2SELECT, CAN2RESET, 2, "Bus 2");
CANBus CANBus3(CAN3SELECT, CAN3RESET, 3, "Bus 3");

byte rx_status;
QueueArray<Message> messageQueue;

CANBus busses[3] = { CANBus1, CANBus2, CANBus3 };
CANBus SerialCommand::busses[3] = { CANBus1, CANBus2, CANBus3 }; // Maybe do this better

byte wheelButton = 0;



void setup(){
  
  Serial.begin( 115200 );
  pinMode( BOOT_LED, OUTPUT );
  
  // Toggle the LED 4 times
  for (int i = 0; i < 4; i++)
      ledToggle();
  
  // Setup CAN Busses 
  CANBus1.begin();
  CANBus1.baudConfig(125);
  // CANBus1.setRxInt(true);
  CANBus1.setMode(NORMAL);
  //attachInterrupt(CAN1INT, handleInterrupt0, LOW);
  
  CANBus2.begin();
  CANBus2.baudConfig(500);
  // CANBus2.setRxInt(true);
  CANBus2.setMode(NORMAL);
  // attachInterrupt(CAN2INT, handleInterrupt1, LOW);
  
  CANBus3.begin();
  CANBus3.baudConfig(125);
  CANBus3.setMode(NORMAL);
  
  // Toggle the LEDs
  ledToggle();
  
  // Middleware setup
  MazdaLED::init( &messageQueue );
  SerialCommand::init( &messageQueue, busses, 0 );
  
}

void handleInterrupt0(){}
void handleInterrupt1(){}

// Toggle the LED on and off
void ledToggle() {
    digitalWrite( BOOT_LED, HIGH );
    delay(100);
    digitalWrite( BOOT_LED, LOW );
    delay(100);
}


void loop() {
 byte button = WheelButton::getButtonDown();
 if( wheelButton != button )
 {
   wheelButton = button;
   
   switch(wheelButton){
     case B10000000: // left
       MazdaLED::nextScreen(-1);
     break;
     case B01000000: // right
       MazdaLED::nextScreen(1);
     break;
     case B00100000: // up
       MazdaLED::nextScreen(1);
     break;
     case B00010000: // down
       MazdaLED::nextScreen(-1);
     break;
     case B1000010:
       MazdaLED::enabled = !MazdaLED::enabled;
       EEPROM.write(EepromEnabledBit, MazdaLED::enabled); // For testing, proper settings in EEPROM TBD
       if(MazdaLED::enabled)
         MazdaLED::showStatusMessage("MazdaLED ON ", 2000);
         else
         MazdaLED::showStatusMessage("MazdaLED OFF", 2000);
     break;
   }
 
 }
 
  // All Middleware ticks (Like loop() for middleware)
  MazdaLED::tick();
  SerialCommand::tick();
  
  readBus( CANBus1 );
  readBus( CANBus2 );
  readBus( CANBus3 );
  
  // Process message stack
  #ifdef DEBUG
  if(!messageQueue.isEmpty() ){ 
    Serial.print("{queueCount:" ); 
    Serial.print( messageQueue.count(), DEC ); 
    Serial.println("}"); 
  }
  #endif
  
  boolean success = true;
  while( !messageQueue.isEmpty() && success ){
    
    Message msg = messageQueue.pop();
    CANBus channel = busses[msg.busId-1];
    
    // TODO: uncomment below to post msg's to the serial port
    SerialCommand::printMessageToSerial(msg);
    success = sendMessage( msg, channel );
    
    if( !success ){
      // TX Failure, add back to queue
      messageQueue.push(msg);
  
      #ifdef DEBUG
        Serial.println("ALL TX BUFFERS FULL ON " + busses[msg.busId-1].name );
      #endif
    }
    
  }
  
}


void sendTestFrame(){
  
  Message msg;
  msg.busId = 2;
  msg.frame_id = 0x7DF;     // 02 01 0D 00 00 00 00 00
  msg.frame_data[0] = 0x02;
  msg.frame_data[1] = 0x01;
  msg.frame_data[2] = 0x0D;
  msg.frame_data[3] = 0x00;
  msg.frame_data[4] = 0x00;
  msg.frame_data[5] = 0x00;
  msg.frame_data[6] = 0x00;
  msg.frame_data[7] = 0x00;
  msg.length = 8;
  msg.dispatch = true;
  messageQueue.push(msg);
  
  Serial.println("Sent a 0x7DF");
  
}


boolean sendMessage( Message msg, CANBus channel ){
  
  if( msg.dispatch == false ) return true;
  
  digitalWrite( BOOT_LED, HIGH );
  
  int ch = channel.getNextTxBuffer();
  
  switch( ch ){
    case 0:
      channel.load_ff_0( msg.length, msg.frame_id, msg.frame_data );
      channel.send_0();
      break;
    case 1:
      channel.load_ff_1( msg.length, msg.frame_id, msg.frame_data );
      channel.send_1();
      break;
    case 2:
      channel.load_ff_2( msg.length, msg.frame_id, msg.frame_data );
      channel.send_2();
      break;
    default:
      // All TX buffers full
      return false;
      break;
  }
  
  #ifdef DEBUG
    Serial.print("Sent a message on TXB");
    Serial.print( ch, DEC );
    Serial.print(" via bus ");
    Serial.println( channel.name );
  #endif
  
  digitalWrite( BOOT_LED, LOW );
  
  return true;
  
}





void readBus( CANBus channel ){
  
  // byte bus_status;
  // bus_status = channel.readRegister(RX_STATUS);
  
  rx_status = channel.readStatus();
  
  // RX Buffer 0?
  if( (rx_status & 0x1) == 0x1 ){
  //if( (bus_status & 0x40) == 0x40 ){
    
    Message msg;
    msg.busStatus = rx_status;
    msg.busId = channel.busId;
    channel.readDATA_ff_0( &msg.length, msg.frame_data, &msg.frame_id );
    processMessage( msg );
    
  }
  
  // RX Buffer 1?
  if( (rx_status & 0x2) == 0x2 ) {
  //if( (bus_status & 0x80) == 0x80 ){
    
    Message msg;
    msg.busStatus = rx_status;
    msg.busId = channel.busId;
    channel.readDATA_ff_1( &msg.length, msg.frame_data, &msg.frame_id );
    processMessage( msg );
    
  }
    
}


void processMessage( Message msg ){
  
  // All Middleware process calls (Augment incoming CAN packets)
  msg = SerialCommand::process( msg );
  msg = MazdaLED::process( msg );
  msg = ChannelSwap::process( msg );
  
  if( msg.dispatch == true ){
    messageQueue.push( msg );
  }
  
}
