# InterruptButton
This is an interrupt based button event library for the ESP32 suitable in the Arduino framework as well as the ESP IDF framework.  It uses the 'onChange' interrupt (rising or falling) for a given pin and the ESP high precision timer to carry the necessary pin-polling to facilitate a simple, but reliable debouncing and timing routines.  Once de-bounced, actions bound to certain button events including 'Key Down', 'Key Up', 'Key Press', 'Long Key Press', 'Auto-Repeat Press', and 'Double-clicks' are available to bind your own functions to.

With a built in user-defined menu/paging system, each button can be bound to multiple layers of different functionality depending on what the user interface is displaying at a given moment.  This means it can work as a simple button, all the way up to a full user interface using a single button with different combinations of special key presses for navigation.

The use of interrupts instead of laborious button polling means that actions bound to the button are NOT limited to the main loop frequency which significantly reduces the chance of missed presses with long main loop durations.

There are 6 Asynchronous events (actioned immediately via intterrupt) , 4 Synchronous events (actioned at main loop) and a menu/paging structure which is only limited to your memory on chip (which is huge for the ESP32).  This means you could attach 10 different functions to a single button per menu level / page (if you dare!)

## It allows for the following:

### Asynchronous Events 
Asynchronous Events are actioned immediately by calling user defined functions attached to specific button events as an Interrupt Service Routine (ISR).
  * **keyDown** - Happens anytime key is depressed (even if held), be it a keyPress, longKeyPress, or a double-click
  * **keyUp** - Happens anytime key is released, be it a keyPress, longKeyPress, end of an AutoRepeatPress, or a double-click
  * **keyPress** - Occurs upon keyUp only if it is not a longKeyPress, AutoRepeatPress, or double-click
  * **longKeyPress** (required press time is user configurable)
  * **AutoRepeatPress** (Rapid fire, if enabled, but not defined, then the standard keyPress action is used)
  * **doubleClick** (max time between clicks is user configurable)
  
Functions applied to the above events should be defined with the IRAM_ATTR attribute to place them in the onboard RAM rather than the flash storage area which is slower and could cause SPI bus clashes though I have not run into this issue yet.  Furthermore, the user should careful calling API functions that are not defined in with IRAM_ATTR which could force the code back into Flash memory.  This is a precautionary measure and a limitation of the chip/frame work (not the libary); however, testing has shown this can still be done without issue in some cases.  For processor intesive routines, use 'Sychronous Events'
  
### Synchronous Events
Synchronous events correspond to the Asynchronous events above, but are actioned when the 'processSyncEvents()' member function is called in the main loop. For this reason, keyUp and keyDown are not included due to the loop timing lag.  Like Asynchronous events, they are actioned by calling user-defined functions as bound to specific button events - Noting that these functions can be defined and bound as Lambda functions.
  * **syncKeyPress**
  * **syncLongKeyPress**
  * **syncAutoRepeatPress**
  * **syncDoubleClick**

These events are based on the same debounce and delay configuration for synchronous events listed above.

Note, you may also do a lower level event management by manually polling the boolean members keyPressOccurred, longKeyPressOccurred, autoRepeatPressOccurred, and doubleClickOccurred to suit your program's requirements and calling functions manually once these values are true.  Remember to clear them once finished by setting to false.

### Multi-page/level events
  This is handy if you have several different GUI pages where all the buttons mean something different on a different page.  
  You can change the menu level of all buttons at once using the static member function 'setMenuLevel(level)'.  Note that you must set the desired number of menus before initialising your first button, as this cannot be changed later (this may be improved later subject to user requests)
  
### Other Features
  * The Asynchronous and Synchronous events can enabled or disabled globally by type (Async and Sync)
  * The timing for debounce, longPress, AutoRepeatPress and doubleClick can be set on a per button instance basis.
  * Asynchronous events are called *Immediately* after debouncing
  * Synchronous events are invoked by calling the 'processSyncEvents()' member function in the main loop and *are subject to the main loop timing.*

### Example Usage
This is an output of the serial port from the example file.  Here just the Serial.Println() function is called, but you can replace that with your own code to do what you need.
```
Menu 0, Button 1: ASYNC Key Down:              7191 ms
Menu 0, Button 1: ASYNC Key Up:                7315 ms
Menu 0, Button 1: ASYNC Key Press:             7515 ms
Menu 0, Button 1: SYNC KeyPress:               [8044 ms]
Menu 0, Button 1: ASYNC Key Down:              12303 ms
Menu 0, Button 1: ASYNC Long Key Press:        13053 ms
Menu 0, Button 1: ASYNC Auto Repeat Key Press: 13303 ms
Menu 0, Button 1: ASYNC Auto Repeat Key Press: 13553 ms
Menu 0, Button 1: ASYNC Auto Repeat Key Press: 13803 ms
Menu 0, Button 1: SYNC LongKeyPress:           [14044 ms]
Menu 0, Button 1: SYNC AutoRepeat Press:       [14044 ms]
Menu 0, Button 1: ASYNC Auto Repeat Key Press: 14053 ms  
Menu 0, Button 1: ASYNC Auto Repeat Key Press: 14303 ms
Menu 0, Button 1: ASYNC Key Up:                14474 ms
Menu 0, Button 1: SYNC AutoRepeat Press:       [15044 ms]
Menu 0, Button 1: ASYNC Key Down:              16115 ms
Menu 0, Button 1: ASYNC Key Up:                16232 ms
Menu 0, Button 1: ASYNC Key Up:                16411 ms
Menu 0, Button 1: ASYNC Double Click: 16411 ms - Changing to Menu Level 1
Menu 0, Button 1: SYNC DoubleClick:            [17044 ms]
Menu 1, Button 1: ASYNC Key Down:              18961 ms
Menu 1, Button 1: ASYNC Key Up:                19104 ms
Menu 1, Button 1: ASYNC Key Press:             19304 ms
Menu 1, Button 1: ASYNC Key Down:              19911 ms
Menu 1, Button 1: SYNC KeyPress:               [20044 ms]
Menu 1, Button 1: ASYNC Long Key Press:        20661 ms
Menu 1, Button 1: ASYNC Auto Repeat Key Press: 20911 ms
Menu 1, Button 1: SYNC LongKeyPress:           [21044 ms]
Menu 1, Button 1: SYNC AutoRepeat Press:       [21044 ms]
Menu 1, Button 1: ASYNC Auto Repeat Key Press: 21161 ms
Menu 1, Button 1: ASYNC Key Up:                21334 ms
Menu 1, Button 1: SYNC AutoRepeat Press:       [22044 ms]
Menu 1, Button 1: ASYNC Key Down:              24802 ms
Menu 1, Button 1: ASYNC Key Up:                24914 ms
Menu 1, Button 1: ASYNC Key Up:                25098 ms
Menu 1, Button 1: ASYNC Double Click: 25098 ms - Changing Back to Menu Level 0
Menu 0, Button 1: ASYNC Key Down:              25960 ms
Menu 1, Button 1: SYNC DoubleClick:            [26044 ms]
Menu 0, Button 1: ASYNC Key Up:                26115 ms
Menu 0, Button 1: ASYNC Key Press:             26315 ms
Menu 0, Button 1: SYNC KeyPress:               [27044 ms]
Menu 0, Button 2: ASYNC Key Down:              34894 ms
Menu 0, Button 2: ASYNC Key Up:                35022 ms
Menu 0, Button 2: ASYNC Key Press:             35234 ms
Menu 0, Button 2: SYNC KeyPress:               [36044 ms]
Menu 0, Button 2: ASYNC Key Down:              44318 ms
Menu 0, Button 2: ASYNC Key Up:                44429 ms
Menu 0, Button 2: ASYNC Key Up:                44619 ms
Menu 0, Button 2: ASYNC Double Click: 44619 ms - Changing to Menu Level 1
Menu 0, Button 2: SYNC DoubleClick:            [45044 ms]
Menu 1, Button 2: ASYNC Key Down:              46000 ms
Menu 1, Button 2: ASYNC Key Press:             46145 ms
Menu 1, Button 2: ASYNC Key Down:              46472 ms
Menu 1, Button 2: ASYNC Key Press:             46597 ms
Menu 1, Button 2: ASYNC Key Down:              46722 ms
Menu 1, Button 2: ASYNC Key Press:             46865 ms
Menu 1, Button 2: SYNC KeyPress:               [47044 ms]
Menu 1, Button 2: ASYNC Key Down:              61035 ms
Menu 1, Button 2: ASYNC Key Press:             61171 ms
Menu 1, Button 2: ASYNC Key Down:              61356 ms
Menu 1, Button 2: ASYNC Key Press:             61476 ms
Menu 1, Button 2: ASYNC Key Down:              61569 ms
Menu 1, Button 2: ASYNC Key Press:             61672 ms
Menu 1, Button 2: ASYNC Key Down:              61768 ms
Menu 1, Button 2: ASYNC Key Press:             61865 ms
Menu 1, Button 2: ASYNC Key Down:              61882 ms
Menu 1, Button 2: ASYNC Key Press:             62034 ms
Menu 1, Button 2: SYNC KeyPress:               [62044 ms]
Menu 1, Button 2: ASYNC Key Down:              62311 ms
Menu 1, Button 2: ASYNC Key Press:             62422 ms
Menu 1, Button 2: ASYNC Key Down:              62502 ms
Menu 1, Button 2: ASYNC Key Press:             62613 ms
Menu 1, Button 2: ASYNC Key Down:              62694 ms
Menu 1, Button 2: ASYNC Key Press:             62777 ms
Menu 1, Button 2: ASYNC Key Down:              62848 ms
Menu 1, Button 2: ASYNC Key Press:             62976 ms
Menu 1, Button 2: SYNC KeyPress:               [63044 ms]
Menu 1, Button 2: ASYNC Key Down:              64009 ms
Menu 1, Button 2: ASYNC Key Press:             64075 ms
Menu 1, Button 2: ASYNC Key Down:              64849 ms
Menu 1, Button 2: SYNC KeyPress:               [65044 ms]
Menu 1, Button 2: ASYNC Long Press: 65599 ms - [NOTE FASTER KEYPRESS RESPONSE IF DOUBLECLICK NOT DEFINED] Changing Back to Menu Level 0
Menu 0, Button 2: ASYNC Auto Repeat Key Press: 65850 ms
Menu 1, Button 2: SYNC LongKeyPress:           [66044 ms]
Menu 0, Button 2: SYNC AutoRepeat Press:       [66044 ms]
Menu 0, Button 2: ASYNC Auto Repeat Key Press: 66100 ms
Menu 0, Button 2: ASYNC Key Up:                66121 ms
Menu 0, Button 2: SYNC AutoRepeat Press:       [67044 ms]
```

## Functional Flow Diagram ##
The flow diagram below shows the basic function of the library.  It is pending an update to include some recent updates and additions such as 'autoRepeatPress'

![This is an image](https://github.com/rwmingis/InterruptButton/blob/d06999159451a2a2e70f43a48129859f3170da28/images/flowDiagram.png)


## Roadmap Forward ##
  * Consider Adding button modes such as momentary, latching, etc.
  * Consider a static member queue common across all instances to maintain order of events (and simplify processing the event actions in the main loop)
  * Consider an RTOS queue.
 
## Known Limitations:
  * Like all ISR's the asynchronous code functions should be LIGHTWEIGHT and FAST
  * The Synchronous routines can be much more robust, but are limited to the main loop frequency

### See the example file, as it covers most interesting things, but I believe it is fairly self-explanatory.

*  This libary should not be used for mission critical or mass deployments.  The developer should satisfy themselves that this library is stable for their purpose.  I feel the code works great and I generated this library because I couldn't find anything similar and the ones based on loop polling didn't work at all with long loop times.  Interrupts can be a bit cantankerous, but this seems to work nearly flawlessly in my experience, but I imagine maybe not so for everyone and welcome any suggestions for improvements.*  

Special thanks to @vortigont for all his input and feedback, particuarly with respect to methodology, implementing the ESP IDF functions to allow this library to work on both platforms, and the suggestion of RTOS queues.
