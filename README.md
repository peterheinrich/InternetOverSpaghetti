# IP_Over_Spaghetti
Transmitting IP packages using two spaghetti as transport medium

![Alt text](doc/08_CompleteAssembly.JPG?raw=true "Title")

Actually this is more an RS232 over spaghetti. But using the SLIP protocol and two Linux machines, one can easily tunnel IP over RS232 thus effectively tunneling IP over the spaghetties.

The whole chain looks like this:
SLIP(Linux) -> FTDI (USB<->RS232) -> ATMega328P -> Relais -> Spaghetti -> Optical Sensor -> ATMega328P -> FTDI (USB<->RS232) -> SLIP(Linux)
