Need a way to control AL5WN-K 5V latching relay with Atmega.

Nice thing about latching relay is it stays at its position, and does not
require power once switched.
The AL5WN in particular is nice because it is the smallest latching signal relay
I could find: just 5.4mm high, 9.1mm wide and 14.15mm long.
Perhaps another relay was sufficient for my purposes though, I did not have
experience.

JeeLabs shows a flexible approach that may work with many different relays?
Use two lines from the uC to get enough current, ie. 4 pins total.

The AL5WN however can be driven from any two of the digital lines on my 3.3V
JeeNode. No need for a second line, yet?--It's not within spec.

Also does not work from analog lines.

All in all in general, researching this I find that diode's are added for protection, and 
that to reverse the current a H-bridge is needed.

Over at Stompville a whole microcontroller (attiny) is dedicated to switching (at 5v)
a latching relay. Seems at 5V no probs with the single line approach there too.
http://stompville.co.uk/?p=260

This longish expose on electronics features the simplest "H bridge" I have found so far.
http://www.talkingelectronics.com/projects/200TrCcts/101-200TrCcts.html

I was looking for a way to control transistors, to get the proper
voltage/current. The H-bridges I tried to build with the transistors at hand
failed, and I need more experience there too.

However, two NPN and what I call a pull-up to the power rail is enough too.
I am now able to switch the relay using just two analog lines also.

