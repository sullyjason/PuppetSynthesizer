//toggle debug mode-----------------------------------------------------------------------------------------------------

#define DEBUG 1


//libraries-------------------------------------------------------------------------------------------------------------

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <MIDIUSB.h>

Adafruit_ADS1115 analogBoard1;    // First ADS1115 at address 0x48
Adafruit_ADS1115 analogBoard2;    // Second ADS1115 at address 0x49

//output ranges---------------------------------------------------------------------------------------------------------

#define OUT_MIN 0               // MIDI range minimum
#define OUT_MAX 127             // MIDI range maximum  
#define defineThreshHold 1      // Threshhold for checking for changes in MIDI values (reduced to 1)


//input mappings---------------------------------------------------------------------------------------------------------

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

InputMapping mappings[] = {

//  Number   functionName       pinNumber   readSignal  inMin, inMax    outMin, outMax  	mappedSignal    midiChannel
// -----------------------------------------------------------------------------------------------------------------------
    {0,      "EMPTY",            0,            0,            0,  0,            OUT_MIN, OUT_MAX,   0,                0            },
    {1,      "MAIN_SPEED",        A2,            0,            1030, 40,        OUT_MIN, OUT_MAX,    0,                9            },
    {2,      "MAIN_FREQ",        A1,            0,            1030, 40,        OUT_MIN, OUT_MAX,    0,                8            },

    {3,      "WAVE_TYPE_1",        A0,            0,            40, 1030,        OUT_MIN, OUT_MAX,    0,                15            },

    {4,      "WAVE_TYPE_2",        3,          0,            0, 33000,        OUT_MIN, OUT_MAX,    0,                16            },
    {5,      "WAVE_TYPE_3",        0,            0,            0, 33000,        OUT_MIN, OUT_MAX,    0,                17            },
    {6,      "FREQ_1",            2,            0,            0, 33000,        OUT_MIN, OUT_MAX,    0,                10            },
    {7,      "FREQ_2",            0,            0,            0, 33000,        OUT_MIN, OUT_MAX,    0,                11            },
    {8,      "FREQ_3",            2,            0,            0, 33000,     OUT_MIN, OUT_MAX,    0,                12            },

    {9,      "AMP_1",            3,            0,            33000, 1500,    OUT_MIN, OUT_MAX,    0,                1            },
    {10,     "AMP_2",            1,            0,            33000, 1500,    OUT_MIN, OUT_MAX,    0,                2            },
    {11,     "AMP_3",            1,            0,            33000, 1500,    OUT_MIN, OUT_MAX,    0,                3            },


    {12,     "EFFECT_1",        A4,            0,            1030, 40,        OUT_MIN, OUT_MAX,    0,                32            },
    {13,     "EFFECT_2",        A5,            0,            1030, 40,        OUT_MIN, OUT_MAX,    0,                33            },
    {14,     "EFFECT_3",        A6,            0,            1030, 40,        OUT_MIN, OUT_MAX,    0,                34            },
};        

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

    //create temporary variables to check for changes
    int lastMappedSignals[15];

    //set lastMappedSignals to current value
    for(int i = 0; i < 15; i++){
        lastMappedSignals[i] = mappings[i].mappedSignal;
    }

    //read the main faders:
    mappings[1].readSignal = analogRead(mappings[1].pinNumber);
    mappings[2].readSignal = analogRead(mappings[2].pinNumber);

    //read the wavetype knobs:
    mappings[3].readSignal = analogRead(mappings[3].pinNumber);
    mappings[4].readSignal = analogBoard2.readADC_SingleEnded(mappings[4].pinNumber);
    mappings[5].readSignal = analogBoard2.readADC_SingleEnded(mappings[5].pinNumber);

    //read the frequency knobs:
    mappings[6].readSignal = analogBoard1.readADC_SingleEnded(mappings[6].pinNumber);
    mappings[7].readSignal = analogBoard1.readADC_SingleEnded(mappings[7].pinNumber);
    mappings[8].readSignal = analogBoard2.readADC_SingleEnded(mappings[8].pinNumber);

    //read the amps:
    mappings[9].readSignal = analogBoard1.readADC_SingleEnded(mappings[9].pinNumber);
    mappings[10].readSignal = analogBoard1.readADC_SingleEnded(mappings[10].pinNumber);
    mappings[11].readSignal = analogBoard2.readADC_SingleEnded(mappings[11].pinNumber);

    //read the effects:
    mappings[12].readSignal = analogRead(mappings[12].pinNumber);
    mappings[13].readSignal = analogRead(mappings[13].pinNumber);
    mappings[14].readSignal = analogRead(mappings[14].pinNumber);

    //maps signal between 0 - 127
    for(int i = 0; i < 15; i++){
        mappings[i].mappedSignal = abs(map(mappings[i].readSignal, mappings[i].inMin, mappings[i].inMax, mappings[i].outMin, mappings[i].outMax));
    }

    //checks if theres a difference between last and current Value - if yes send MIDI EVENT
    for(int i = 0; i < 15; i++){ 
        if(lastMappedSignals[i] - mappings[i].mappedSignal > defineThreshHold) {
            if(i < 12){
                sendMIDIControlChange(mappings[i].midiChannel,abs(mappings[i].mappedSignal));
                #if DEBUG 
                    Serial.print(mappings[i].functionName); Serial.print(": Pin "); Serial.print(mappings[i].pinNumber);
                    Serial.print(" Read Signal: "); Serial.println(mappings[i].readSignal);
                    Serial.print(" Mapped Signal: "); Serial.println(mappings[i].mappedSignal);
                #endif
            }
        }
    }
    
    // Print the entire mappings table
    #if DEBUG
    printMappingsTable();
    #endif
    
    //Ensure all Midi messages are sent
    MidiUSB.flush();

    // Add a small delay to reduce the frequency of prints
    delay(500);
}

// Function to print the entire mappings table
void printMappingsTable() {
    Serial.println("Mappings Table:");
    Serial.println("--------------------------------------------------");
    Serial.println("Number | Function Name | Pin | Read Signal | Mapped Signal");
    Serial.println("--------------------------------------------------");
    for(int i = 0; i < 15; i++){
        Serial.print(mappings[i].inputNumber); Serial.print("   | ");
        Serial.print(mappings[i].functionName); Serial.print("       | ");
        Serial.print(mappings[i].pinNumber); Serial.print("       | ");
        Serial.print(mappings[i].readSignal); Serial.print("       | ");
        Serial.println(mappings[i].mappedSignal);
    }
    Serial.println("--------------------------------------------------");
}



// -------------------- MIDI HELPER ------------------------

void sendMIDIControlChange(byte control, byte value) {
    // CC on Channel 1 => 0xB0
    midiEventPacket_t event = {0x0B, 0xB0, control, value};
    MidiUSB.sendMIDI(event);
}