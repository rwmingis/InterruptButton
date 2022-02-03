#include "InterruptButton.h"


// Initialise Static Member Variables -------------------------------------------
uint8_t InterruptButton::m_menuLevel          { 0 };
bool    InterruptButton::m_enableSyncEvents   { true };
bool    InterruptButton::m_enableAsyncEvents  { true };

// Constructor ------------------------------------------------------------------
InterruptButton::InterruptButton(uint8_t pin, uint8_t pinMode, uint8_t pressedState, uint16_t longPressMS, uint16_t doubleClickMS, uint32_t debounceUS) {
  m_pin = pin;
  m_pinMode = pinMode;
  m_pressedState = pressedState;
  m_longKeyPressMS = longPressMS;
  m_doubleClickMS = doubleClickMS;
  m_debounceIntervalUS = min(debounceUS, uint32_t(65535 * m_targetPolls));
  m_pollIntervalUS = m_debounceIntervalUS / m_targetPolls;
}

// Destructor --------------------------------------------------------------------
InterruptButton::~InterruptButton() {
  detachInterrupt(digitalPinToInterrupt(m_pin));
  killTimer();
  killRepeatTimer();
}

// Initialiser -------------------------------------------------------------------
void InterruptButton::begin(void){
  if(m_pinMode == INPUT_PULLUP || m_pinMode == INPUT_PULLDOWN || m_pinMode == INPUT) {
    pinMode(m_pin, m_pinMode);                                              // Set pullups for button pin, if any
    setButtonChangeInterrupt(true);                                         // Enable onchange interrupt for button pin 
    m_state = (digitalRead(m_pin) == m_pressedState) ? pressed : released;  // Set to current state when initialising
  } else {
    Serial.println("Error: Interrupt Button must be 'INPUT_PULLUP' or 'INPUT_PULLDOWN' or 'INPUT'");
  }
}

// Static Class wrapper with reference to the readButton instance, necessary for being called by interrupt
void IRAM_ATTR InterruptButton::readButton_static(void *arg){ 
  reinterpret_cast<InterruptButton*>(arg)->readButton();
}

// Main ISR routine, which is to be stored in RAM rather than FLASH since it is called in an interrupt.
void IRAM_ATTR InterruptButton::readButton(){                   // If we get here, the interrupt has seen some action
  static bool debouncingDoubleClick = false;
  static uint16_t validPolls = 0, totalPolls = 0;
  static uint32_t buttonDownTime, buttonUpTime;

  switch(m_state){
  case released:                                              // Was sitting released but just detected a signal from the button
    setButtonChangeInterrupt(false);                          // Turn off this interrupt to ignore inputs while we wait to check if valid press
    validPolls = 1; totalPolls = 1;                           // Was released, just detected a change, must be a valid press so count it.
    startTimer(m_pollIntervalUS);                             // Begin debouncing the button input
    //Serial.println("\n\n\n\n[1.0]  Released, changing to 'confirmingPress'");
    m_state = confirmingPress;
    break;

  case confirmingPress:                                       // we get here each time the debounce timer expires (onchange interrupt disabled remember)
    totalPolls++;                                             // Count the number of total reads
    if(digitalRead(m_pin) == m_pressedState) validPolls++;    // Count the number of valid 'PRESSED' reads
    if(totalPolls >= InterruptButton::m_targetPolls){         // If we have checked the button enough times, then make a decision on key state
      if(validPolls * 2 > totalPolls) {                       // Key is assumed pressed if it had valid polls more than half the time
        if(m_enableAsyncEvents && keyDown[m_menuLevel] != nullptr) keyDown[m_menuLevel]();
        buttonDownTime = millis();
        m_state = pressed;
        startTimer(m_longKeyPressMS * 1000);                  // we should get back and check if key is still pressed after m_longKeyPressMS interval event if not interrupt occured
      } else {                                                // Otherwise it was a false alarm
        m_state = released;
        //Serial.println("[2.2]  'confirmingPress', changing to (still)'released'\n");
      }
      setButtonChangeInterrupt(true);                         // Begin monitoring pin again
    } else {
      startTimer(m_pollIntervalUS);                           // Keep sampling pin state
    }
    break;

  case pressed: {                                             // Currently pressed until now, but there was a change on the pin
    if (millis() - buttonDownTime >= m_longKeyPressMS){       // longPress timer expired and we still have button pressed, let's run the action immideatly
      longKeyPressOccurred = true;  m_longKeyPressMenuLevel = m_menuLevel;
      if(m_enableAsyncEvents && longKeyPress[m_menuLevel] != nullptr)  longKeyPress[m_menuLevel]();

      if (repeat && !m_repeatTimer)                           // run 'repeat' timer if "repeat on Hold" is enabled
        startRepeatTimer(m_holdRepeatUS);
    }

    m_state = confirmingRelease;                              // from now on we just wait for proper button release
    if(digitalRead(m_pin) == m_pressedState){
      validPolls = 0; totalPolls = 0;                         // Button is still pressed, keep watching for interrupt
      return;
    }

    // otherwise we do have interrupt, let's confirm
    validPolls = 1; totalPolls = 1;                           // This is first poll and it was just released by definition of state
    setButtonChangeInterrupt(false);                          // Turn off this interrupt to ignore inputs while we wait to check if valid release
    startTimer(m_pollIntervalUS);                             // Start timer and start polling the button to debounce it
//Serial.println("[3.0]  Pressed, Changing to 'confirmingRelease'");
    break;
  }

  case confirmingRelease: {                                   // we get here when debounce timer alarms (onchange interrupt disabled remember)
    // Evaluate polling history and make a decision; done differently because we can't afford to miss a release, we keep polling until it is released.
    totalPolls++;

    if(digitalRead(m_pin) == m_pressedState){                 // false release
      if(validPolls > 0)
        validPolls--;
      else
        totalPolls = 0;                                       // Key is being held down, don't let total polls get too far ahead.

      startTimer(m_pollIntervalUS);                           // Keep sampling pin state until released
      return;
    }
    
    validPolls++;
    if(totalPolls < InterruptButton::m_targetPolls || validPolls * 2 <= totalPolls){                        // not enough polls to decide
      startTimer(m_pollIntervalUS);                           // Keep sampling pin state until release is confirmed
      return;
    }

    // if it is just a release after long_press
    if (millis() - buttonDownTime >= m_longKeyPressMS){
      if(m_enableAsyncEvents && keyUp[m_menuLevel] != nullptr)
        keyUp[m_menuLevel]();
      m_state = released;
      killRepeatTimer();                                    // stop repeat on button release
      setButtonChangeInterrupt(true);                       // Begin monitoring pin again
      return;
    }

    if(debouncingDoubleClick){                            // JUST COMPLETED DEBOUNCING A DOUBLECLICK
      doubleClickOccurred = true;   m_doubleClickMenuLevel = m_menuLevel;
      if(m_enableAsyncEvents && keyUp[m_menuLevel] != nullptr)         keyUp[m_menuLevel]();
      if(m_enableAsyncEvents && doubleClick[m_menuLevel] != nullptr)   doubleClick[m_menuLevel]();
      debouncingDoubleClick = false;
      m_state = released;
      setButtonChangeInterrupt(true);                       // Begin monitoring pin again
      return;
      //Serial.println("[4.2]  'confirmingRelease', debounce of doubleClick completed, changing to 'released'");
    }

    // there is a double-click callback defined, need to wait for possible second click
    if (doubleClick[m_menuLevel] || syncDoubleClick[m_menuLevel]){
        m_state = wtgForDoubleClick;
        buttonUpTime = millis();
        setButtonChangeInterrupt(true);                       // Begin monitoring pin again
        startTimer(uint32_t(m_doubleClickMS * 1000));         // Set timeout to come back in case it was a single click
        return;
    }

    // THE EVENT WAS A BASIC KEYPRESS/Click
    keyPressOccurred = true; m_keyPressMenuLevel = m_menuLevel;
    if(m_enableAsyncEvents && keyUp[m_menuLevel] != nullptr)         keyUp[m_menuLevel]();
    if(m_enableAsyncEvents && keyPress[m_menuLevel] != nullptr)      keyPress[m_menuLevel]();
    m_state = released;                                     // Since we timedout, no second press and we have debounced already
    setButtonChangeInterrupt(true);                         // Begin monitoring pin again
    break;
  }

  case wtgForDoubleClick: {
    if((millis() - buttonUpTime) < m_doubleClickMS) {         // THE EVENT WAS A DOUBLECLICK
      setButtonChangeInterrupt(false);                        // Turn off this interrupt to ignore inputs while we wait to check if valid release
      validPolls = 1; totalPolls = 1;                         // This is first poll and it was just released by definition of state
      debouncingDoubleClick = true;
      m_state = confirmingRelease;
      startTimer(m_pollIntervalUS);                           // Kills doublclick interval timer, and starts debounce timer and start polling the button
      return;
      //Serial.println(String("[5.0]  'wtgForDoubleClick', doubleclick detected, changing to 'confirmingRelease' - ClickTiming: ") + (millis() - buttonUpTime));
    } else {                                                  // too late for double-click, consider EVENT AS A BASIC KEYPRESS
      debouncingDoubleClick = false;
      keyPressOccurred = true; m_keyPressMenuLevel = m_menuLevel;
      if(m_enableAsyncEvents && keyUp[m_menuLevel] != nullptr)         keyUp[m_menuLevel]();
      if(m_enableAsyncEvents && keyPress[m_menuLevel] != nullptr)      keyPress[m_menuLevel]();
      killRepeatTimer();
      m_state = released;                                     // Since we timedout, no second press and we have debounced already
      setButtonChangeInterrupt(true);                         // Begin monitoring pin again
    }
    break;
  }
  default:                                                    // catch-all case
    return;
  }
}

void InterruptButton::setMenuLevel(uint8_t level) { 
  if(level >= 0 && level < InterruptButton::m_maxMenus) m_menuLevel = level; 
}

uint8_t InterruptButton::getMenuLevel() { 
  return m_menuLevel; 
}

void InterruptButton::enableEvents()        { m_enableSyncEvents =  true;  m_enableAsyncEvents = true;  }
void InterruptButton::disableEvents()       { m_enableSyncEvents =  false; m_enableAsyncEvents = false; }
void InterruptButton::enableSyncEvents()    { m_enableSyncEvents =  true;  }
void InterruptButton::disableSyncEvents()   { m_enableSyncEvents =  false; }
void InterruptButton::enableAsyncEvents()   { m_enableAsyncEvents = true;  }
void InterruptButton::disableAsyncEvents()  { m_enableAsyncEvents = false; }



bool InterruptButton::inputOccurred(void){
  return (keyPressOccurred || longKeyPressOccurred || doubleClickOccurred);
}
void InterruptButton::clearInputs(void){
  keyPressOccurred = false;
  longKeyPressOccurred = false;
  doubleClickOccurred = false;
}

// Synchronous Event Routines ------------------------------- --
void InterruptButton::processSyncEvents(void){
  if(!m_enableSyncEvents)
    return;

    if(keyPressOccurred) {
      if(syncKeyPress[m_keyPressMenuLevel] != nullptr)         syncKeyPress[m_keyPressMenuLevel]();
      keyPressOccurred = false;
    }
    if(longKeyPressOccurred){
      if(syncLongKeyPress[m_longKeyPressMenuLevel] != nullptr) syncLongKeyPress[m_longKeyPressMenuLevel]();
      longKeyPressOccurred = false;
    }
    if(doubleClickOccurred){
      if(syncDoubleClick[m_doubleClickMenuLevel] != nullptr)   syncDoubleClick[m_doubleClickMenuLevel]();
      doubleClickOccurred = false;
    }
    if (onhold && syncKeyHold[m_onHoldMenuLevel]){
      syncKeyHold[m_onHoldMenuLevel]();
      onhold = false;
    }
}

void IRAM_ATTR InterruptButton::setButtonChangeInterrupt(bool enabled, bool clearFlags){
  // Clear interrupts on this pin (only) before re-enabling to ignore spurious hits when it was disabled.
  if(clearFlags){
    if(m_pin < 32) {
      bitSet(GPIO.status_w1tc, m_pin);
    } else {
      bitSet(GPIO.status1_w1tc.val, m_pin - 32);
    }
  }
  // Set the onChange interrupt state for the pin
  if(enabled) {
    attachInterrupt(digitalPinToInterrupt(m_pin), std::bind(&InterruptButton::readButton_static, this), CHANGE);
  } else {
    detachInterrupt(digitalPinToInterrupt(m_pin));
  }
}

void IRAM_ATTR InterruptButton::startTimer(uint32_t intervalUS){
  esp_timer_create_args_t tmrConfig;
    tmrConfig.arg = reinterpret_cast<void*>(this);
    tmrConfig.callback = &InterruptButton::readButton_static;
    tmrConfig.dispatch_method = ESP_TIMER_TASK;
    tmrConfig.name = "button_timer";

  killTimer();
  esp_timer_create(&tmrConfig, &m_buttonTimer);
  esp_timer_start_once(m_buttonTimer, intervalUS);
}

void IRAM_ATTR InterruptButton::killTimer(){
  if(m_buttonTimer) {
    esp_timer_stop(m_buttonTimer);
    esp_timer_delete(m_buttonTimer);
    m_buttonTimer = nullptr;
  }
}


void IRAM_ATTR InterruptButton::repeatButton_static(void *arg){ 
  reinterpret_cast<InterruptButton*>(arg)->doButtonRepeat();
}

void IRAM_ATTR InterruptButton::startRepeatTimer(uint64_t period){
  esp_timer_create_args_t tmrConfig;
  tmrConfig.arg = reinterpret_cast<void*>(this);
  tmrConfig.callback = &InterruptButton::repeatButton_static;
  tmrConfig.dispatch_method = ESP_TIMER_TASK;
  tmrConfig.skip_unhandled_events = true;
  tmrConfig.name = "btn_repeat_timer";

  killRepeatTimer();
  esp_timer_create(&tmrConfig, &m_repeatTimer);
  esp_timer_start_periodic(m_repeatTimer, period);
}

void IRAM_ATTR InterruptButton::killRepeatTimer(){
  if(m_repeatTimer) {
    esp_timer_stop(m_repeatTimer);
    esp_timer_delete(m_repeatTimer);
    m_repeatTimer = nullptr;
  }
}

void IRAM_ATTR InterruptButton::doButtonRepeat(){
  if (!repeat){ // oops, missfire
    killRepeatTimer();
    return;
  }

  if (m_enableAsyncEvents && keyHold[m_menuLevel])
    keyHold[m_menuLevel]();
  else if (m_enableAsyncEvents && keyPress[m_menuLevel])    // simulate keyPress if no specific handler for keyHold
    keyPress[m_menuLevel]();

  if (m_enableSyncEvents){
    onhold = true;
    m_onHoldMenuLevel = m_menuLevel;
  }
}