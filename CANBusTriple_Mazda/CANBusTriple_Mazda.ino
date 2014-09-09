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
#include "TroubleCodes.h"


#define BOOT_LED 7

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

static byte wheelButton = 0;


void setup()
{  
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
    
    WheelButton::setLongPressHandler(&longButtonPressHandler);
}


// Toggle the LED on and off
void ledToggle() {
    digitalWrite( BOOT_LED, HIGH );
    delay(100);
    digitalWrite( BOOT_LED, LOW );
    delay(100);
}


void loop()
{
   byte button = WheelButton::getButtonDown();
   
   if( wheelButton != button )
   {
       wheelButton = button;
       
       if (wheelButton == B_ARROW_DOWN) {
           // Handle switching the trouble codes
           if (MazdaLED::currentScreen == 100)
               previousCode(&MazdaLED::codes);
           else
           {
               // This is used for switching to the max values for some screens
               if (MazdaLED::subScreen > 0)
                   MazdaLED::subScreen--;
           }
       }
       else if (wheelButton == B_ARROW_UP) {
           // Handle switching trouble codes
           if (MazdaLED::currentScreen == 100)
               nextCode(&MazdaLED::codes);
           else
           {
               // This is used for switching to the max values for some screens
               if (MazdaLED::subScreen == 0)
                   MazdaLED::subScreen++;
           }
       }
       else if (wheelButton == B_ARROW_LEFT) {
           MazdaLED::nextScreen(-1);
       }
       else if (wheelButton == B_ARROW_RIGHT) {
           MazdaLED::nextScreen(1);
       }
       else if (wheelButton == (B_INFO_BACK | B_ARROW_RIGHT)) {
           MazdaLED::enabled = !MazdaLED::enabled;
           EEPROM.write(EepromEnabledBit, MazdaLED::enabled); // For testing, proper settings in EEPROM TBD
           if(MazdaLED::enabled)
               MazdaLED::showStatusMessage("MazdaLED ON ", 2000);
           else
               MazdaLED::showStatusMessage("MazdaLED OFF", 2000);
       }
       else if (wheelButton == B_ARROW_ENTER) {
           // Clear the codes if the user presses enter on the code page
           if (MazdaLED::currentScreen == 100) {
               if (getNumCodes(&MazdaLED::codes) > 0)
               {
                   MazdaLED::clearMIL();
                   MazdaLED::showStatusMessage("Codes Clear!", 2000);
               }
               else
               {
                   MazdaLED::showStatusMessage("None to clr", 2000);
               }
           }
       }
       else if (wheelButton == (B_INFO_BACK | B_ARROW_LEFT)) {
           // Run the service call to scan for check engine light codes
           MazdaLED::checkMILStatus();
           MazdaLED::currentScreen = 100;
           MazdaLED::celsProcessed = false;
       }
       /* No actions for this one, using it could cause issues with the info display
       else if (wheelButton == B_INFO_INFO) {
       }
       */
   }
   
    // All Middleware ticks (Like loop() for middleware)
    MazdaLED::tick();
    
    readBus( CANBus1 );
    readBus( CANBus2 );
    readBus( CANBus3 );
    
    boolean success = true;
    while( !messageQueue.isEmpty() && success )
    {
        Message msg = messageQueue.pop();
        CANBus channel = busses[msg.busId-1];
        success = sendMessage( msg, channel );
        
        if( !success )
        {
            // Print to serial if the message wasn't already printed
            if (!msg.printedToSerial)
                SerialCommand::printMessageToSerial(msg);
            
            // TX Failure, add back to queue
            messageQueue.push(msg);
        }
    }
}


boolean sendMessage( Message msg, CANBus channel )
{  
  if( msg.dispatch == false ) return true;
  
  digitalWrite( BOOT_LED, HIGH );
  
  int ch = channel.getNextTxBuffer();
  
  switch( ch )
  {
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
  
  digitalWrite( BOOT_LED, LOW );
  
  return true;
}


void readBus( CANBus channel )
{  
    // byte bus_status;
    // bus_status = channel.readRegister(RX_STATUS);
    
    rx_status = channel.readStatus();
    
    // RX Buffer 0?
    if( (rx_status & 0x1) == 0x1 )
    {
        Message msg;
        msg.busStatus = rx_status;
        msg.busId = channel.busId;
        channel.readDATA_ff_0( &msg.length, msg.frame_data, &msg.frame_id );
        processMessage( msg );    
    }
    
    // RX Buffer 1?
    if( (rx_status & 0x2) == 0x2 )
    {
        Message msg;
        msg.busStatus = rx_status;
        msg.busId = channel.busId;
        channel.readDATA_ff_1( &msg.length, msg.frame_data, &msg.frame_id );
        processMessage( msg );
    }
}


void processMessage( Message msg )
{  
    // All Middleware process calls (Augment incoming CAN packets)
    msg = MazdaLED::process( msg );
    msg = ChannelSwap::process( msg );
    
    // Don't print messages that we overwrite later - This is the display screen
    if (!(msg.frame_id == 0x290 || msg.frame_id == 0x291 || msg.frame_id == 0x28F))
    {
        SerialCommand::printMessageToSerial(msg);
        msg.printedToSerial = true;
    }
    
    if( msg.dispatch == true ){
        messageQueue.push( msg );
    } 
}


/* Function to handle long presses*/
void longButtonPressHandler()
{
    switch (WheelButton::btnState)
    {
      case B_INFO_BACK:
        WheelButton::controlsEnabled = !WheelButton::controlsEnabled;
         
        if (WheelButton::controlsEnabled)
          MazdaLED::showStatusMessage("Controls On", 1500);
        else
          MazdaLED::showStatusMessage("Controls Off", 1500);
        break;
    }
}
