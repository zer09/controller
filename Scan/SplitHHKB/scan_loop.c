/**
 * Copyright (C) 2014-2016 by Jacob Alexander
 * Copyright (C) 2016 by Tom Smalley
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// ----- Includes -----

// Compiler Includes
#include <Lib/ScanLib.h>

// Project Includes
#include <stdbool.h>
#include <cli.h>
#include <print.h>
//#include <matrix_scan.h>
#include <macro.h>
#include <output_com.h>
#include <Lib/delay.h>

// Local Includes
#include "scan_loop.h"
#include "eeprom.c"
#include "hardware.h"
#include "calibration.c"


// ----- Function Declarations -----

// ----- Variables -----

KeyState keyStates[ NUM_READS * NUM_STROBES ];

// Setup
inline void Scan_setup()
{
	// Setup GPIO pins for mux (PTA5, PTA12, PTA13)
	PORTA_PCR5 = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
	GPIOA_PDDR |= (1<<5);
	PORTA_PCR12 = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
	GPIOA_PDDR |= (1<<12);
	PORTA_PCR13 = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
	GPIOA_PDDR |= (1<<13);
	// Setup drain pin (PTA4)
	PORTA_PCR4 = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_ODE | PORT_PCR_MUX(1);
	GPIOA_PDDR |= (1<<4);
	// Setup strobes (PTD0-4)
	PORTD_PCR0 = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
	GPIOD_PDDR |= (1 << 0);
	PORTD_PCR1 = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
	GPIOD_PDDR |= (1 << 1);
	PORTD_PCR2 = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
	GPIOD_PDDR |= (1 << 2);
	PORTD_PCR3 = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
	GPIOD_PDDR |= (1 << 3);
	PORTD_PCR4 = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(1);
	GPIOD_PDDR |= (1 << 4);

	// Initialise ADC
	adcInit();

	// Initialise EEPROM
	eeprom_initialize();

	// Setup calibration
	calibration_setup();

	//Matrix_setup();
	for ( uint8_t i = 0; i < NUM_READS * NUM_STROBES; i++ )
	{
		keyStates[i].depth = 0;
		keyStates[i].pressed = false;
	}

}

// Main Detection Loop
inline uint8_t Scan_loop()
{

	// Go through all read lines
	for (int read = 0; read < NUM_READS; read++)
	{
		// Select read line on mux
		selectReadLine(read);

		// Strobe all lines
		for (int strobe = 0; strobe < NUM_STROBES; strobe++)
		{
			// Key ID
			uint8_t key = keyID(read, strobe);
			KeyState *state = &keyStates[ key ];

			uint8_t value = strobeRead(strobe);
			uint8_t calMin = calibration_get_min(key);
			uint8_t calMax = calibration_get_max(key);
			state->depth = normalise(calMin, calMax, value);

			uint8_t actDepth = get_actuation_depth();
			uint8_t relDepth = actDepth - 3 * calibration_get_noise(key);

			// Hysteresis for doing digital press
			if (!state->pressed && state->depth > actDepth)
			{
				// Key just pressed
				state->pressed = true;
				// Send press
				//Macro_keyState( key, 0x01 );
			}
			else if (state -> pressed && state->depth < relDepth)
			{
				// Key just released
				state->pressed = false;
				// Send release
				//Macro_keyState( key, 0x03 );
			}

		}
	}

	return 0;
}


// Signal from Macro Module that all keys have been processed (that it knows about)
inline void Scan_finishedWithMacro( uint8_t sentKeys )
{
}


// Signal from Output Module that all keys have been processed (that it knows about)
inline void Scan_finishedWithOutput( uint8_t sentKeys )
{
}


// Signal from the Output Module that the available current has changed
// current - mA
void Scan_currentChange( unsigned int current )
{
	// Indicate to all submodules current change
}

