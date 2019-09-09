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
 *  Main source file for the USBtoSerial project. This file contains the main tasks of
 *  the project and is responsible for the initial application hardware configuration.
 */

#include "USBtoSerial.h"

int mode = 1; //0:Serial --- 1:MIDI

uint16_t tx_ticks = 0; 
uint16_t rx_ticks = 0; 
const uint16_t TICK_COUNT = 5000; 

/** Circular buffer to hold data from the host before it is sent to the device via the serial port. */
static RingBuffer_t USBtoUSART_Buffer;

/** Underlying data buffer for \ref USBtoUSART_Buffer, where the stored bytes are located. */
static uint8_t      USBtoUSART_Buffer_Data[128];

/** Circular buffer to hold data from the serial port before it is sent to the host. */
static RingBuffer_t USARTtoUSB_Buffer;

/** Underlying data buffer for \ref USARTtoUSB_Buffer, where the stored bytes are located. */
static uint8_t      USARTtoUSB_Buffer_Data[128];

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber         = INTERFACE_ID_CDC_CCI,
				.DataINEndpoint                 =
					{
						.Address                = CDC_TX_EPADDR,
						.Size                   = CDC_TXRX_EPSIZE,
						.Banks                  = 1,
					},
				.DataOUTEndpoint                =
					{
						.Address                = CDC_RX_EPADDR,
						.Size                   = CDC_TXRX_EPSIZE,
						.Banks                  = 1,
					},
				.NotificationEndpoint           =
					{
						.Address                = CDC_NOTIFICATION_EPADDR,
						.Size                   = CDC_NOTIFICATION_EPSIZE,
						.Banks                  = 1,
					},
			},
	};


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();
	
	if(mode == 0){
		RingBuffer_InitBuffer(&USBtoUSART_Buffer, USBtoUSART_Buffer_Data, sizeof(USBtoUSART_Buffer_Data));
		RingBuffer_InitBuffer(&USARTtoUSB_Buffer, USARTtoUSB_Buffer_Data, sizeof(USARTtoUSB_Buffer_Data));

		LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
		GlobalInterruptEnable();

		for (;;)
		{
			/* Only try to read in bytes from the CDC interface if the transmit buffer is not full */
			if (!(RingBuffer_IsFull(&USBtoUSART_Buffer)))
			{
				int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

				/* Store received byte into the USART transmit buffer */
				if (!(ReceivedByte < 0))
				  RingBuffer_Insert(&USBtoUSART_Buffer, ReceivedByte);
			}

			uint16_t BufferCount = RingBuffer_GetCount(&USARTtoUSB_Buffer);
			if (BufferCount)
			{
				Endpoint_SelectEndpoint(VirtualSerial_CDC_Interface.Config.DataINEndpoint.Address);

				/* Check if a packet is already enqueued to the host - if so, we shouldn't try to send more data
				 * until it completes as there is a chance nothing is listening and a lengthy timeout could occur */
				if (Endpoint_IsINReady())
				{
					/* Never send more than one bank size less one byte to the host at a time, so that we don't block
					 * while a Zero Length Packet (ZLP) to terminate the transfer is sent if the host isn't listening */
					uint8_t BytesToSend = MIN(BufferCount, (CDC_TXRX_EPSIZE - 1));

					/* Read bytes from the USART receive buffer into the USB IN endpoint */
					while (BytesToSend--)
					{
						/* Try to send the next byte of data to the host, abort if there is an error without dequeuing */
						if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface,
												RingBuffer_Peek(&USARTtoUSB_Buffer)) != ENDPOINT_READYWAIT_NoError)
						{
							break;
						}

						/* Dequeue the already sent byte from the buffer now we have confirmed that no transmission error occurred */
						RingBuffer_Remove(&USARTtoUSB_Buffer);
					}
				}
			}

			/* Load the next byte from the USART transmit buffer into the USART if transmit buffer space is available */
			if (Serial_IsSendReady() && !(RingBuffer_IsEmpty(&USBtoUSART_Buffer)))
			  Serial_SendByte(RingBuffer_Remove(&USBtoUSART_Buffer));

			CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
			USB_USBTask();
		}
	} else if (mode == 1){
		GlobalInterruptEnable();

		sei();

		for (;;)
		{
			if (tx_ticks > 0) 
			{
				tx_ticks--;
			}
			else if (tx_ticks == 0)
			{
				LEDs_TurnOffLEDs(LEDS_LED2);
			}
										
			if (rx_ticks > 0)
			{
				rx_ticks--;
			}
			else if (rx_ticks == 0)
			{
				LEDs_TurnOffLEDs(LEDS_LED1);
			}
				
			MIDI_To_Arduino();
			MIDI_To_Host();

			USB_USBTask();
		}
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	DDRB = 0x00;
	PORTB = 0x04;
	
	if ((PINB & 0x04) == 1){
		mode = 1;
	} else if ((PINB & 0x04) == 0){
		mode = 0;
	}
	
	if(mode == 0){
		#if (ARCH == ARCH_AVR8)
			/* Disable watchdog if enabled by bootloader/fuses */
			MCUSR &= ~(1 << WDRF);
			wdt_disable();

			/* Disable clock division */
			clock_prescale_set(clock_div_1);
		#endif

		/* Hardware Initialization */
		LEDs_Init();
		USB_Init();
	} else if (mode == 1){
		// Disable watchdog if enabled by bootloader/fuses
		MCUSR &= ~(1 << WDRF);
		wdt_disable();

		Serial_Init(31250, false);

		// Start the flush timer so that overflows occur rapidly to
		// push received bytes to the USB interface
		TCCR0B = (1 << CS02);
				
		// Serial Interrupts
		UCSR1B = 0;
		UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));

		// https://github.com/ddiakopoulos/hiduino/issues/13
		/* Target /ERASE line is active HIGH: there is a mosfet that inverts logic */
		// These are defined in the makefile... 
		AVR_ERASE_LINE_PORT |= AVR_ERASE_LINE_MASK;
		AVR_ERASE_LINE_DDR |= AVR_ERASE_LINE_MASK; 

		// Disable clock division
		clock_prescale_set(clock_div_1);

		LEDs_Init();
		USB_Init();
	}
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;
	if (mode == 0){
		/* Setup Serial Data Endpoints */
		ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
	} else if (mode == 1){
		/* Setup MIDI Data Endpoints */
		ConfigSuccess &= Endpoint_ConfigureEndpoint(MIDI_STREAM_IN_EPADDR, EP_TYPE_BULK, MIDI_STREAM_EPSIZE, 1);
		ConfigSuccess &= Endpoint_ConfigureEndpoint(MIDI_STREAM_OUT_EPADDR, EP_TYPE_BULK, MIDI_STREAM_EPSIZE, 1);
	}

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

///////////////////////////////////////////////////////////////////////////////
// MIDI Worker Functions
///////////////////////////////////////////////////////////////////////////////

// From Arduino/Serial to USB/Host 
void MIDI_To_Host(void)
{
	// Device must be connected and configured for the task to run
	if (USB_DeviceState != DEVICE_STATE_Configured) return;

	// Select the MIDI IN stream
	Endpoint_SelectEndpoint(MIDI_STREAM_IN_EPADDR);

	if (Endpoint_IsINReady())
	{
		if (mPendingMessageValid == true)
		{
			mPendingMessageValid = false;

			// Write the MIDI event packet to the endpoint
			Endpoint_Write_Stream_LE(&mCompleteMessage, sizeof(mCompleteMessage), NULL);

			// Clear out complete message
			memset(&mCompleteMessage, 0, sizeof(mCompleteMessage)); 

			// Send the data in the endpoint to the host
			Endpoint_ClearIN();

			LEDs_TurnOnLEDs(LEDS_LED2);
			tx_ticks = TICK_COUNT; 
		}
	}

}

// From USB/Host to Arduino/Serial
void MIDI_To_Arduino(void)
{
	// Device must be connected and configured for the task to run
	if (USB_DeviceState != DEVICE_STATE_Configured) return;

	// Select the MIDI OUT stream
	Endpoint_SelectEndpoint(MIDI_STREAM_OUT_EPADDR);

	/* Check if a MIDI command has been received */
	if (Endpoint_IsOUTReceived())
	{
		MIDI_EventPacket_t MIDIEvent;

		/* Read the MIDI event packet from the endpoint */
		Endpoint_Read_Stream_LE(&MIDIEvent, sizeof(MIDIEvent), NULL);

		// Passthrough to Arduino
		Serial_SendByte(MIDIEvent.Data1);
		Serial_SendByte(MIDIEvent.Data2); 
		Serial_SendByte(MIDIEvent.Data3); 

		LEDs_TurnOnLEDs(LEDS_LED1);
		rx_ticks = TICK_COUNT;

		/* If the endpoint is now empty, clear the bank */
		if (!(Endpoint_BytesInEndpoint()))
		{
			/* Clear the endpoint ready for new packet */
			Endpoint_ClearOUT();
		}
	}

}

/** ISR to manage the reception of data from the serial port, placing received bytes into a circular buffer
 *  for later transmission to the host.
 */
ISR(USART1_RX_vect, ISR_BLOCK)
{
	if(mode == 0){
		uint8_t ReceivedByte = UDR1;

		if ((USB_DeviceState == DEVICE_STATE_Configured) && !(RingBuffer_IsFull(&USARTtoUSB_Buffer)))
		  RingBuffer_Insert(&USARTtoUSB_Buffer, ReceivedByte);
	} else if (mode == 1){
		// Device must be connected and configured for the task to run
	if (USB_DeviceState != DEVICE_STATE_Configured) return;

	const uint8_t extracted = UDR1;

	// Borrowed + Modified from Francois Best's Arduino MIDI Library
	// https://github.com/FortySevenEffects/arduino_midi_library
    if (mPendingMessageIndex == 0)
    {
        // Start a new pending message
        mPendingMessage[0] = extracted;

        // Check for running status first
        if (isChannelMessage(getTypeFromStatusByte(mRunningStatus_RX)))
        {
            // Only these types allow Running Status

            // If the status byte is not received, prepend it to the pending message
            if (extracted < 0x80)
            {
                mPendingMessage[0]   = mRunningStatus_RX;
                mPendingMessage[1]   = extracted;
                mPendingMessageIndex = 1;
            }
            // Else we received another status byte, so the running status does not apply here.
            // It will be updated upon completion of this message.
        }

        switch (getTypeFromStatusByte(mPendingMessage[0]))
        {
            // 1 byte messages
            case Start:
            case Continue:
            case Stop:
            case Clock:
            case ActiveSensing:
            case SystemReset:
            case TuneRequest:
                // Handle the message type directly here.
            	mCompleteMessage.Event 	 = MIDI_EVENT(0, getTypeFromStatusByte(mPendingMessage[0]));
                mCompleteMessage.Data1   = mPendingMessage[0];
                mCompleteMessage.Data2   = 0;
                mCompleteMessage.Data3   = 0;
                mPendingMessageValid  	 = true;

                // We still need to reset these
                mPendingMessageIndex = 0;
                mPendingMessageExpectedLength = 0;

                return;
                break;

            // 2 bytes messages
            case ProgramChange:
            case AfterTouchChannel:
            case TimeCodeQuarterFrame:
            case SongSelect:
                mPendingMessageExpectedLength = 2;
                break;

            // 3 bytes messages
            case NoteOn:
            case NoteOff:
            case ControlChange:
            case PitchBend:
            case AfterTouchPoly:
            case SongPosition:
                mPendingMessageExpectedLength = 3;
                break;

            case SystemExclusive:
                break;

            case InvalidType:
            default:
                // Something bad happened
                break;
        }

        if (mPendingMessageIndex >= (mPendingMessageExpectedLength - 1))
        {
            // Reception complete
            mCompleteMessage.Event = MIDI_EVENT(0, getTypeFromStatusByte(mPendingMessage[0]));
            mCompleteMessage.Data1 = mPendingMessage[0]; // status = channel + type
 			mCompleteMessage.Data2 = mPendingMessage[1];

            // Save Data3 only if applicable
            if (mPendingMessageExpectedLength == 3)
                mCompleteMessage.Data3 = mPendingMessage[2];
            else
                mCompleteMessage.Data3 = 0;

            mPendingMessageIndex = 0;
            mPendingMessageExpectedLength = 0;
            mPendingMessageValid = true;
            return;
        }
        else
        {
            // Waiting for more data
            mPendingMessageIndex++;
        }
    }
    else
    {
        // First, test if this is a status byte
        if (extracted >= 0x80)
        {
            // Reception of status bytes in the middle of an uncompleted message
            // are allowed only for interleaved Real Time message or EOX
            switch (extracted)
            {
                case Clock:
                case Start:
                case Continue:
                case Stop:
                case ActiveSensing:
                case SystemReset:

                    // Here we will have to extract the one-byte message,
                    // pass it to the structure for being read outside
                    // the MIDI class, and recompose the message it was
                    // interleaved into. Oh, and without killing the running status..
                    // This is done by leaving the pending message as is,
                    // it will be completed on next calls.
           		 	mCompleteMessage.Event = MIDI_EVENT(0, getTypeFromStatusByte(extracted));
            		mCompleteMessage.Data1 = extracted;
                    mCompleteMessage.Data2 = 0;
                    mCompleteMessage.Data3 = 0;
                   	mPendingMessageValid   = true;
                    return;
                    break;
                default:
                    break;
            }
        }

        // Add extracted data byte to pending message
        mPendingMessage[mPendingMessageIndex] = extracted;

        // Now we are going to check if we have reached the end of the message
        if (mPendingMessageIndex >= (mPendingMessageExpectedLength - 1))
        {

        	mCompleteMessage.Event = MIDI_EVENT(0, getTypeFromStatusByte(mPendingMessage[0]));
            mCompleteMessage.Data1 = mPendingMessage[0];
            mCompleteMessage.Data2 = mPendingMessage[1];

            // Save Data3 only if applicable
            if (mPendingMessageExpectedLength == 3)
                mCompleteMessage.Data3 = mPendingMessage[2];
            else
                mCompleteMessage.Data3 = 0;

            // Reset local variables
            mPendingMessageIndex = 0;
            mPendingMessageExpectedLength = 0;
            mPendingMessageValid = true;

            // Activate running status (if enabled for the received type)
            switch (getTypeFromStatusByte(mPendingMessage[0]))
            {
                case NoteOff:
                case NoteOn:
                case AfterTouchPoly:
                case ControlChange:
                case ProgramChange:
                case AfterTouchChannel:
                case PitchBend:
                    // Running status enabled: store it from received message
                    mRunningStatus_RX = mPendingMessage[0];
                    break;

                default:
                    // No running status
                    mRunningStatus_RX = InvalidType;
                    break;
            }
            return;
        }
        else
        {
            // Not complete? Then update the index of the pending message.
            mPendingMessageIndex++;
        }
    }
	}
}

/** Event handler for the CDC Class driver Line Encoding Changed event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
	uint8_t ConfigMask = 0;

	switch (CDCInterfaceInfo->State.LineEncoding.ParityType)
	{
		case CDC_PARITY_Odd:
			ConfigMask = ((1 << UPM11) | (1 << UPM10));
			break;
		case CDC_PARITY_Even:
			ConfigMask = (1 << UPM11);
			break;
	}

	if (CDCInterfaceInfo->State.LineEncoding.CharFormat == CDC_LINEENCODING_TwoStopBits)
	  ConfigMask |= (1 << USBS1);

	switch (CDCInterfaceInfo->State.LineEncoding.DataBits)
	{
		case 6:
			ConfigMask |= (1 << UCSZ10);
			break;
		case 7:
			ConfigMask |= (1 << UCSZ11);
			break;
		case 8:
			ConfigMask |= ((1 << UCSZ11) | (1 << UCSZ10));
			break;
	}

	/* Keep the TX line held high (idle) while the USART is reconfigured */
	PORTD |= (1 << 3);

	/* Must turn off USART before reconfiguring it, otherwise incorrect operation may occur */
	UCSR1B = 0;
	UCSR1A = 0;
	UCSR1C = 0;

	/* Set the new baud rate before configuring the USART */
	UBRR1  = SERIAL_2X_UBBRVAL(CDCInterfaceInfo->State.LineEncoding.BaudRateBPS);

	/* Reconfigure the USART in double speed mode for a wider baud rate range at the expense of accuracy */
	UCSR1C = ConfigMask;
	UCSR1A = (1 << U2X1);
	UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));

	/* Release the TX line after the USART has been reconfigured */
	PORTD &= ~(1 << 3);
}

///////////////////////////////////////////////////////////////////////////////
// MIDI Utility Functions
///////////////////////////////////////////////////////////////////////////////

MidiMessageType getStatus(MidiMessageType inType, uint8_t inChannel) 
{
    return ((uint8_t)inType | ((inChannel - 1) & 0x0f));
}

MidiMessageType getTypeFromStatusByte(uint8_t inStatus)
{
    if ((inStatus  < 0x80) ||
        (inStatus == 0xf4) ||
        (inStatus == 0xf5) ||
        (inStatus == 0xf9) ||
        (inStatus == 0xfD))
    {
        // Data bytes and undefined.
        return InvalidType;
    }

    if (inStatus < 0xf0)
    {
        // Channel message, remove channel nibble.
        return (inStatus & 0xf0);
    }

    return inStatus;
}

uint8_t getChannelFromStatusByte(uint8_t inStatus)
{
	return (inStatus & 0x0f) + 1;
}

bool isChannelMessage(uint8_t inType)
{
    return (inType == NoteOff           ||
            inType == NoteOn            ||
            inType == ControlChange     ||
            inType == AfterTouchPoly    ||
            inType == AfterTouchChannel ||
            inType == PitchBend         ||
            inType == ProgramChange);
}
