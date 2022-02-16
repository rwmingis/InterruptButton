#include "InterruptButton.h"

/* ToDo
  Setup the interrupt in begin and hopefully just toggle it with a single bit in the routine.
  Make pinread and interrupt setup platform independent.
  dynamically add/remove menu levels maybe through std::vector
  have getters and setters for individual timings.
  Add boolean blocker for repeat on hold or just define or undefine the action, TBC.
*/

// Initialise Static Member Variables -------------------------------------------
uint8_t InterruptButton::m_menuLevel          { 0 };
bool    InterruptButton::m_enableSyncEvents   { true };
bool    InterruptButton::m_enableAsyncEvents  { true };

// Constructor ------------------------------------------------------------------
InterruptButton::InterruptButton(uint8_t pin, uint8_t pinMode, uint8_t pressedState, uint16_t longPressMS, 
                                 uint16_t autoRepeatMS, uint16_t doubleClickMS, uint32_t debounceUS) {
  m_pin = pin;
  m_pinMode = pinMode;
  m_pressedState = pressedState;
  m_longKeyPressMS = longPressMS;
  m_autoRepeatMS = autoRepeatMS;
  m_doubleClickMS = doubleClickMS;
  m_pollIntervalUS = uint16_t(min(debounceUS / m_targetPolls, uint32_t(65535)));
}

// Destructor --------------------------------------------------------------------
InterruptButton::~InterruptButton() {
  detachInterrupt(digitalPinToInterrupt(m_pin));
  killTimer(m_buttonPollTimer);
  killTimer(m_buttonLPandRepeatTimer);
  killTimer(m_buttonDoubleClickTimer);
}

// Initialiser -------------------------------------------------------------------
void InterruptButton::begin(void){
  if(m_pinMode == INPUT_PULLUP || m_pinMode == INPUT_PULLDOWN || m_pinMode == INPUT) {
    pinMode(m_pin, m_pinMode);                                              // Set pullups for button pin, if any
    setButtonChangeInterrupt(this, true);                                   // Enable onchange interrupt for button pin 
    m_state = (digitalRead(m_pin) == m_pressedState) ? pressed : released;  // Set to current state when initialising
  } else {
    Serial.println("Error: Interrupt Button must be 'INPUT_PULLUP' or 'INPUT_PULLDOWN' or 'INPUT'");
  }
}

// Static Class wrapper with reference to the readButton instance, necessary for being called by interrupt
void IRAM_ATTR InterruptButton::readButton(void *arg){
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);

  switch(btn->m_state){
    case released:                                              // Was sitting released but just detected a signal from the button
      btn->setButtonChangeInterrupt(btn, false);                // Ignore change inputs while we poll for a valid press
      btn->m_validPolls = 1; btn->m_totalPolls = 1;             // Was released, just detected a change, must be a valid press so count it.
      btn->m_longPress_preventKeyPress = false;
      startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn);  // Begin debouncing the button input
      btn->m_state = confirmingPress;
      break;

    case confirmingPress:                                       // we get here each time the debounce timer expires (onchange interrupt disabled remember)
      btn->m_totalPolls++;                                                    // Count the number of total reads
      if(digitalRead(btn->m_pin) == btn->m_pressedState) btn->m_validPolls++; // Count the number of valid 'PRESSED' reads
      if(btn->m_totalPolls >= m_targetPolls){                                 // If we have checked the button enough times, then make a decision on key state
        if(btn->m_validPolls * 2 > btn->m_totalPolls) {                       // VALID KEYDOWN, assumed pressed if it had valid polls more than half the time
          if(!btn->m_wtgForDoubleClick){
            // Keydown confirmed, fire the asynchronous keyDown event
            if(m_enableAsyncEvents && btn->keyDown[m_menuLevel] != nullptr)           btn->keyDown[m_menuLevel](); 

            // Commence longKeyPress / Autopress timers (will be cancelled if released before timer expires)
            startTimer(btn->m_buttonLPandRepeatTimer, uint64_t(btn->m_longKeyPressMS * 1000), &longPressDelay, btn);
          }
          btn->m_state = pressed;
        } else {                                                              // Otherwise it was a false alarm
          btn->m_state = released;
        }
        btn->setButtonChangeInterrupt(btn, true);                             // Begin monitoring pin again
      } else {                                                                // Not yet enough polls to confirm state
        startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn);  // Keep sampling pin state
      }
      break;

    case pressed:                                                             // Currently pressed until now, but there was a change on the pin
      btn->setButtonChangeInterrupt(btn, false);                              // Turn off this interrupt to ignore inputs while we wait to check if valid release
      startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn);  // Start timer and start polling the button to debounce it
      btn->m_validPolls = 1; btn->m_totalPolls = 1;                           // This is first poll and it was just released by definition of state
      btn->m_state = waitingForRelease;
      break;

    case waitingForRelease:  // we get here when debounce timer alarms (onchange interrupt disabled remember)
      // Evaluate polling history and make a decision; done differently because we can't afford to miss a release, we keep polling until it is released.
      btn->m_totalPolls++;
      if(digitalRead(btn->m_pin) != btn->m_pressedState){
        btn->m_validPolls++;
        if(btn->m_totalPolls >= m_targetPolls && btn->m_validPolls * 2 > btn->m_totalPolls) { // VALID RELEASE DETECTED
          killTimer(btn->m_buttonLPandRepeatTimer);                                           // Kill the longPress and autoRepeat timer associated with holding button down

          if((btn->doubleClick[m_menuLevel] != nullptr || btn->syncDoubleClick[m_menuLevel] != nullptr) && !btn->m_wtgForDoubleClick) {  // Setup doubleclick check if defined
            btn->m_wtgForDoubleClick = true;
            btn->m_buttonUpTimeMS = millis();
            startTimer(btn->m_buttonDoubleClickTimer, uint64_t(btn->m_doubleClickMS * 1000), &doubleClickTimeout, btn);

          } else if (btn->m_wtgForDoubleClick && (millis() - btn->m_buttonUpTimeMS) < btn->m_doubleClickMS) { // VALID DOUBLE-CLICK
            killTimer(btn->m_buttonDoubleClickTimer);
            btn->m_wtgForDoubleClick = false;
            btn->m_doubleClickMenuLevel = m_menuLevel;
            if(m_enableAsyncEvents && btn->keyUp[m_menuLevel] != nullptr)             btn->keyUp[m_menuLevel]();
            if(m_enableAsyncEvents && btn->doubleClick[m_menuLevel] != nullptr)       btn->doubleClick[m_menuLevel]();
            btn->doubleClickOccurred = true;

          } else {                                                                                            // ASSSUMED VALID KEY-PRESS IF LONGPRESS DIDN'T OCCUR
            btn->m_wtgForDoubleClick = false;
            if(m_enableAsyncEvents && btn->keyUp[m_menuLevel] != nullptr)             btn->keyUp[m_menuLevel]();
            if(!btn->m_longPress_preventKeyPress) {
              btn->m_keyPressMenuLevel = m_menuLevel;
              if(m_enableAsyncEvents && btn->keyPress[m_menuLevel] != nullptr)        btn->keyPress[m_menuLevel]();
              btn->keyPressOccurred = true;
            }
          }
          btn->m_state = released;
          btn->setButtonChangeInterrupt(btn, true);                                           // Begin monitoring pin again
        } else {
          startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn);        // Keep sampling pin state until release is confirmed
        }
      } else {
        if(btn->m_validPolls > 0) {
          btn->m_validPolls--;
        } else {
          btn->m_totalPolls = 0;                                                              // Key is being held down, don't let total polls get too far ahead.
        }
        startTimer(btn->m_buttonPollTimer, btn->m_pollIntervalUS, &readButton, btn);          // Keep sampling pin state until released
      }
      break;
  } // End of SWITCH statement
} // End of readButton function

void IRAM_ATTR InterruptButton::longPressDelay(void *arg){
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);
  btn->m_longKeyPressMenuLevel = m_menuLevel;
  if(m_enableAsyncEvents && btn->longKeyPress[m_menuLevel] != nullptr) btn->longKeyPress[m_menuLevel]();
  btn->longKeyPressOccurred = true;                 // Used to fire sync keypress later.
  btn->m_longPress_preventKeyPress = true;          // used to prevent regular keypress later on.
  
  //Initiate the autorepeat function here
  startTimer(btn->m_buttonLPandRepeatTimer, uint64_t(btn->m_autoRepeatMS * 1000), &autoRepeatPressEvent, btn);
}

void IRAM_ATTR InterruptButton::autoRepeatPressEvent(void *arg){
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);

  btn->m_autoRepeatPressMenuLevel = m_menuLevel;
  if(m_enableAsyncEvents && btn->autoRepeatPress[m_menuLevel] != nullptr) btn->autoRepeatPress[m_menuLevel]();
  btn->autoRepeatPressOccurred = true; 
  startTimer(btn->m_buttonLPandRepeatTimer, uint64_t(btn->m_autoRepeatMS * 1000), &autoRepeatPressEvent, btn);
}

void IRAM_ATTR InterruptButton::doubleClickTimeout(void *arg){
  InterruptButton* btn = reinterpret_cast<InterruptButton*>(arg);
  btn->m_state = waitingForRelease;
  btn->readButton(arg);
}

void IRAM_ATTR InterruptButton::setButtonChangeInterrupt(InterruptButton* btn, bool enabled, bool clearFlags){
  // Clear interrupts on this pin (only) before re-enabling to ignore spurious hits when it was disabled.
  if(clearFlags){
    if(btn->m_pin < 32) {
      bitSet(GPIO.status_w1tc, btn->m_pin);
    } else {
      bitSet(GPIO.status1_w1tc.val, btn->m_pin - 32);
    }
  }
  // Set the onChange interrupt state for the pin
  if(enabled) {
    attachInterrupt(digitalPinToInterrupt(btn->m_pin), std::bind(&InterruptButton::readButton, btn), CHANGE);
  } else {
    detachInterrupt(digitalPinToInterrupt(btn->m_pin));
  }
}

void IRAM_ATTR InterruptButton::startTimer(esp_timer_handle_t &timer, uint32_t duration_US, void (*callBack)(void* arg), InterruptButton* btn){
  esp_timer_create_args_t tmrConfig;
    tmrConfig.arg = reinterpret_cast<void*>(btn);
    tmrConfig.callback = callBack;
    tmrConfig.dispatch_method = ESP_TIMER_TASK;
    tmrConfig.name = "InterruptButtonTimer";
  killTimer(timer);
  esp_timer_create(&tmrConfig, &timer);
  esp_timer_start_once(timer, duration_US);
}

void IRAM_ATTR InterruptButton::killTimer(esp_timer_handle_t &timer){
  if(timer){
    esp_timer_stop(timer);
    esp_timer_delete(timer);
    timer = nullptr;
  }
}

void InterruptButton::setMenuLevel(uint8_t level) { if(level >= 0 && level < m_maxMenus) m_menuLevel = level; }
uint8_t InterruptButton::getMenuLevel()     { return m_menuLevel; }
void InterruptButton::enableEvents()        { m_enableSyncEvents =  true;  m_enableAsyncEvents = true;  }
void InterruptButton::disableEvents()       { m_enableSyncEvents =  false; m_enableAsyncEvents = false; }
void InterruptButton::enableSyncEvents()    { m_enableSyncEvents =  true;  }
void InterruptButton::disableSyncEvents()   { m_enableSyncEvents =  false; }
void InterruptButton::enableAsyncEvents()   { m_enableAsyncEvents = true;  }
void InterruptButton::disableAsyncEvents()  { m_enableAsyncEvents = false; }
bool InterruptButton::inputOccurred(void)   { return (keyPressOccurred || longKeyPressOccurred || autoRepeatPressOccurred || doubleClickOccurred); }
void InterruptButton::clearInputs(void)     { keyPressOccurred = false; longKeyPressOccurred = false; doubleClickOccurred = false; }

// Synchronous Event Routines ------------------------------- --
void InterruptButton::processSyncEvents(void){
  if(m_enableSyncEvents){
    if(keyPressOccurred) {
      if(syncKeyPress[m_keyPressMenuLevel] != nullptr)         syncKeyPress[m_keyPressMenuLevel]();
      keyPressOccurred = false; 
    }        
    if(longKeyPressOccurred){
      if(syncLongKeyPress[m_longKeyPressMenuLevel] != nullptr) syncLongKeyPress[m_longKeyPressMenuLevel]();
      longKeyPressOccurred = false; 
    }
    if(autoRepeatPressOccurred){
      if(syncAutoRepeatPress[m_autoRepeatPressMenuLevel] != nullptr) syncAutoRepeatPress[m_autoRepeatPressMenuLevel]();
      autoRepeatPressOccurred = false; 
    }
    if(doubleClickOccurred){
      if(syncDoubleClick[m_doubleClickMenuLevel] != nullptr)   syncDoubleClick[m_doubleClickMenuLevel]();
      doubleClickOccurred = false; 
    }
  }
}
