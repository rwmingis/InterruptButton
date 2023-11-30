#include "InterruptButton.h"

// Include reference req'd for debugging and warnings across serial port.
#ifdef ARDUINO
#include "esp32-hal-log.h"
#else
#include "esp_log.h"
#endif


#define ESP_INTR_FLAG_DEFAULT   0
#define EVENT_TASK_PRIORITY     2             // One level higher than arduino's loop() which is priority level 1
#define EVENT_TASK_STACK        4096          // Stack size associated with the queue servicer 
#define EVENT_TASK_NAME         "BTN_ACTN"
#define EVENT_TASK_CORE         1             // Same core as setup() and loop()

static const char* TAG = "IBTN";              // IDF log tag



/* ToDo
  1. Need to confirm if any ISR's need to blocked/disabled from other ISR entry, ie portMUX highlevel/lowlevel, etc.
  2. Consider Adding button eventTypes such as momentary, latching, etc.
  3. Consider adding chord combinations (2 or more buttons pressed concurently) as a event.  Added to static class member, when any one button is pushed.
  corresponding buttons are checked to see if they are in waiting for release state (will be a factorial type check to minimise redundant checks)

  4. Consider allowing a single button to follow it's own menu level and depart from the global menulevel.
  This would be usefull when you have a powerbutton that doesn't change function and menu buttons that do
  change their function based on what the current gui menu level is (like a soft key)
*/

//-- STATIC CLASS MEMBERS AND METHODS (COMMON ACROSS ALL INSTANCES TO SAVE MEMORY) -----------------------
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------

//-- Initialise Static Member Variables ------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
uint32_t      InterruptButton::m_RTOSservicerStackDepth                     { 2048 };
uint8_t       InterruptButton::m_numMenus                                   { 0 };  // 0 Means not initialised, can be set by user; once set it can't be changed.
uint8_t       InterruptButton::m_menuLevel                                  { 0 };
modes         InterruptButton::m_mode                                       { Mode_Asynchronous };
bool          InterruptButton::m_classInitialised                           { false };
func_ptr_t    InterruptButton::m_asyncEventQueue[ASYNC_EVENT_QUEUE_DEPTH] = { nullptr };
func_ptr_t    InterruptButton::m_syncEventQueue[SYNC_EVENT_QUEUE_DEPTH] =   { nullptr };
TaskHandle_t  InterruptButton::m_asyncQueueServicerHandle                   { nullptr };
bool          InterruptButton::m_deleteInProgress                           { false };

// This is used to initialise the queue(s) and also switch between them.
bool InterruptButton::setMode(modes mode){
  // Flush both queues
  for(uint8_t i = 0; i < ASYNC_EVENT_QUEUE_DEPTH; i++) m_asyncEventQueue[i] = nullptr;
  for(uint8_t i = 0; i < SYNC_EVENT_QUEUE_DEPTH;  i++) m_syncEventQueue[i]  = nullptr;
  
  if(mode == Mode_Asynchronous || mode == Mode_Hybrid) {
    m_mode = mode;

    // Start the RTOS queue action service/task
    bool retVal;
    if(m_asyncQueueServicerHandle != nullptr) {
      vTaskResume(m_asyncQueueServicerHandle);   // Assuming it may have been paused earlier.
      retVal = true;
    } else {
      retVal = xTaskCreatePinnedToCore(asyncQueueServicer, EVENT_TASK_NAME, m_RTOSservicerStackDepth, NULL, 
                                       EVENT_TASK_PRIORITY, &m_asyncQueueServicerHandle, EVENT_TASK_CORE) == pdPASS;
    }
    if(!retVal) ESP_LOGE(TAG, "setMode(): Failed to create RTOS queue servicing task!");
    return retVal;

  } else if(mode == Mode_Synchronous) {
    m_mode = mode;
    if(m_asyncQueueServicerHandle != nullptr)  vTaskSuspend(m_asyncQueueServicerHandle);
    return true;

  } else {
    ESP_LOGE(TAG, "setMode(): Invalid mode specified!");
    return false;
  }
}

modes InterruptButton::getMode(){
  return m_mode;
}

void InterruptButton::asyncQueueServicer(void* pvParams){
  while(1){
    if(m_asyncEventQueue[0] != nullptr) {
      m_asyncEventQueue[0]();                                   // Action the first entry

      for(uint8_t i = 1; i < ASYNC_EVENT_QUEUE_DEPTH; i++) {    // Shift the rest down or clear this entry.
        m_asyncEventQueue[i - 1] = m_asyncEventQueue[i];
        if(m_asyncEventQueue[i] == nullptr) break;
        if(i == ASYNC_EVENT_QUEUE_DEPTH - 1) m_asyncEventQueue[i] = nullptr;
      }
    }
    vTaskDelay(10 / ((TickType_t)1000 / configTICK_RATE_HZ));    // Required to yield to RTOS scheduler during idle times
  }
  vTaskDelete(NULL);    // Only reached if we put a condition in the primary while loop based on mode
}

void InterruptButton::processSyncEvents() {
  while(m_syncEventQueue[0] != nullptr) {
    m_syncEventQueue[0]();                                   // Action the first entry

    for(uint8_t i = 1; i < SYNC_EVENT_QUEUE_DEPTH; i++) {    // Shift the rest down or clear this entry.
        m_syncEventQueue[i - 1] = m_syncEventQueue[i];
        if(m_syncEventQueue[i] == nullptr) break;
        if(i == SYNC_EVENT_QUEUE_DEPTH - 1) m_syncEventQueue[i] = nullptr;
    }
  }
}


//-- Method to monitor button, called by button change and various timer interrupts ----------------------
void IRAM_ATTR InterruptButton::readButton(void *arg){
  if(m_deleteInProgress) return;
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);

  switch(btn->m_state){
    case Released:                                              // Was sitting released but just detected a signal from the button
      gpio_intr_disable(btn->m_pin);                            // Ignore change inputs while we poll for a valid press
      btn->m_validPolls = 1; btn->m_totalPolls = 1;             // Was released, just detected a change, must be a valid press so count it.
      btn->m_blockKeyPress = false;
      startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn, "DB_begin_");  // Begin debouncing the button input
      btn->m_state = ConfirmingPress;

      break;

    case ConfirmingPress:                                       // we get here each time the debounce timer expires (onchange interrupt disabled remember)
      btn->m_totalPolls++;                                      // Count the number of total reads
      if(gpio_get_level(btn->m_pin) == btn->m_pressedState) btn->m_validPolls++; // Count the number of valid 'PRESSED' reads
      if(btn->m_totalPolls >= TARGET_POLLS){                   // If we have checked the button enough times, then make a decision on key state
        if(btn->m_validPolls * 2 <= btn->m_totalPolls) {        // Then it was a false alarm
          btn->m_state = Released;                                        
          gpio_intr_enable(btn->m_pin);
          return;
        }                                                       // Otherwise, spill over to "Pressing"
      } else {                                                  // Not yet enough polls to confirm state
        startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn, "CP2_");  // Keep sampling pin state
        return;
      }
      [[fallthrough]];                                           // Planned spill through here (no break) if logic requires, ie keyDown confirmed.
    case Pressing:                                              // VALID KEYDOWN, assumed pressed if it had valid polls more than half the time
      btn->action(btn, Event_KeyDown);                          // Add the keyDown action to the relevant queue
      if(btn->eventEnabled(Event_LongKeyPress) && btn->eventActions[m_menuLevel][Event_LongKeyPress] != nullptr){
        startTimer(btn->m_buttonLPandRepeatTimer, uint64_t(btn->m_longKeyPressMS * 1000), &longPressEvent, btn, "CP1_");
      } else if (btn->eventEnabled(Event_AutoRepeatPress)) {
        startTimer(btn->m_buttonLPandRepeatTimer, uint64_t(btn->m_autoRepeatMS * 1000), &autoRepeatPressEvent, btn, "CP1_");
      }

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
        if(btn->m_totalPolls < TARGET_POLLS || btn->m_validPolls * 2 <= btn->m_totalPolls) {           // If we haven't polled enough or not high enough success rate
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
      [[fallthrough]];                                           // Intended spill through here (no break) to "Releasing" once keyUp confirmed.

    case Releasing:
      killTimer(btn->m_buttonLPandRepeatTimer);
      btn->action(btn, Event_KeyUp);                            // Add the keyUp action to the relevant queue

      if(btn->eventEnabled(Event_DoubleClick) && btn->eventEnabled(Event_All) &&     // If double-clicks are enabled and defined
         btn->eventActions[m_menuLevel][Event_DoubleClick] != nullptr) {

        if(btn->m_wtgForDblClick) {                             // VALID DOUBLE-CLICK (second keyup without a timeout, would normally check 
          killTimer(btn->m_buttonDoubleClickTimer);             // esp_timer_is_active, but function not available in esp32 arduino core.
          btn->m_wtgForDblClick = false;
          btn->action(btn, Event_DoubleClick);                  // Add the double-click action to the relevant queue

        } else if (!btn->m_blockKeyPress) {                     // Commence double-click detection process               
          btn->m_wtgForDblClick = true;
          btn->m_doubleClickMenuLevel = m_menuLevel;            // Save menuLevel in case this is converted to a keyPress later
          startTimer(btn->m_buttonDoubleClickTimer, uint64_t(btn->m_doubleClickMS * 1000), &doubleClickTimeout, btn, "W4R_DCsetup_");
        }
      } else if(!btn->m_blockKeyPress) {                        // Otherwise, treat as a basic keyPress
        btn->action(btn, Event_KeyPress);                       // Then treat as a normal keyPress
      } 
      btn->m_state = Released;
      gpio_intr_enable(btn->m_pin);
      break;
  } // End of SWITCH statement

  return;
} // End of readButton function


//-- Method to handle longKeyPresses (called by timer)----------------------------------------------------
void InterruptButton::longPressEvent(void *arg){
  if(m_deleteInProgress) return;
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);

  btn->action(btn, Event_LongKeyPress);                                     // Add the long keypress action to the relevant queue
  btn->m_blockKeyPress = true;                                              // Used to prevent regular keypress or doubleclick later on in procedure.
  
  //Initiate the autorepeat function
  if(btn->eventEnabled(Event_AutoRepeatPress) && gpio_get_level(btn->m_pin) == btn->m_pressedState) { // Sanity check to stop autorepeats in case we somehow missed button release
    startTimer(btn->m_buttonLPandRepeatTimer, uint64_t(btn->m_autoRepeatMS * 1000), &autoRepeatPressEvent, btn, "LPD_");
  }
}

//-- Method to handle autoRepeatPresses (called by timer)-------------------------------------------------
void InterruptButton::autoRepeatPressEvent(void *arg){
  if(m_deleteInProgress) return;
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);
  btn->m_blockKeyPress = true;                                              // Used to prevent regular keypress or doubleclick later on in procedure.

  if(btn->eventActions[m_menuLevel][Event_AutoRepeatPress] != nullptr) {
    btn->action(btn, Event_AutoRepeatPress);                                // Action the Async Auto Repeat KeyPress Event if defined
  } else {
    btn->action(btn, Event_KeyPress);                                       // Action the Async KeyPress Event otherwise
  }
  if(btn->eventEnabled(Event_AutoRepeatPress) && gpio_get_level(btn->m_pin) == btn->m_pressedState) { // Sanity check to stop autorepeats in case we somehow missed button release
    startTimer(btn->m_buttonLPandRepeatTimer, uint64_t(btn->m_autoRepeatMS * 1000), &autoRepeatPressEvent, btn, "LPD_");
  }
}

//-- Method to return to interpret previous keyUp as a keyPress instead of a doubleClick if it times out.
void InterruptButton::doubleClickTimeout(void *arg){
  if(m_deleteInProgress) return;
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);
  btn->m_wtgForDblClick = false;
  if(gpio_get_level(btn->m_pin) != btn->m_pressedState)
    btn->action(btn, Event_KeyPress, btn->m_doubleClickMenuLevel);                    // Then treat as a normal keyPress at the menuLevel when first click occurred
                                                                                      // Note, this timer is never started if previous press was a longpress
}

//-- Helper method to simplify starting a timer ----------------------------------------------------------
void IRAM_ATTR InterruptButton::startTimer(esp_timer_handle_t &timer, uint32_t duration_US, void (*callBack)(void* arg), InterruptButton* btn, const char *msg){
  if(m_deleteInProgress) return;
  esp_timer_create_args_t tmrConfig;
    tmrConfig.arg = reinterpret_cast<void*>(btn);
    tmrConfig.callback = callBack;
    tmrConfig.dispatch_method = ESP_TIMER_TASK;
    tmrConfig.name = msg;
    // this line crashes the esp if button was created dynamically.
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

void IRAM_ATTR InterruptButton::action(InterruptButton* btn, events event, uint8_t menuLevel){  
  if(m_deleteInProgress)                                                      return;
  if(menuLevel >= m_numMenus)                                                 return;   // Invalid menu level
  if(!btn->eventEnabled(event) || !btn->eventEnabled(Event_All))              return;   // Specific event is or all events are disabled
  if(btn->eventActions[menuLevel][event] == nullptr)                          return;   // Event is not defined

  if(m_mode == Mode_Asynchronous || (m_mode == Mode_Hybrid && (event == Event_KeyDown || event == Event_KeyUp))) {
    for(uint8_t i = 0; i < ASYNC_EVENT_QUEUE_DEPTH; i++){            // Action immediatley using RTOS asynchronous Queue
      //ESP_LOGD(TAG,"Searching for free spot to add action: %d", i);
      if(m_asyncEventQueue[i] == nullptr){
        //ESP_LOGD(TAG,"\tAdding entry at position: %d", i);
        m_asyncEventQueue[i] = btn->eventActions[menuLevel][event];
        break;
      }
    }
  } else {                                                           // Action when called in main loop hook using synchronous Queue
    for(uint8_t i = 0; i < SYNC_EVENT_QUEUE_DEPTH; i++){
      if(m_syncEventQueue[i] == nullptr){
        m_syncEventQueue[i] = btn->eventActions[menuLevel][event];
        break;
      }
    }
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

  if (GPIO_IS_VALID_GPIO(pin))                  // Check for a valid pin first
    m_pin = static_cast<gpio_num_t>(pin);
  else {
    ESP_LOGW(TAG, "%d is not valid gpio on this platform", pin);
    m_pin = static_cast<gpio_num_t>(-1);        //GPIO_NUM_NC (enum not showing up as defined);
  }
  m_pollIntervalUS = (debounceUS / TARGET_POLLS > 65535) ? 65535 : debounceUS / TARGET_POLLS;
}

// Destructor --------------------------------------------------------------------
InterruptButton::~InterruptButton() {
  m_deleteInProgress = true;
  gpio_isr_handler_remove(m_pin);
  killTimer(m_buttonPollTimer); killTimer(m_buttonLPandRepeatTimer); killTimer(m_buttonDoubleClickTimer);
  gpio_reset_pin(m_pin);

  for(int menu = 0; menu < m_numMenus; menu++) delete [] eventActions[menu];
  delete [] eventActions;
  m_deleteInProgress = false;
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

    eventActions = new func_ptr_t*[m_numMenus];             // Define the array of actions associated with each button
    for(int menu = 0; menu < m_numMenus; menu++){
      eventActions[menu] = new func_ptr_t[NumEventTypes];
      for(int evt = 0; evt < NumEventTypes; evt++){
        eventActions[menu][evt] = nullptr;
      }
    }
    gpio_config_t gpio_conf = {};                           // Configure the interrupt associated with the pin
      gpio_conf.mode = m_pinMode;
      gpio_conf.pin_bit_mask = BIT64(static_cast<uint8_t>(m_pin));
      gpio_conf.pull_down_en = (m_pressedState) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
      gpio_conf.pull_up_en =   (m_pressedState) ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE;
      gpio_conf.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&gpio_conf);
    gpio_isr_handler_add(m_pin, InterruptButton::readButton, reinterpret_cast<void*>(this));
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
  if(!m_thisButtonInitialised) initialiseInstance();    // Auto initialisation (typical begin() function)

  if(menuLevel >= m_numMenus) {
    ESP_LOGE(TAG, "Specified menu level is greater than the number of menus!");
  } else if(event >= NumEventTypes) {
    ESP_LOGE(TAG, "Specified event is invalid!");
  } else {
    eventActions[menuLevel][event] = action;            // Bind external action to button
    if(!eventEnabled(event)) enableEvent(event);        // Assume if we are binding it, we want it enabled.
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
