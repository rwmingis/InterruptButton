#include <Arduino.h>
#include "InterruptButton.h"

#define BUTTON_1  0                 // Top Left or Bottom Right button
#define BUTTON_2  35                // Bottom Left or Top Right button


//-- BUTTON VARIABLES ---------------------------------------------------------
InterruptButton button1(BUTTON_1, LOW);   // Statically allocated button (safest)
InterruptButton* button2;                 // Dynamically allocated button (if you know what you are doing)


//-- EXAMPLE ACTION FUCTION TO BIND TO AN EVENT LATER -------------------------
void menu0Button1keyDown(void) {
  Serial.printf("Menu 0, Button 1: Key Down:              %lu ms\n", millis());
}

//== MAIN SETUP FUNCTION ===========================================================================
//==================================================================================================

void setup() {
  Serial.begin(115200);                           // Remember to match platformio.ini setting here
  while(!Serial);                                 // Wait for serial port to start up

  // SETUP THE BUTTONS -----------------------------------------------------------------------------
  button2 = new InterruptButton(BUTTON_2, LOW);   // Create dynamically allocated button
  uint8_t numMenus = 3;                           // Maximum number of menus/pages (think relative to gui menus) a single button will have
  InterruptButton::setMenuCount(numMenus);
  InterruptButton::setMenuLevel(0);               // Use the functions bound to the first menu associated with the button
  //InterruptButton::m_RTOSservicerStackDepth = 4096; // Use larger values for more memory intensive functions if using Asynchronous mode.
  InterruptButton::setMode(Mode_Asynchronous);    // Defaults to Asynchronous (immediate like an ISR and not actioned in main loop)

  // -- Menu/UI Page 00 Functions -------------------------------------------------
  // ------------------------------------------------------------------------------

  // BIND BUTTON NUMBER 1 ACTIONS
  // Bind the address of a predefined function to an event as per below
  uint8_t thisMenuLevel = 0;
  button1.bind(Event_KeyDown,          thisMenuLevel, &menu0Button1keyDown);

  // Or simply bind LAMBDA functions (with no name) directly to the event as per the remaining examples
  button1.bind(Event_KeyUp,            thisMenuLevel, [](){ Serial.printf("Menu 0, Button 1: Key Up:                %lu ms\n", millis()); });
  button1.bind(Event_KeyPress,         thisMenuLevel, [](){ Serial.printf("Menu 0, Button 1: Key Press:             %lu ms\n", millis()); });
  button1.bind(Event_LongKeyPress,     thisMenuLevel, [](){ Serial.printf("Menu 0, Button 1: Long Key Press:        %lu ms\n", millis()); });
  button1.bind(Event_AutoRepeatPress,  thisMenuLevel, [](){ Serial.printf("Menu 0, Button 1: Auto Repeat Key Press: %lu ms\n", millis()); });
  button1.bind(Event_DoubleClick,      thisMenuLevel, [](){ 
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
  } });


  // BIND BUTTON NUMBER 2 ACTIONS
  button2->bind(Event_KeyDown,          thisMenuLevel, [](){ Serial.printf("Menu 0, Button 2: Key Down:              %lu ms\n", millis()); });  
  button2->bind(Event_KeyUp,            thisMenuLevel, [](){ Serial.printf("Menu 0, Button 2: Key Up  :              %lu ms\n", millis()); });
  button2->bind(Event_KeyPress,         thisMenuLevel, [](){ Serial.printf("Menu 0, Button 2: Key Press:             %lu ms\n", millis()); });
  
  button2->bind(Event_LongKeyPress,     thisMenuLevel, [](){ 
    Serial.printf("Menu 0, Button 2: Long Press:            %lu ms - Disabling doubleclicks and switing to menu level ", millis());
    button2->disableEvent(Event_DoubleClick);
    InterruptButton::setMenuLevel(1);
    Serial.println(InterruptButton::getMenuLevel());
  });

  button2->bind(Event_AutoRepeatPress,  thisMenuLevel, [](){ Serial.printf("Menu 0, Button 2: Auto Repeat Key Press: %lu ms\n", millis()); });
  button2->bind(Event_DoubleClick,      thisMenuLevel, [](){ Serial.printf("Menu 0, Button 2: Double Click:          %lu ms - Changing to Menu Level ", millis());
    InterruptButton::setMenuLevel(1);
    Serial.println(InterruptButton::getMenuLevel()); 
  });




  // -- Menu/UI Page 01 Functions -------------------------------------------------
  // ------------------------------------------------------------------------------
  thisMenuLevel = 1;
  button1.bind(Event_KeyDown,          thisMenuLevel, [](){ Serial.printf("Menu 1, Button 1: Key Down:              %lu ms\n", millis()); });
  button1.bind(Event_KeyUp,            thisMenuLevel, [](){ Serial.printf("Menu 1, Button 1: Key Up:                %lu ms\n", millis()); });
  button1.bind(Event_KeyPress,         thisMenuLevel, [](){ Serial.printf("Menu 1, Button 1: Key Press:             %lu ms\n", millis()); });
  button1.bind(Event_LongKeyPress,     thisMenuLevel, [](){ Serial.printf("Menu 1, Button 1: Long Key Press:        %lu ms\n", millis()); });
  button1.bind(Event_AutoRepeatPress,  thisMenuLevel, [](){ Serial.printf("Menu 1, Button 1: Auto Repeat Key Press: %lu ms\n", millis());});
  button1.bind(Event_DoubleClick,      thisMenuLevel, [](){ 
    Serial.printf("Menu 1, Button 1: Double Click:          %lu ms - Changing to ASYNCHRONOUS mode and to menu level ", millis());
    InterruptButton::setMode(Mode_Asynchronous);
    InterruptButton::setMenuLevel(0);
    Serial.println(InterruptButton::getMenuLevel());
  });


  // Button 2 actions are defined as LAMDA functions which have no name (for exaample purposes)
  button2->bind(Event_KeyDown,          thisMenuLevel, [](){ Serial.printf("Menu 1, Button 2: Key Down:              %lu ms\n", millis()); });  
  button2->bind(Event_KeyUp,            thisMenuLevel, [](){ Serial.printf("Menu 1, Button 2: Key Up  :              %lu ms\n", millis()); });
  button2->bind(Event_KeyPress,         thisMenuLevel, [](){ Serial.printf("Menu 1, Button 2: Key Press:             %lu ms\n", millis()); });
  
  button2->bind(Event_LongKeyPress,     thisMenuLevel, [](){ 
    Serial.printf("Menu 1, Button 2: Long Press:            %lu ms - [Re-enabling doubleclick - NOTE FASTER KEYPRESS RESPONSE SINCE DOUBLECLICK WAS DISABLED] Changing back to Menu Level ", millis());
    button2->enableEvent(Event_DoubleClick);
    InterruptButton::setMenuLevel(0);
    Serial.println(InterruptButton::getMenuLevel());
  });

  button2->bind(Event_AutoRepeatPress,  thisMenuLevel, [](){ Serial.printf("Menu 1, Button 2: Auto Repeat Key Press: %lu ms\n", millis()); });
  
  button2->bind(Event_DoubleClick,      thisMenuLevel, [](){  
    Serial.print("Menu 1, Button 2: Double Click - Changing Back to Menu Level ");
    //button2->disableEvent(Event_DoubleClick);
    InterruptButton::setMenuLevel(2);
    Serial.println(InterruptButton::getMenuLevel());
  });





  // -- Menu/UI Page 02 Functions -------------------------------------------------
  // ------------------------------------------------------------------------------
  thisMenuLevel = 2;
  button1.bind(Event_KeyPress,         thisMenuLevel, [](){
    Serial.printf("Menu 2, Button 1: keyPress:              %lu ms\n", millis());
  });
  button1.bind(Event_DoubleClick,      thisMenuLevel, [](){
    Serial.printf("Menu 2, Button 1: Double Click:          %lu ms - Deleting Button 2, press Button 2 quick to test for crash!\n", millis());
    delay(2000);   
    if(button2 != nullptr) {
      delete button2; 
      button2 = nullptr;
    }
  });

  button2->bind(Event_KeyDown,       thisMenuLevel, [](){ Serial.printf("Menu 2, Button 2: Key Down:              %lu ms\n", millis()); });
  button2->bind(Event_KeyUp,         thisMenuLevel, [](){ Serial.printf("Menu 2, Button 2: Key Up:                %lu ms\n", millis()); });
  button2->bind(Event_KeyPress,      thisMenuLevel, [](){
    Serial.printf("Menu 2, Button 2: keyPress:              %lu ms - Didn't seem to Crash!\n", millis());
  });
}














//== MAIN LOOP FUNCTION =====================================================================================
//===========================================================================================================

void loop() { 
  if(InterruptButton::getMode() != Mode_Asynchronous) InterruptButton::processSyncEvents();

  // Normally main program will run here and cause various inconsistant loop timing in syncronous events.

  delay(2000);
}
