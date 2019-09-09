/*
             LUFA Library
     Copyright (C) Dean Camera, 2017.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2017  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for USBtoSerial.c.
 */

#ifndef _USB_SERIAL_H_
#define _USB_SERIAL_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/interrupt.h>
		#include <avr/power.h>
		#include <stdbool.h>

		#include "Descriptors.h"

		#include <LUFA/Drivers/Board/LEDs.h>
		#include <LUFA/Drivers/Peripheral/Serial.h>
		#include <LUFA/Drivers/Misc/RingBuffer.h>
		#include <LUFA/Drivers/USB/USB.h>
		#include <LUFA/Platform/Platform.h>

	/* Macros: */
		/** LED mask for the library LED driver, to indicate that the USB interface is not ready. */
		#define LEDMASK_USB_NOTREADY      LEDS_LED1

		/** LED mask for the library LED driver, to indicate that the USB interface is enumerating. */
		#define LEDMASK_USB_ENUMERATING  (LEDS_LED2 | LEDS_LED3)

		/** LED mask for the library LED driver, to indicate that the USB interface is ready. */
		#define LEDMASK_USB_READY        (LEDS_LED2 | LEDS_LED4)

		/** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
		#define LEDMASK_USB_ERROR        (LEDS_LED1 | LEDS_LED3)

	/* Function Prototypes: */
		void SetupHardware(void);

		void MIDI_To_Arduino(void);
		void MIDI_To_Host(void);
	
		typedef enum
		{
			InvalidType           = 0x00,    ///< For notifying errors
			NoteOff               = 0x80,    ///< Note Off
			NoteOn                = 0x90,    ///< Note On
			AfterTouchPoly        = 0xA0,    ///< Polyphonic AfterTouch
			ControlChange         = 0xB0,    ///< Control Change / Channel Mode
			ProgramChange         = 0xC0,    ///< Program Change
			AfterTouchChannel     = 0xD0,    ///< Channel (monophonic) AfterTouch
			PitchBend             = 0xE0,    ///< Pitch Bend
			SystemExclusive       = 0xF0,    ///< System Exclusive
			TimeCodeQuarterFrame  = 0xF1,    ///< System Common - MIDI Time Code Quarter Frame
			SongPosition          = 0xF2,    ///< System Common - Song Position Pointer
			SongSelect            = 0xF3,    ///< System Common - Song Select
			TuneRequest           = 0xF6,    ///< System Common - Tune Request
			Clock                 = 0xF8,    ///< System Real Time - Timing Clock
			Start                 = 0xFA,    ///< System Real Time - Start
			Continue              = 0xFB,    ///< System Real Time - Continue
			Stop                  = 0xFC,    ///< System Real Time - Stop
			ActiveSensing         = 0xFE,    ///< System Real Time - Active Sensing
			SystemReset           = 0xFF,    ///< System Real Time - System Reset
		} MidiMessageType;

		uint8_t		mRunningStatus_RX;
		uint8_t		mRunningStatus_TX;
		uint8_t		mPendingMessage[3];
		uint8_t		mPendingMessageExpectedLength;
		uint8_t		mPendingMessageIndex;
		bool		mPendingMessageValid;
		
		MIDI_EventPacket_t mCompleteMessage;

		MidiMessageType getStatus(MidiMessageType inType, uint8_t inChannel);
		MidiMessageType getTypeFromStatusByte(uint8_t inStatus);
		uint8_t getChannelFromStatusByte(uint8_t inStatus);
		bool isChannelMessage(uint8_t inType);
		
		void EVENT_USB_Device_Connect(void);
		void EVENT_USB_Device_Disconnect(void);
		void EVENT_USB_Device_ConfigurationChanged(void);
		void EVENT_USB_Device_ControlRequest(void);

		void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo);

#endif

