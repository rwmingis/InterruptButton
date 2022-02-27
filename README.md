# InterruptButton
This is an interrupt based button event library for the ESP32 suitable in the Arduino framework and soon to be ESP IDF framework. It uses the 'onChange' interrupt for a given pin and the ESP high precision timer to carry the necessary pin-polling to facilitate a simple, but very reliable debouncing routine.  Once de-bounced, actions bound to certain button events including 'Key Down', 'Key Up', 'Key Press', 'Long Key Press', 'Auto-Repeat Press', and 'Double-clicks' are available to bind your own functions to.

With a built in user-defined menu/paging system, each button can be bound to many layers of different functionality depending on what the user interface is displaying at a given moment.  This means it can work as a simple button, all the way up to a full user interface using a single button with different combinations of special key presses for navigation.

The use of interrupts instead of laborious button polling means that actions bound to the button are NOT limited to the main loop frequency which significantly reduces the chance of missed presses with long main loop durations.

There are 6 Asynchronous events (actioned immediately via intterrupt) , 4 Synchronous events (actioned at main loop) and a menu/pageing structure which is only limited to your memory on chip (which is huge for the ESP32).  This means you could attach 10 different functions to a single button per menu level / page (if you dare!)

## It allows for the following:

### Asynchronous Events 
Asynchronous Events are actioned immediately by calling user defined functions attached to specific button events as an Interrupt Service Routine (ISR).
  * keyDown
  * keyUp
  * keyPress (combination of keyDown and keyUp)
  * longKeyPress (required press time is user configurable)
  * AutoRepeatPress (Rapid fire, if enabled, but not defined, then the standard keyPress action is used)
  * doubleClick (max time between clicks is user configurable)
  
Functions applied to the above events should be defined with the IRAM_ATTR attribute to place them in the onboard RAM rather than the flash storage area which is slower and could cause SPI bus clashes though I have not run into this issue yet.  Furthermore, the user should careful calling API functions that are not defined in with IRAM_ATTR which could force the code back into Flash memory.  This is a precautionary measure and a limitation of the chip/frame work (not the libary); however, testing has shown this can still be done without issue in some cases.  For processor intesive routines, use 'Sychronous Events'
  
### Synchronous Events
Synchronous events are actioned when the 'processSyncEvents()' member function is called in the main loop.  Like Asynchronous events, they are actioned by calling user-defined functions as bound to specific button events.
  * syncKeyPress
  * syncLongKeyPress
  * syncAutoRepeatPress
  * syncDoubleClick

These events are based on the same debounce and delay configuration for synchronous events listed above.

### Multi-page/level events
  This is handy if you have several different GUI pages where all the buttons mean something different on a different page.  
  You can change the menu level of all buttons at once using the static member function 'setMenuLevel(level)'.  Note that you must set the desired number of menus before initialising your first button, as this cannot be changed later (this may be improved later subject to user requests)
  
### Other Features
  * The Asynchronous and Synchronous events can enabled or disabled globally by type (Async and Sync)
  * The pin, the pin level, and pull up / pull down mode can all be set on a per button instance basis.
  * The timing for debounce, longPress, AutoRepeatPress and doubleClick can be set on a per button instance basis.
  * Asynchronous events are called *Immediately* after debouncing
  * Synchronous events are invoked by calling the 'processSyncEvents()' member function in the main loop and *are subject to the main loop timing.*

### Example Usage
This is an output of the serial port from the example file.  Here just the Serial.Println(), but you can replace that with your own code to do what you need.  (also note, I got 'sync' and 'async' backwards in screenshot below)
![This is an image](https://github.com/rwmingis/InterruptButton/blob/fba0949d9165099286d435f54c975e718684fcfc/images/example.png)

## Roadmap Forward ##
  * Consider Adding button modes such as momentary, latching, etc.
  * Remove any reference to the Arduino API so that this libary will work on the ESP-IDF OR the Arduino Frameworks
  

## Known Limitations:
  * While the ISR code is generally placed in the ram area using the IRAM_ATTR attribute, there are some Arduino and ESP IDF functions called from within the ISR. This may place ISR code back in the flash and potentially cause issues if the SPI bus is busy, but so far, hasn't caused any issues on my testing and implementation.  Future versions intend to remdedy this.
  * Like all ISR's the asynchronous code functions should be LIGHTWEIGHT and FAST
  * The Synchronous routines can be much more robust, but are limited to the main loop frequency

### See the example file, as it covers most interesting things, but I believe it is fairly self-explanatory.

*  This libary should not be used for mission critical or mass deployments with out the developer satisfying themselves that it is stabile for their purpose.  My poor formatting was based on bad habits from my C++ programming course in university in 1998 and hasn't improved since.  I feel the code works great and I generated this library because I couldn't find anything similar and the ones based on loop polling didn't work at all with long loop times.  Interrupts can be a bit cantankerous, but this seems to work nearly flawlessly in my experience, but I imagine maybe not so for everyone and welcome any suggestions for improvements.*




