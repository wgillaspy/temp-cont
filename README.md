# temp(erature)-cont(roller)

An very small atemga328p based temperature controller that can be used to incubate animals :lizard: :honeybee: :hatching_chick: and plants :rose: :cactus:. 

It uses up to five ds18b20 temperature probes connected into a single audio jack to monitor temperature.
Heating can be provided through a 5v-12vDC heating source.  I use the 12v rail of a spare computer power supply.

The board connects to any computer over usb serial and prints the temperature statuses in Json about every second.  The 
default target temperature can be changed while running and is stored in EEPROM to survive power offs.  The target temperature 
can also be set without writing to EEPROM.  This is useful when using a secondary application to maintain a temperature profile.
Right now the temperature can only be set with integers.
 
Programming of the atmega is done over a mini-usb (ftdi) or via the isp headers.

##### Setting the target temperature

Set the target temperature to 91F and write the value to EEPROM to persist across power offs.
```json
{"a":"tgt","tgt":91} 
```

Set the target temperature to 100F and **do not** write the value to EEPROM.  The target temperature will revert back to what is stored in EEPROM if powered off and back on.
```json
{"a":"tgt_tmp","tgt":100}
``` 

##### Reading the output
Every second or so, the board sends a json string so you know the status of your incubator.  t0 and t1 are the two 
temperatures probes (the code allows up to five).  Avg is the probe's temperature averaged together.  It's the temperature
used to compare against the tgt value.  Ht is whether the heater mostfet is on or not.  And fan is the number of pulses received between readings.  If the pulse fall to zero, the heating element will be disabled to prevent overheating.
```json
{ "tgt": "81",
  "t0": "80.60",
  "t1": "81.50",
  "avg": "81.05",
  "ht": "0",
  "fan": "126" }
``` 
 

Top layer of the board, about life size.  
![Board Top](./resources/board.png)

The controller board is hooked up to a raspberry pi running the [nodejs controller](nodejs/src/main/controller) container which
maintains a temperature profile and sends the output of the board to a personal splunk instance for graphing.   
![SplunkPanel](./resources/splunk.png)

How updates are deployed.

- Board code is tested locally on the workstation and once confirmed as working then
- The code is pushed to github.
- The github webhook calls out to Jenkins on the same private network as the temperature controller.
- The Jenkins instance creates a build container
- The build container then packages up the arduino code into an [avr-dude container](https://github.com/wgillaspy/arm64v8-avrdude) and sends it to a raspberry pi that the temperature controller is connected to via usb.
- The build container requests that the container start then
- At container start, the temperature controller code is compiled within the avr-dude container on the remote raspberry pi and then deployed to the temperature controller board.

![Deploy](./resources/deployment.png)

Why the additional complexity with docker?  Mostly to take advantage of an immutable system that is easy to recover.
If the raspberry pi SD fails (which they infrequently do) I can quickly stand up a new raspberry pi with the docker REST api exposed using a simple ansible script.
Once the REST api is exposed and the raspberry pi is placed back on the network, jenkins can deploy both the temperature controller board code (if necessary) and the application that manages the temperature profile.

Since I don't keep my incubator near my workstation, having a deployment pipeline that can update both the firmware and
secondary application that controls the temperature profile to remote locations is super awesome.  I don't have to lug
my laptop around just to add a feature or set a default temperature.


Hopefully some portion of this project will be useful to you :)  

#### New Things
- [x] Add SMS notifications.
- [x] Update the NodeJs application to write to a splunk endpoint. 

#### :building_construction: Board changes in [feature/new-board-layout](https://github.com/wgillaspy/temp-cont/tree/feature/new-board-layout)
- [x] Remove the serial display jumper in favor of another 5V fan jumper.  
- [x] Add an additional mosfet for the additional fan.
- [x] Better isolate the two fan jumpers, provide their own 5v separate from atmega board to reduce noise.  
- [x] Move the mosfet gate pins onto analog output pins.  Right now the heater can only be off or on.
- [x] Remove the ftdi programmer pads from the bottom of the board.
- [x] Add a jumper for the heater mosfet gate that can be used to drive an external AC mosfet. (feature/new-board-layout)

#### Upcoming board changes
- [ ] Convert the mini usb connector to usb c.
- [ ] Update the BOM and pad sizes to be consistent across components.
- [ ] Does 0402 makes sense?  
- [ ] Retrace the board to reduce overall board size.
- [ ] Figure out compatibility issues with atmega328pb


      

