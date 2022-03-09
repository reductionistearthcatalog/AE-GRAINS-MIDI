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
// added ramp library for portamento
#include <Ramp.h>

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
// try setting up a counter for the loop function
//byte count = 0;
// for portamento
bool port = false;
uint16_t portTime = 0;
rampInt portRamp;

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

//maps to samples of phase increase each time we output a new value
//this might need tuning in the future
//table mapping desired phase increase per sample
const uint16_t freqTable[] PROGMEM = {
69,   69,   69,   69,   70,   70,   70,   70,   70,   71,   71,   71,   71,   72,   72,
72,   72,   73,   73,   73,   73,   74,   74,   74,   74,   75,   75,   75,   75,   76,
76,   76,   76,   77,   77,   77,   77,   78,   78,   78,   79,   79,   79,   79,   80,
80,   80,   80,   81,   81,   81,   82,   82,   82,   82,   83,   83,   83,   83,   84,
84,   84,   85,   85,   85,   85,   86,   86,   86,   87,   87,   87,   88,   88,   88,
88,   89,   89,   89,   90,   90,   90,   91,   91,   91,   91,   92,   92,   92,   93,
93,   93,   94,   94,   94,   95,   95,   95,   96,   96,   96,   97,   97,   97,   98,
98,   98,   99,   99,   99,   100,  100,  100,  101,  101,  101,  102,  102,  102,  103,
103,  103,  104,  104,  104,  105,  105,  105,  106,  106,  107,  107,  107,  108,  108,
108,  109,  109,  109,  110,  110,  111,  111,  111,  112,  112,  112,  113,  113,  114,
114,  114,  115,  115,  116,  116,  116,  117,  117,  117,  118,  118,  119,  119,  119,
120,  120,  121,  121,  122,  122,  122,  123,  123,  124,  124,  124,  125,  125,  126,
126,  127,  127,  127,  128,  128,  129,  129,  130,  130,  130,  131,  131,  132,  132,
133,  133,  134,  134,  135,  135,  135,  136,  136,  137,  137,  138,  138,  139,  139,
140,  140,  141,  141,  142,  142,  142,  143,  143,  144,  144,  145,  145,  146,  146,
147,  147,  148,  148,  149,  149,  150,  150,  151,  151,  152,  152,  153,  154,  154,
155,  155,  156,  156,  157,  157,  158,  158,  159,  159,  160,  160,  161,  161,  162,
163,  163,  164,  164,  165,  165,  166,  166,  167,  168,  168,  169,  169,  170,  170,
171,  172,  172,  173,  173,  174,  175,  175,  176,  176,  177,  178,  178,  179,  179,
180,  181,  181,  182,  182,  183,  184,  184,  185,  186,  186,  187,  187,  188,  189,
189,  190,  191,  191,  192,  193,  193,  194,  195,  195,  196,  197,  197,  198,  199,
199,  200,  201,  201,  202,  203,  203,  204,  205,  205,  206,  207,  207,  208,  209,
210,  210,  211,  212,  212,  213,  214,  215,  215,  216,  217,  218,  218,  219,  220,
220,  221,  222,  223,  223,  224,  225,  226,  227,  227,  228,  229,  230,  230,  231,
232,  233,  234,  234,  235,  236,  237,  238,  238,  239,  240,  241,  242,  242,  243,
244,  245,  246,  247,  247,  248,  249,  250,  251,  252,  252,  253,  254,  255,  256,
257,  258,  259,  259,  260,  261,  262,  263,  264,  265,  266,  267,  267,  268,  269,
270,  271,  272,  273,  274,  275,  276,  277,  278,  278,  279,  280,  281,  282,  283,
284,  285,  286,  287,  288,  289,  290,  291,  292,  293,  294,  295,  296,  297,  298,
299,  300,  301,  302,  303,  304,  305,  306,  307,  308,  309,  310,  311,  312,  314,
315,  316,  317,  318,  319,  320,  321,  322,  323,  324,  325,  327,  328,  329,  330,
331,  332,  333,  334,  335,  337,  338,  339,  340,  341,  342,  344,  345,  346,  347,
348,  349,  351,  352,  353,  354,  355,  357,  358,  359,  360,  361,  363,  364,  365,
366,  368,  369,  370,  371,  373,  374,  375,  376,  378,  379,  380,  382,  383,  384,
385,  387,  388,  389,  391,  392,  393,  395,  396,  397,  399,  400,  401,  403,  404,
405,  407,  408,  410,  411,  412,  414,  415,  417,  418,  419,  421,  422,  424,  425,
427,  428,  429,  431,  432,  434,  435,  437,  438,  440,  441,  443,  444,  446,  447,
449,  450,  452,  453,  455,  456,  458,  460,  461,  463,  464,  466,  467,  469,  471,
472,  474,  475,  477,  479,  480,  482,  484,  485,  487,  488,  490,  492,  493,  495,
497,  498,  500,  502,  504,  505,  507,  509,  510,  512,  514,  516,  517,  519,  521,
523,  524,  526,  528,  530,  532,  533,  535,  537,  539,  541,  542,  544,  546,  548,
550,  552,  554,  555,  557,  559,  561,  563,  565,  567,  569,  571,  573,  575,  577,
579,  580,  582,  584,  586,  588,  590,  592,  594,  596,  598,  600,  602,  605,  607,
609,  611,  613,  615,  617,  619,  621,  623,  625,  627,  630,  632,  634,  636,  638,
640,  642,  645,  647,  649,  651,  653,  656,  658,  660,  662,  665,  667,  669,  671,
674,  676,  678,  681,  683,  685,  687,  690,  692,  695,  697,  699,  702,  704,  706,
709,  711,  714,  716,  718,  721,  723,  726,  728,  731,  733,  736,  738,  741,  743,
746,  748,  751,  753,  756,  758,  761,  764,  766,  769,  771,  774,  777,  779,  782,
784,  787,  790,  793,  795,  798,  801,  803,  806,  809,  812,  814,  817,  820,  823,
825,  828,  831,  834,  837,  839,  842,  845,  848,  851,  854,  857,  860,  862,  865,
868,  871,  874,  877,  880,  883,  886,  889,  892,  895,  898,  901,  904,  907,  910,
914,  917,  920,  923,  926,  929,  932,  935,  939,  942,  945,  948,  951,  955,  958,
961,  964,  968,  971,  974,  978,  981,  984,  988,  991,  994,  998,  1001, 1004, 1008,
1011, 1015, 1018, 1022, 1025, 1028, 1032, 1035, 1039, 1042, 1046, 1050, 1053, 1057, 1060,
1064, 1067, 1071, 1075, 1078, 1082, 1086, 1089, 1093, 1097, 1100, 1104, 1108, 1112, 1115,
1119, 1123, 1127, 1131, 1135, 1138, 1142, 1146, 1150, 1154, 1158, 1162, 1166, 1170, 1174,
1178, 1182, 1186, 1190, 1194, 1198, 1202, 1206, 1210, 1214, 1218, 1222, 1226, 1231, 1235,
1239, 1243, 1247, 1252, 1256, 1260, 1264, 1269, 1273, 1277, 1282, 1286, 1290, 1295, 1299,
1303, 1308, 1312, 1317, 1321, 1326, 1330, 1335, 1339, 1344, 1348, 1353, 1357, 1362, 1367,
1371, 1376, 1381, 1385, 1390, 1395, 1399, 1404, 1409, 1414, 1418, 1423, 1428, 1433, 1438,
1443, 1448, 1452, 1457, 1462, 1467, 1472, 1477, 1482, 1487, 1492, 1497, 1502, 1508, 1513,
1518, 1523, 1528, 1533, 1538, 1544, 1549, 1554, 1559, 1565, 1570, 1575, 1581, 1586, 1591,
1597, 1602, 1608, 1613, 1619, 1624, 1630, 1635, 1641, 1646, 1652, 1657, 1663, 1669, 1674,
1680, 1686, 1691, 1697, 1703, 1709, 1714, 1720, 1726, 1732, 1738, 1744, 1750, 1756, 1762,
1768, 1774, 1780, 1786, 1792, 1798, 1804, 1810, 1816, 1822, 1828, 1835, 1841, 1847, 1853,
1860, 1866, 1872, 1879, 1885, 1891, 1898, 1904, 1911, 1917, 1924, 1930, 1937, 1943, 1950,
1956, 1963, 1970, 1976, 1983, 1990, 1997, 2003, 2010, 2017, 2024, 2031, 2037, 2044, 2051,
2058, 2065, 2072, 2079, 2086, 2093, 2101, 2108, 2115, 2122, 2129, 2136, 2144, 2151, 2158,
2165, 2173, 2180, 2188
};

//maps midi note to indexes in freqTable
//this might need tuning in the future
//table mapping midi values measured to phase increase per sample
const uint16_t midiTable[] PROGMEM = {
0,    17,   33,   51,   66,   86,   101,  119,  136,  153,  169,  187,  205,  221,  238, 
255,  273,  290,  307,  325,  341,  359,  375,  393,  410,  427,  444,  461,  478,  495, 
512,  529,  546,  563,  581,  597,  615,  631,  649,  666,  683,  699,  717,  734,  751, 
768,  785,  802,  819,  836,  853,  870,  888,  904,  922,  939,  956,  973,  990, 1007, 
1023
};

uint16_t mapMIDI(uint16_t input)
{
  //return pgm_read_word_near(midiTable + input);
	return pgm_read_word_near(freqTable + pgm_read_word_near(midiTable + input));
}

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
    if (port) {
      portRamp.go(mapMIDI(midinote), portTime, LINEAR, ONCEFORWARD);
      }
    else {
      portRamp.go(mapMIDI(midinote));
    }
}

// added this function to handle MIDI noteOff information
void handleNoteOff(byte channel, byte pitch, byte velocity)
{
    digitalWrite(DIGITAL_OUT_PIN, LOW);
    // Note that NoteOn messages with 0 velocity are interpreted as NoteOffs.
}

// control change
void handleControlChange(byte channel, byte num, byte val)
{
  if (num == 5) {
      if (val == 0) {
        port = false;
      }
      else {
        port = true;
        portTime = 20 * val;
      }
    }
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

  // for portamento
  // set up ramp grain to 1 ms
  MIDI.setHandleControlChange(handleControlChange);
  portRamp.setGrain(1);
  portRamp.go(mapMIDI(midinote));
  
  // set up phase_inc
  phase_inc = portRamp.update();
}

//reads modulation inputs
void loop()
{
  //added to read MIDI information
  MIDI.read();
	// the following is commented out because we won't be using the original pitch determination method
	//calculate the pitch
	//int pwmv = min( 1023,  analogRead(PITCH_CV) + analogRead(PITCH_KNOB));
  phase_inc = portRamp.update();

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
