// New in version 2.0.0
// Implemented RTOS queue and deleted sync events
// Added a STATIC class Synchronous Queue.
// Added mode selection to change how events are actioned (but no change in detections)
// All external functions are executed outside of ISR and in RTOS scope, so no need for IRAM


#ifndef InterruptButton_h
#define InterruptButton_h

#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define ASYNC_EVENT_QUEUE_DEPTH 5
#define SYNC_EVENT_QUEUE_DEPTH (ASYNC_EVENT_QUEUE_DEPTH * 2)

typedef void (*func_ptr_t)(void);    // Type def to faciliate managine pointers to external action functions

enum modes {
  Mode_Synchronous,
  Mode_Asynchronous
};

enum events:uint8_t {
  Event_KeyDown = 0,
  Event_KeyUp,
  Event_KeyPress,
  Event_LongKeyPress,
  Event_AutoRepeatPress,
  Event_DoubleClick,
  NumEventTypes,         // Not an event, but this value used to size the number of columns in event/action array.
  Event_All
};


// -- Interrupt Button and Debouncer ---------------------------------------------------------------------------------------
// -- ----------------------------------------------------------------------------------------------------------------------
class InterruptButton {
  private:
    enum pinStates {                // Enumeration to assist with program flow at state machine for reading button
      Released, 
      ConfirmingPress,
      Pressing,
      Pressed, 
      WaitingForRelease,
      Releasing
    };

    // Static class members shared by all instances of this object (common across all instances of the class)
    // ------------------------------------------------------------------------------------------------------
    static bool initialiseClass(void);                                      // One time intialiser for entire Class, not each instance (called during first bind)
    static void readButton(void* arg);                                      // Static function to read button state (must be static to bind to GPIO and timer ISR)
    static void longPressEvent(void *arg);                                  // Wrapper / callback to excecute a longPress event
    static void autoRepeatPressEvent(void *arg);                            // Wrapper / callback to excecute a autoRepeatPress event
    static void doubleClickTimeout(void *arg);                              // Used to separate double-clicks from regular keyPress's
    static void startTimer(esp_timer_handle_t &timer, uint32_t duration_US, void (*callBack)(void* arg), InterruptButton* btn, const char *msg);
    static void killTimer(esp_timer_handle_t &timer);                       // Helper function to kill a timer


    static const uint8_t m_targetPolls = 10;                                // Desired number of polls required to debounce a button
    static bool m_classInitialised;
    static func_ptr_t syncEventQueue[];
    static bool m_firstButtonInitialised;                                   // Used to block any further changes to m_numMenus
    static uint8_t m_numMenus;                                              // Total number of menu sets, can be set by user, but only before initialising first button
    static uint8_t m_menuLevel;                                             // Current menulevel for all buttons (global in class so common across all buttons)
    static modes m_mode;

    // Non-static instance specific member declarations
    // ------------------------------------------------
    void initialiseInstance(void);                                          // Setup interrupts and event-action array
    bool m_thisButtonInitialised = false;                                   // Allows us to intialise when binding functions (ie detect if already done)
    volatile pinStates m_state;                                            // Instance specific state machine variable (intialised when intialising button)
    volatile bool m_wtgForDblClick = false;
    esp_timer_handle_t m_buttonPollTimer;                                   // Instance specific timer for button debouncing
    esp_timer_handle_t m_buttonLPandRepeatTimer;                            // Instance specific timer for button longPress and autoRepeat timing
    esp_timer_handle_t m_buttonDoubleClickTimer;                            // Instance specific timer for policing double-clicks

    uint8_t m_pressedState;                                                 // State of button when it is pressed (LOW or HIGH)
    gpio_num_t m_pin;                                                       // Button gpio
    gpio_mode_t m_pinMode;                                                  // GPIO mode: IDF's input/output mode

    volatile uint8_t m_doubleClickMenuLevel;                                // Stores current menulevel when possible double-click occurs ...
                                                                            // ... may have been changed by time we convert back to a regular keyPress if not double-click
    uint16_t m_pollIntervalUS, m_longKeyPressMS, m_autoRepeatMS,            // Timing variables
             m_doubleClickMS;
     
    volatile bool m_longPress_preventKeyPress;                              // Boolean flag to prevent firing a keypress if a long press occurred (outside of polling fuction)
    volatile uint16_t m_validPolls = 0, m_totalPolls = 0;                   // Variables to conduct debouncing algoritm

    func_ptr_t** eventActions = nullptr;                                    // Pointer to 2D array, event actions by row, menu levels by column.
    uint16_t eventMask = 0b0000010000111;   // Default to keyUp, keyDown, and keyPress enabled, and no blanked disable
                                            // When binding functions, longKeyPress, autoKeyPresses, and double clicks are automatically enabled.

  public:

    // Static class members shared by all instances of this object -----------------------
    static bool setMode(modes mode);                                  // Toggle between Synchronous (Static Queue) or Asynchronous modes (RTOS Queue)
    static void processSyncEvents(void);                              // Process Sync Events, called from main looop
    static void    setMenuCount(uint8_t numberOfMenus);               // Sets number of menus/pages that each button has (can only be done before intialising first button)
    static uint8_t getMenuCount(void);                                // Retrieves total number of menus.
    static void    setMenuLevel(uint8_t level);                       // Sets menu level across all buttons (ie buttons mean something different each page)
    static uint8_t getMenuLevel();                                    // Retrieves menu level


    // Non-static instance specific member declarations ----------------------------------
    InterruptButton(uint8_t pin, uint8_t pressedState, gpio_mode_t pinMode = GPIO_MODE_INPUT,     // Class Constructor
                    uint16_t longKeyPressMS = 750, uint16_t autoRepeatMS = 250,
                    uint16_t doubleClickMS = 333, uint32_t debounceUS = 8000);
    ~InterruptButton();                                               // Class Destructor

    void enableEvent(events event);
    void disableEvent(events event);
    bool eventEnabled(events event);

    void      setLongPressInterval(uint16_t intervalMS);              // Updates LongPress Interval
    uint16_t  getLongPressInterval(void);
    void      setAutoRepeatInterval(uint16_t intervalMS);             // Updates autoRepeat Interval
    uint16_t  getAutoRepeatInterval(void);
    void      setDoubleClickInterval(uint16_t intervalMS);            // Updates autoRepeat Interval
    uint16_t  getDoubleClickInterval(void);


    // Routines to manage interface with external action functions associated with each event ---
    void bind(events event, uint8_t menuLevel, func_ptr_t action);                                 // Used to bind/unbind action to an event at current m_menuLevel
    inline void bind(  events event, func_ptr_t action) { bind(event, m_menuLevel, action); }

    void unbind(events event, uint8_t menuLevel);
    inline void unbind(events event) { unbind(event, m_menuLevel); };

    void action(events event, uint8_t menuLevel, BaseType_t *pxHigherPriorityTaskWoken = nullptr); // Helper function to simplify calling actions at specified menulevel (req'd for sync)
    inline void action(events event, BaseType_t *pxHigherPriorityTaskWoken = nullptr) { action(event, m_menuLevel, pxHigherPriorityTaskWoken); };
};

#endif
