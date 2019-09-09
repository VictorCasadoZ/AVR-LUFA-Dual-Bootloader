# This is a dual "bootloader" for AVR architecture MCUs, based in LUFA 170418 for Serial and MIDI modes when start-up your board.

## Building and Compiling
 You can recompile the code to meet your requirements, in the Descriptors.c file you can set the Manufacturer, Product, VIDs and PIDs of the serial and MIDI devices
 
## Utilization
 To start the device in SERIAL MODE, you must place a jumper between the PB4(MOSI of the ATmega 16U2) and GND pins; for MIDI MODE you must place a jumper between the PB4 and the GND pins of the ICSP header(for Arduino boards)
 
This bootloader works as a Serial-USB interface for your ATmega 328p(Arduino UNO MCU), 2560(Arduino MEGA MCU) and any other AVR MCUs as a normal Arduino serial port would, and in MIDI mode it has been succesfully tested with the MIDI.h library without problem(others MIDI bootloaders don't work with this library due to how it communicates, but mine works!)

Thanks to the LUFA project: http://www.fourwalledcubicle.com/LUFA.php and AVRFreaks: https://www.avrfreaks.net/ for the help
