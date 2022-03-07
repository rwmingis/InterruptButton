#include <Arduino.h>
#include "InterruptButton.h"

#define BUTTON_1  0                 // Top Left or Bottom Right button
#define BUTTON_2  35                // Bottom Left or Top Right button

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
InterruptButton button1(BUTTON_1, LOW);
InterruptButton button2(BUTTON_2, LOW);

//== MAIN SETUP FUNCTION ===========================================================================
//==================================================================================================

void setup() {
  Serial.begin(115200);                               // Remember to match platformio.ini setting here
  while(!Serial);                                     // Wait for serial port to start up

  // SETUP THE BUTTONS -----------------------------------------------------------------------------
  uint8_t numMenus = 2; 
  InterruptButton::setMenuCount(numMenus);
  InterruptButton::setMenuLevel(0);
  button1.begin(); button2.begin();

  // -- Menu/UI Page 0 Functions --------------------------------------------------
  // ------------------------------------------------------------------------------

  // Asynchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button1.bind(InterruptButton::KeyDown,          0, &menu0Button1keyDown);
  button1.bind(InterruptButton::KeyUp,            0, &menu0Button1keyUp);
  button1.bind(InterruptButton::KeyPress,         0, &menu0Button1keyPress);
  button1.bind(InterruptButton::LongKeyPress,     0, &menu0Button1longKeyPress);
  button1.bind(InterruptButton::AutoRepeatPress,  0, &menu0Button1autoRepeatPress);
  button1.bind(InterruptButton::DoubleClick,      0, &menu0Button1doubleClick);
  // Synchronous, so fires when triggered in main loop, can be Lamda
  button1.bind(InterruptButton::SyncKeyPress,     0, [](){ Serial.printf("Menu 0, Button 1: SYNC KeyPress:              [%lu ms]\n", millis()); });  
  button1.bind(InterruptButton::SyncLongKeyPress, 0, [](){ Serial.printf("Menu 0, Button 1: SYNC LongKeyPress:          [%lu ms]\n", millis()); });  
  button1.bind(InterruptButton::SyncAutoKeyPress, 0, [](){ Serial.printf("Menu 0, Button 1: SYNC AutoRepeat Press:      [%lu ms]\n", millis()); });  
  button1.bind(InterruptButton::SyncDoubleClick,  0, [](){ Serial.printf("Menu 0, Button 1: SYNC DoubleClick:           [%lu ms]\n", millis()); });  

  // Asynchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button2.bind(InterruptButton::KeyDown,          0, &menu0Button2keyDown);
  button2.bind(InterruptButton::KeyUp,            0, &menu0Button2keyUp);
  button2.bind(InterruptButton::KeyPress,         0, &menu0Button2keyPress);
  button2.bind(InterruptButton::LongKeyPress,     0, &menu0Button2longKeyPress);
  button2.bind(InterruptButton::AutoRepeatPress,  0, &menu0Button2autoRepeatPress);
  button2.bind(InterruptButton::DoubleClick,      0, &menu0Button2doubleClick);
  // Synchronous, so fires when triggered in main loop, can be Lamda
  button2.bind(InterruptButton::SyncKeyPress,     0, [](){ Serial.printf("Menu 0, Button 2: SYNC KeyPress:              [%lu ms]\n", millis()); });  
  button2.bind(InterruptButton::SyncLongKeyPress, 0, [](){ Serial.printf("Menu 0, Button 2: SYNC LongKeyPress:          [%lu ms]\n", millis()); });  
  button2.bind(InterruptButton::SyncAutoKeyPress, 0, [](){ Serial.printf("Menu 0, Button 2: SYNC AutoRepeat Press:      [%lu ms]\n", millis()); });  
  button2.bind(InterruptButton::SyncDoubleClick,  0, [](){ Serial.printf("Menu 0, Button 2: SYNC DoubleClick:           [%lu ms]\n", millis()); });  


  // -- Menu/UI Page 1 Functions --------------------------------------------------
  // ------------------------------------------------------------------------------

  // Asynchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button1.bind(InterruptButton::KeyDown,          1, &menu1Button1keyDown);
  button1.bind(InterruptButton::KeyUp,            1, &menu1Button1keyUp);
  button1.bind(InterruptButton::KeyPress,         1, &menu1Button1keyPress);
  button1.bind(InterruptButton::LongKeyPress,     1, &menu1Button1longKeyPress);
  button1.bind(InterruptButton::AutoRepeatPress,  1, &menu1Button1autoRepeatPress);
  button1.bind(InterruptButton::DoubleClick,      1, &menu1Button1doubleClick);
  // Synchronous, so fires when triggered in main loop, can be Lamda
  button1.bind(InterruptButton::SyncKeyPress,     1, [](){ Serial.printf("Menu 1, Button 1: SYNC KeyPress:              [%lu ms]\n", millis()); });  
  button1.bind(InterruptButton::SyncLongKeyPress, 1, [](){ Serial.printf("Menu 1, Button 1: SYNC LongKeyPress:          [%lu ms]\n", millis()); });  
  button1.bind(InterruptButton::SyncAutoKeyPress, 1, [](){ Serial.printf("Menu 1, Button 1: SYNC AutoRepeat Press:      [%lu ms]\n", millis()); });  
  button1.bind(InterruptButton::SyncDoubleClick,  1, [](){ Serial.printf("Menu 1, Button 1: SYNC DoubleClick:           [%lu ms]\n", millis()); });  

  // Asynchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button2.bind(InterruptButton::KeyDown,          1, &menu1Button2keyDown);
  button2.bind(InterruptButton::KeyUp,            1, &menu1Button2keyUp);
  button2.bind(InterruptButton::KeyPress,         1, &menu1Button2keyPress);
  button2.bind(InterruptButton::LongKeyPress,     1, &menu1Button2longKeyPress);
  button2.bind(InterruptButton::AutoRepeatPress,  1, &menu1Button2autoRepeatPress);
  //button2.bind(InterruptButton::DoubleClick,      1, &menu1Button2doubleClick);
  // Synchronous, so fires when triggered in main loop, can be Lamda
  button2.bind(InterruptButton::SyncKeyPress,     1, [](){ Serial.printf("Menu 1, Button 2: SYNC KeyPress:              [%lu ms]\n", millis()); });  
  button2.bind(InterruptButton::SyncLongKeyPress, 1, [](){ Serial.printf("Menu 1, Button 2: SYNC LongKeyPress:          [%lu ms]\n", millis()); });  
  button2.bind(InterruptButton::SyncAutoKeyPress, 1, [](){ Serial.printf("Menu 1, Button 2: SYNC AutoRepeat Press :     [%lu ms]\n", millis()); });  
  //button2.bind(InterruptButton::SyncDoubleClick,  1, [](){ Serial.printf("Menu 1, Button 2: SYNC DoubleClick:           [%lu ms]\n", millis()); });  

  //Clear any spurious clicks made during setup so we start fresh in the main loop.
  button1.clearInputs(); button2.clearInputs();
  InterruptButton::setMenuLevel(0); 
}

//== INTERRUPT SERVICE ROUTINES ====================================================================
//==================================================================================================
// Button1 ISR's (Top left).
void IRAM_ATTR menu0Button1keyDown(void)         { Serial.printf("Menu 0, Button 1: ASYNC Key Down:              %lu ms\n", millis()); }
void IRAM_ATTR menu0Button1keyUp(void)           { Serial.printf("Menu 0, Button 1: ASYNC Key Up:                %lu ms\n", millis()); }     
void IRAM_ATTR menu0Button1keyPress(void)        { Serial.printf("Menu 0, Button 1: ASYNC Key Press:             %lu ms\n", millis()); }     
void IRAM_ATTR menu0Button1longKeyPress(void)    { Serial.printf("Menu 0, Button 1: ASYNC Long Key Press:        %lu ms\n", millis()); }     
void IRAM_ATTR menu0Button1autoRepeatPress(void) { Serial.printf("Menu 0, Button 1: ASYNC Auto Repeat Key Press: %lu ms\n", millis()); }     
void IRAM_ATTR menu0Button1doubleClick(void)  {
  Serial.printf("Menu 0, Button 1: ASYNC Double Click:          %lu ms - Changing to Menu Level ", millis());
  InterruptButton::setMenuLevel(1);
  Serial.println(InterruptButton::getMenuLevel());
} 

void IRAM_ATTR menu1Button1keyDown(void)         { Serial.printf("Menu 1, Button 1: ASYNC Key Down:              %lu ms\n", millis()); }     
void IRAM_ATTR menu1Button1keyUp(void)           { Serial.printf("Menu 1, Button 1: ASYNC Key Up:                %lu ms\n", millis()); }      
void IRAM_ATTR menu1Button1keyPress(void)        { Serial.printf("Menu 1, Button 1: ASYNC Key Press:             %lu ms\n", millis()); }     
void IRAM_ATTR menu1Button1longKeyPress(void)    { Serial.printf("Menu 1, Button 1: ASYNC Long Key Press:        %lu ms\n", millis()); }     
void IRAM_ATTR menu1Button1autoRepeatPress(void) { Serial.printf("Menu 1, Button 1: ASYNC Auto Repeat Key Press: %lu ms\n", millis()); }     
void IRAM_ATTR menu1Button1doubleClick(void)  { 
  Serial.printf("Menu 1, Button 1: ASYNC Double Click:          %lu ms - Changing Back to Menu Level ", millis());
  InterruptButton::setMenuLevel(0);
  Serial.println(InterruptButton::getMenuLevel());
} 

// Button2 ISR's (Bottom Left).
void IRAM_ATTR menu0Button2keyDown(void)         { Serial.printf("Menu 0, Button 2: ASYNC Key Down:              %lu ms\n", millis()); }     
void IRAM_ATTR menu0Button2keyUp(void)           { Serial.printf("Menu 0, Button 2: ASYNC Key Up:                %lu ms\n", millis()); }     
void IRAM_ATTR menu0Button2keyPress(void)        { Serial.printf("Menu 0, Button 2: ASYNC Key Press:             %lu ms\n", millis()); }         
void IRAM_ATTR menu0Button2longKeyPress(void)    { Serial.printf("Menu 0, Button 2: ASYNC Long Key Press:        %lu ms\n", millis()); }     
void IRAM_ATTR menu0Button2autoRepeatPress(void) { Serial.printf("Menu 0, Button 2: ASYNC Auto Repeat Key Press: %lu ms\n", millis()); }     
void IRAM_ATTR menu0Button2doubleClick(void)  { 
  Serial.printf("Menu 0, Button 2: ASYNC Double Click:          %lu ms - Changing to Menu Level ", millis());
  InterruptButton::setMenuLevel(1);
  Serial.println(InterruptButton::getMenuLevel());
} 

void IRAM_ATTR menu1Button2keyDown(void)         { Serial.printf("Menu 1, Button 2: ASYNC Key Down:              %lu ms\n", millis()); }     
void IRAM_ATTR menu1Button2keyUp(void)           { Serial.printf("Menu 1, Button 2: ASYNC Key Up:                %lu ms\n", millis()); }     
void IRAM_ATTR menu1Button2keyPress(void)        { Serial.printf("Menu 1, Button 2: ASYNC Key Press:             %lu ms\n", millis()); }            
void IRAM_ATTR menu1Button2longKeyPress(void) { 
  Serial.printf("Menu 1, Button 2: ASYNC Long Press:            %lu ms - [NOTE FASTER KEYPRESS RESPONSE IF DOUBLECLICK NOT DEFINED] Changing Back to Menu Level ", millis());
  InterruptButton::setMenuLevel(0);
  Serial.println(InterruptButton::getMenuLevel());
 }
void IRAM_ATTR menu1Button2autoRepeatPress(void) { Serial.printf("Menu 1, Button 2: ASYNC Auto Repeat Key Press: %lu ms\n", millis()); }     
void IRAM_ATTR menu1Button2doubleClick(void) { 
  Serial.print("Menu 1, Button 2: ASYNC Double Click - Changing Back to Menu Level ");
  InterruptButton::setMenuLevel(0);
  Serial.println(InterruptButton::getMenuLevel());
}

//== MAIN LOOP FUNCTION =====================================================================================
//===========================================================================================================

void loop() { 
  // ACTION ANY SYNCHRONOUS BUTTON EVENTS NOT DONE ASYNCHRONOUSLY IN ISR
  button1.processSyncEvents(); button2.processSyncEvents();

  // Normally main program will run here and cause various inconsistant loop timing in syncronous events.

  delay(1000);
}
