# InterruptButton
This is an interrupt based button event library for the ESP32. It uses the 'onChange' interrupt for a given pin, and also uses the ESP high precision timer to carry out debouncing, longPresses, and doubleClicks. 

This means that button polling and debouncing are NOT limited to the main loop frequency which significantly reduces the chance of missed presses with long main loop durations.

There are 3 synchronous events, 5 asynchronous events, and a pageing structure allowing 5 levels of action for each button.  This means you could attach 40 different functions to a single button (if you dare!)

It is written mostly using the Arduino platform, but does reference some functions for the ESP IDF in order to make the interrupt driven events possible.

## It allows for the following:

### Asynchronous Events 
Asynchronous Events are actioned immediately by calling user defined functions attached to specific button events as an Interrupt Service Routine (ISR).
  * keyDown
  * keyUp
  * keyPress (combination of keyDown and keyUp)
  * longKeyPress (required press time is user configurable)
  * doubleClick (max time between clicks is user configurable)
  
Functions applied to the above events should be defined with the IRAM_ATTR attribute to place them in the onboard RAM rather than the flash storage area which is much slower and could cause SPI bus clashes though I have not run into this issue yet.
  
### Synchronous Events
Synchronous events are actioned when the 'processAsyncEvents()' member function is called in the main loop.  Like Asynchronous events, they are actioned by calling user defined functions attached to specific button events.
  * keyPress
  * longKeyPress
  * doubleClick

These events are basesd on the same debounce and delay configuration for synchronous events listed above.

### Multi-page/level events
  This is handy if you have several different GUI pages where all the buttons mean something different on a different page.  
  You can change the menu level of all buttons at once using the static member function 'setMenuLevel(level)'
  Currently limited to 5 levels, but easily changed inside of library (these may be changed to use dynamic memory allocation if this library has good takeup in the future)

### Other Features
  * The Asynchronous and Synchronous events can enabled or disabled globally by type (Async and Sync)
  * The pin, the pin level, and pull up / pull down mode can all be set on a per button instance basis.
  * The timing for debounce, longPress, and DoubleClick can be set on a per button instance basis.
  * Asynchronous events are called *Immediately* after debouncing
  * Synchronous events are invoked by calling the 'processAsyncEvents()' member function in the main loop and *are subject to the main loop timing.*

### Example Usage
This is an output of the serial port from the example file.  Here just the Serial.Println(), but you can replace that with your own code to do what you need.  (also note, I got 'sync' and 'async' backwards in screenshot below)
![This is an image](https://github.com/rwmingis/InterruptButton/blob/fba0949d9165099286d435f54c975e718684fcfc/images/example.png)



## Known Limitations:
  * While the ISR code is generally placed in the ram area using the IRAM_ATTR attribute, there are some Arduino and ESP IDF functions called from within the ISR. This may place ISR code back in the flash and potentially cause issues if the SPI bus is busy, but so far, hasn't caused any issues on my testing and implementation.  Future versions may remedy this, but may complicate the code.
  * Like all ISR's the asynchronous code functions should be LIGHTWEIGHT and FAST
  * The Synchronous routines can be much more robust, but are limited to the main loop frequency

### See the example file, as it covers most interesting things, but I believe it is fairly self-explanatory.

*This is my first library so please be kind.  It is not intended for mission critical mass deployments, more so for personal non-critical non-hazardous projects.  My poor formatting was based on bad habits from my C++ programming course in university in 1998 and hasn't improved since.  I feel the code works great and I generated this library because I couldn't find anything similar and the ones based on loop polling didn't work at all with long loop times.  Interrupts can be a bit cantankerous, but this seems to work nearly flawlessly in my experience, but I imagine maybe not so for everyone and welcome any suggestions for improvements.*




