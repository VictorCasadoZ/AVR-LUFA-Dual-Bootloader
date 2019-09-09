# This is a dual "bootloader" for AVR architecture MCUs, based in LUFA 170418 for Serial and MIDI modes when start-up your board.

## Building and Compiling
 You can recompile the code to meet your requirements, in the Descriptors.c file you can set the Manufacturer, Product, VIDs and PIDs of the serial and MIDI devices
 To build this project, you should have intalled gcc, gcc-avr and Flip(to upload it to the MCU), then download the LUFA 170418 zip and copy the DUALBOOTLOADER file in the Projects folder of the LUFA folder, then go to the DUALBOOTLOADER path, and:(for Windows users)
 ```
 cd C/:..../LUFA-170418/Projects/DUALBOOTLOADER
 make clean
 make
 ```
 
 Start flip, put the MCU in DFU mode(ATmega 16U2 enters in DFU mode by wiring RESET to GND) and upload the code
 
## Utilization
 To start the device in SERIAL MODE, you must place a jumper between the PB4(MOSI of the ATmega 16U2) and GND pins; for MIDI MODE you must place a jumper between the PB4 and the GND pins of the ICSP header(for Arduino boards)
 
This bootloader works as a Serial-USB interface for your ATmega 328p(Arduino UNO MCU), 2560(Arduino MEGA MCU) and any other AVR MCUs as a normal Arduino serial port would, and in MIDI mode it has been succesfully tested with the MIDI.h library without problem(others MIDI bootloaders don't work with this library due to how it communicates, but mine works!)

Thanks to the LUFA project: http://www.fourwalledcubicle.com/LUFA.php and AVRFreaks: https://www.avrfreaks.net/ for the help
