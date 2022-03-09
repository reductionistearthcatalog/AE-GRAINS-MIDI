// Wavegrains V2.0
// 
// based on the Auduino code by Peter Knight, revised and finetuned by Ginkosynthese for use with cv inputs. 
// Change to a wavetable oscillator by Janne G:son Berg (jgb @ muffwiggler)
// Version for PCB V2
//
// inputs
// Analog in 0: Pitch CV 0-5 V
// Analog in 1: Wavetable select 0-5 V
// Analog in 2: Sweep between two wavetables 0-5 V
// output
// Digital 11: Audio out (PWM)


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
// added MIDI library
#include <MIDI.h>
// added ramp library for portamento
#include <Ramp.h>

uint16_t syncPhaseAcc;
uint16_t syncPhaseInc;
uint8_t wavetableStep;
uint8_t wavetableA;
uint8_t wavetableB;
uint16_t sweepPosition;

// set up a global variable for midi note information. set C1 as the minimum midi note, 84 is max to retain roughly the original pitch range
uint16_t midinote = 0;

// for portamento
bool port = false;
uint16_t portTime = 0;
int lastRampValue;
rampInt portRamp;

// Define wavetable parameters
#define WT_LENGTH 16
#define WT_NO_OF 32
#define WT_POT_SCALING 5

// Map Analogue channels
// commented out frequency lines because we won't be using them
//#define FREQUENCY         (0)
#define WAVETABLE_SELECT   (1)
#define SWEEP  (2)
//#define FREQUENCY_OFFSET         (3)

// Changing these will also requires rewriting audioOn()
//    Output is on pin 11
#define PWM_PIN       11
#define PWM_VALUE     OCR2A
#define LED_PIN       13
#define LED_PORT      PORTB
#define LED_BIT       5
// this line added for digital output
#define DIGITAL_OUT_PIN 8
#define PWM_INTERRUPT TIMER2_OVF_vect

// added this line to create MIDI default instance
MIDI_CREATE_DEFAULT_INSTANCE();

// For converting incoming CV to pitch
const uint16_t freqTable[] PROGMEM = {
545,547,549,551,552,554,556,558,560,561,563,565,567,569,570,572,574,576,578,580,582,584,586,588,590,592,594,596,598,600,602,604,606,608,610,612,614,616,618,621,623,625,627,629,631,633,636,638,640,642,644,646,649,651,653,655,658,660,662,664,667,669,671,673,676,678,680,683,685,687,690,692,694,697,699,701,704,706,709,711,713,716,718,721,723,726,728,731,733,736,738,741,743,746,748,751,753,756,759,761,764,766,769,772,774,777,779,782,785,787,790,793,796,798,801,804,806,809,812,815,817,820,823,826,829,831,834,837,840,843,846,849,852,854,857,860,863,866,869,872,875,878,881,884,887,890,893,896,899,902,905,908,911,915,918,921,924,927,930,933,937,940,943,946,949,953,956,959,962,966,969,972,976,979,982,986,989,992,996,999,1002,1006,1009,1013,1016,1020,1023,1027,1030,1034,1037,1041,1044,1048,1051,1055,1059,1062,1066,1069,1073,1077,1080,1084,1088,1091,1095,1098,1102,1106,1109,1113,1116,1120,1123,1127,1131,1134,1138,1142,1145,1149,1153,1156,1160,1164,1168,1172,1176,1180,1184,1188,1192,1196,1200,1205,1209,1213,1217,1221,1225,1229,1234,1238,1242,1246,1250,1255,1259,1263,1268,1272,1276,1281,1285,1289,1294,1298,1303,1307,1311,1316,1320,1325,1329,1334,1338,1343,1348,1352,1357,1361,1366,1371,1375,1380,1385,1389,1394,1399,1404,1408,1413,1418,1423,1428,1433,1438,1442,1447,1452,1457,1462,1467,1472,1477,1482,1487,1492,1497,1502,1508,1513,1518,1523,1528,1533,1539,1544,1549,1554,1560,1565,1570,1576,1581,1586,1592,1597,1603,1608,1614,1619,1625,1630,1636,1641,1647,1653,1658,1664,1669,1675,1681,1687,1692,1698,1704,1710,1715,1721,1727,1733,1739,1745,1751,1757,1763,1769,1775,1781,1787,1793,1799,1805,1811,1818,1824,1830,1836,1842,1849,1855,1861,1868,1874,1880,1887,1893,1900,1906,1913,1919,1926,1932,1939,1945,1952,1959,1965,1972,1979,1985,1992,1999,2006,2013,2020,2026,2033,2040,2047,2054,2061,2068,2075,2082,2089,2096,2104,2111,2118,2125,2132,2140,2147,2154,2162,2169,2176,2184,2191,2198,2205,2212,2219,2226,2233,2241,2248,2255,2262,2269,2277,2284,2291,2299,2306,2314,2322,2329,2337,2345,2353,2361,2369,2377,2386,2394,2402,2410,2418,2426,2435,2443,2451,2460,2468,2476,2485,2493,2502,2510,2519,2527,2536,2545,2553,2562,2571,2579,2588,2597,2606,2615,2624,2633,2642,2651,2660,2669,2678,2687,2696,2705,2714,2724,2733,2742,2752,2761,2770,2780,2789,2799,2808,2818,2827,2837,2847,2856,2866,2876,2886,2895,2905,2915,2925,2935,2945,2955,2965,2975,2985,2996,3006,3016,3026,3037,3047,3057,3068,3078,3089,3099,3110,3120,3131,3142,3152,3163,3174,3185,3195,3206,3217,3228,3239,3250,3261,3272,3283,3295,3306,3317,3328,3340,3351,3363,3374,3385,3397,3409,3420,3432,3443,3455,3467,3479,3491,3502,3514,3526,3538,3550,3563,3575,3587,3599,3611,3624,3636,3648,3661,3673,3686,3698,3711,3723,3736,3749,3762,3774,3787,3800,3813,3826,3839,3852,3865,3878,3892,3905,3918,3932,3945,3958,3972,3985,3999,4012,4026,4040,4054,4067,4081,4095,4109,4123,4137,4151,4165,4180,4194,4208,4222,4237,4251,4266,4280,4295,4309,4324,4339,4353,4368,4383,4398,4413,4428,4443,4458,4473,4489,4504,4519,4535,4550,4566,4581,4597,4612,4628,4644,4660,4676,4691,4707,4723,4740,4756,4772,4788,4804,4821,4837,4854,4870,4887,4903,4920,4937,4954,4970,4987,5004,5021,5038,5056,5073,5090,5107,5125,5142,5160,5177,5195,5213,5230,5248,5266,5284,5302,5320,5338,5356,5375,5393,5411,5430,5448,5467,5485,5504,5523,5541,5560,5579,5598,5617,5636,5656,5675,5694,5714,5733,5753,5772,5792,5811,5831,5851,5871,5891,5911,5931,5951,5972,5992,6012,6033,6053,6074,6095,6115,6136,6157,6178,6199,6220,6241,6263,6284,6305,6327,6348,6370,6392,6413,6435,6457,6479,6501,6523,6545,6568,6590,6613,6635,6658,6680,6703,6726,6749,6772,6795,6818,6841,6864,6888,6911,6935,6958,6982,7006,7030,7054,7078,7102,7126,7150,7174,7199,7223,7248,7273,7297,7322,7347,7372,7397,7422,7448,7473,7498,7524,7550,7575,7601,7627,7653,7679,7705,7731,7758,7784,7811,7837,7864,7891,7917,7944,7971,7999,8026,8053,8081,8108,8136,8163,8191,8219,8247,8275,8303,8332,8360,8388,8417,8446,8474,8503,8532,8561,8590,8619,8649,8678,8708,8737,8767,8797,8827,8857,8887,8917,8948,8978,9009,9039,9070,9101,9132,9163,9194,9226,9257,9289,9320,9352,9384,9416,9448,9480,9512,9545,9577,9610,9642,9675,9708,9741,9774,9808,9841,9874,9908,9942,9976,10010,10044,10078,10112,10147,10181,10216,10251,10285,10320,10356,10391,10426,10462,10497,10533,10569,10605,10641,10677,10713,10750,10787,10823,10860,10897,10934,10971,11009,11046,11084,11122,11159,11197,11235,11274,11312,11351,11389,11428,11467,11506,11545,11584,11624,11663,11703,11743,11783,11823,11863,11904,11944,11985,12026,12067,12108,12149,12190,12232,12273,12315,12357,12399,12441,12484,12526,12569,12611,12654,12697,12741,12784,12828,12871,12915,12959,13003,13047,13092,13136,13181,13226,13271,13316,13361,13407,13453,13498,13544,13590,13637,13683,13730,13776,13823,13870,13918,13965,14012,14060,14108,14156,14204,14253,14301,14350,14399,14448,14497,14546,14596,14645,14695,14745,14795,14846,14896,14947,14998,15049,15100,15152,15203,15255,15307,15359,15411,15464,15516,15569,15622,15675,15729,15782,15836,15890,15944,15998,16053,16107,16162,16217,16272,16328,16383,16439,16495,16551,16607,16664,16721,16778,16835,16892,16949,17007,17065,17123,17181,17240,17299,17357,17416,17476};

//maps midi note to indexes in freqTable
//this might need tuning in the future
//table mapping midi values measured to phase increase per sample
const uint16_t midiTable[] PROGMEM = {
  0,18,35,52,69,86,103,120,137,154,171,188,205,223,240,257,274,291,308,325,342,359,376,393,410,428,445,462,479,496,513,530,547,564,581,598,615,632,649,666,683,700,717,734,751,768,785,802,819,836,853,870,887,904,921,938,955,972,989,1006,1023
};

// Wavetables, to be filled in by randomization and some precalculated waveforms
uint16_t wavetable[WT_NO_OF][WT_LENGTH];

uint16_t mapMIDI(uint16_t input)
{
  //return pgm_read_word_near(midiTable + input);
  return pgm_read_word_near(freqTable + pgm_read_word_near(midiTable + input));
}

uint16_t mapFreq(uint16_t input) {
  return (pgm_read_word_near(freqTable + input));
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

void audioOn() {
  // Set up PWM to 31.25kHz, phase accurate

  TCCR2A = _BV(COM2A1) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  OCR2A = 15;
  TIMSK2 = _BV(TOIE2);
}


void setup() {
  pinMode(PWM_PIN,OUTPUT);
  audioOn();
  pinMode(LED_PIN,OUTPUT);
  
  //set up digital output pin
  pinMode(DIGITAL_OUT_PIN, OUTPUT);
  
  // Change random seed value to change the randomized wavetables
  randomSeed(123717284);
  int tmpTable[WT_LENGTH];
  uint16_t maxVal;
  
  for (int j = 0; j< WT_NO_OF; j++)
  {
    maxVal = 0;    
    for (int i = 0; i < WT_LENGTH; i++)
    {
      tmpTable[i] = random(-32768, 32767);
      maxVal = abs(tmpTable[i]) > maxVal ? abs(tmpTable[i]) : maxVal;
      
    }
    // Normalize wavetables
    for (int i = 0; i < WT_LENGTH; i++)
    {
      wavetable[j][i] = (32768*tmpTable[i])/maxVal + 32768;
    }

    // Use up some more random values... totally unnecessary
    for (int i = 0; i < random(4); i++)
    {
      i == i;
    }    
  }

  // Hardcoded wavetables sprinkled among the randomized ones

  // Square one octave up
  wavetable[0][0] = 65535;
  wavetable[0][1] = 65535;
  wavetable[0][2] = 65535;
  wavetable[0][3] = 65535;
  wavetable[0][4] = 0;
  wavetable[0][5] = 0;
  wavetable[0][6] = 0;
  wavetable[0][7] = 0;
  wavetable[0][8] = 65535;
  wavetable[0][9] = 65535;
  wavetable[0][10] = 65535;
  wavetable[0][11] = 65535;
  wavetable[0][12] = 0;
  wavetable[0][13] = 0;
  wavetable[0][14] = 0;
  wavetable[0][15] = 0;

  // Lowest square possible
  wavetable[4][0] = 65535;
  wavetable[4][1] = 65535;
  wavetable[4][2] = 65535;
  wavetable[4][3] = 65535;
  wavetable[4][4] = 65535;
  wavetable[4][5] = 65535;
  wavetable[4][6] = 65535;
  wavetable[4][7] = 65535;
  wavetable[4][8] = 0;
  wavetable[4][9] = 0;
  wavetable[4][10] = 0;
  wavetable[4][11] = 0;
  wavetable[4][12] = 0;
  wavetable[4][13] = 0;
  wavetable[4][14] = 0;
  wavetable[4][15] = 0;

  // Square with two extra peaks
  wavetable[8][0] = 65535;
  wavetable[8][1] = 65535;
  wavetable[8][2] = 65535;
  wavetable[8][3] = 65535;
  wavetable[8][4] = 65535;
  wavetable[8][5] = 65535;
  wavetable[8][6] = 65535;
  wavetable[8][7] = 65535;
  wavetable[8][8] = 0;
  wavetable[8][9] = 65535;
  wavetable[8][10] = 0;
  wavetable[8][11] = 0;
  wavetable[8][12] = 65535;
  wavetable[8][13] = 0;
  wavetable[8][14] = 0;
  wavetable[8][15] = 0;

  // Somewhat random?
  wavetable[9][0] = 8345;
  wavetable[9][1] = 50000;
  wavetable[9][2] = 7643;
  wavetable[9][3] = 65535;
  wavetable[9][4] = 52000;
  wavetable[9][5] = 10000;
  wavetable[9][6] = 20000;
  wavetable[9][7] = 40000;
  wavetable[9][8] = 300;
  wavetable[9][9] = 5120;
  wavetable[9][10] = 60240;
  wavetable[9][11] = 2048;
  wavetable[9][12] = 65535;
  wavetable[9][13] = 0;
  wavetable[9][14] = 58755;
  wavetable[9][15] = 36289;

  // Sawtooth
  wavetable[16][0] = 0x0000;
  wavetable[16][1] = 0x1000;
  wavetable[16][2] = 0x2000;
  wavetable[16][3] = 0x3000;
  wavetable[16][4] = 0x4000;
  wavetable[16][5] = 0x5000;
  wavetable[16][6] = 0x6000;
  wavetable[16][7] = 0x7000;
  wavetable[16][8] = 0x8000;
  wavetable[16][9] = 0x9000;
  wavetable[16][10] = 0xa000;
  wavetable[16][11] = 0xb000;
  wavetable[16][12] = 0xc000;
  wavetable[16][13] = 0xd000;
  wavetable[16][14] = 0xe000;
  wavetable[16][15] = 0xf000;

  // Sine
  wavetable[24][0] = 128 << 8;
  wavetable[24][1] = 176 << 8;
  wavetable[24][2] = 218 << 8;
  wavetable[24][3] = 246 << 8;
  wavetable[24][4] = 255 << 8;
  wavetable[24][5] = 246 << 8;
  wavetable[24][6] = 218 << 8;
  wavetable[24][7] = 176 << 8;
  wavetable[24][8] = 128 << 8;
  wavetable[24][9] = 79 << 8;
  wavetable[24][10] = 37 << 8;
  wavetable[24][11] = 9 << 8;
  wavetable[24][12] = 0 << 8;
  wavetable[24][13] = 9 << 8;
  wavetable[24][14] = 37 << 8;
  wavetable[24][15] = 79 << 8;

  wavetableStep=0;

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
  syncPhaseInc = portRamp.update();
}

void loop() {
  //added to read MIDI information
  MIDI.read();

  syncPhaseInc = portRamp.update();

  // the following is commented out because we won't be using the original pitch determination method
  // Get CV in and convert it to the phase accumulator
  //  int pwmv = analogRead(FREQUENCY)+analogRead(FREQUENCY_OFFSET);
  //if (pwmv > 1023) pwmv = 1023;
  //    if (pwmv < 0) pwmv = 0;
  //syncPhaseInc = mapFreq(midinote);

  // Read position of sweep
  sweepPosition = analogRead(SWEEP);

  // Read current wavetable and set up wavetables to morph between
  uint8_t readWavetable = analogRead(WAVETABLE_SELECT) >> WT_POT_SCALING;
  wavetableA = readWavetable;
  wavetableB = (readWavetable+1) & 0x1f;
}

SIGNAL(PWM_INTERRUPT)
{
  uint16_t output;
  uint16_t waveA;
  uint16_t waveB;
  uint32_t delta;  
  bool aLarger;

  // Phase accumulator
  syncPhaseAcc += syncPhaseInc;
  if (syncPhaseAcc < syncPhaseInc) 
  {
    // Time to increase the wavetable step
    wavetableStep++;
    wavetableStep &= WT_LENGTH - 1;
    LED_PORT ^= 1 << LED_BIT; // Faster than using digitalWrite
  }

  // Calculate morphing between wavetables, keep delta value positive
  waveA = (wavetable[wavetableA][wavetableStep]);
  waveB = (wavetable[wavetableB][wavetableStep]);
  if (waveA >= waveB)
  {
    aLarger = true;
    delta = waveA - waveB;
  }
  else
  {
    aLarger = false;
    delta = waveB - waveA;
  }
    
  delta = (delta * sweepPosition) >> 10;
  output = aLarger ? waveA - delta : waveA + delta;

  // Output to PWM (this is faster than using analogWrite)  
  // Scale down to 8 bit value
  PWM_VALUE = output  >> 8;
}
