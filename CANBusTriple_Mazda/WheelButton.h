

int arrowButtonIn = A0;
int infoButtonIn = A1;

// Button states
#define ARROW_BUTTON_NONE 9
#define ARROW_BUTTON_ENTER 7
#define ARROW_BUTTON_UP 1
#define ARROW_BUTTON_DOWN 2
#define ARROW_BUTTON_LEFT 3
#define ARROW_BUTTON_RIGHT 5
#define INFO_BUTTON_NONE 10
#define INFO_BUTTON_NAV 1
#define INFO_BUTTON_BACK 2
#define INFO_BUTTON_INFO 3

// Button debouncing 
#define BTN_DEBOUNCE_TIME 50


/*
*  Returns a byte indicating wheel buttons that are down
 *  
 */

class WheelButton {
public:
  static byte getButtonDown();
  static boolean arrowButtonIsDown;
  static boolean infoButtonIsDown;
  
  static byte btnState;            // Stores the current button reading
  static long lastDebounceTime;    // The last time the output was toggled
private:

};


// Setup default values
boolean WheelButton::arrowButtonIsDown = false;
boolean WheelButton::infoButtonIsDown = false;
long WheelButton::lastDebounceTime = 0;
byte WheelButton::btnState = B0;


byte WheelButton::getButtonDown()
{
  int aBtn = analogRead(arrowButtonIn) / 100;
  byte currentReading = B0;

  switch(aBtn){
    case ARROW_BUTTON_NONE:
      break;
    case ARROW_BUTTON_LEFT:
      currentReading += B10000000;
      break;
    case ARROW_BUTTON_RIGHT:
      currentReading += B01000000;
      break;
    case ARROW_BUTTON_UP:
      currentReading += B00100000;
      break;
    case ARROW_BUTTON_DOWN:
      currentReading += B00010000;
      break;
    case ARROW_BUTTON_ENTER:
      currentReading += B00001000;
      break;
  }

  int iBtn = analogRead(infoButtonIn) / 100;
  switch( iBtn ){
    case INFO_BUTTON_NONE:
      break;
    case INFO_BUTTON_NAV:
      currentReading += B00000100;
      break;
    case INFO_BUTTON_BACK:
      currentReading += B00000010;
      break;
    case INFO_BUTTON_INFO:
      currentReading += B0000001;
      break;
  }
  
  // Lets do some debouncing
  if (currentReading != btnState)
    lastDebounceTime = millis();
  
  // if the button is still pressed after our predefined wait time
  // then it's the button we want to press.
  if ((millis() - lastDebounceTime) > BTN_DEBOUNCE_TIME)
  {
    // If different than what's already pressed save it.
    if (currentReading != btnState)
      btnState = currentReading;
  }
  
  return btnState;
}



