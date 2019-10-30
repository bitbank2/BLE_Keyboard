Nano 33 BLE Keyboard<br>
Project started 10/28/2019<br>
Copyright (c) 2019 BitBank Software, Inc.<br>
Written by Larry Bank<br>
bitbank@pobox.com<br>
<br>
![BBQ10 Keyboard](/bbq10kbd.jpg?raw=true "BBQ10KBD PMOD")
<br>
The purpose of this code is to use the Arduino 33 BLE along with the BBQ10 keyboard
to create a functional BLE HID keyboard. The challenge is that the current Arduino
33 BLE library code does not contain any HID support, so all of the services and
characteristics that must be implemented to create an HID keyboard have to be done
manually in this code. So far, I've implemented the communication with the I2C
keyboard and most of the BLE code that needs to be done, but something is still
not right because connecting to it causes my Mac Bluetooth Preferences panel to
crash. I will continue to work on this code and if anyone can point out problems
or make contributions it would be appreciated.
<br>

