#include "daisysp.h"
#include "daisy_seed.h"
#include "../../utils/basics.h"

using namespace daisysp;
using namespace daisy;

smoothedparameter amp;
smoothedparameter freq;

DaisySeed hw;
Oscillator osc;
MidiUsbHandler midi;
int print = 0;
bool led = true;
int samplerate = 44100;

void AudioCallback(	AudioHandle::InterleavingInputBuffer	in,
					AudioHandle::InterleavingOutputBuffer	out,
					size_t									size) {
	float sig;
	for(size_t i = 0; i < size; i += 2) {
		osc.SetAmp(amp.nextvalue());
		osc.SetFreq(freq.nextvalue());
		sig = osc.Process();
		out[i] = sig;
		out[i+1] = sig;
	}
}

int main(void) {
	hw.Configure();
	hw.Init();
	hw.SetAudioBlockSize(4);
	hw.StartLog();
	samplerate = hw.AudioSampleRate();

	MidiUsbHandler::Config midi_cfg;
	midi.Init(midi_cfg);

	amp = smoothedparameter(samplerate,.1f,.5f);
	amp.value = 0;
	freq = smoothedparameter(samplerate,.0001f,1000.f);

	osc.Init(samplerate);
	osc.SetWaveform(osc.WAVE_SIN);

	hw.StartAudio(AudioCallback);

	while(true) {

		if(++print > 10000000) {
			hw.SetLed(led);
			led = !led;
			print = 0;
		}

		midi.Listen();

		while(midi.HasEvents()) {
			auto msg = midi.PopEvent();
			switch(msg.type) {
				case NoteOn: {
					auto note_msg = msg.AsNoteOn();
					amp.target = note_msg.velocity/127.f;
					freq.target = mtof(note_msg.note);
					break;
				}
				case NoteOff: {
					amp.target = 0.f;
					break;
				}
				case ControlChange: {
					ControlChangeEvent p = msg.AsControlChange();
					switch(p.control_number) {
						case 11:
							freq.target = maptolog10(p.value/127.f,20,20000);
							break;
						case 12:
							amp.target = p.value/127.f;
							break;
						default: break;
					}
					break;
				}
				default: break;
			}
		}
	}
}
