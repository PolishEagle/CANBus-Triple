#include "Middleware.h"
#include "TroubleCodes.h"
#include "SerialCommand.h"
#include <string.h>

#define SERVICE_CALL_TIMEOUT 5
//#define COBB_AP_CONNECTED


// This enum is used to identify which byte in the eeprom is used for what.
// Prevent someone from reusing a byte that's already used
enum {
  EepromEnabledBit = 0,
  EepromCurrentScreen = 1
};



class MazdaLED : Middleware
{
  private:
    static QueueArray<Message>* mainQueue;
    static unsigned long updateCounter;
    static void pushNewMessage();
    static int fastUpdateDelay;
    static void generateScreenMsg(Message msg);
  public:
    static void init( QueueArray<Message> *q );
    static void tick();
    static void setOverrideTime( int n );
    static void setStatusTime( int n );
    static void showStatusMessage(char* str, int time);
    static Message process(Message msg);
    
    static char* currentLcdString();
    static void cobbServiceCall();
    static void checkMILStatus();
    static void getCodes();
    static void clearMIL();
    static void nextScreen(short dir);    
    
    static boolean enabled;
    static boolean currentScreen;
    static short totalScreens;
    static TroubleCodeHead codes;
    static boolean celsProcessed;
    static char lcdString[13];
    static char lcdStockString[13];
    static char lcdStatusString[13];
    static unsigned long stockOverrideTimer;
    static unsigned long statusOverrideTimer;
    static unsigned long animationCounter;
    static short subScreen;
};


// Set the default number of pages that are supported
short MazdaLED::totalScreens = 8;
static char * screenDescription[13] = { "AFR & KR    ", "KPH & RPM   ", "Boost & BAT ", "FuelPr Exh.T", 
                                        "IAT(C) LTFT" , "MAF & Spark ", "Throttle Pos", "Coolant Temp" };
boolean MazdaLED::currentScreen = 0;


boolean MazdaLED::enabled = EEPROM.read(EepromEnabledBit);
unsigned long MazdaLED::updateCounter = 0;
int MazdaLED::fastUpdateDelay = 500;
QueueArray<Message>* MazdaLED::mainQueue;
char MazdaLED::lcdString[13] = "CANBusTriple";
char MazdaLED::lcdStockString[13] = "            ";
char MazdaLED::lcdStatusString[13] = "            ";
short MazdaLED::subScreen = 0;

// This variable is used for increasing the timeout for service calls
int extraTime = 0;

// The variables used for all the pages
int afr = 0;
int krd = 0;
int boost = 0;
int egt = 0;
int sparkAdvance = 0;
int speedKph = 0;
int engineRPM = 0;
int bat = 0;
int fuelPressure = 0;
int ltft = 0;
int iat = 0;
int maf = 0;
int spark = 0;
int coolant = 0;
int throttle = 0;
TroubleCodeHead MazdaLED::codes;
boolean MazdaLED::celsProcessed = false;
int max1 = 0;
int max2 = 0;

unsigned long MazdaLED::animationCounter = 0;
unsigned long MazdaLED::stockOverrideTimer = 4000;
unsigned long MazdaLED::statusOverrideTimer = 0;


void MazdaLED::init( QueueArray<Message> *q )
{
    mainQueue = q;
    sprintf(screenDescription[4], "IAT(%cC) LTFT", 0xDF);
}


void MazdaLED::tick()
{
    if(!enabled) return;
    
    static unsigned long lastRunTime = 0;
    
    // New LED update CAN message for fast updates
    // check stockOverrideTimer, no need to do fast updates while stock override is active
    if( (updateCounter+fastUpdateDelay) < millis() && stockOverrideTimer < millis() ){
        pushNewMessage();
        updateCounter = millis();
    }
  
    // Run the service calls every xxx ms
    if (millis() >= (SERVICE_CALL_TIMEOUT + extraTime) + lastRunTime) { lastRunTime = millis(); }
    else { return; }
    
    if (currentScreen == 100)
    {
        char * currCode = getCurrentCode(&codes);
        if (currCode == NULL)
            sprintf(lcdString, "Codes: %2d    ", getNumCodes(&codes));
        else
            sprintf(lcdString, "Code: %5s  ", currCode);
      
    }
    else
    {
        #ifndef COBB_AP_CONNECTED
        MazdaLED::cobbServiceCall();
        #endif
    }
}


void MazdaLED::showStatusMessage(char* str, int time)
{
    sprintf( MazdaLED::lcdStatusString , str);
    MazdaLED::setStatusTime(time);
}


void MazdaLED::pushNewMessage()
{
    char* lcd = currentLcdString();
  
    Message msg;
    msg.busId = 3;
    msg.frame_id = 0x290;
    msg.frame_data[0] = 0x90;  // Look this up from log
    msg.frame_data[1] = lcd[0];
    msg.frame_data[2] = lcd[1];
    msg.frame_data[3] = lcd[2];
    msg.frame_data[4] = lcd[3];
    msg.frame_data[5] = lcd[4];
    msg.frame_data[6] = lcd[5];
    msg.frame_data[7] = lcd[6];
    msg.length = 8;
    msg.dispatch = true;
    mainQueue->push(msg);
  
    Message msg2;
    msg2.busId = 3;
    msg2.frame_id = 0x291;
    msg2.frame_data[0] = 0x87;  // Look this up from log
    msg2.frame_data[1] = lcd[7];
    msg2.frame_data[2] = lcd[8];
    msg2.frame_data[3] = lcd[9];
    msg2.frame_data[4] = lcd[10];
    msg2.frame_data[5] = lcd[11];
    msg2.length = 8;
    msg2.dispatch = true;
    mainQueue->push(msg2);
  
    // Turn off extras and periods on screen
    // 02 03 02 8F C0 00 00 00 01 27 10 40 08
  
    Message msg3;
    msg3.busId = 1;
    msg3.frame_id = 0x28F;
    msg3.frame_data[0] = 0x90;
    msg3.frame_data[1] = 0x0;
    msg3.frame_data[2] = 0x0;
    msg3.frame_data[3] = 0x0;
    msg3.frame_data[4] = 0x1;
    msg3.frame_data[5] = 0x27;
    msg3.frame_data[6] = 0x10;
    msg3.frame_data[7] = 0x40;
    msg3.length = 8;
    msg3.dispatch = true;
    mainQueue->push(msg3);
}


char* MazdaLED::currentLcdString()
{
    if( stockOverrideTimer > millis() )
        return lcdStockString;
    else if( statusOverrideTimer > millis() )
        return lcdStatusString;
    else
        return lcdString;
}


void MazdaLED::setOverrideTime( int n )
{
    stockOverrideTimer = millis() + n;
}


void MazdaLED::setStatusTime( int n )
{
    statusOverrideTimer = millis() + n;
}


Message MazdaLED::process(Message msg)
{
    if(!enabled) return msg;
  
    if( msg.frame_id == 0x28F && stockOverrideTimer < millis() )
    {
        // Block extras
        //msg.frame_data[0] = 0x90;
        msg.frame_data[1] = 0x0;
        msg.frame_data[2] = 0x0;
        msg.frame_data[3] = 0x0;
        // msg.frame_data[4] = 0x01; // Looks like phone light
        msg.frame_data[5] = 0x27;
        msg.frame_data[6] = 0x10;
        // msg.frame_data[7] = 0x40; looks like music note light
        msg.dispatch = true;
    }
  
    if( msg.frame_id == 0x290 )
    {
        if( msg.frame_data[1] != lcdStockString[0] ||
            msg.frame_data[2] != lcdStockString[1] || 
            msg.frame_data[3] != lcdStockString[2] || 
            msg.frame_data[4] != lcdStockString[3] || 
            msg.frame_data[5] != lcdStockString[4] || 
            msg.frame_data[6] != lcdStockString[5] || 
            msg.frame_data[7] != lcdStockString[6])
        {  
          lcdStockString[0] = msg.frame_data[1];
          lcdStockString[1] = msg.frame_data[2];
          lcdStockString[2] = msg.frame_data[3];
          lcdStockString[3] = msg.frame_data[4];
          lcdStockString[4] = msg.frame_data[5];
          lcdStockString[5] = msg.frame_data[6];
          lcdStockString[6] = msg.frame_data[7];
        }
        
        char* lcd = currentLcdString();    
        msg.frame_data[1] = lcd[0];
        msg.frame_data[2] = lcd[1];
        msg.frame_data[3] = lcd[2];
        msg.frame_data[4] = lcd[3];
        msg.frame_data[5] = lcd[4];
        msg.frame_data[6] = lcd[5];
        msg.frame_data[7] = lcd[6];
        msg.dispatch = true;
    }
  
    if( msg.frame_id == 0x291 )
    {
        if( msg.frame_data[1] != lcdStockString[7] ||
            msg.frame_data[2] != lcdStockString[8] || 
            msg.frame_data[3] != lcdStockString[9] || 
            msg.frame_data[4] != lcdStockString[10] || 
            msg.frame_data[5] != lcdStockString[11] )
        {
            lcdStockString[7] = msg.frame_data[1];
            lcdStockString[8] = msg.frame_data[2];
            lcdStockString[9] = msg.frame_data[3];
            lcdStockString[10] = msg.frame_data[4];
            lcdStockString[11] = msg.frame_data[5];
      
            setOverrideTime(1500);
        }
    
        char* lcd = currentLcdString();    
        msg.frame_data[1] = lcd[7];
        msg.frame_data[2] = lcd[8];
        msg.frame_data[3] = lcd[9];
        msg.frame_data[4] = lcd[10];
        msg.frame_data[5] = lcd[11];
        msg.dispatch = true;
    }
  
    // Turn off extras like decimal point. Needs verification!
    if( msg.frame_id == 0x201 )
    {
        msg.dispatch = false;
    }
  
    // Get our cutom message generated
    generateScreenMsg(msg);
  
    return msg;
}


void MazdaLED::generateScreenMsg(Message msg)
{
    if (currentScreen != 100)
    {
        // Display an empty string just incase nothing is selected
        sprintf(lcdString, "            ");
    }
    else
    {
        // Page for handling check engine searching
        if(msg.frame_id == 0x7E8 && msg.frame_data[2] == 0x1 && !celsProcessed)
        {
            // Initialize the codes struct so we have enough space for all of them
            initializeCodesStruct(&codes, msg.frame_data[3] & 0x3F);
            
            // Make a service call to get the codes
            getCodes();
            
            celsProcessed = true;
        }
        // This code is used for handling DTC retrieval
        else if (msg.frame_id == 0x7E8 && msg.frame_data[1] == 0x43)
        {
            Serial.write("DTC Precessing");
            
            processDTC(&codes, msg.frame_data[2], msg.frame_data[3]);
            processDTC(&codes, msg.frame_data[4], msg.frame_data[5]);
            processDTC(&codes, msg.frame_data[6], msg.frame_data[7]);
         }
         
         return;
    }
    

    // Process the frames accordingly
    if (msg.frame_id == 0xFD)
    {
        boost = (msg.frame_data[6] * 14.5) - 1450;
    }
    if (msg.frame_id == 0x7E8 && msg.frame_data[0] == 0x10)
    {
        krd = ((msg.frame_data[6] << 8) + msg.frame_data[7]);
    }
    else if (msg.frame_id == 0x7E8 && msg.frame_data[0] == 0x21)
    {
        bat = msg.frame_data[2] - 40;
        afr = (msg.frame_data[3] * 23) / 20;
        ltft = (msg.frame_data[5] - 128) * 100;
        engineRPM = ((msg.frame_data[6] << 8) + msg.frame_data[7]) / 4;
    }
    else if (msg.frame_id == 0x7E8 && msg.frame_data[0] == 0x22)
    {
        speedKph = msg.frame_data[1];
        fuelPressure = ((msg.frame_data[5] << 8) + msg.frame_data[6]) * 1.45;
        maf = (msg.frame_data[3] << 8) + msg.frame_data[4];
        spark = msg.frame_data[2] - 128;
    }
    else if (msg.frame_id == 0x7E8 && msg.frame_data[0] == 0x23)
    {
        throttle = (msg.frame_data[3] * 100) / 225;
    }
    else if(msg.frame_id == 0x7E8 && msg.frame_data[0] == 0x24)
    {
        iat = msg.frame_data[1] - 40;
    }
    else if (msg.frame_id == 0x420 && msg.busId == 2)
    {
        coolant = msg.frame_data[0] - 40;
    }
    
    
    // Display the screen that the user wants
    switch(currentScreen)
    {
        // First page to show the AFR and Knock
        case 0:
            if (krd > max1) max1 = krd;
          
            // Show the screen that's desired
            if (MazdaLED::subScreen == 0)
                sprintf(lcdString, "A%2d.%1d KR%1d.%02d", afr/10, afr % 10, krd / 180, krd % 180);
            // Print the current knock and max
            else if (MazdaLED::subScreen >= 1)
                sprintf(lcdString, "KR%1d.%02d M%1d.%02d", krd / 100, krd % 100, max1 / 100, max1 % 100);
            break;
      
          // Page two to show RPM and KM/h
        case 1:
            sprintf(lcdString, "S:%3d R:%4d", speedKph, engineRPM);
            break;
        
        // Page three to show Boost and BAT
        case 2:      
            sprintf(lcdString, "B%3d.%02d T%3d", boost/100, abs(boost % 100), bat);
            break;
        
        // Fuel preasure (PSI) and Exhaust gas temp
        case 3:
            sprintf(lcdString, "F:%4d E:%3d", fuelPressure, egt);
            break;
        
        // Long Term Fuel Trim  &  Intake Air Temp (Deg. C)
        case 4:
            sprintf(lcdString, "I%3d  L", iat);
            dtostrf((double)ltft / 128.0, 1, 2, &lcdString[7]);
            break;
          
        // MAF & Spark
        case 5:
            if (maf > max1)
                  max1 = maf;
                  
            if (spark > max2)
                max2 = spark;
          
            // Print the maf floating point value
            if (MazdaLED::subScreen == 0)
            {
                // Mxxx.xx Sxx.x
                sprintf(lcdString, "M");
                dtostrf((double)maf / 100.0, 1, 2, &lcdString[1]);
                sprintf(&lcdString[6], " S");
                dtostrf((double)spark / 2.0, 1, 2, &lcdString[8]);
            }
            else
            {
                sprintf(lcdString, ">M");
                dtostrf((double)max1 / 100.0, 1, 2, &lcdString[2]);
                sprintf(&lcdString[7], " S");
                dtostrf((double)max2 / 2.0, 1, 2, &lcdString[9]);
            }
            break;
            
        // Throttle Position
        case 6:
            sprintf(lcdString, "T.Pos.: %3d%%", throttle);  
            break;
          
        // coolant temp
        case 7:    
            sprintf(lcdString, "Temp: %3d%cC", coolant, 0xDF);
            break;
    }
}


void MazdaLED::nextScreen(short dir)
{
    if ((currentScreen + dir) >= totalScreens)
        currentScreen = 0;
    else if ((currentScreen + dir) < totalScreens && (currentScreen + dir) >= 0)
        currentScreen += dir;
    else if ((currentScreen + dir) < 0)
        currentScreen = (totalScreens - 1);
  
    // Display what gauges are shown for this gauge
    MazdaLED::showStatusMessage(screenDescription[currentScreen], 1500);
    
    // Reset the CEL count page
    celsProcessed = false;
    
    // Reset the max
    max1 = 0;
    max2 = 0;
    
    // Reset the sub page
    MazdaLED::subScreen = 0;
    
    // Clear the screen message when switching pages
    sprintf(lcdString, "            ");
  
    // Write the new screen value to the eeprom
    //EEPROM.write(EepromCurrentScreen, currentScreen);
}



void MazdaLED::checkMILStatus()
{
    Message msg;
    msg.busId = 2;
    msg.frame_id = 0x7E0;
    msg.frame_data[0] = 0x02;
    msg.frame_data[1] = 0x01;
    msg.frame_data[2] = 0x01;
    msg.frame_data[3] = 0x00;
    msg.frame_data[4] = 0x00;
    msg.frame_data[5] = 0x00;
    msg.frame_data[6] = 0x00;
    msg.frame_data[7] = 0x00;
    msg.length = 8;
    msg.dispatch = true;
    mainQueue->push(msg);
    
    // Clear the codes from the previous run
    destroyCodes(&codes);
}

void MazdaLED::getCodes()
{
    Message msg;
    msg.busId = 2;
    msg.frame_id = 0x7E0;
    msg.frame_data[0] = 0x01;
    msg.frame_data[1] = 0x03;
    msg.frame_data[2] = 0x00;
    msg.frame_data[3] = 0x00;
    msg.frame_data[4] = 0x00;
    msg.frame_data[5] = 0x00;
    msg.frame_data[6] = 0x00;
    msg.frame_data[7] = 0x00;
    msg.length = 8;
    msg.dispatch = true;
    mainQueue->push(msg);
}

void MazdaLED::clearMIL()
{
    Message msg;
    msg.busId = 2;
    msg.frame_id = 0x7E0;
    msg.frame_data[0] = 0x01;
    msg.frame_data[1] = 0x04;
    msg.frame_data[2] = 0x00;
    msg.frame_data[3] = 0x00;
    msg.frame_data[4] = 0x00;
    msg.frame_data[5] = 0x00;
    msg.frame_data[6] = 0x00;
    msg.frame_data[7] = 0x00;
    msg.length = 8;
    msg.dispatch = true;
    mainQueue->push(msg);
    
    sprintf(lcdString, "CEL Cleared");
}

// Cobb AP service calls
void MazdaLED::cobbServiceCall()
{
    Message msg;
    msg.busId = 2;
    msg.frame_id = 0x7E0;
    msg.frame_data[0] = 0x03;
    msg.frame_data[1] = 0x22;
    msg.frame_data[2] = 0x1E;
    msg.frame_data[3] = 0x04;
    msg.frame_data[4] = 0x00;
    msg.frame_data[5] = 0x00;
    msg.frame_data[6] = 0x00;
    msg.frame_data[7] = 0x00;
    msg.length = 8;
    msg.dispatch = true;
    mainQueue->push(msg);
    
    /* Not sure what this is just yet, so taking it out
    msg.frame_data[0] = 0x03;
    msg.frame_data[1] = 0x22;
    msg.frame_data[2] = 0x03;
    msg.frame_data[3] = 0x6C;
    msg.frame_data[4] = 0x00;
    msg.frame_data[5] = 0x00;
    msg.frame_data[6] = 0x00;
    msg.frame_data[7] = 0x00;
    msg.length = 8;
    msg.dispatch = true;
    mainQueue->push(msg);
    */
    
    msg.frame_data[0] = 0x30;
    msg.frame_data[1] = 0x0;
    msg.frame_data[2] = 0x0;
    msg.frame_data[3] = 0x0;
    msg.frame_data[4] = 0x0;
    msg.frame_data[5] = 0x0;
    msg.frame_data[6] = 0x0;
    msg.frame_data[7] = 0x0;
    mainQueue->push(msg);
}
