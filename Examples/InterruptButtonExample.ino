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
void menu0Button1doubleClick(void); 

void menu0Button2keyDown(void);
void menu0Button2keyUp(void);
void menu0Button2keyPress(void);                 
void menu0Button2longKeyPress(void); 
void menu0Button2doubleClick(void); 

void menu1Button1keyDown(void);
void menu1Button1keyUp(void);
void menu1Button1keyPress(void);                 
void menu1Button1longKeyPress(void); 
void menu1Button1doubleClick(void); 

void menu1Button2keyDown(void);
void menu1Button2keyUp(void);
void menu1Button2keyPress(void);                 
void menu1Button2longKeyPress(void); 
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
  button1.keyDown[0] =            &menu0Button1keyDown;         // Synchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button1.keyUp[0] =              &menu0Button1keyUp;           // Synchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button1.keyPress[0] =           &menu0Button1keyPress;                 
  button1.longKeyPress[0] =       &menu0Button1longKeyPress; 
  button1.doubleClick[0] =        &menu0Button1doubleClick; 
  button1.asyncKeyPress[0] =      [](){ Serial.println("Menu 0, Button 1: Async KeyPress"); };      // Asynchronous, so fires when updated in main loop, can be Lamda
  button1.asyncLongKeyPress[0] =  [](){ Serial.println("Menu 0, Button 1: Async LongKeyPress"); };  // Asynchronous, so fires when updated in main loop, can be Lamda
  button1.asyncDoubleClick[0] =   [](){ Serial.println("Menu 0, Button 1: Async DoubleClick"); };   // Asynchronous, so fires when updated in main loop, can be Lamda

  button2.begin();
  button2.keyDown[0] =            &menu0Button2keyDown;         // Synchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button2.keyUp[0] =              &menu0Button2keyUp;           // Synchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button2.keyPress[0] =           &menu0Button2keyPress;                 
  button2.longKeyPress[0] =       &menu0Button2longKeyPress; 
  button2.doubleClick[0] =        &menu0Button2doubleClick; 
  button2.asyncKeyPress[0] =      [](){ Serial.println("Menu 0, Button 2: Async KeyPress"); };      // Asynchronous, so fires when updated in main loop, can be Lamda
  button2.asyncLongKeyPress[0] =  [](){ Serial.println("Menu 0, Button 2: Async LongKeyPress"); };  // Asynchronous, so fires when updated in main loop, can be Lamda
  button2.asyncDoubleClick[0] =   [](){ Serial.println("Menu 0, Button 2: Async DoubleClick"); };   // Asynchronous, so fires when updated in main loop, can be Lamda

  // -- Menu/UI Page 1 Functions -------------------
  button1.keyDown[1] =            &menu1Button1keyDown;         // Synchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button1.keyUp[1] =              &menu1Button1keyUp;           // Synchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button1.keyPress[1] =           &menu1Button1keyPress;                 
  button1.longKeyPress[1] =       &menu1Button1longKeyPress; 
  button1.doubleClick[1] =        &menu1Button1doubleClick; 
  button1.asyncKeyPress[1] =      [](){ Serial.println("Menu 1, Button 1: Async KeyPress"); };      // Asynchronous, so fires when updated in main loop, can be Lamda
  button1.asyncLongKeyPress[1] =  [](){ Serial.println("Menu 1, Button 1: Async LongKeyPress"); };  // Asynchronous, so fires when updated in main loop, can be Lamda
  button1.asyncDoubleClick[1] =   [](){ Serial.println("Menu 1, Button 1: Async DoubleClick"); };   // Asynchronous, so fires when updated in main loop, can be Lamda

  button2.keyDown[1] =            &menu1Button2keyDown;         // Synchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button2.keyUp[1] =              &menu1Button2keyUp;           // Synchronous, fires as an interrupt so should be IRAM_ATTR and be FAST!
  button2.keyPress[1] =           &menu1Button2keyPress;                 
  button2.longKeyPress[1] =       &menu1Button2longKeyPress; 
  button2.doubleClick[1] =        &menu1Button2doubleClick; 
  button2.asyncKeyPress[1] =      [](){ Serial.println("Menu 1, Button 2: Async KeyPress"); };      // Asynchronous, so fires when updated in main loop, can be Lamda
  button2.asyncLongKeyPress[1] =  [](){ Serial.println("Menu 1, Button 2: Async LongKeyPress"); };  // Asynchronous, so fires when updated in main loop, can be Lamda
  button2.asyncDoubleClick[1] =   [](){ Serial.println("Menu 1, Button 2: Async DoubleClick"); };   // Asynchronous, so fires when updated in main loop, can be Lamda

  //Clear any spurious clicks made during setup so we start fresh in the main loop.
  button1.clearInputs(); button2.clearInputs();
}

//== INTERRUPT SERVICE ROUTINES ====================================================================
//==================================================================================================
// Button1 ISR's (Top left).
void IRAM_ATTR menu0Button1keyDown(void)      { Serial.println("Menu 0, Button 1: Sync Key Down"); }      
void IRAM_ATTR menu0Button1keyUp(void)        { Serial.println("Menu 0, Button 1: Sync Key Up"); }         
void IRAM_ATTR menu0Button1keyPress(void)     { Serial.println("Menu 0, Button 1: Sync Key Press"); }                 
void IRAM_ATTR menu0Button1longKeyPress(void) { Serial.println("Menu 0, Button 1: Sync Long Key Press"); } 
void IRAM_ATTR menu0Button1doubleClick(void)  {
  Serial.print("Menu 0, Button 1: Sync Double Click - Changing to Menu Level ");
  InterruptButton::setMenuLevel(1);
  Serial.println(InterruptButton::getMenuLevel());
} 

void IRAM_ATTR menu1Button1keyDown(void)      { Serial.println("Menu 1, Button 1: Sync Key Down"); }      
void IRAM_ATTR menu1Button1keyUp(void)        { Serial.println("Menu 1, Button 1: Sync Key Up"); }         
void IRAM_ATTR menu1Button1keyPress(void)     { Serial.println("Menu 1, Button 1: Sync Key Press"); }                 
void IRAM_ATTR menu1Button1longKeyPress(void) { Serial.println("Menu 1, Button 1: Sync Long Key Press"); } 
void IRAM_ATTR menu1Button1doubleClick(void)  { 
  Serial.print("Menu 1, Button 1: Sync Double Click - Changing Back to Menu Level ");
  InterruptButton::setMenuLevel(0);
  Serial.println(InterruptButton::getMenuLevel());
} 

// Button2 ISR's (Bottom Left).
void IRAM_ATTR menu0Button2keyDown(void)      { Serial.println("Menu 0, Button 2: Sync Key Down"); }      
void IRAM_ATTR menu0Button2keyUp(void)        { Serial.println("Menu 0, Button 2: Sync Key Up"); }         
void IRAM_ATTR menu0Button2keyPress(void)     { Serial.println("Menu 0, Button 2: Sync Key Press"); }                 
void IRAM_ATTR menu0Button2longKeyPress(void) { Serial.println("Menu 0, Button 2: Sync Long Key Press"); } 
void IRAM_ATTR menu0Button2doubleClick(void)  { 
  Serial.print("Menu 0, Button 2: Sync Double Click - Changing to Menu Level ");
  InterruptButton::setMenuLevel(1);
  Serial.println(InterruptButton::getMenuLevel());
} 

void IRAM_ATTR menu1Button2keyDown(void)      { Serial.println("Menu 1, Button 2: Sync Key Down"); }      
void IRAM_ATTR menu1Button2keyUp(void)        { Serial.println("Menu 1, Button 2: Sync Key Up"); }         
void IRAM_ATTR menu1Button2keyPress(void)     { Serial.println("Menu 1, Button 2: Sync Key Press"); }                 
void IRAM_ATTR menu1Button2longKeyPress(void) { Serial.println("Menu 1, Button 2: Sync Long Key Press"); } 
void IRAM_ATTR menu1Button2doubleClick(void)  { 
  Serial.print("Menu 1, Button 2: Sync Double Click - Changing Back to Menu Level ");
  InterruptButton::setMenuLevel(0);
  Serial.println(InterruptButton::getMenuLevel());
} 

//== MAIN LOOP FUNCTION =====================================================================================
//===========================================================================================================

void loop() { 
  // ACTION ANY ASYNCHRONOUS BUTTON EVENTS NOT DONE SYNCHRONOUSLY IN ISR
  button1.processAsyncEvents(); button2.processAsyncEvents();

  delay(1000);
}
