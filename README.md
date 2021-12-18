# InterruptButton
This is an interrupt based button event library for the ESP32. It uses the 'onChange' interrupt for a given pin, and also uses the ESP high precision timer to carry out debouncing, longPresses, and doubleClicks.

It is written mostly using the Arduino platform, but does reference some functions for the ESP IDF in order to make the interrupt driven events possible.

## It allows for the following:

### Synchronous Events (actioned inside of an ISR)
  * keyDown
  * keyUp
  * keyPress (combination of keyDown and keyUp)
  * longKeyPress (required press time is user configurable)
  * doubleClick (max time between clicks is user configurable)
  
Functions applied to the above events should be defined with the IRAM_ATTR attribute to place them in the onboard RAM rather than the flash storage area which is much slower.
  
### Asynchronous Events, actioned when 'processAsyncEvents()' member function is called in the main loop, which have the same attributes as synchronous events listed above
  * keyPress
  * longKeyPress
  * doubleClick

### Multi-page/level events
  This is handy if you several different GUI pages where all the different events mean something different.  
  If you change the page index, you can change all button behavior at once.  Currently limited to 5 levels, but relatively easily changed.

### Other Features
  * The Asynchronous and Synchronous events can enabled or disabled by type (Async and Sync)
  * The pin, the pin level, and pull up / pull down mode can all be set on a per instance basis.
  * The timing for debounce, longPress, and DoubleClick can be set on a per instance basis.
  * Synchronous events are called *Immediately* after debouncing
  * Asynchronous events are invoked by calling the 'processAsyncEvents()' member function in the main loop and *are subject to the main loop timing.*

## Known Limitations:
  * While the ISR code is generally placed in the ram area using the IRAM_ATTR attribute, there are some Arduina and ESP IDF functions called from within the ISR. This may place ISR code back in the flash and potentially cause issues if the SPI bus is busy, but so far, hasn't caused any issues on my testing and implementation.  Future version may remedy this, but may complicate the code.
  * Like all ISR's the synchronous code functions should be LIGHTWEIGHT and FAST
  * The Asynchronous routines can be much more robust, but are limited to the mainloop frequency.

### See the example file, as it covers most interesting things, but I believe it is fairly self-explanatory.

*This is my first library so please be kind.  It is not intended for mission critical mass deployments, more so for personal non-critical non-hazardous projects.  My poor formatting was based on bad habits from my C++ programming course in university in 1998 and hasn't improved since.  I feel the code works great and I generated this library because I couldn't find anything similar and the ones based on loop polling didn't work at all with long loop times.  Interrupts can be a bit cantankerous, but this seems to work nearly flawlessly in my experience, but I imagine maybe not so for everyone and welcome any suggestions for improvements.*




