# MotorAlarm
The MotorAlarm is an temprature alarm for combustion engines. I made it for the diesel
engine of my boat. The temprature sensor DS18B20 has a metal shell and can be put in 
the cooling water outlet of the engine.
The project is based on an Arduino pro mini (a future version may be based on an
atmel 328 chip). There is a schematic and board in eagle. There is also a openScad
file so you can print the case.
The display is a sainsmart 1.8" tft.
When the engine is not running the display displays the wind in knots and Beaufort
and also draws a graph of the wind trend.
The engine running or not is detected by a 12V (12V=engine running) input to an 
optocoupler to protect the arduino.
The wind data is received from a NASA NMEA 0183 wind head.
The alarm output is with an open collector pnp transistor. So you can add a 12V 
buzzer.
There is no voltage regulator in the board, I have a seperate 5V net onboard for
the arduino gatgets.