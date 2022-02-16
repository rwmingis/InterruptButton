#ifndef InterruptButton_h
#define InterruptButton_h

#include <Arduino.h>
#include <FunctionalInterrupt.h>  // Necessary to use std::bind to bind arguments to ISR in attachInterrupt()

// -- Interrupt Button and Debouncer ---------------------------------------------------------------------------------------
// -- ----------------------------------------------------------------------------------------------------------------------
class InterruptButton {
  enum pinState_t {released, confirmingPress, pressed, waitingForRelease};

  private:
    // Static class members shared by all instances of this object (common across all instances of the class)
    static void readButton(void* arg);                                      // Static function required to allow interrupt with argument inside of class
    static void longPressDelay(void *arg);                                  // Wrapper / callback to excecute a longPress event
    static void autoRepeatPressEvent(void *arg);                            // Wrapper / callback to excecute a autoRepeatPress event
    static void doubleClickTimeout(void *arg);                              // Used to separate double-clicks from regular keyPress's
    static void setButtonChangeInterrupt(InterruptButton* btn, bool enabled, bool clearFlags = true);    // Helper function to simplify toggling the pin change interrupt
    static void startTimer(esp_timer_handle_t &timer, uint32_t duration_US, void (*callBack)(void* arg), InterruptButton* btn);

    static void killTimer(esp_timer_handle_t &timer);                       // Helper function to kill a timer
    static const uint8_t m_targetPolls = 10;                                // Desired number of polls required to debounce a button
    static const uint8_t m_maxMenus = 5;                                    // Max number of menu sets (eventually will update to dynamic memory allocation)
    static uint8_t m_menuLevel;                                             // Current menulevel for all buttons (global in class)
    static bool m_enableSyncEvents;
    static bool m_enableAsyncEvents;

    // Non-static instance specific member declarations
    pinState_t m_state;                                                     // Instance specific state machine variable
    esp_timer_handle_t m_buttonPollTimer;                                   // Instance specific timer for button debouncing
    esp_timer_handle_t m_buttonLPandRepeatTimer;                            // Instance specific timer for button longPress and autoRepeat timing
    esp_timer_handle_t m_buttonDoubleClickTimer;                            // Instance specific timer for policing double-clicks

    uint8_t m_pin, m_pinMode, m_pressedState;                               // Instance specific particulars about the hardware button
    uint8_t m_keyPressMenuLevel, m_longKeyPressMenuLevel,                   // Stores current menulevel when event occurs
            m_autoRepeatPressMenuLevel , m_doubleClickMenuLevel;            // (may have been changed by time sync event called in main loop))
    uint16_t m_pollIntervalUS, m_longKeyPressMS, m_autoRepeatMS, m_doubleClickMS; // Timing variables, in milliseconds
     
    bool m_wtgForDoubleClick = false;
    bool m_longPress_preventKeyPress;
    uint32_t m_buttonUpTimeMS;
    uint16_t m_validPolls = 0, m_totalPolls = 0;


  public:
    // Static class members shared by all instances of this object -----------------------
    static void setMenuLevel(uint8_t level);                                // Sets menu level across all buttons (ie buttons mean something different each page)
    static uint8_t getMenuLevel();                                          // Retrieves menu level

    // Non-static instance specific member declarations ----------------------------------
    InterruptButton(uint8_t pin, uint8_t pinMode, uint8_t pressedState,     // Class Constructor
                    uint16_t longPressMS = 750, uint16_t autoRepeatMS = 250,
                    uint16_t doubleClickMS = 200, uint32_t debounceUS = 8000);
    ~InterruptButton();                                                     // Class Destructor
    void begin();                                                           // Instance initialiser    
    void enableEvents(),      disableEvents();                              // Enable / Disable all events
    void enableSyncEvents(),  disableSyncEvents();                          // Enable / Disable synchronous events only
    void enableAsyncEvents(), disableAsyncEvents();                         // Enable / Disable Asynchronous events only

    // Asynchronous Event Routines ------------------                       // References to external functions for Asychronous (instantaneous) events
    void (*keyDown[m_maxMenus])(void);                                      // These should be defined with IRAM_ATTR and be as short as possible.
    void (*keyUp[m_maxMenus])(void);
    void (*keyPress[m_maxMenus])(void);
    void (*longKeyPress[m_maxMenus])(void);
    void (*autoRepeatPress[m_maxMenus])(void);
    void (*doubleClick[m_maxMenus])(void);

    // Synchronous Event Routines -----------------                         // References to external functions for Synchronous events
    void (*syncKeyPress[m_maxMenus])(void);                                 // These are run inside of main loop at button.processSyncEvents()
    void (*syncLongKeyPress[m_maxMenus])(void);
    void (*syncAutoRepeatPress[m_maxMenus])(void);
    void (*syncDoubleClick[m_maxMenus])(void);
    bool keyPressOccurred = false;
    bool longKeyPressOccurred = false;
    bool autoRepeatPressOccurred = false;
    bool doubleClickOccurred = false;
    bool inputOccurred(void);                                               // Used as a simple test if there was ANY action on button, ie if polling.
    void clearInputs(void);                                                 // Resets all Syncronous events flags
    void processSyncEvents(void);                                           // Used to action any Synchronous events recorded.
};



#endif
