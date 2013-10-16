#include <stdlib.h>

int arrowButtonIn = A0;
int infoButtonIn = A1;

// Button states
#define ARROW_BUTTON_NONE 100
#define ARROW_BUTTON_ENTER 72
#define ARROW_BUTTON_UP 10
#define ARROW_BUTTON_DOWN 23
#define ARROW_BUTTON_LEFT 38
#define ARROW_BUTTON_RIGHT 55
#define INFO_BUTTON_NONE 102
#define INFO_BUTTON_NAV 10
#define INFO_BUTTON_BACK 23
#define INFO_BUTTON_INFO 39

//Button codes
#define B_ARROW_NONE   B00000000
#define B_ARROW_LEFT   B10000000
#define B_ARROW_RIGHT  B01000000
#define B_ARROW_UP     B00100000
#define B_ARROW_DOWN   B00010000
#define B_ARROW_ENTER  B00001000
#define B_INFO_NONE    B00000000
#define B_INFO_NAV     B00000100
#define B_INFO_BACK    B00000010
#define B_INFO_INFO    B00000001

// Button debouncing 
#define BTN_DEBOUNCE_TIME 85

#define LONG_PRESS_MIN_TIME 2000

/*
*  Returns a byte indicating wheel buttons that are down
 *  
 */

class WheelButton {
public:
  static byte getButtonDown();
  static void setLongPressHandler(void (*handler)(void));
  
  static boolean arrowButtonIsDown;
  static boolean infoButtonIsDown;
  static boolean controlsEnabled;
  static boolean longPressTriggered;
  
  static byte btnState;            // Stores the current button reading
  static long lastDebounceTime;    // The last time the output was toggled
private:
  static byte error;  // Reading error range
  static boolean compare ( byte a, byte b);
};


// Setup default values
boolean WheelButton::arrowButtonIsDown = false;
boolean WheelButton::infoButtonIsDown = false;
long WheelButton::lastDebounceTime = 0;
byte WheelButton::btnState = 0;

// Long press
boolean WheelButton::controlsEnabled = true;
boolean WheelButton::longPressTriggered = false;
static void (*longPressFunction)(void) = NULL;

// Keep track of the time a button is pressed
static unsigned long pressStart = 0;

byte WheelButton::error = 6;

byte WheelButton::getButtonDown()
{  
  byte currentReading = 0;
  static byte lclBtnState = 0;
  
  // Arrow buttons
  int btn = analogRead(arrowButtonIn) / 10;
  
  if(compare( btn , ARROW_BUTTON_UP ))
    currentReading += B_ARROW_UP;
  
  if(compare( btn , ARROW_BUTTON_DOWN ))
    currentReading += B_ARROW_DOWN;
  
  if(compare( btn , ARROW_BUTTON_LEFT ))
    currentReading += B_ARROW_LEFT;
  
  if(compare( btn , ARROW_BUTTON_RIGHT ))
    currentReading += B_ARROW_RIGHT;
  
  if(compare( btn , ARROW_BUTTON_ENTER ))
    currentReading += B_ARROW_ENTER;
  
  
  // Info Button
  btn = analogRead(infoButtonIn) / 10;
  
  if(compare( btn , INFO_BUTTON_NAV ))
    currentReading += B_INFO_NAV;
  
  if(compare( btn , INFO_BUTTON_BACK ))
    currentReading += B_INFO_BACK;
    
  if(compare( btn , INFO_BUTTON_INFO ))
    currentReading += B_INFO_INFO;
    
  
  // Lets do some debouncing
  if (currentReading != lclBtnState)
  {
    lastDebounceTime = millis();
    lclBtnState = currentReading;
  }
  
  // if the button is still pressed after our predefined wait time
  // then it's the button we want to press.
  if ((millis() - lastDebounceTime) > BTN_DEBOUNCE_TIME && btnState != lclBtnState)
  {
    btnState = lclBtnState;
    
    // Save the button down time for long press calc.
    pressStart = millis();
  }
  
  // if the button is down lets determine if its a long pres
  if (!longPressTriggered && btnState != 0 && (millis() - pressStart) >= LONG_PRESS_MIN_TIME)
  {
    longPressTriggered = true;
    
    // Call the function if there's a handler
    (*longPressFunction)();
  }
    
  if (btnState == 0)
    longPressTriggered = false;
  
  // if controls are disabled only send the back button
  if (!controlsEnabled && currentReading != B_INFO_BACK)
      return 0;
  else
      return btnState;
}

boolean WheelButton::compare ( byte a, byte b)
{
  return a > b-error && a < b+error;
}


/* Sets the function that handes the long press */
void WheelButton::setLongPressHandler(void (*handler)(void))
{
    longPressFunction = handler;
}

