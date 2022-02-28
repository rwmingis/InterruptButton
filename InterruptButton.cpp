#include "InterruptButton.h"
// LOGGING
#ifdef ARDUINO
#include "esp32-hal-log.h"
#else
#include "esp_log.h"
#endif

#define ESP_INTR_FLAG_DEFAULT 0


static const char* TAG = "IBTN";    // IDF log tag

/* ToDo
  make the following procedures arduino independent:
    pin onChange interrupt control, ie setup fully once, then simple bit set enable/disable.    (DONE!)
    pin reading (digitalRead) change to IDF's MUX register acces              (DONE!)
    setting inputs (pinMode)                                                  (DONE!)
  hopefully set pin onchange interrupt toggle to a single command once set up (DONE!)

digitalPinToInterrupt() and functional callback might be more complicated, depends on further lib's architecture - an open question to consider

*/

//-- STATIC CLASS MEMBERS AND METHODS (COMMON ACROSS ALL INSTANCES TO SAVE MEMORY) -----------------------
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------

//-- Initialise Static Member Variables ------------------------------------------------------------------
bool    InterruptButton::m_firstButtonInitialised { false };
uint8_t InterruptButton::m_numMenus               { 1 };
uint8_t InterruptButton::m_menuLevel              { 0 };

//-- Method to monitor button, called by button change and various timer interrupts ----------------------
void IRAM_ATTR InterruptButton::readButton(void *arg){
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);

  switch(btn->m_state){
    case Released:                                              // Was sitting released but just detected a signal from the button
      btn->setButtonChangeInterrupt(btn->getGPIO(), false);                // Ignore change inputs while we poll for a valid press
      btn->m_validPolls = 1; btn->m_totalPolls = 1;             // Was released, just detected a change, must be a valid press so count it.
      btn->m_longPress_preventKeyPress = false;
      startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn, "DB_begin_");  // Begin debouncing the button input
      btn->m_state = ConfirmingPress;
      break;

    case ConfirmingPress:                                       // we get here each time the debounce timer expires (onchange interrupt disabled remember)
      btn->m_totalPolls++;                                                    // Count the number of total reads
      if(gpio_get_level(btn->m_pin) == btn->m_pressedState) btn->m_validPolls++; // Count the number of valid 'PRESSED' reads
      if(btn->m_totalPolls >= m_targetPolls){                                 // If we have checked the button enough times, then make a decision on key state
        if(btn->m_validPolls * 2 > btn->m_totalPolls) {                       // VALID KEYDOWN, assumed pressed if it had valid polls more than half the time
          if(!btn->m_wtgForDoubleClick){
            btn->action(KeyDown, btn->asyncEventsEnabled);                    // Keydown confirmed, fire the asynchronous keyDown event

            // Commence longKeyPress / Autopress timers (will be cancelled if released before timer expires)
            startTimer(btn->m_buttonLPandRepeatTimer, uint64_t(btn->m_longKeyPressMS * 1000), &longPressDelay, btn, "CP1_");
          }
          btn->m_state = Pressed;
        } else {                                                              // Otherwise it was a false alarm
          btn->m_state = Released;
        }
        btn->setButtonChangeInterrupt(btn->getGPIO(), true);                             // Begin monitoring pin again
      } else {                                                                // Not yet enough polls to confirm state
        startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn, "CP2_");  // Keep sampling pin state
      }
      break;

    case Pressed:                                                             // Currently pressed until now, but there was a change on the pin
      btn->setButtonChangeInterrupt(btn->getGPIO(), false);                              // Turn off this interrupt to ignore inputs while we wait to check if valid release
      startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn, "PR_");  // Start timer and start polling the button to debounce it
      btn->m_validPolls = 1; btn->m_totalPolls = 1;                           // This is first poll and it was just released by definition of state
      btn->m_state = WaitingForRelease;
      break;

    case WaitingForRelease:  // we get here when debounce timer alarms (onchange interrupt disabled remember)
      // Evaluate polling history and make a decision; done differently because we can't afford to miss a release, we keep polling until it is released.
      btn->m_totalPolls++;
      if(gpio_get_level(btn->m_pin) != btn->m_pressedState){
        btn->m_validPolls++;
        if(btn->m_totalPolls >= m_targetPolls && btn->m_validPolls * 2 > btn->m_totalPolls) { // VALID RELEASE DETECTED
          killTimer(btn->m_buttonLPandRepeatTimer);                                           // Kill the longPress and autoRepeat timer associated with holding button down

          if(btn->doubleClickEnabled && !btn->m_wtgForDoubleClick && 
            (btn->eventActions[m_menuLevel][DoubleClick] != nullptr || btn->eventActions[m_menuLevel][SyncDoubleClick] != nullptr)) {  // Setup doubleclick check if defined
            btn->m_wtgForDoubleClick = true;
            btn->m_buttonUpTimeUS = esp_timer_get_time();
            startTimer(btn->m_buttonDoubleClickTimer, uint64_t(btn->m_doubleClickMS * 1000), &doubleClickTimeout, btn, "W4R_DCsetup_");

          } else if (btn->m_wtgForDoubleClick && uint16_t((esp_timer_get_time() - btn->m_buttonUpTimeUS) / 1000) < btn->m_doubleClickMS) { // VALID DOUBLE-CLICK
            killTimer(btn->m_buttonDoubleClickTimer);
            btn->m_wtgForDoubleClick = false;
            btn->m_doubleClickMenuLevel = m_menuLevel;

            btn->action(KeyUp,        btn->asyncEventsEnabled);
            btn->action(DoubleClick,  btn->asyncEventsEnabled); btn->doubleClickOccurred = true;
            
          } else {                                                                                            // ASSSUMED VALID KEY-PRESS IF LONGPRESS DIDN'T OCCUR
            btn->m_wtgForDoubleClick = false;
            btn->action(KeyUp,        btn->asyncEventsEnabled);
            if(!btn->m_longPress_preventKeyPress) {
              btn->action(KeyPress,   btn->asyncEventsEnabled); btn->keyPressOccurred = true; btn->m_keyPressMenuLevel = m_menuLevel;
            }
          }
          btn->m_state = Released;
          btn->setButtonChangeInterrupt(btn->getGPIO(), true);                                           // Begin monitoring pin again
        } else {
          startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn, "W4R_polling_");        // Keep sampling pin state until release is confirmed
        }
      } else {
        if(btn->m_validPolls > 0) {
          btn->m_validPolls--;
        } else {
          btn->m_totalPolls = 0;                                                              // Key is being held down, don't let total polls get too far ahead.
        }
        startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn, "W4R_invalidPoll");          // Keep sampling pin state until released
      }
      break;
  } // End of SWITCH statement
} // End of readButton function


//-- Method to handle longKeyPresses (called by timer)----------------------------------------------------
void IRAM_ATTR InterruptButton::longPressDelay(void *arg){
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);
  
  btn->action(LongKeyPress, btn->asyncEventsEnabled && btn->longPressEnabled);        // Action the Async LongKeyPress Event
  btn->longKeyPressOccurred = true; btn->m_longKeyPressMenuLevel = m_menuLevel;       // Setup for Sync LongKeyPress Event
  btn->m_longPress_preventKeyPress = true;                                            // Used to prevent regular keypress later on in procedure.
  
  //Initiate the autorepeat function
  startTimer(btn->m_buttonLPandRepeatTimer, uint64_t(btn->m_autoRepeatMS * 1000), &autoRepeatPressEvent, btn, "LPD_");
}

//-- Method to handle autoRepeatPresses (called by timer)-------------------------------------------------
void IRAM_ATTR InterruptButton::autoRepeatPressEvent(void *arg){
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);

  if(btn->eventActions[m_menuLevel][AutoRepeatPress] != nullptr) {
    btn->action(AutoRepeatPress, btn->asyncEventsEnabled && btn->autoRepeatEnabled);  // Action the Async Auto Repeat KeyPress Event if defined
  } else {
    btn->action(KeyPress, btn->asyncEventsEnabled && btn->autoRepeatEnabled);         // Action the Async KeyPress Event otherwise
  }
  btn->autoRepeatPressOccurred = true; btn->m_autoRepeatPressMenuLevel = m_menuLevel; // Setup for Sync AutoRepeatePress Event
  startTimer(btn->m_buttonLPandRepeatTimer, uint64_t(btn->m_autoRepeatMS * 1000), &autoRepeatPressEvent, btn, "ARPE_");
}

//-- Method to return to readButton method to stop waiting for doubleClick upon timeout (called by timer)-
void IRAM_ATTR InterruptButton::doubleClickTimeout(void *arg){
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);
  btn->m_state = WaitingForRelease;
  btn->readButton(arg);
}

//-- Helper method enable or disable the 'onChange' interrupt for the button -----------------------------
void IRAM_ATTR InterruptButton::setButtonChangeInterrupt(gpio_num_t gpio, bool enabled){
  enabled ? gpio_intr_enable(gpio) : gpio_intr_disable(gpio);
}

//-- Helper method to simplify starting a timer ----------------------------------------------------------
void IRAM_ATTR InterruptButton::startTimer(esp_timer_handle_t &timer, uint32_t duration_US, void (*callBack)(void* arg), InterruptButton* btn, const char *msg){
  esp_timer_create_args_t tmrConfig;
    tmrConfig.arg = reinterpret_cast<void*>(btn);
    tmrConfig.callback = callBack;
    tmrConfig.dispatch_method = ESP_TIMER_TASK;
    tmrConfig.name = msg;
    //tmrConfig.name = String(msg + String(uint32_t(esp_timer_get_time()/1000))).c_str();             // not sure if such a complex timer naming makes any practical sense
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
InterruptButton::InterruptButton(uint8_t pin, uint8_t pressedState, gpio_mode_t pinMode, uint16_t longPressMS,
                                 uint16_t autoRepeatMS, uint16_t doubleClickMS, uint32_t debounceUS) :
                                 m_pressedState(pressedState),
                                 m_pinMode(pinMode),
                                 m_longKeyPressMS(longPressMS),
                                 m_autoRepeatMS(autoRepeatMS),
                                 m_doubleClickMS(doubleClickMS)
{
  // gpio number sanity check
  if (GPIO_IS_VALID_GPIO(pin))
    m_pin = static_cast<gpio_num_t>(pin);
  else {
    ESP_LOGW(TAG, "%d is not valid gpio on this platform", pin);
    m_pin = GPIO_NUM_NC;
  }

  m_pollIntervalUS = (debounceUS / m_targetPolls > 65535) ? 65535 : debounceUS / m_targetPolls;

  eventActions = new func_ptr*[m_numMenus];
  for(int menu = 0; menu < m_numMenus; menu++){
    eventActions[menu] = new func_ptr[NumEventTypes];
    for(int evt = 0; evt < NumEventTypes; evt++){
      eventActions[menu][evt] = nullptr;
    }
  }
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
void InterruptButton::begin(void){

    if(!m_firstButtonInitialised) m_firstButtonInitialised = true;

    gpio_config_t gpio_conf = {};
    gpio_conf.mode = m_pinMode;
    gpio_conf.pin_bit_mask = BIT64(m_pin);
    gpio_conf.pull_down_en = m_pressedState ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en = m_pressedState ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE;
    gpio_conf.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&gpio_conf);                                                  //configure GPIO with the given settings

    esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);          // it' OK to call this function multiple times
    ESP_LOGD(TAG, "GPIO ISR service installed with exit status: %d", err);

    gpio_isr_handler_add(m_pin, InterruptButton::readButton, (void*)this);

    m_state = (gpio_get_level(m_pin) == m_pressedState) ? Pressed : Released; // Set to current state when initialising
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

void InterruptButton::bind(uint8_t event, func_ptr action){
  bind(event, m_menuLevel, action);
}
void InterruptButton::bind(uint8_t event, uint8_t menuLevel, func_ptr action){
  eventActions[menuLevel][event] = action;
  if(event == LongKeyPress || event == SyncLongKeyPress){
    if(!longPressEnabled)   longPressEnabled = true;
  } else if(event == AutoRepeatPress || event == SyncAutoKeyPress){
    if(!autoRepeatEnabled)  autoRepeatEnabled = true;
  } else if(event == DoubleClick || event == SyncDoubleClick){
    if(!doubleClickEnabled) doubleClickEnabled = true;
  }
}

void InterruptButton::unbind(uint8_t event){
  unbind(event, m_menuLevel);
}
void InterruptButton::unbind(uint8_t event, uint8_t menuLevel){
  eventActions[menuLevel][event] = nullptr;
  if(event == LongKeyPress || event == SyncLongKeyPress){
    if(longPressEnabled)   longPressEnabled = false;
  } else if(event == AutoRepeatPress || event == SyncAutoKeyPress){
    if(autoRepeatEnabled)  autoRepeatEnabled = false;
  } else if(event == DoubleClick || event == SyncDoubleClick){
    if(doubleClickEnabled) doubleClickEnabled = false;
  }
}



void InterruptButton::action(uint8_t event, bool enabler){
  action(event, m_menuLevel, enabler);
}

void InterruptButton::action(uint8_t event, uint8_t menuLevel, bool enabler){
  if(enabler && eventActions[menuLevel][event] != nullptr)  eventActions[menuLevel][event]();
}

void InterruptButton::setMenuCount(uint8_t numberOfMenus){           // This can only be set before initialising first button
  if(!m_firstButtonInitialised && numberOfMenus > 1) m_numMenus = numberOfMenus;
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

uint8_t   InterruptButton::getMenuLevel(){
  return m_menuLevel;
}



//-- SYNCHRONOUS EVENT ROUTINES --------------------------------------------------------------------------
bool InterruptButton::inputOccurred(void){
  return (keyPressOccurred || longKeyPressOccurred || autoRepeatPressOccurred || doubleClickOccurred); 
}

void InterruptButton::clearInputs(void){
  keyPressOccurred = false; longKeyPressOccurred = false; doubleClickOccurred = false;
}

void InterruptButton::processSyncEvents(void){
  if(syncEventsEnabled){
    if(keyPressOccurred) {
      action(SyncKeyPress, m_keyPressMenuLevel, true);                            keyPressOccurred = false; 
    }        
    if(longKeyPressOccurred){
      action(SyncLongKeyPress, m_longKeyPressMenuLevel, longPressEnabled);        longKeyPressOccurred = false; 
    }
    if(autoRepeatPressOccurred){
      uint8_t evtType = (eventActions[m_autoRepeatPressMenuLevel][SyncAutoKeyPress]) ? SyncAutoKeyPress : SyncKeyPress;
      action(evtType, m_autoRepeatPressMenuLevel, autoRepeatEnabled);             autoRepeatPressOccurred = false;
    }
    if(doubleClickOccurred){
      action(SyncDoubleClick, m_doubleClickMenuLevel, doubleClickEnabled);        doubleClickOccurred = false; 
    }
  }
}
