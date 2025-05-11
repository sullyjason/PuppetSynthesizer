/*
TODO:
- Check mapping accuracy and direction
- make sure to reach max and min
- connect resistors to rotary switches to turn into potentiometers
- map rotary switches correctly
- toggle debugging mode
*/

//toggle debug mode-----------------------------------------------------------------------------------------------------

#define DEBUG 0


//libraries-------------------------------------------------------------------------------------------------------------

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <MIDIUSB.h>

Adafruit_ADS1115 analogBoard1;    // First ADS1115 at address 0x48
Adafruit_ADS1115 analogBoard2;    // Second ADS1115 at address 0x49

//output ranges---------------------------------------------------------------------------------------------------------

#define OUT_MIN 0               // MIDI range minimum
#define OUT_MAX 127             // MIDI range maximum  
#define defineThreshHold 1      //Threshhold for checking for changes in MIDI values


//input leonardo
// ---------------------------------------------------------------------------------------------------------

struct InputMapping {
    int inputNumber;            // The index of the input
    const char* functionName;   // What the input does
    int pinNumber;              // The pin number
    int readSignal;             // The signal received by the Arduino
    int inMin;                  // Input range minimum
    int inMax;                  // Input range maximum
    int outMin;                 // Output range minimum
    int outMax;                 // Output range maximum
    int mappedSignal;           // The mapped signal to send to MIDI
    int midiChannel;            // MIDI Channel number ("control" in helper function)
};


// The following values need to be modified if any changes are made to the hardware. Currently configured with 2 ADS1115s, called analogBoard1 and analogBoard2.
// The remaining analog inputs are connected to the Arduino directly.

InputMapping leonardo [] = {

  //  Index   functionName       pinNumber   readSignal  inMin, inMax    outMin, outMax  	mappedSignal    midiChannel
  // -----------------------------------------------------------------------------------------------------------------------
      {0,      "EMPTY",			      0,			      0,		    0,  0,			  OUT_MIN, OUT_MAX,        0,				0		  	},
      {1,      "FREQ_3",			    A2,			      0,		    0, 1023, 	    OUT_MIN, OUT_MAX,	       0,				12			}, 
      {2,      "AMP_1",			      A5,			      0,		    15, 1000,	    OUT_MIN, OUT_MAX,	       0,				1		  	},
      {3,     "AMP_2",			      A4,			      0,		    15, 1000,	    OUT_MIN, OUT_MAX,	       0,				2		  	},
      {4,     "AMP_3",			      A1,			      0,		    15, 1000,	    OUT_MIN, OUT_MAX,	       0,				3		  	},
      {5,     "EFFECT_1",		      A0,			      0,		    1023, 510,		OUT_MIN, OUT_MAX,	       0,				32			},
      {6,     "EFFECT_3",		      A3,			      0,		    1023, 510,		OUT_MIN, OUT_MAX,	       0,				34			},
};		

InputMapping ads1[] = {
    //  Index   functionName       pinNumber   readSignal  inMin, inMax    outMin, outMax  	mappedSignal    midiChannel
    // -----------------------------------------------------------------------------------------------------------------------
        {0,      "EFFECT_2",			  0,		    	0,		    	4400, 32700,	  OUT_MIN, OUT_MAX,   0,				33      },
        {1,      "WAVE_TYPE_2",		  3,  	    	0,		    	20200, 29800,	  OUT_MIN, OUT_MAX,	  0,				16			},
        {2,      "FREQ_1",			    2,		    	0,		    	-10, 33000,		  OUT_MIN, OUT_MAX,	  0,				10			},
        {3,      "FREQ_2",			    1,		    	0,		    	-10, 33000,	  	OUT_MIN, OUT_MAX,	  0,				11			},
};	

InputMapping ads2[] = {
    //  Index   functionName       pinNumber   readSignal  inMin, inMax    outMin, outMax  	mappedSignal    midiChannel
    // -----------------------------------------------------------------------------------------------------------------------
    {0,      "EMPTY",			          0,		    	0,		    	0,  0,			    OUT_MIN, OUT_MAX,   0,			  0			  },
    {1,      "MAIN_SPEED",		      2,		    	0,		    	0, 32000,		    OUT_MIN, OUT_MAX,	  0,				9			  },  
    {2,      "MAIN_FREQ",		        3,		    	0,		    	60, 32000,		  OUT_MIN, OUT_MAX,	  0,				8			  },  
    {3,      "WAVE_TYPE_1",		      0,		    	0,		    	20200, 29900,	  OUT_MIN, OUT_MAX,	  0,				15			},  
    {4,      "WAVE_TYPE_3",		      1,		    	0,		    	20200,29900,	  OUT_MIN, OUT_MAX,	  0,				17			},  
};


// Move these to global scope so they persist between loop iterations
int lastMappedSignalsLeonardo[7] = {0};
int lastMappedSignalsAds1[4] = {0};
int lastMappedSignalsAds2[5] = {0};

void setup(){
    Serial.begin(9600);
    Wire.begin(); // Initialize I2C communication
    Wire.setClock(400000); // Set I2C clock speed to 400kHz (optional)
  
    // Enable internal pull-up resistors on SDA and SCL
    digitalWrite(SDA, HIGH);
    digitalWrite(SCL, HIGH);
  
    // Initialize the first ADS1115 (address 0x48)
    if (!analogBoard1.begin(0x48)) {
      Serial.println("Failed to initialize analogBoard1115 at address 0x48.");
      while (1);
    }
  
    // Initialize the second ADS1115 (address 0x49)
    if (!analogBoard2.begin(0x49)) {
      Serial.println("Failed to initialize ADS1115 at address 0x49.");
      while (1);
    }
  
    // Set the gain (optional, default is +/- 4.096V)
    analogBoard1.setGain(GAIN_ONE); // +/- 4.096V
    analogBoard2.setGain(GAIN_ONE); // +/- 4.096V
}

void loop(){

  ReadInputs();
  MapInValues();

  DetectChange(leonardo, 7, lastMappedSignalsLeonardo);
  DetectChange(ads1, 4, lastMappedSignalsAds1);
  Serial.println(ads1[0].readSignal);
  DetectChange(ads2, 5, lastMappedSignalsAds2);
    
  //Ensure all Midi messages are sent
  MidiUSB.flush();
}

// -------------------- MIDI HELPER ------------------------

void sendMIDIControlChange(byte control, byte value) {
    // CC on Channel 1 => 0xB0
    midiEventPacket_t event = {0x0B, 0xB0, control, value};
    MidiUSB.sendMIDI(event);
}

void ReadInputs()
{
  // Read all inputs first, then process them
  //read the main faders:
  ads2[1].readSignal = analogBoard2.readADC_SingleEnded(ads2[1].pinNumber);
  ads2[2].readSignal = analogBoard2.readADC_SingleEnded(ads2[2].pinNumber);

  //read the wavetype knobs:
  ads2[3].readSignal = analogBoard2.readADC_SingleEnded(ads2[3].pinNumber);
  ads1[1].readSignal = analogBoard1.readADC_SingleEnded(ads1[1].pinNumber);
  ads2[4].readSignal = analogBoard2.readADC_SingleEnded(ads2[4].pinNumber);

  //read the frequency knobs:
  ads1[2].readSignal = analogBoard1.readADC_SingleEnded(ads1[2].pinNumber);
  ads1[3].readSignal = analogBoard1.readADC_SingleEnded(ads1[3].pinNumber);
  leonardo[1].readSignal = analogRead(leonardo[1].pinNumber);

  //read the amps:
  leonardo[2].readSignal = analogRead(leonardo[2].pinNumber);
  leonardo[3].readSignal = analogRead(leonardo[3].pinNumber);
  leonardo[4].readSignal = analogRead(leonardo[4].pinNumber);

  //read the effects:
  leonardo[5].readSignal = analogRead(leonardo[5].pinNumber);
  leonardo[6].readSignal = analogRead(leonardo[6].pinNumber);
  ads1[0].readSignal = analogBoard1.readADC_SingleEnded(ads1[0].pinNumber);
}

void MapInValues()  //it works here
{
    //maps signal between inMin - inMax (not 0-127)

    //LEONARDO
    for(int i = 0; i < 7; i++){
      if(i<5)
      {
        leonardo[i].mappedSignal = map(leonardo[i].readSignal, leonardo[i].inMin, leonardo[i].inMax, leonardo[i].outMin, leonardo[i].outMax);
      }else{
        if(leonardo[i].readSignal < 500)  //default
        {
          leonardo[i].mappedSignal = 0;
        }else if(leonardo[i].readSignal > 500 && leonardo[i].readSignal < 1000)
        {
          leonardo[i].mappedSignal = 64;
        }else{
          leonardo[i].mappedSignal = 127;
        }

      }

    }

    //ADS 1
    for(int i = 0; i < 4; i++){
        if(i == 0)
        {
          if(ads1[i].readSignal < 5000)  //default
          {
            ads1[i].mappedSignal = 64;
          }
          else if(ads1[i].readSignal > 5000 && ads1[i].readSignal < 25000)
          {
            ads1[i].mappedSignal = 0;
          }else{
            ads1[i].mappedSignal = 127;
          }
          
        }else if(i == 2 || i == 3)
        { 
          ads1[i].mappedSignal = -map(ads1[i].readSignal, ads1[i].inMin, ads1[i].inMax, ads1[i].outMin, ads1[i].outMax);
        }
        else{

          ads1[i].mappedSignal = map(ads1[i].readSignal, ads1[i].inMin, ads1[i].inMax, ads1[i].outMin, ads1[i].outMax);
          
        }
    }

    //ADS 2
    for(int i = 0; i < 5; i++){
      ads2[i].mappedSignal = map(ads2[i].readSignal, ads2[i].inMin, ads2[i].inMax, ads2[i].outMin, ads2[i].outMax);
    }
}

void DetectChange(InputMapping * array, int arrayLength, int lastArray[])
{
  for(int i = 0; i < arrayLength; i++){ 
    if(abs(lastArray[i] - array[i].mappedSignal) > defineThreshHold) 
    {

      //Update last values AFTER comparison
      lastArray[i] = array[i].mappedSignal;

      //constrain mappedSignal between 0-127
      array[i].mappedSignal = constrain(array[i].mappedSignal, 0, 127);

      //Send MIDI event
      sendMIDIControlChange(array[i].midiChannel, array[i].mappedSignal);

    }
  }
}
