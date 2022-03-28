#include "InterruptButton.h"

// Include reference req'd for debugging and warnings across serial port.
#ifdef ARDUINO
#include "esp32-hal-log.h"
#else
#include "esp_log.h"
#endif

#define ESP_INTR_FLAG_DEFAULT 0
#define EVENT_TASK_PRIORITY     2             // a bit higher than arduino's loop()
#define EVENT_TASK_STACK        4096
#define EVENT_TASK_NAME         "BTN_ACTN"

static const char* TAG = "IBTN";            // IDF log tag







/* ToDo
  Need to confirm if any ISR's need to blocked/disabled from other ISR entry, ie portMUX highlevel/lowlevel, etc.
  Confirm all necessary varibables are volatile (noting timers throw an error when defined as volatile)

IF second press of a doubleclick is held down, it won't longpress or autorepeat presss.

Consider Adding button eventTypes such as momentary, latching, etc.
Consider a static member queue common across all instances to maintain order of all Synchronous events across all buttons (and simplify processing the event actions in the main loop)
Consider an RTOS queue.
Consider adding chord combinations (2 or more buttons pressed conccurently) as a event.  Added to static class member, when any one button is pushed.
corresponding buttons are checked to see if they are in waiting for release state (will be a factorial type check to minimise redundant checks)
considered vortigont's note on thread safety:
    "Thinking forward to thread safety, it might be even better to keep those methods not as static class members
    but private classless functions (or namespace members). Passing pointers to dynamically created class instances
    always has a tiny chance to hit the dangling pointer :))) Although it's not that critical in current implementation."
*/

//-- STATIC CLASS MEMBERS AND METHODS (COMMON ACROSS ALL INSTANCES TO SAVE MEMORY) -----------------------
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------

//-- Initialise Static Member Variables ------------------------------------------------------------------
uint8_t       InterruptButton::m_numMenus                               { 0 };                  // 0 Means not initialised, can be set by user; once set it can't be changed.
uint8_t       InterruptButton::m_menuLevel                              { 0 };
modes         InterruptButton::m_mode                                   { Mode_Asynchronous };
bool          InterruptButton::m_classInitialised                       { false };
func_ptr_t    InterruptButton::syncEventQueue[SYNC_EVENT_QUEUE_DEPTH] = { nullptr };
QueueHandle_t InterruptButton::asyncEventQueue                          { nullptr };
TaskHandle_t  InterruptButton::queueServicerHandle                      { nullptr };

bool InterruptButton::setMode(modes mode){
  // Flush the Synchronous Static event queue
  for(uint8_t i = 0; i < SYNC_EVENT_QUEUE_DEPTH; i++) syncEventQueue[i] = nullptr;

  // Flush the Asynchronous RTOS queue, if it exists
  if(asyncEventQueue != nullptr) xQueueReset(asyncEventQueue);
  
  if(mode == Mode_Asynchronous || mode == Mode_Hybrid) {
    m_mode = mode;
    // create the RTOS queue
    if(asyncEventQueue == nullptr) asyncEventQueue = xQueueCreate( ASYNC_EVENT_QUEUE_DEPTH, sizeof(func_ptr_t) );
    if(asyncEventQueue == nullptr) {
      ESP_LOGE(TAG, "setMode(): Failed to create RTOS queue!");
      return false;
    }

    // Start the RTOS queue action service/task
    bool retVal;
    if(queueServicerHandle != nullptr) {
      vTaskResume(queueServicerHandle);   // Assuming it may have been paused earlier.
      retVal = true;
    } else {
      retVal = xTaskCreate(queueServicer, EVENT_TASK_NAME, EVENT_TASK_STACK, NULL, EVENT_TASK_PRIORITY, &queueServicerHandle) == pdPASS;
    }
    if(!retVal) {
      ESP_LOGE(TAG, "setMode(): Failed to create RTOS queue servicing task!");
      return false;
    }
    return true;

  } else if(mode == Mode_Synchronous) {
    m_mode = mode;
    if(queueServicerHandle != nullptr)  vTaskSuspend(queueServicerHandle);
    return true;

  } else {
    ESP_LOGE(TAG, "setMode(): Invalid mode specified!");
    return false;
  }
}

modes InterruptButton::getMode(){
  return m_mode;
}

void InterruptButton::queueServicer(void* pvParams){
  func_ptr_t theAction = nullptr;
  for (;;){
    if (xQueueReceive(asyncEventQueue, (void*)&theAction, (portTickType)portMAX_DELAY)) {  // execute callback if btntrigger_t matches the one provided via Q
      if(theAction != nullptr) theAction();
      // if we ever break, this loop is deleted.
    }
  }
  vTaskDelete(NULL);
}

void InterruptButton::processSyncEvents() {
  for(uint8_t i = 0; i < SYNC_EVENT_QUEUE_DEPTH; i++) {
    if(syncEventQueue[i] != nullptr){
      syncEventQueue[i]();
      syncEventQueue[i] = nullptr;
    } else {
      break;
    }
  }
}

//-- Method to monitor button, called by button change and various timer interrupts ----------------------
void IRAM_ATTR InterruptButton::readButton(void *arg){
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);
  BaseType_t xCntxSwitch = pdFALSE;


  switch(btn->m_state){
    case Released:                                              // Was sitting released but just detected a signal from the button
      gpio_intr_disable(btn->m_pin);                            // Ignore change inputs while we poll for a valid press
      btn->m_validPolls = 1; btn->m_totalPolls = 1;             // Was released, just detected a change, must be a valid press so count it.
      btn->m_longPress_preventKeyPress = false;
      startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn, "DB_begin_");  // Begin debouncing the button input
      btn->m_state = ConfirmingPress;
      break;

    case ConfirmingPress:                                       // we get here each time the debounce timer expires (onchange interrupt disabled remember)
      btn->m_totalPolls++;                                      // Count the number of total reads
      if(gpio_get_level(btn->m_pin) == btn->m_pressedState) btn->m_validPolls++; // Count the number of valid 'PRESSED' reads
      if(btn->m_totalPolls >= m_targetPolls){                   // If we have checked the button enough times, then make a decision on key state
        if(btn->m_validPolls * 2 <= btn->m_totalPolls) {        // Then it was a false alarm
          btn->m_state = Released;                                        
          gpio_intr_enable(btn->m_pin);
          return;
        }                                                       // Otherwise, spill over to "Pressing"
      } else {                                                  // Not yet enough polls to confirm state
        startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn, "CP2_");  // Keep sampling pin state
        return;
      }
    // Planned spill through here (no break) if logic requires, ie keyDown confirmed.
    case Pressing:                                              // VALID KEYDOWN, assumed pressed if it had valid polls more than half the time
      btn->action(Event_KeyDown, &xCntxSwitch);                 // Fire the asynchronous keyDown event
      startTimer(btn->m_buttonLPandRepeatTimer, uint64_t(btn->m_longKeyPressMS * 1000), &longPressEvent, btn, "CP1_");
      btn->m_state = Pressed;
      gpio_intr_enable(btn->m_pin);                             // Begin monitoring pin again
      break;

    case Pressed:                                               // Currently pressed until now, but there was a change on the pin
      gpio_intr_disable(btn->m_pin);                            // Turn off this interrupt to ignore inputs while we wait to check if valid release
      startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn, "PR_");  // Start timer and start polling the button to debounce it
      btn->m_validPolls = 1; btn->m_totalPolls = 1;             // This is first poll and it was just released by definition of state
      btn->m_state = WaitingForRelease;
      break;

    case WaitingForRelease: // we get here when debounce timer or doubleclick timeout timer alarms (onchange interrupt disabled remember)
                            // stay in this state until released, because button could remain locked down if release missed.
      btn->m_totalPolls++;
      if(gpio_get_level(btn->m_pin) != btn->m_pressedState){
        btn->m_validPolls++;
        if(btn->m_totalPolls < m_targetPolls || btn->m_validPolls * 2 <= btn->m_totalPolls) {           // If we haven't polled enough or not high enough success rate
          startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn, "W4R_polling_");  // Then keep sampling pin state until release is confirmed
          return;
        }                                                       // Otherwise, spill through to "Releasing"
      } else {
        if(btn->m_validPolls > 0) {
          btn->m_validPolls--;
        } else {
          btn->m_totalPolls = 0;                                // Key is being held down, don't let total polls get too far ahead.
        }
        startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn, "W4R_invalidPoll"); // Keep sampling pin state until released
      }
    // Intended spill through here (no break) to "Releasing" once keyUp confirmed.

    case Releasing:
      killTimer(btn->m_buttonLPandRepeatTimer);
      btn->action(Event_KeyUp, &xCntxSwitch);

      if(btn->eventEnabled(Event_DoubleClick) && btn->eventEnabled(Event_All) &&                        // If double-clicks are enabled and defined
         btn->eventActions[m_menuLevel][Event_DoubleClick] != nullptr) {

        if(btn->m_wtgForDblClick) {                             // VALID DOUBLE-CLICK (second keyup without a timeout, would normally check 
          killTimer(btn->m_buttonDoubleClickTimer);             // esp_timer_is_active, but function not available in esp32 arduino core.
          btn->m_wtgForDblClick = false;
          btn->action(Event_DoubleClick, &xCntxSwitch);

        } else if (!btn->m_longPress_preventKeyPress) {         // Commence double-click detection process               
          btn->m_wtgForDblClick = true;
          btn->m_doubleClickMenuLevel = m_menuLevel;            // Save menuLevel in case this is converted to a keyPress later
          startTimer(btn->m_buttonDoubleClickTimer, uint64_t(btn->m_doubleClickMS * 1000), &doubleClickTimeout, btn, "W4R_DCsetup_");
        }
      } else if(!btn->m_longPress_preventKeyPress) {            // Otherwise, treat as a basic keyPress
        btn->action(Event_KeyPress, &xCntxSwitch);              // Then treat as a normal keyPress
      } 
      btn->m_state = Released;
      gpio_intr_enable(btn->m_pin);
      break;
  } // End of SWITCH statement
  return;

  if( xCntxSwitch ) portYIELD_FROM_ISR ();  // perform a context switch if we unblocked a higher priority task when adding to the queue.
} // End of readButton function


//-- Method to handle longKeyPresses (called by timer)----------------------------------------------------
void IRAM_ATTR InterruptButton::longPressEvent(void *arg){
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);

    btn->action(Event_LongKeyPress);                                                  // Action the Async LongKeyPress Event
    btn->m_longPress_preventKeyPress = true;                                          // Used to prevent regular keypress later on in procedure.
    
    //Initiate the autorepeat function
    if(gpio_get_level(btn->m_pin) == btn->m_pressedState) {                           // Sanity check to stop autorepeats in case we somehow missed button release
      startTimer(btn->m_buttonLPandRepeatTimer, uint64_t(btn->m_autoRepeatMS * 1000), &autoRepeatPressEvent, btn, "LPD_");
    }
}

//-- Method to handle autoRepeatPresses (called by timer)-------------------------------------------------
void IRAM_ATTR InterruptButton::autoRepeatPressEvent(void *arg){
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);

  if(btn->eventActions[m_menuLevel][Event_AutoRepeatPress] != nullptr) {
    btn->action(Event_AutoRepeatPress);                                               // Action the Async Auto Repeat KeyPress Event if defined
  } else {
    btn->action(Event_KeyPress);                                                      // Action the Async KeyPress Event otherwise
  }
  if(gpio_get_level(btn->m_pin) == btn->m_pressedState) {                             // Sanity check to stop autorepeats in case we somehow missed button release
    startTimer(btn->m_buttonLPandRepeatTimer, uint64_t(btn->m_autoRepeatMS * 1000), &autoRepeatPressEvent, btn, "ARPE_");
  }
}

//-- Method to return to interpret previous keyUp as a keyPress instead of a doubleClick if it times out.
void IRAM_ATTR InterruptButton::doubleClickTimeout(void *arg){
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);
  btn->action(Event_KeyPress, btn->m_doubleClickMenuLevel);                                 // Then treat as a normal keyPress at the menuLevel when first click occurred
                                                                                      // Note, this timer is never started if previous press was a longpress
  btn->m_wtgForDblClick = false;
}

//-- Helper method to simplify starting a timer ----------------------------------------------------------
void IRAM_ATTR InterruptButton::startTimer(esp_timer_handle_t &timer, uint32_t duration_US, void (*callBack)(void* arg), InterruptButton* btn, const char *msg){
  esp_timer_create_args_t tmrConfig;
    tmrConfig.arg = reinterpret_cast<void*>(btn);
    tmrConfig.callback = callBack;
    tmrConfig.dispatch_method = ESP_TIMER_TASK;
    tmrConfig.name = msg;
  killTimer(timer);
  esp_timer_create(&tmrConfig, &timer);
  esp_timer_start_once(timer, duration_US);
}

//-- Helper method to kill a timer -----------------------------------------------------------------------
void IRAM_ATTR InterruptButton::killTimer(esp_timer_handle_t &timer){
  if(timer){
    esp_timer_stop(timer);
    esp_timer_delete(timer);
    timer = nullptr;
  }
}


//-- CLASS MEMBERS AND METHODS SPECIFIC TO A SINGLE INSTANCE (BUTTON) ------------------------------------
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------

// Class object control and setup functions -------------------------------------
// ------------------------------------------------------------------------------

// Constructor ------------------------------------------------------------------
InterruptButton::InterruptButton(uint8_t pin, uint8_t pressedState, gpio_mode_t pinMode,
                                 uint16_t longKeyPressMS, uint16_t autoRepeatMS, 
                                 uint16_t doubleClickMS,  uint32_t debounceUS) :
                                 m_pressedState(pressedState),
                                 m_pinMode(pinMode),
                                 m_longKeyPressMS(longKeyPressMS),
                                 m_autoRepeatMS(autoRepeatMS),
                                 m_doubleClickMS(doubleClickMS) {

  // gpio number sanity check
  if (GPIO_IS_VALID_GPIO(pin))
    m_pin = static_cast<gpio_num_t>(pin);
  else {
    ESP_LOGW(TAG, "%d is not valid gpio on this platform", pin);
    m_pin = static_cast<gpio_num_t>(-1);    //GPIO_NUM_NC (enum not showing up as defined);
  }

  m_pollIntervalUS = (debounceUS / m_targetPolls > 65535) ? 65535 : debounceUS / m_targetPolls;
}

// Destructor --------------------------------------------------------------------
InterruptButton::~InterruptButton() {
  gpio_isr_handler_remove(m_pin);
  killTimer(m_buttonPollTimer); killTimer(m_buttonLPandRepeatTimer); killTimer(m_buttonDoubleClickTimer);
  
  for(int menu = 0; menu < m_numMenus; menu++) delete [] eventActions[menu];
  delete [] eventActions;
  gpio_reset_pin(m_pin);
}

// Initialiser -------------------------------------------------------------------
void InterruptButton::initialiseInstance(void){
    if(m_thisButtonInitialised) return;

    if(!m_classInitialised){                    // We must be initialising the first button
      if(m_numMenus == 0) m_numMenus = 1;       // Default to a single menu level if not set prior to initialising first button
      esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
      if(err != ESP_OK) ESP_LOGD(TAG, "GPIO ISR service installed with exit status: %d", err);
      m_classInitialised = setMode(m_mode) && (err == ESP_OK || err == ESP_ERR_INVALID_STATE);
    }

    eventActions = new func_ptr_t*[m_numMenus];
    for(int menu = 0; menu < m_numMenus; menu++){
      eventActions[menu] = new func_ptr_t[NumEventTypes];
      for(int evt = 0; evt < NumEventTypes; evt++){
        eventActions[menu][evt] = nullptr;
      }
    }
    gpio_config_t gpio_conf = {};
      gpio_conf.mode = m_pinMode;
      gpio_conf.pin_bit_mask = BIT64(m_pin);
      gpio_conf.pull_down_en = (m_pressedState) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
      gpio_conf.pull_up_en =   (m_pressedState) ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE;
      gpio_conf.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&gpio_conf);                                                      // configure GPIO with the given settings

    gpio_isr_handler_add(m_pin, InterruptButton::readButton, (void*)this);
    m_state = (gpio_get_level(m_pin) == m_pressedState) ? Pressed : Released;     // Set to current state when initialising
    m_thisButtonInitialised = true;
}


//-- TIMING INTERVAL GETTERS AND SETTERS -----------------------------------------------------------------
void      InterruptButton::setLongPressInterval(uint16_t intervalMS)    { m_longKeyPressMS = intervalMS; }
uint16_t  InterruptButton::getLongPressInterval(void)                   { return m_longKeyPressMS;       }
void      InterruptButton::setAutoRepeatInterval(uint16_t intervalMS)   { m_autoRepeatMS = intervalMS;   }
uint16_t  InterruptButton::getAutoRepeatInterval(void)                  { return m_autoRepeatMS;         }
void      InterruptButton::setDoubleClickInterval(uint16_t intervalMS)  { m_doubleClickMS = intervalMS;  }
uint16_t  InterruptButton::getDoubleClickInterval(void)                 { return m_doubleClickMS;        }


// -- FUNCTIONS RELATED TO EXTERNAL ACTIONS --------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------
void InterruptButton::bind(events event, uint8_t menuLevel, func_ptr_t action){
  if(menuLevel >= m_numMenus) {
    ESP_LOGE(TAG, "Specified menu level is greater than the number of menus!");
  } else if(event >= NumEventTypes) {
    ESP_LOGE(TAG, "Specified event is invalid!");
  } else {
    if(!m_thisButtonInitialised) initialiseInstance(); 
    eventActions[menuLevel][event] = action;
    if(!eventEnabled(event)) enableEvent(event);
  }
}

void InterruptButton::unbind(events event, uint8_t menuLevel){
  if(m_numMenus == 0){
    ESP_LOGE(TAG, "You must have bound at least one function prior to unbinding it from a button!");
  } else if(menuLevel >= m_numMenus) {
    ESP_LOGE(TAG, "Specified menu level is greater than the number of menus!");
  } else if(event >= NumEventTypes) {
    ESP_LOGE(TAG, "Specified event is invalid!");
  } else {
    eventActions[menuLevel][event] = nullptr;
  }
  return;
}



void InterruptButton::action(events event, uint8_t menuLevel, BaseType_t *pxHigherPriorityTaskWoken){
  if(menuLevel >= m_numMenus)                                                 return;   // Invalid menu level
  if(!eventEnabled(event))                                                    return;   // Specific event is disabled
  if(eventActions[menuLevel][event] == nullptr)                               return;   // Event is not defined
  if(event >= Event_KeyDown && event <= Event_DoubleClick && !eventEnabled(Event_All))    return;   // This is an async event and they are disabled
  
  if(m_mode == Mode_Asynchronous || (m_mode == Mode_Hybrid && (event == Event_KeyDown || event == Event_KeyUp))) {  // Call action using RTOS
    if(pxHigherPriorityTaskWoken == nullptr){
      xQueueSendToBack(asyncEventQueue, (void*)&eventActions[menuLevel][event], (TickType_t)0);
    } else {
      xQueueSendToBackFromISR(asyncEventQueue, (void*)&eventActions[menuLevel][event], pxHigherPriorityTaskWoken);  
    }
  } else {                                                                // Otherwise call action using Static Queue
    for(uint8_t i = 0; i < ASYNC_EVENT_QUEUE_DEPTH; i++){
      if(syncEventQueue[i] == nullptr){
        syncEventQueue[i] = eventActions[menuLevel][event];
        break;
      }
    }
  }  
}

void InterruptButton::enableEvent(events event){
  if(event <= Event_All && event != NumEventTypes) eventMask |= (1UL << (event));    // Set the relevant bit
}
void InterruptButton::disableEvent(events event){
  if(event <= Event_All && event != NumEventTypes) eventMask &= ~(1UL << (event));   // Clear the relevant bit
}
bool InterruptButton::eventEnabled(events event) {
  return ((eventMask >> event) & 0x01) == 0x01;
}

void InterruptButton::setMenuCount(uint8_t numberOfMenus){           // This can only be set before initialising first button
  if(!m_classInitialised && numberOfMenus >= 1) m_numMenus = numberOfMenus;
}
uint8_t InterruptButton::getMenuCount(void) {
  return m_numMenus;
}

void InterruptButton::setMenuLevel(uint8_t level) {
  if(level < m_numMenus) {
    m_menuLevel = level;
  } else {
    ESP_LOGE(TAG, "Menu level '%d' must be >= 0 AND < number of menus (zero origin): ", level);
  }
}
uint8_t InterruptButton::getMenuLevel(){
  return m_menuLevel;
}
