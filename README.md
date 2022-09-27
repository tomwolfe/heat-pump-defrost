# heat-pump-defrost
Arduino esp32 project to defrost window AC's used as heat pumps. In development.
Arduino uno branch tested and working [demo on youtube](https://www.youtube.com/watch?v=FBvkNjNMhIM).

# parts
* esp32-wroom-32d ($3 aliexpress)
* 20A (or more) relay ($15 amazon)
* passive buzzer (Arduino kit ~$30)
* LCD1602 w/i2c ($5 aliexpress)
* 3x dht11 temperature sensors ($11 amazon)
* button (Arduino kit ~$30)
* wires (Arduino kit ~$30) and sufficient gauge wire for relay.
* 16 awg male spade crimp connectors (to connect relay to compressor) ~$1.50/ebay

# AC setup
Safety first: Disconnect power and then discharge your capacitor before connecting relay. If your unit is mounted with the electronics outside, an awning or something to protect the electronics would be a good idea. Mine is mounted inside with ducting.

I connected the relay between the compressor and the AC's power switch, replacing the temperature control switch (optional to bypass, but it'll likely shut off the compressor around 60F rather than 35F). Similar video example in the credits below.

# How it works
One temperature sensor will be for ambient, one for outdoor and one for heat from the exhaust. When the temperature differential between ambient and exhaust becomes too low, it turns off the compressor and just runs the fan to defrost. It'll also turn off the compressor if the outdoor temperature is too low or if the target temperature (minus setback) is reached.

The button switch can change temperature with a single click, sets temperature to 55F with a double click and turns off audible alarms with long presses.

# Why
Heat pumps are super efficient above 40F or so. I'm in a rental house so I can't install a split/mini split system. I'm mostly looking to save on my liquid propane bill since right now (Aug 2021) propane is about $1.87/gal and my time-of-use electric is between $0.06-0.12/kWh. [calculator for heating cost](https://www.efficiencymaine.com/at-home/heating-cost-comparison/). It could potentially save a few hundred dollars per year on heating until it gets below freezing (Northen Wisconsin here). Seemed worth the $50 in parts and it's kind of a fun project. Win win.

# Issues
The LCD display gets corrupted after turning on and off the relay. My relay does have a flyback diode so I don't think it's that. It's also optically isolated. I've tried powering from a different circuit than the air conditioner, twisting the positive and negative wires to the relay from the Arduino and wrapping the Arduino in foil to act as a faraday cage. None of that helped. I could try a separate power supply for the relay, a solid state relay, running the LCD in 8-bit mode rather than 4-bit or putting a filtering capacitor on the clock of the LCD. I was able to solve it with a software hack (see function displayReset()).

# credits
Idea from [Franklin Hu](https://www.youtube.com/watch?v=wpsMVukBvG0&t=152s).

# EasyEDA PCB for esp32-wroom-32d
Update 9/22/22:

[EasyEDA PCB](https://u.easyeda.com/tomwolfe/endall)

Updated code coming soon for esp32/Arduino cloud. wifi + arduino cloud = control over the cloud

I have a few extra PCBs from above that I can sell, contact me.

# Setup
[drivers for esp32](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads)
