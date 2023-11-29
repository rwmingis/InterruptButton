// New in version 2.0.1
// Moved RTOS task servicer to Core 1
// Blocked doubleclicks and key presses in the event of an autokeypress event.

#ifndef INTERRUPTBUTTON_H_
#define INTERRUPTBUTTON_H_


#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <functional>

#define ASYNC_EVENT_QUEUE_DEPTH   5     // This queue is serviced very quickly so can be short
#define SYNC_EVENT_QUEUE_DEPTH    10    // This queue is limited to mainloop frequency so actions can backup
#define TARGET_POLLS              10    // Number of times to poll a button to determine it's state


typedef std::function<void()> func_ptr_t; // Typedef to faciliate managing pointers to external action functions

enum modes {
  Mode_Asynchronous,                    // All actions performed via Asynchronous RTOS queue
  Mode_Hybrid,                          // keyUp and keyDown performed by RTOS queue, remaining actions by Static Synchronous Queue.
  Mode_Synchronous                      // All actions performed by Synchronous Queue (static class member array, FIFO).
};

enum events:uint8_t {
  Event_KeyDown = 0,
  Event_KeyUp,
  Event_KeyPress,
  Event_LongKeyPress,
  Event_AutoRepeatPress,
  Event_DoubleClick,
  NumEventTypes,                        // Not an event, but this value used to size the number of columns in event/action array.
  Event_All                             // Used to enable or disable all events
};


// -- Interrupt Button and Debouncer ---------------------------------------------------------------------------------------
// -- ----------------------------------------------------------------------------------------------------------------------
class InterruptButton {
  private:
    enum buttonStates {                 // Enumeration to assist with program flow at state machine for reading button
      Released,
      ConfirmingPress,
      Pressing,
      Pressed,
      WaitingForRelease,
      Releasing
    };

    // STATIC class members shared by all instances of this object (common across all instances of the class)
    // ------------------------------------------------------------------------------------------------------
    static void asyncQueueServicer(void* pvParams);                   // Function used as RTOS task to receive and process action from RTOS message queue.
    static void readButton(void* arg);                                // function to read button state (must be static to bind to GPIO and timer ISR)
    static void longPressEvent(void *arg);                            // Callback to excecute a longPress event, called by timer
    static void autoRepeatPressEvent(void *arg);                      // Callback to excecute a autoRepeatPress event, called by timer
    static void doubleClickTimeout(void *arg);                        // Callback used to separate double-clicks from regular keyPress's, called by timer
    static void startTimer(esp_timer_handle_t &timer,                 // Helper func to start timer.
                           uint32_t duration_US,
                           void (*callBack)(void* arg),
                           InterruptButton* btn,
                           const char *msg);
    static void killTimer(esp_timer_handle_t &timer);                 // Helper function to kill a timer

    static void action(InterruptButton  *btn,                         // Helper function to simplify calling actions at specified menulevel
                       events           event,
                       uint8_t          menuLevel);
    inline static void action(InterruptButton* btn, events event) { action(btn, event, m_menuLevel); };

    static bool           m_classInitialised;                         // Boolean flag to control class initialisation
    static bool           m_firstButtonInitialised;                   // Used to block any further changes to m_numMenus
    static TaskHandle_t   m_asyncQueueServicerHandle;                 // Pointer/handle to the RTOS task that actions the RTOS Queue messages
    static func_ptr_t     m_asyncEventQueue[];                        // Array used as the Static Synchronous Event Queue
    static func_ptr_t     m_syncEventQueue[];                         // Array used as the Static Synchronous Event Queue

    static uint8_t        m_numMenus;                                 // Total number of menu sets, can be set by user, but only before initialising first button
    static uint8_t        m_menuLevel;                                // Current menulevel for all buttons (global in class so common across all buttons)
    static modes          m_mode;
    static bool           m_deleteInProgress;                         // Precautionary blocker to prevent asyc calls of object methods while they are being deleted

    // Non-static instance specific member declarations
    // ------------------------------------------------
    void                  initialiseInstance(void);                   // Setup interrupts and event-action array
    bool                  m_thisButtonInitialised = false;            // Allows us to intialise when binding functions (ie detect if already done)
    gpio_num_t            m_pin;                                      // Button gpio
    uint8_t               m_pressedState;                             // State of button when it is pressed (LOW or HIGH)
    gpio_mode_t           m_pinMode;                                  // GPIO mode: IDF's input/output mode
    volatile buttonStates m_state;                                    // Instance specific state machine variable (intialised when intialising button)
    volatile bool         m_wtgForDblClick = false;
    esp_timer_handle_t    m_buttonPollTimer = nullptr;                // Instance specific timer for button debouncing
    esp_timer_handle_t    m_buttonLPandRepeatTimer = nullptr;         // Instance specific timer for button longPress and autoRepeat timing
    esp_timer_handle_t    m_buttonDoubleClickTimer = nullptr;         // Instance specific timer for discerning double-clicks from regular keyPresses

    volatile uint8_t      m_doubleClickMenuLevel;                     // Stores current menulevel while differentiating between regular keyPress or a double-click
    uint16_t              m_pollIntervalUS;                           // Timing variables
    uint16_t              m_longKeyPressMS;
    uint16_t              m_autoRepeatMS;
    uint16_t              m_doubleClickMS;

    volatile bool         m_blockKeyPress;                            // Boolean flag to prevent firing a keypress if a longPress or AutoRepeatPress occurred (outside of polling fuction)
    volatile uint16_t     m_validPolls = 0;                           // Variables to conduct debouncing algoritm
    volatile uint16_t     m_totalPolls = 0;

    func_ptr_t**          eventActions = nullptr;                     // Pointer to 2D array, event actions by row, menu levels by column.
    uint16_t              eventMask = 0b0000010000111;                // Default to keyUp, keyDown, and keyPress enabled, and no blanket disable
                                                                      // When binding functions, longKeyPress, autoKeyPresses, & double-clicks are automatically enabled.

  public:
    // Static class members shared by all instances of this object -----------------------
    static bool     setMode(modes mode);                              // Toggle between Synchronous (Static Queue), Hybrid, or Asynchronous modes (RTOS Queue)
    static modes    getMode(void);
    static void     processSyncEvents(void);                          // Process Sync Events, called from main looop
    static void     setMenuCount(uint8_t numberOfMenus);              // Sets number of menus/pages that each button has (can only be done before intialising first button)
    static uint8_t  getMenuCount(void);                               // Retrieves total number of menus.
    static void     setMenuLevel(uint8_t level);                      // Sets menu level across all buttons (ie buttons mean something different each page)
    static uint8_t  getMenuLevel();                                   // Retrieves menu level
    static uint32_t m_RTOSservicerStackDepth;                         // Allows the user to set the depth of RTOS servicer function (for bound functions)
                                                                      // Must be set before initialsing/binding first button or calling setMode().

    // Non-static instance specific member declarations ----------------------------------
    InterruptButton(uint8_t pin,                                      // Class Constructor, pin to monitor
                    uint8_t pressedState,                             // State of the pin when pressed (HIGH or LOW)
                    gpio_mode_t pinMode = GPIO_MODE_INPUT,
                    uint16_t longKeyPressMS = 750,
                    uint16_t autoRepeatMS =   250,
                    uint16_t doubleClickMS =  333,
                    uint32_t debounceUS =     8000);
    ~InterruptButton();                                               // Class Destructor

    void            enableEvent(events event);                        // Enable the event passed as argument (updates bitmask)
    void            disableEvent(events event);                       // Disable the event passed as argument (updates bitmask)
    bool            eventEnabled(events event);                       // Read bitmask and determine if event is enabled
    void            setLongPressInterval(uint16_t intervalMS);        // Updates LongPress Interval
    uint16_t        getLongPressInterval(void);
    void            setAutoRepeatInterval(uint16_t intervalMS);       // Updates autoRepeat Interval
    uint16_t        getAutoRepeatInterval(void);
    void            setDoubleClickInterval(uint16_t intervalMS);      // Updates autoRepeat Interval
    uint16_t        getDoubleClickInterval(void);


    // Routines to manage interface with external action functions associated with each event ---
    void            bind(events     event,                                  // Used to bind an action to an event at a given menulevel
                         uint8_t    menuLevel,
                         func_ptr_t action);
    inline void     bind(events event, func_ptr_t action) { bind(event, m_menuLevel, action); } // Above function defaulting to current menulevel

    void            unbind(events   event,                                  // Used to unbind an action to an event at a given menulevel
                           uint8_t  menuLevel);
    inline void     unbind(events event) { unbind(event, m_menuLevel); };   // Above function defaulting to current menulevel
};

#endif // INTERRUPTBUTTON_H_
