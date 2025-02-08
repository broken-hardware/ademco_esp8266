# ademco_esp8266
ademco 6148 keypad emulator

I have a home alarm system installed in 2007. It is based on Ademco 4110XM controller with 3 6148 keypads and various sensors.

This system uses a land line to communicate with a central monitoring station where a real person will check the notifictions sent 
by the 4110XM controller. Usually they will call me in 2 minutes and inform that a particular alarm/notification was triggered and 
if everything was ok.

Users input commands (key sequencies) on the keypads and if the 4110XM recognized the commands, it will config the system in 
proper mode, for example STAY or AWAY alarm mode, etc. At the same time, the 4110XM also sends commands to the keypads to proplerly
dim or light LEDs, displaying messages, etc.

The 4110XM controller worked as designed but is outdated and has no upgrade path to IP based notifications.

This project is to use a dinky ESP-01S board to make the existing alarm system an IoT. The hardware and software had to be reverse-
engineered to achieve the objective as very limited information is available on the web.

System architecture:
Aside from power, ground, sensor pins, the 4110XM use DI and DO to communicate with the keypads. All keypads are wired in parallel
so the 4110XM DO pin must be an open-drain type output. All keypads see the same data from 4110XM so they should all dim/light the 
same LEDs and display the same messages.

DI pin: Input of 4110XM. This is also the output of the keypad. Let's call it TX. Signaling is 12V and output is 0V when there is 
nothing to send. There are 12 keys on the keypad: 0-9, #, *. Each key is encoded with an 8-bit patter. When a key is pressed, the 
specific bit pattern is sent 3 times without gaps. Baud rate is 2300 as described in the manual and it correct. Bit patterns can be 
easily captured with a scope. TX patterns can only be sent at certain moments called TX window and TX starts at 4ms after TX window
begins. Patterns sent outside of TX window are ignored by 4110XM.

DO pin: Outputs from 4110XM. This is the input of the keypad. Let's call it RX. Singnaling is 12V and stays at 12V when iding. 
However it will cycle low at ~6Hz for about 14.5ms which is the TX window. In addition, the 4110XM periodically sends status or
response messages on the same line every ~6s. This overrides TX window. With a scope, the 4110XM message is found to be made of the 
following bits:

1. Starting condition
   It begins with 14 0s and 14 1s at 2300 baud.

2. Dwell time
   It stays low for roughly 3 bit time (435us x 3 = 1.3ms) but can vary up to 360us

3. Header byte
   11011111. Note that since the endianness is unknown, the bits are arranged as seen on the scope for easy human reading

4. 6 bytes of payload followed by all 1s

So the keypad emulator can be split into 2 designs: TX and RX.

TX challenges:
1. Need to detect TX window before sending patterns. TX window is assumed when RX is low for 4ms.
2. However the starting condition is 6ms low followed by 6ms high. So it is possible the starting condition was mistaken for TX window.
3. When this happens, TX is aborted and will re-transmit the pattern when the next TX window arrives.

RX challenge:
1. Very few info can be found regarding encoding and protocol of the 4110XM data packets so only a few essential functions were decoded.
2. Since 2300 baud rate is not popular, the built-in hardware serial or software serial can't be used. Bit banging is used instead. 


