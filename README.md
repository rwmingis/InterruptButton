# InterruptButton
This is an interrupt based button event library for the ESP32. 

It allows for the following:

Synchronous Events (actioned inside of an ISR)
  keyDown
  keyUp
  keyPress (combination of keyDown and keyUp)
  longKeyPress (required press time is user configurable)
  doubleClick (max time between clicks is user configurable)
  
Functions applied to the above events should be defined with the IRAM_ATTR attribute to place them in the onboard RAM rather than the flash storage area which is much slower.
  
Asynchronous Events (actioned when 'processAsyncEvents()' is called in the main loop and have the same attributes as synchronous events listed above
  keyPress
  longKeyPress
  doubleClick

Multi-page/level events
  This is handy if you several different GUI pages where all the differnt events mean something different.  
  If you change the page, you can change all button behavior at once by simply changing the menu level programmatically


Known Limitations:
  While the ISR code is generally placed in the ram area using the IRAM_ATTR attribute, there are some Arduina and ESP platform functions called from within the ISR
  This may place ISR code back in the flash and potentially cause issues if the SPI bus is busy, but so far, hasn't caused any issues on my testing and implementation.
  Like all ISR's the synchronous code functions should be LIGHTWEIGHT and FAST
  The Asynchronous routines can be much more robust, but are limited to the mainloop frequency.
  
This is my first library so please be kind.  My poor formatting was based on bad habits from my C++ programming course in university in 1998 and hasn't improved since.  I feel the code works great and I generated this library because I couldn't find anything similar and the ones based on loop polling didn't work at all with long loop times.  Interrupts can be a bit cantankerous, but this seems to work nearly flawlessly in my experience, but I imagine maybe not so for everyone and welcome any suggestions for improvements.




