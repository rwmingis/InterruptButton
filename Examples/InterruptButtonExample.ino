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

void menu1Button1keyDown(void);
void menu1Button1keyUp(void);
void menu1Button1keyPress(void);                 
void menu1Button1longKeyPress(void); 
void menu1Button1autoRepeatPress(void); 
void menu1Button1doubleClick(void); 


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

  // -- Menu/UI Page 0 Functions --------------------------------------------------
  // ------------------------------------------------------------------------------

  button1.bind(Event_KeyDown,          0, &menu0Button1keyDown);
  button1.bind(Event_KeyUp,            0, &menu0Button1keyUp);
  button1.bind(Event_KeyPress,         0, &menu0Button1keyPress);
  button1.bind(Event_LongKeyPress,     0, &menu0Button1longKeyPress);
  button1.bind(Event_AutoRepeatPress,  0, &menu0Button1autoRepeatPress);
  button1.bind(Event_DoubleClick,      0, &menu0Button1doubleClick);

  // Button 2 actions are defined as LAMDA functions which have no name (for exaample purposes)
  button2.bind(Event_KeyDown,          0, [](){ Serial.printf("Menu 0, Button 2: Key Down:              %lu ms\n", millis()); });  
  button2.bind(Event_KeyUp,            0, [](){ Serial.printf("Menu 0, Button 2: Key Up  :              %lu ms\n", millis()); });
  button2.bind(Event_KeyPress,         0, [](){ Serial.printf("Menu 0, Button 2: Key Press:             %lu ms\n", millis()); });
  
  button2.bind(Event_LongKeyPress,     0, [](){ 
    Serial.printf("Menu 0, Button 2: Long Press:            %lu ms - Disabling doubleclicks and switing to menu level ", millis());
    button2.disableEvent(Event_DoubleClick);
    InterruptButton::setMenuLevel(1);
    Serial.println(InterruptButton::getMenuLevel());
  });

  button2.bind(Event_AutoRepeatPress,  0, [](){ Serial.printf("Menu 0, Button 2: Auto Repeat Key Press: %lu ms\n", millis()); });
  button2.bind(Event_DoubleClick,      0, [](){ Serial.printf("Menu 0, Button 2: Double Click:          %lu ms - Changing to Menu Level ", millis());
    InterruptButton::setMenuLevel(1);
    Serial.println(InterruptButton::getMenuLevel()); 
  });

  // -- Menu/UI Page 1 Functions --------------------------------------------------
  // ------------------------------------------------------------------------------

  button1.bind(Event_KeyDown,          1, &menu1Button1keyDown);
  button1.bind(Event_KeyUp,            1, &menu1Button1keyUp);
  button1.bind(Event_KeyPress,         1, &menu1Button1keyPress);
  button1.bind(Event_LongKeyPress,     1, &menu1Button1longKeyPress);
  button1.bind(Event_AutoRepeatPress,  1, &menu1Button1autoRepeatPress);
  button1.bind(Event_DoubleClick,      1, &menu1Button1doubleClick);

  // Button 2 actions are defined as LAMDA functions which have no name (for exaample purposes)
  button2.bind(Event_KeyDown,          1, [](){ Serial.printf("Menu 1, Button 2: Key Down:              %lu ms\n", millis()); });  
  button2.bind(Event_KeyUp,            1, [](){ Serial.printf("Menu 1, Button 2: Key Up  :              %lu ms\n", millis()); });
  button2.bind(Event_KeyPress,         1, [](){ Serial.printf("Menu 1, Button 2: Key Press:             %lu ms\n", millis()); });
  
  button2.bind(Event_LongKeyPress,     1, [](){ 
    Serial.printf("Menu 1, Button 2: Long Press:            %lu ms - [Re-enabling doubleclick - NOTE FASTER KEYPRESS RESPONSE SINCE DOUBLECLICK WAS DISABLED] Changing back to Menu Level ", millis());
    button2.enableEvent(Event_DoubleClick);
    InterruptButton::setMenuLevel(0);
    Serial.println(InterruptButton::getMenuLevel());
  });

  button2.bind(Event_AutoRepeatPress,  1, [](){ Serial.printf("Menu 1, Button 2: Auto Repeat Key Press: %lu ms\n", millis()); });
  
  button2.bind(Event_DoubleClick,      1, [](){  
    Serial.print("Menu 1, Button 2: Double Click - Changing Back to Menu Level ");
    button2.disableEvent(Event_DoubleClick);
    InterruptButton::setMenuLevel(0);
    Serial.println(InterruptButton::getMenuLevel());
  });

  InterruptButton::setMenuLevel(0); 
  //InterruptButton::setMode(Sync);
}

//== FUNCTIONS/PROCEDURES TO BIND/ATTACH TO BUTTON EVENTS ==========================================
//==================================================================================================

// MENU 0 -----
void IRAM_ATTR menu0Button1keyDown(void)         { Serial.printf("Menu 0, Button 1: Key Down:              %lu ms\n", millis()); }
void IRAM_ATTR menu0Button1keyUp(void)           { Serial.printf("Menu 0, Button 1: Key Up:                %lu ms\n", millis()); }     
void IRAM_ATTR menu0Button1keyPress(void)        { Serial.printf("Menu 0, Button 1: Key Press:             %lu ms\n", millis()); }     
void IRAM_ATTR menu0Button1longKeyPress(void)    { Serial.printf("Menu 0, Button 1: Long Key Press:        %lu ms\n", millis()); }     
void IRAM_ATTR menu0Button1autoRepeatPress(void) { Serial.printf("Menu 0, Button 1: Auto Repeat Key Press: %lu ms\n", millis()); }     
void IRAM_ATTR menu0Button1doubleClick(void)  {
  Serial.printf("Menu 0, Button 1: Double Click:          %lu ms - ", millis());
  switch(InterruptButton::getMode()){
    case Mode_Asynchronous:
    InterruptButton::setMode(Mode_Hybrid);
    Serial.println("Changing to HYBRID mode.");
  break;
    case Mode_Hybrid:
    InterruptButton::setMode(Mode_Synchronous);
    Serial.println("Changing to SYNCHRONOUS mode.");
  break;
    case Mode_Synchronous:
    InterruptButton::setMode(Mode_Asynchronous);     
    Serial.println("Changing to ASYNCHRONOUS mode.");
  break;
  }
} 

// MENU 1 -----
void IRAM_ATTR menu1Button1keyDown(void)         { Serial.printf("Menu 1, Button 1: Key Down:              %lu ms\n", millis()); }     
void IRAM_ATTR menu1Button1keyUp(void)           { Serial.printf("Menu 1, Button 1: Key Up:                %lu ms\n", millis()); }      
void IRAM_ATTR menu1Button1keyPress(void)        { Serial.printf("Menu 1, Button 1: Key Press:             %lu ms\n", millis()); }     
void IRAM_ATTR menu1Button1longKeyPress(void)    { Serial.printf("Menu 1, Button 1: Long Key Press:        %lu ms\n", millis()); }     
void IRAM_ATTR menu1Button1autoRepeatPress(void) { Serial.printf("Menu 1, Button 1: Auto Repeat Key Press: %lu ms\n", millis()); }     
void IRAM_ATTR menu1Button1doubleClick(void)  { 
  Serial.printf("Menu 1, Button 1: Double Click:          %lu ms - Changing to ASYNCHRONOUS mode and to menu level ", millis());
  InterruptButton::setMode(Mode_Asynchronous);
  InterruptButton::setMenuLevel(0);
  Serial.println(InterruptButton::getMenuLevel());
} 


//== MAIN LOOP FUNCTION =====================================================================================
//===========================================================================================================

void loop() { 
  InterruptButton::processSyncEvents();

  // Normally main program will run here and cause various inconsistant loop timing in syncronous events.

  delay(2000);
}
