#ifndef InterruptButton_h
#define InterruptButton_h

#include "driver/gpio.h"
#include "esp_timer.h"
#include <functional>     // Necessary to use std::bind to bind arguments to ISR in attachInterrupt()

// -- Interrupt Button and Debouncer ---------------------------------------------------------------------------------------
// -- ----------------------------------------------------------------------------------------------------------------------
class InterruptButton {
  private:
    enum pinState_t {                     // Enumeration to assist with program flow at state machine for reading button
      Released, 
      ConfirmingPress,
      Pressing,
      Pressed, 
      WaitingForRelease,
      Releasing
    };

    typedef void (*func_ptr)(void);       // Type def to faciliate managine pointers to external action functions

    // Static class members shared by all instances of this object (common across all instances of the class)
    // ------------------------------------------------------------------------------------------------------
    static void readButton(void* arg);                                      // Static function to read button state (must be static to bind to GPIO and timer ISR)
    static void longPressEvent(void *arg);                                  // Wrapper / callback to excecute a longPress event
    static void autoRepeatPressEvent(void *arg);                            // Wrapper / callback to excecute a autoRepeatPress event
    static void doubleClickTimeout(void *arg);                              // Used to separate double-clicks from regular keyPress's
    static void startTimer(esp_timer_handle_t &timer, uint32_t duration_US, void (*callBack)(void* arg), InterruptButton* btn, const char *msg);
    static void killTimer(esp_timer_handle_t &timer);                       // Helper function to kill a timer

    static const uint8_t m_targetPolls = 10;                                // Desired number of polls required to debounce a button
    static bool m_firstButtonInitialised;                                   // Used to block any further changes to m_numMenus
    static uint8_t m_numMenus;                                              // Total number of menu sets, can be set by user, but only before initialising first button
    static uint8_t m_menuLevel;                                             // Current menulevel for all buttons (global in class so common across all buttons)

    // Non-static instance specific member declarations
    // ------------------------------------------------
    void initialise(void);                                                  // Setup interrupts and event-action array
    bool m_thisButtonInitialised = false;                                   // Allows us to intialise when binding functions (ie detect if already done)
    volatile pinState_t m_state;                                            // Instance specific state machine variable (intialised when intialising button)
    volatile bool m_wtgForDblClick = false;
    esp_timer_handle_t m_buttonPollTimer;                                   // Instance specific timer for button debouncing
    esp_timer_handle_t m_buttonLPandRepeatTimer;                            // Instance specific timer for button longPress and autoRepeat timing
    esp_timer_handle_t m_buttonDoubleClickTimer;                            // Instance specific timer for policing double-clicks

    uint8_t m_pressedState;                                                 // State of button when it is pressed (LOW or HIGH)
    gpio_num_t m_pin;                                                       // Button gpio
    gpio_mode_t m_pinMode;                                                  // GPIO mode: IDF's input/output mode

    volatile uint8_t m_keyPressMenuLevel, m_longKeyPressMenuLevel,          // Variables to store current menulevel when event occurs (the menu level ...
                     m_autoRepeatPressMenuLevel, m_doubleClickMenuLevel;    // ... may have been changed by time sync event called in main loop))
    uint16_t m_pollIntervalUS, m_longKeyPressMS, m_autoRepeatMS,            // Timing variables
             m_doubleClickMS;
     
    volatile bool m_longPress_preventKeyPress;                              // Boolean flag to prevent firing a keypress if a long press occurred (outside of polling fuction)
    volatile uint16_t m_validPolls = 0, m_totalPolls = 0;                   // Variables to conduct debouncing algoritm

    func_ptr** eventActions = nullptr;                                      // Pointer to 2D array, event actions by row, menu levels by column.
    uint16_t eventMask = 0b1100001000111;   // Default to keyUp, keyDown, and keyPress enabled (sync an async)
                                            // When binding functions, longKeyPress, autoKeyPresses, and double clicks are automatically enabled.


  public:
    enum event_t {
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
      NumEventTypes,         // Not an event, but this value used to size the number of columns in event/action array.
      AsyncEvents,
      SyncEvents
    };

    // Static class members shared by all instances of this object -----------------------
    static void    setMenuCount(uint8_t numberOfMenus);               // Sets number of menus/pages that each button has (can only be done before intialising first button)
    static uint8_t getMenuCount(void);                                // Retrieves total number of menus.
    static void    setMenuLevel(uint8_t level);                       // Sets menu level across all buttons (ie buttons mean something different each page)
    static uint8_t getMenuLevel();                                    // Retrieves menu level


    // Non-static instance specific member declarations ----------------------------------
    InterruptButton(uint8_t pin, uint8_t pressedState, gpio_mode_t pinMode = GPIO_MODE_INPUT,     // Class Constructor
                    uint16_t longKeyPressMS = 750, uint16_t autoRepeatMS = 250,
                    uint16_t doubleClickMS = 200, uint32_t debounceUS = 8000);
    ~InterruptButton();                                               // Class Destructor
    void enableEvent(event_t event);
    void disableEvent(event_t event);
    bool eventEnabled(event_t event);

    void      setLongPressInterval(uint16_t intervalMS);              // Updates LongPress Interval
    uint16_t  getLongPressInterval(void);
    void      setAutoRepeatInterval(uint16_t intervalMS);             // Updates autoRepeat Interval
    uint16_t  getAutoRepeatInterval(void);
    void      setDoubleClickInterval(uint16_t intervalMS);            // Updates autoRepeat Interval
    uint16_t  getDoubleClickInterval(void);


    // Routines to manage interface with external action functions associated with each event ---
    //   Any functions bound to Asynchronous (ISR driven) events should be defined with IRAM_ATTR attribute and be as brief as possible
    //   Any functions bound to Synchronous events (Actioned by loop call of "button.processSyncEvents()") may be longer and also may be defined as Lambda functions

    void bind(  event_t event, func_ptr action);                      // Used to bind/unbind action to an event at current m_menuLevel
    void unbind(event_t event);
    void bind(  event_t event, uint8_t menuLevel, func_ptr action);   // Used to bind/unbind action to an event at specified menu level
    void unbind(event_t event, uint8_t menuLevel);
    void action(event_t event);                         // Helper function to simplify calling actions at current m_menulevel (good for async)
    void action(event_t event, uint8_t menuLevel);      // Helper function to simplify calling actions at specified menulevel (req'd for sync)

    // For processing Synchronous events.
    volatile bool keyPressOccurred = false;
    volatile bool longKeyPressOccurred = false;
    volatile bool autoRepeatPressOccurred = false;
    volatile bool doubleClickOccurred = false;
    bool inputOccurred(void);                                         // Used as a simple test if there was ANY action on button, ie if polling.
    void clearSyncInputs(void);                                           // Resets all Syncronous events flags
    void processSyncEvents(void);                                     // Used to action any Synchronous events recorded.
};

#endif
