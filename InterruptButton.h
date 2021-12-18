#ifndef InterruptButton_h
#define InterruptButton_h

#include <Arduino.h>
#include <FunctionalInterrupt.h>  // Necessary to use std::bind to bind arguments to ISR in attachInterrupt()


//#include <FunctionalInterrupt.h>  // Necessary to use std::bind to bind arguments to ISR in attachInterrupt()

// -- Interrupt Button and Debouncer ---------------------------------------------------------------------------------------
// -- ----------------------------------------------------------------------------------------------------------------------
class InterruptButton {
  enum pinState_t {released, confirmingPress, pressed, confirmingRelease, wtgForDoubleClick};

  private:
    // Static class members shared by all instances of this object (common across all instances of the class)
    static const uint8_t m_targetPolls = 10;
    static const uint8_t m_maxMenus = 5;
    static uint8_t m_menuLevel;
    static bool m_enableSyncEvents;
    static bool m_enableAsyncEvents;

    // Non-static instance specific member declarations
    pinState_t m_state;                                                     // Main state machine variable
    esp_timer_handle_t m_buttonTimer;                                       // Instance specific timer
    uint8_t m_pin, m_pinMode, m_pressedState;                               // Particulars about the hardware button
    uint8_t m_keyPressMenuLevel, m_longKeyPressMenuLevel, m_doubleClickMenuLevel;  // Used to store menulevel when event occurs for Async events to later use.
    uint16_t m_pollIntervalUS;                                              // Total debounce time / m_targetPolls
    uint32_t m_longKeyPressMS, m_doubleClickMS, m_debounceIntervalUS;       // Timing Variables
 
    void setButtonChangeInterrupt(bool enabled, bool clearFlags = true);    // Helper function to simplify toggling the pin change interrupt
    void startTimer(uint32_t interval);                                     // Helper function to start the timer interrupt
    void killTimer();                                                       // And to destroy it.

  public:
    // Static class members shared by all instances of this object -----------------------
    static void readButton_static(void* arg);                               // Static function required to allow interrupt with argument inside of class
    static void setMenuLevel(uint8_t level);                                // Sets menu level across all buttons (ie buttons mean something different each page)
    static uint8_t getMenuLevel();                                          // Retrieves menu level

    // Non-static instance specific member declarations ----------------------------------
    InterruptButton(uint8_t pin, uint8_t pinMode, uint8_t pressedState,     // Class Constructor
                    uint16_t longPressMS = 750, uint16_t doubleClickMS = 100, uint32_t debounceUS = 8000);
    ~InterruptButton();                                                     // Class Destructor
    void begin();                                                           // Instance initialiser
    void readButton(void);                                                  // ISR that does all the work, unique per instance.

    // Synchronous Event Routines ------------------                        // References to extnernal functions for sychronous (instantaneous) events
    void (*keyDown[m_maxMenus])(void);                                               // These should be defined with IRAM_ATTR and be as short as possible.
    void (*keyUp[m_maxMenus])(void);
    void (*keyPress[m_maxMenus])(void);
    void (*longKeyPress[m_maxMenus])(void);
    void (*doubleClick[m_maxMenus])(void);

    // Asynchronous Event Routines -----------------                        // References to extnernal functions for asychronous events
    void (*asyncKeyPress[m_maxMenus])(void);                                         // These are run inside of main loop at button.processAsyncEvents()
    void (*asyncLongKeyPress[m_maxMenus])(void);
    void (*asyncDoubleClick[m_maxMenus])(void);
    bool keyPressOccurred = false;
    bool longKeyPressOccurred = false;
    bool doubleClickOccurred = false;
    bool inputOccurred(void);                                               // Used as a simple test if there was ANY action on button, ie if polling.
    void clearInputs(void);                                                 // Resets all asyncronous events flags
    void processAsyncEvents(void);                                          // Used to action any events asynchronous events recorded.
};



#endif