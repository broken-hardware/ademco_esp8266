# ademco_esp8266
ademco 6148 keypad emulator

The TX/RX codes were completely authored by me through reverse engineering and carry no warranties whatsoever. 

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
so the 4110XM DO pin must be an open-drain/collector type output. All keypads see the same data from 4110XM so they should all 
dim/light the same LEDs and display the same messages.

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

To put them in use, combine the TX and RX code and use some sort of web services or app such that the emulator can be controlled on a
smart phone. I was able to compile the codes into an ESP-01S board and use Blynk to control the keypad.

Some scope shots:
1. TX pattern transmitted ok
![tx_success](https://github.com/user-attachments/assets/d456dae8-4eb6-4554-ad74-d6f59c47daa4)
2. TX patther zoomed in
![tx_success_zoomed_in](https://github.com/user-attachments/assets/5c3e03ba-4287-4d9f-8a41-ffe4e09d2fb7)
3. TX abort and re-transmit
![tx_abort_re_send](https://github.com/user-attachments/assets/69dff075-9e5e-4fa9-a8dd-1f1d3c89f0af)
4. TX abort zoomed in
![tx_abort_zoomed_in](https://github.com/user-attachments/assets/4c8a8b76-a381-43a7-93c5-e204ef2543ef)
5. Typical RX packet
![rx_typical_packet](https://github.com/user-attachments/assets/3ce72683-ee2a-4c3c-921f-345c5dae3a07)

A crude schematics showing how an ESP-01S is wired to the 4110XM/6148 system:
<img width="1087" alt="Screen Shot 2025-02-08 at 8 03 49 PM" src="https://github.com/user-attachments/assets/94135ca8-1922-4327-9bf9-b1809fc9cb52" />





