#ifndef InterruptButton_h
#define InterruptButton_h

#include "driver/gpio.h"
#include "esp_timer.h"
#include <functional>  // Necessary to use std::bind to bind arguments to ISR in attachInterrupt()


// -- Interrupt Button and Debouncer ---------------------------------------------------------------------------------------
// -- ----------------------------------------------------------------------------------------------------------------------
class InterruptButton {
  private:
    enum pinState_t {
      Released, 
      ConfirmingPress, 
      Pressed, 
      WaitingForRelease
    };

    typedef void (*func_ptr)(void);


    // Static class members shared by all instances of this object (common across all instances of the class)
    // ------------------------------------------------------------------------------------------------------
    static void readButton(void* arg);                                      // Static function required to allow interrupt with argument inside of class
    static void longPressDelay(void *arg);                                  // Wrapper / callback to excecute a longPress event
    static void autoRepeatPressEvent(void *arg);                            // Wrapper / callback to excecute a autoRepeatPress event
    static void doubleClickTimeout(void *arg);                              // Used to separate double-clicks from regular keyPress's
    static void setButtonChangeInterrupt(gpio_num_t gpio, bool enabled);    // Helper function to simplify toggling the pin change interrupt
    static void startTimer(esp_timer_handle_t &timer, uint32_t duration_US, void (*callBack)(void* arg), InterruptButton* btn, const char *msg);
    static void killTimer(esp_timer_handle_t &timer);                       // Helper function to kill a timer

    static const uint8_t m_targetPolls = 10;                                // Desired number of polls required to debounce a button
    static bool m_firstButtonInitialised;                                   // Used to block any further changes to m_numMenus
    static uint8_t m_numMenus;                                              // Total number of menu sets, can be set by user, but only before initialising first button
    static uint8_t m_menuLevel;                                             // Current menulevel for all buttons (global in class)

    // Non-static instance specific member declarations
    // ------------------------------------------------
    volatile pinState_t m_state;                                            // Instance specific state machine variable
    esp_timer_handle_t m_buttonPollTimer;                                   // Instance specific timer for button debouncing
    esp_timer_handle_t m_buttonLPandRepeatTimer;                            // Instance specific timer for button longPress and autoRepeat timing
    esp_timer_handle_t m_buttonDoubleClickTimer;                            // Instance specific timer for policing double-clicks

    uint8_t m_pressedState;                                                 // Instance specific particulars about the hardware button
    gpio_num_t m_pin;                                                       // Button gpio
    gpio_mode_t m_pinMode;                                                  // GPIO mode: IDF's input/output mode
    volatile uint8_t m_keyPressMenuLevel, m_longKeyPressMenuLevel,          // Variables to store current menulevel when event occurs (the menu level ...
                     m_autoRepeatPressMenuLevel, m_doubleClickMenuLevel;    // ... may have been changed by time sync event called in main loop))
    uint16_t m_pollIntervalUS, m_longKeyPressMS, m_autoRepeatMS,            // Timing variables
             m_doubleClickMS;
     
    volatile bool m_wtgForDoubleClick = false;                              // Boolean flag to manage program flow around doubleclick timing
    volatile bool m_longPress_preventKeyPress;                              // Boolean flag to prevent firing a keypress if a long press occurred (outside of polling fuction)
    volatile uint64_t m_buttonUpTimeUS;                                     // Timing variable to track double click interval.
    volatile uint16_t m_validPolls = 0, m_totalPolls = 0;                   // Variables to conduct debouncing algoritm


  public:
    enum eventTypes {
      KeyDown = 0,          // These first 6 events are asycnchronous unless explicitly stated and actioned immediately in an ISR ...
      KeyUp,                // ... Asynchronous events should be defined with the IRAM_ATTR attributed so that ...
      KeyPress,             // ... Their code is stored in the RAM so as to not rely on SPI bus for timing to get from flash memory
      LongKeyPress, 
      AutoRepeatPress, 
      DoubleClick, 
      SyncKeyPress,         // Sycnchronous events are actioned inside of main loop at button.processSyncEvents() ...
      SyncLongKeyPress,     // ... and can be defined as Lambdas
      SyncAutoKeyPress, 
      SyncDoubleClick,
      NumEventTypes         // Not an event, but this value used to size the number of columns in event/action array.
    };

    // Static class members shared by all instances of this object -----------------------
    static void setMenuCount(uint8_t numberOfMenus);
    static uint8_t getMenuCount(void);
    static void setMenuLevel(uint8_t level);                                // Sets menu level across all buttons (ie buttons mean something different each page)
    static uint8_t getMenuLevel();                                          // Retrieves menu level

    // Non-static instance specific member declarations ----------------------------------
    InterruptButton(uint8_t pin, uint8_t pressedState, gpio_mode_t pinMode = GPIO_MODE_INPUT,      // Class Constructor
                    uint16_t longPressMS = 750, uint16_t autoRepeatMS = 250,
                    uint16_t doubleClickMS = 200, uint32_t debounceUS = 8000);
    ~InterruptButton();                                                     // Class Destructor
    void begin();                                                           // Instance initialiser    
    gpio_num_t getGPIO() const { return m_pin;}                                    // return monitored gpio number

    void enableEvents(),            disableEvents();                        // Enable / Disable sync and async events together
    bool syncEventsEnabled = true,  asyncEventsEnabled = true;
    bool longPressEnabled = false,  autoRepeatEnabled = false,  doubleClickEnabled = false;

    void      setLongPressInterval(uint16_t intervalMS);                    // Updates LongPress Interval
    uint16_t  getLongPressInterval(void);
    void      setAutoRepeatInterval(uint16_t intervalMS);                   // Updates autoRepeat Interval
    uint16_t  getAutoRepeatInterval(void);
    void      setDoubleClickInterval(uint16_t intervalMS);                  // Updates autoRepeat Interval
    uint16_t  getDoubleClickInterval(void);
   

    // Asynchronous Event Routines ------------------   // References to external functions for Asychronous (instantaneous) events
// References to external functions for Asychronous (instantaneous) events
// These should be defined with IRAM_ATTR and be as short as possible.
// Each one is a pointer to a dynamic array of function pointers
    func_ptr** eventActions = nullptr;
    void bind(  uint8_t event, func_ptr action);                              // Used to bind/unbind action to an event at current m_menuLevel
    void unbind(uint8_t event);
    void bind(  uint8_t event, uint8_t menuLevel, func_ptr action);           // Used to bind/unbind action to an event at specified menu level
    void unbind(uint8_t event, uint8_t menuLevel);

    void action(uint8_t event, bool enabler);                                 // Helper function to simplify calling actions at current m_menulevel (good for async)
    void action(uint8_t event, uint8_t menuLevel, bool enabler);              // Helper function to simplify calling actions at specified menulevel (req'd for sync)

    // Synchronous Event Routines -----------------   // References to external functions for Synchronous events
    // These are run inside of main loop at button.processSyncEvents()
    volatile bool keyPressOccurred = false;
    volatile bool longKeyPressOccurred = false;
    volatile bool autoRepeatPressOccurred = false;
    volatile bool doubleClickOccurred = false;
    bool inputOccurred(void);                         // Used as a simple test if there was ANY action on button, ie if polling.
    void clearInputs(void);                           // Resets all Syncronous events flags
    void processSyncEvents(void);                     // Used to action any Synchronous events recorded.
};

#endif
