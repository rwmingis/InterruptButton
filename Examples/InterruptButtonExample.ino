#include <Arduino.h>
#include "InterruptButton.h"


#define pinButton1  0                 // Top Left or Bottom Right button
#define pinButton2  35                // Bottom Left or Top Right button


//== FUNCTION DECLARATIONS (Necessary if using PlatformIO as it doesn't do function hoisting) ======
//==================================================================================================
void menu0Button1keyDown(void);
void menu0Button1keyUp(void);
void menu0Button1keyPress(void);                 
void menu0Button1longKeyPress(void); 
void menu0Button1autoRepeatPress(void); 
void menu0Button1doubleClick(void); 

void menu0Button2keyDown(void);
void menu0Button2keyUp(void);
void menu0Button2keyPress(void);                 
void menu0Button2longKeyPress(void); 
void menu0Button2autoRepeatPress(void); 
void menu0Button2doubleClick(void); 

void menu1Button1keyDown(void);
void menu1Button1keyUp(void);
void menu1Button1keyPress(void);                 
void menu1Button1longKeyPress(void); 
void menu1Button1autoRepeatPress(void); 
void menu1Button1doubleClick(void); 

void menu1Button2keyDown(void);
void menu1Button2keyUp(void);
void menu1Button2keyPress(void);                 
void menu1Button2longKeyPress(void); 
void menu1Button2autoRepeatPress(void); 
void menu1Button2doubleClick(void); 

//-- BUTTON VARIABLES -----------------------------------------
InterruptButton button1(pinButton1, INPUT_PULLUP, LOW);
InterruptButton button2(pinButton2, INPUT_PULLUP, LOW);

//== MAIN SETUP FUNCTION ===========================================================================
//==================================================================================================

void setup() {
  Serial.begin(115200);                               // Remember to match platformio.ini setting here
  while(!Serial);                                     // Wait for serial port to start up

  // SETUP THE BUTTONS -----------------------------------------------------------------------------
  // -- Menu/UI Page 0 Functions -------------------
  button1.begin();
  button1.keyDown[0] =            &menu0Button1keyDown;         // Asynchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button1.keyUp[0] =              &menu0Button1keyUp;           // Asynchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button1.keyPress[0] =           &menu0Button1keyPress;                 
  button1.longKeyPress[0] =       &menu0Button1longKeyPress; 
  button1.autoRepeatPress[0] =    &menu0Button1autoRepeatPress; 
  button1.doubleClick[0] =        &menu0Button1doubleClick; 
  button1.syncKeyPress[0] =      [](){ Serial.printf("Menu 0, Button 1: Sync KeyPress: %d\n", millis()); };      // Synchronous, so fires when updated in main loop, can be Lamda
  button1.syncLongKeyPress[0] =  [](){ Serial.printf("Menu 0, Button 1: Sync LongKeyPress: %d\n", millis()); };  // Synchronous, so fires when updated in main loop, can be Lamda
  button1.syncAutoRepeatPress[0] = [](){ Serial.printf("Menu 0, Button 1: Sync AutoRepeat Press: %d\n", millis()); };  // Synchronous, so fires when updated in main loop, can be Lamda
  button1.syncDoubleClick[0] =   [](){ Serial.printf("Menu 0, Button 1: Sync DoubleClick: %d\n", millis()); };   // Synchronous, so fires when updated in main loop, can be Lamda

  button2.begin();
  button2.keyDown[0] =            &menu0Button2keyDown;         // Asynchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button2.keyUp[0] =              &menu0Button2keyUp;           // Asynchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button2.keyPress[0] =           &menu0Button2keyPress;                 
  button2.longKeyPress[0] =       &menu0Button2longKeyPress; 
  button2.autoRepeatPress[0] =    &menu0Button2autoRepeatPress; 
  button2.doubleClick[0] =        &menu0Button2doubleClick; 
  button2.syncKeyPress[0] =      [](){ Serial.printf("Menu 0, Button 2: Sync KeyPress: %d\n", millis()); };      // Synchronous, so fires when updated in main loop, can be Lamda
  button2.syncLongKeyPress[0] =  [](){ Serial.printf("Menu 0, Button 2: Sync LongKeyPress: %d\n", millis()); };  // Synchronous, so fires when updated in main loop, can be Lamda
  button2.syncAutoRepeatPress[0] = [](){ Serial.printf("Menu 0, Button 2: Sync AutoRepeat Press: %d\n", millis()); };  // Synchronous, so fires when updated in main loop, can be Lamda
  button2.syncDoubleClick[0] =   [](){ Serial.printf("Menu 0, Button 2: Sync DoubleClick: %d\n", millis()); };   // Synchronous, so fires when updated in main loop, can be Lamda

  // -- Menu/UI Page 1 Functions -------------------
  button1.keyDown[1] =            &menu1Button1keyDown;         // Asynchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button1.keyUp[1] =              &menu1Button1keyUp;           // Asynchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button1.keyPress[1] =           &menu1Button1keyPress;                 
  button1.longKeyPress[1] =       &menu1Button1longKeyPress; 
  button1.autoRepeatPress[1] =    &menu1Button1autoRepeatPress; 
  button1.doubleClick[1] =        &menu1Button1doubleClick; 
  button1.syncKeyPress[1] =      [](){ Serial.printf("Menu 1, Button 1: Sync KeyPress: %d\n", millis()); };      // Synchronous, so fires when updated in main loop, can be Lamda
  button1.syncLongKeyPress[1] =  [](){ Serial.printf("Menu 1, Button 1: Sync LongKeyPress: %d\n", millis()); };  // Synchronous, so fires when updated in main loop, can be Lamda
  button1.syncAutoRepeatPress[1] = [](){ Serial.printf("Menu 1, Button 1: Sync AutoRepeat Press: %d\n", millis()); };  // Synchronous, so fires when updated in main loop, can be Lamda
  button1.syncDoubleClick[1] =   [](){ Serial.printf("Menu 1, Button 1: Sync DoubleClick: %d\n", millis()); };   // Synchronous, so fires when updated in main loop, can be Lamda

  button2.keyDown[1] =            &menu1Button2keyDown;         // Asynchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button2.keyUp[1] =              &menu1Button2keyUp;           // Asynchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button2.keyPress[1] =           &menu1Button2keyPress;                 
  button2.longKeyPress[1] =       &menu1Button2longKeyPress;
  button2.autoRepeatPress[1] =    &menu1Button2autoRepeatPress; 
  //button2.doubleClick[1] =        &menu1Button2doubleClick; 
  button2.syncKeyPress[1] =      [](){ Serial.printf("Menu 1, Button 2: Sync KeyPress: %d\n", millis()); };      // Synchronous, so fires when updated in main loop, can be Lamda
  button2.syncLongKeyPress[1] =  [](){ Serial.printf("Menu 1, Button 2: Sync LongKeyPress: %d\n", millis()); };  // Synchronous, so fires when updated in main loop, can be Lamda
  button2.syncAutoRepeatPress[1] = [](){ Serial.printf("Menu 1, Button 2: Sync AutoRepeat Press: %d\n", millis()); };  // Synchronous, so fires when updated in main loop, can be Lamda
  //button2.syncDoubleClick[1] =   [](){ Serial.printf("Menu 1, Button 2: Sync DoubleClick: %d\n", millis()); };   // Synchronous, so fires when updated in main loop, can be Lamda

  //Clear any spurious clicks made during setup so we start fresh in the main loop.
  button1.clearInputs(); button2.clearInputs();
}

//== INTERRUPT SERVICE ROUTINES ====================================================================
//==================================================================================================
// Button1 ISR's (Top left).
void IRAM_ATTR menu0Button1keyDown(void)      { Serial.printf("Menu 0, Button 1: Async Key Down: %d\n", millis()); }
void IRAM_ATTR menu0Button1keyUp(void)        { Serial.printf("Menu 0, Button 1: Async Key Up: %d\n", millis()); }
void IRAM_ATTR menu0Button1keyPress(void)     { Serial.printf("Menu 0, Button 1: Async Key Press: %d\n", millis()); }
void IRAM_ATTR menu0Button1longKeyPress(void) { Serial.printf("Menu 0, Button 1: Async Long Key Press: %d\n", millis()); }
void IRAM_ATTR menu0Button1autoRepeatPress(void) { Serial.printf("Menu 0, Button 1: Async Auto Repeat Key Press: %d\n", millis()); }
void IRAM_ATTR menu0Button1doubleClick(void)  {
  InterruptButton::setMenuLevel(1);
  Serial.printf("Menu 0, Button 1: Async Double Click - Changing to Menu Level %d : %d\n", InterruptButton::getMenuLevel(), millis());
}

void IRAM_ATTR menu1Button1keyDown(void)      { Serial.printf("Menu 1, Button 1: Async Key Down: %d\n", millis()); }      
void IRAM_ATTR menu1Button1keyUp(void)        { Serial.printf("Menu 1, Button 1: Async Key Up: %d\n", millis()); }         
void IRAM_ATTR menu1Button1keyPress(void)     { Serial.printf("Menu 1, Button 1: Async Key Press: %d\n", millis()); }  
void IRAM_ATTR menu1Button1longKeyPress(void) { Serial.printf("Menu 1, Button 1: Async Long Key Press: %d\n", millis()); } 
void IRAM_ATTR menu1Button1autoRepeatPress(void) { Serial.printf("Menu 1, Button 1: Async Auto Repeat Key Press: %d\n", millis()); }  
void IRAM_ATTR menu1Button1doubleClick(void)  { 
  InterruptButton::setMenuLevel(0);
  Serial.printf("Menu 1, Button 1: Async Double Click - Changing Back to Menu Level %d : %d\n", InterruptButton::getMenuLevel(), millis());
} 

// Button2 ISR's (Bottom Left).
void IRAM_ATTR menu0Button2keyDown(void)      { Serial.printf("Menu 0, Button 2: Async Key Down: %d\n", millis()); }      
void IRAM_ATTR menu0Button2keyUp(void)        { Serial.printf("Menu 0, Button 2: Async Key Up: %d\n", millis()); }         
void IRAM_ATTR menu0Button2keyPress(void)     { Serial.printf("Menu 0, Button 2: Async Key Press: %d\n", millis()); }              
void IRAM_ATTR menu0Button2longKeyPress(void) { Serial.printf("Menu 0, Button 2: Async Long Key Press: %d\n", millis()); } 
void IRAM_ATTR menu0Button2autoRepeatPress(void) { Serial.printf("Menu 0, Button 2: Async Auto Repeat Key Press: %d\n", millis()); }    
void IRAM_ATTR menu0Button2doubleClick(void)  { 
  InterruptButton::setMenuLevel(1);
  Serial.printf("Menu 0, Button 2: Async Double Click - Changing to Menu Level %d : %d\n", InterruptButton::getMenuLevel(), millis());
} 

void IRAM_ATTR menu1Button2keyDown(void)      { Serial.printf("Menu 1, Button 2: Async Key Down: %d\n", millis()); }      
void IRAM_ATTR menu1Button2keyUp(void)        { Serial.printf("Menu 1, Button 2: Async Key Up: %d\n", millis()); }         
void IRAM_ATTR menu1Button2keyPress(void)     { Serial.printf("Menu 1, Button 2: Async Key Press: %d\n", millis()); }                 
void IRAM_ATTR menu1Button2longKeyPress(void) { 
  InterruptButton::setMenuLevel(0);
  Serial.printf("Menu 1, Button 2: Async Double Click - Changing Back to Menu Level %d : %d\n", InterruptButton::getMenuLevel(), millis());
}
void IRAM_ATTR menu1Button2autoRepeatPress(void) { Serial.printf("Menu 1, Button 2: Async Auto Repeat Key Press: %d\n", millis()); }    
/*void IRAM_ATTR menu1Button2doubleClick(void)  { 
  Serial.print("Menu 1, Button 2: Async Double Click - Changing Back to Menu Level ");
  InterruptButton::setMenuLevel(0);
  Serial.printf(InterruptButton::getMenuLevel());
}*/

//== MAIN LOOP FUNCTION =====================================================================================
//===========================================================================================================

void loop() { 
  // ACTION ANY SYNCHRONOUS BUTTON EVENTS NOT DONE ASYNCHRONOUSLY IN ISR
  button1.processSyncEvents(); button2.processSyncEvents();

  delay(100);
}
