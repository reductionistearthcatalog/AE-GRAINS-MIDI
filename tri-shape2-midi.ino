/*
"Tri-Shape2", a wave shaping  oscilator for Ginkosynthese's "grains" module.
	By Kassen Oud. 
MIDI implementation added by the Reductionist Earth Catalog.

LICENSE:
This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

DESCRIPION;
	A triangle-based osclitator with a mixture of phase-distortion and 
	hard-sync in the phase domain, followed by asymetrical wave-wrapping in the
	amplitude domain.

MANUAL;
	Knob 1 / mod 1; hard-sync/phase distortion
		Applies a mixture of hard-sync and phase distortion to the basic 
		triangle wave.
	Knob 2/ mod 2; Wave folding
		Amplifies the base triangle wave, then, once it hits the limits of the
		range, "folds" it back in the opposite direction. With enough
		amplification this will happen again at the other side, creating a
		rich spectrum. This amplification is asymetrical, which maintains more
		of the low-end of the spectrum.
	Knob 3: No function.
	Mod 3; No function.

MIDI input version
  Sample rate is 31.25 kHz
  Triangle table is 16 bit
  
*/
		


//the libraries we use
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
// added MIDI library
#include <MIDI.h>

//variables
uint16_t phase_accumulator;
uint16_t treated_phase;
uint16_t phase_inc = 1;
bool flip = false;
uint16_t wrap = 1;
uint16_t output = 127;
uint16_t last_out = 127;
uint16_t phase_dist_amount = 0;
uint16_t drive_factor = 0;
uint16_t d_mix_l = 0;
uint16_t d_mix_r = 0;
const uint16_t quarter_max_int = 16383;

// set up a global variable for midi note information. set C1 as the minimum midi note, 84 is max to retain roughly the original pitch range
uint16_t midinote = 0;

// Map inputs
#define HS_CONTROL			(A2)
#define WRAP_CONTROL		(A1)
// comment the following out because we won't be using the pitch knob for now
//#define PITCH_KNOB			(A0)
//#define PITCH_CV			(A3)

// Changing these will also requires rewriting setup()
//    Output is on pin 11
#define PWM_PIN       11
#define PWM_VALUE     OCR2A
#define LED_PORT      PORTB
#define LED_BIT       5
// this line added for digital output
#define DIGITAL_OUT_PIN 8
#define PWM_INTERRUPT TIMER2_OVF_vect

// added this line to create MIDI default instance
MIDI_CREATE_DEFAULT_INSTANCE();

//maps midi note to samples of phase increase each time we output a new value
//this might need tuning in the future
//table mapping midi values measured to phase increase per sample
const uint16_t freqTable[] PROGMEM = {
69,   73,   77,   82,   86,   92,   97,   103,  109,  115,  122,  129,  137,  145,  154, 
163,  173,  183,  194,  206,  218,  231,  244,  259,  274,  291,  308,  326,  346,  366, 
388,  411,  435,  461,  489,  518,  549,  581,  616,  652,  691,  732,  776,  822,  871, 
923,  978, 1036, 1097, 1163, 1232, 1305, 1383, 1465, 1552, 1644, 1742, 1845, 1955, 2071, 
2195
};

uint16_t mapFreq(uint16_t input)
{
	return pgm_read_word_near(freqTable + input);
}

// added this function to handle MIDI noteOn information
void handleNoteOn(byte channel, byte pitch, byte velocity)
{
    digitalWrite(DIGITAL_OUT_PIN, HIGH);

    // limit ourselves to the original pitch range, subtract out 24 (the low end of the range) so we get an index into phase increase table
    midinote = max( 24, min( 84, pitch ) ) - 24;
    // to make sure we won't overflow: minimum value of the above is 0 (maximum of either 24 or x is at least 24, 24-24 = 0)
    //                                maximum value of the above is 60 (minimum of either 84 or pitch is at most 84, 84-24 = 60, so the 61st entry in pitch table)
    phase_inc = mapFreq(midinote);
}

// added this function to handle MIDI noteOff information
void handleNoteOff(byte channel, byte pitch, byte velocity)
{
    digitalWrite(DIGITAL_OUT_PIN, LOW);
    // Note that NoteOn messages with 0 velocity are interpreted as NoteOffs.
}

//sets up pins and configures the samplerate and frequency of the PWM output
void setup()
{
  //set up digital output pin
  pinMode(DIGITAL_OUT_PIN, OUTPUT);
    
	TCCR2A = _BV(COM2A1) | _BV(WGM20);
	TCCR2B = _BV(CS20);
	TIMSK2 = _BV(TOIE2);
	pinMode(PWM_PIN,OUTPUT);
	PWM_VALUE = 127;

  //Added for MIDI implementation:
  // Initiate MIDI communications, listen to channel 1 by default (?)
  MIDI.begin(3);
  // begin baud rate at 115200
  Serial.begin(115200);
  
  // Connect the handleNoteOn function to the library,
  // so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function

  // Do the same for NoteOffs
  MIDI.setHandleNoteOff(handleNoteOff);
}

//reads modulation inputs
void loop()
{
  //added to read MIDI information
  MIDI.read();
	// the following is commented out because we won't be using the original pitch determination method
	//calculate the pitch
	//int pwmv = min( 1023,  analogRead(PITCH_CV) + analogRead(PITCH_KNOB));

  // try putting this in handleNoteOn
	//look up the phase increase per sample, note is a global variable updated in handleNoteOn
	//phase_inc = mapFreq(midinote);
	//read the control for wave-folding
	wrap =  min( 64 + (analogRead(WRAP_CONTROL) >> 1), 511);
	phase_dist_amount = analogRead(HS_CONTROL) >> 2;
}

SIGNAL(PWM_INTERRUPT)
{
	//increase the phase
	phase_accumulator += phase_inc;

	//hard sync & phase distortion
	treated_phase = phase_accumulator >> 8;
	drive_factor = treated_phase * phase_dist_amount;
	drive_factor >>= 8;
	d_mix_l = 3 * treated_phase * drive_factor;
	d_mix_r = (treated_phase * (255 - phase_dist_amount));
	treated_phase = d_mix_l + d_mix_r;

	//make the phase distortion line up with the triangle wave
	treated_phase += quarter_max_int;

	//2nd half of the phase will be the negative part of the cycle
	flip = treated_phase & 0b1000000000000000;

	//turn the phase acumulator into 4 up-ramps
	output = (treated_phase &  0b0011111111111111);
	//get these into 7 bit range. "flip" will be the 8th bit of the output
	output = output >> 7;
	//invert the 2nd and 4th of our 4 up-ramps to create our triangle wave
	if (treated_phase & 0b0100000000000000) output = (uint16_t)127 - output;

	//amplify the signal for the wave-wrapping
	output *= flip?wrap:max(wrap>>1, 64);
	output = output >> 6;
	//detect whether folding will be more involved than inverting the range
	//from 128 to 254
	if (output & 0b1111111100000000) 
	{
		//values between 255 and 511 fold back past the "0" line
		if (output & 0b0000000100000000) flip = !flip;
		//mask out bits beyond which the process just repeats
		output &= 0b0000000011111111;
	}
	//actual folding
	if (output > (uint16_t)127)
	{
		output = 127 - (output & 0b0000000001111111);
	}

	if (output >  (uint16_t)127) output = (uint16_t)127;

	//turn our 7bit unipolar value into a 8bit bipolar one
	if ( flip ) output = (uint16_t)127 - output;
	else output += (uint16_t)127;

	//slight amount of smoothing, with an extra amount of emphasis on smoothing out the edge caused by the hard-sync
	if (phase_accumulator > phase_inc) output = (output + last_out + last_out + last_out) >> 2;
	else output = (output + last_out) >> 1;
	last_out = output;

	//write out the output
	PWM_VALUE = output;
}
