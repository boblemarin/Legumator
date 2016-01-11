#include "MIDIUSB.h"
#include <Wire.h>
#include <Snowtouch.h>



#define NUM_SENSORS 12
#define MIDI_CHANNEL 1
#define BASE_NOTE 54
#define NOTE_STEP 3

#define THRESHOLD_ON 5
#define THRESHOLD_OFF 5

#define SERIAL_PRINT true

// First parameter is the event type (0x09 = note on, 0x08 = note off).
// Second parameter is note-on/note-off, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the velocity (64 = normal, 127 = fastest).

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  /*Serial.print("Note On ");
  Serial.print(pitch);
  Serial.println(velocity);*/
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  /*Serial.print("Note Off ");
  Serial.print(pitch);
  Serial.println(velocity);*/
}

// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
  /*Serial.print("CC ");
  Serial.print(control);
  Serial.println(value);*/
}

Snowtouch snowtouch;



int values[NUM_SENSORS];
int baselines[NUM_SENSORS];
bool states[NUM_SENSORS];
int average = 0;
int numAverage = 0;
int i = 0;

void setup()
{   
    // start serial port at 9600 bps and wait for port to open:
    Serial.begin(115200);
    Wire.begin();
    snowtouch.begin();

    delay(100);
    delay(100);
    delay(100);
    delay(100);
    getBaselines();
}

void loop() {
  midiEventPacket_t rx;
  do {
    rx = MidiUSB.read();
    if (rx.header != 0) {
      if (rx.byte2 == 1) {
        getBaselines();
        delay( 20 );
        return;
      }
    }
  } while (rx.header != 0);

  getData();
  delay( 10 );
}

void getBaselines() {
  snowtouch.read();
  for (i = 0; i < NUM_SENSORS; ++i) {
    values[i] = snowtouch.getTouch(i);
    baselines[i] = values[i];
    states[i] = false;
  }
}

bool getData() {
  // prepare
  snowtouch.read();
  average = 0;
  numAverage = 0;

  // collect values
  for (i = 0; i < NUM_SENSORS; ++i) {
    values[i] = snowtouch.getTouch(i);

    if (states[i]) {
      if (values[i] > baselines[i] - THRESHOLD_OFF) {
        states[i] = false;
        noteOff(MIDI_CHANNEL, BASE_NOTE + (NUM_SENSORS - i) * NOTE_STEP, baselines[i] - values[i]);
      } else {
        ++numAverage;
        average += (baselines[i] - values[i]);
      }
    } else {
      if (values[i] < baselines[i] - THRESHOLD_ON) {
          states[i] = true;
          noteOn(MIDI_CHANNEL, BASE_NOTE + (NUM_SENSORS - i) * NOTE_STEP, baselines[i] - values[i]);
          ++numAverage;
          average += (baselines[i] - values[i]);
        }
    }
  }

  // compute average
  if (numAverage > 0) {
    average /= numAverage;
    controlChange(MIDI_CHANNEL, 11, constrain(average, 0, 127));
  }

  MidiUSB.flush();
  
  return true;
}

