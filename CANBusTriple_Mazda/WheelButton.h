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
byte WheelButton::btnState = 0;


byte WheelButton::getButtonDown()
{
  int aBtn = analogRead(arrowButtonIn) / 100;
  byte currentReading = 0;
  static byte lclBtnState = 0;

  switch(aBtn){
    case ARROW_BUTTON_NONE:
      break;
    case ARROW_BUTTON_LEFT:
      currentReading += B_ARROW_LEFT;
      break;
    case ARROW_BUTTON_RIGHT:
      currentReading += B_ARROW_RIGHT;
      break;
    case ARROW_BUTTON_UP:
      currentReading += B_ARROW_UP;
      break;
    case ARROW_BUTTON_DOWN:
      currentReading += B_ARROW_DOWN;
      break;
    case ARROW_BUTTON_ENTER:
      currentReading += B_ARROW_ENTER;
      break;
  }

  int iBtn = analogRead(infoButtonIn) / 100;
  switch( iBtn ){
    case INFO_BUTTON_NONE:
      break;
    case INFO_BUTTON_NAV:
      currentReading += B_INFO_NAV;
      break;
    case INFO_BUTTON_BACK:
      currentReading += B_INFO_BACK;
      break;
    case INFO_BUTTON_INFO:
      currentReading += B_INFO_INFO;
      break;
  }
  
  // Lets do some debouncing
  if (currentReading != lclBtnState)
  {
    lastDebounceTime = millis();
    lclBtnState = currentReading;
  }
  
  // if the button is still pressed after our predefined wait time
  // then it's the button we want to press.
  if ((millis() - lastDebounceTime) > BTN_DEBOUNCE_TIME && btnState != lclBtnState)
    btnState = lclBtnState;
  
  return btnState;
}
