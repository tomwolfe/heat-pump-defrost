# heat-pump-defrost
Arduino project to defrost window AC's used as heat pumps.
Tested and working [demo on youtube](https://www.youtube.com/watch?v=FBvkNjNMhIM).
It's my first Arduino project and I'm definitely a noob with hardware/electronics.

# parts
* Arduino uno r3 (Arduino kit ~$30)
* relay ($6 amazon)
* passive buzzer (Arduino kit ~$30)
* LCD1602 (Arduino kit ~$30)
* 3x dht11 temperature sensors ($11 amazon)
* button (Arduino kit ~$30)
* trimpot (Arduino kit ~$30)
* development breakout breadboard (Arduino kit ~$30)
* wires (Arduino kit ~$30)
* 16 awg male spade crimp connectors (to connect relay to compressor) ~$1.50/ebay

estimated cost, about $50. Probably less if you go with a pro mini or already have parts.

# breadboard setup
![v2_bb](https://user-images.githubusercontent.com/620331/129489193-ff17e5d4-de0d-43f2-bca9-d625eb382157.png)
might need a resistor for your temperature sensors (I used dht11's with built-in resistors) and for your LCD. On the relay connect COM pin and NC pin (normally closed, high level trigger select) on the relay jumper.

# AC setup
Safety first: Disconnect power and then discharge your capacitor before connecting relay. If your unit is mounted with the electronics outside, an awning or something to protect the electronics would be a good idea. Mines mounted inside with ducting.

I connected the relay between the compressor and the AC's power switch, bypassing the temperature control relay (optional to bypass, but it'll likely shut off the compressor around 60F rather than 35F). Similar video example in the credits below.

# How it works
One temperature sensor will be for ambient, one for outdoor and one for heat from the exhaust. When the temperature differential between ambient and exhaust becomes too low, it turns off the compressor and just runs the fan to defrost. It'll also turn off the compressor if the outdoor temperature is too low or if the target temperature (minus setback) is reached.

The button switch can change temperature with a single click, sets temperature to 55F with a double click and turns off audible alarms with long presses.

# Why
Heat pumps are super efficient above 40F or so. I'm in a rental house so I can't install a split/mini split system. I'm mostly looking to save on my liquid propane bill since right now (Aug 2021) propane is about $1.87/gal and my time-of-use electric is between $0.06-0.12/kWh. [calculator for heating cost](https://www.efficiencymaine.com/at-home/heating-cost-comparison/). It could potentially save a few hundred dollars per year on heating until it gets below freezing (Northen Wisconsin here). Seemed worth the $50 in parts and it's kind of a fun project. Win win.

# Issues
The LCD display gets corrupted after turning on and off the relay. My relay does have a flyback diode so I don't think it's that. It's also optically isolated. I've tried powering from a different circuit than the air conditioner, twisting the positive and negative wires to the relay from the Arduino and wrapping the Arduino in foil to act as a faraday cage. None of that helped. I could try a separate power supply for the relay, a solid state relay, running the LCD in 8-bit mode rather than 4-bit or putting a filtering capacitor on the clock of the LCD. I was able to solve it with a software hack (see function displayReset()).

# credits
Idea from [Franklin Hu](https://www.youtube.com/watch?v=wpsMVukBvG0&t=152s).
