#include "daisy_seed.h"
#include "../../utils/basics.h"
#include "../../utils/dampening.h"

#define MAX_DLY 6
#define MIN_DLY 0.02

using namespace daisy;

dampendparameter delayamt;
smoothedparameter feedbackamt;
smoothedparameter mixamt;

DaisySeed hw;
MidiUsbHandler midi;

float DSY_SDRAM_BSS delay[96000*MAX_DLY*2];
int samplerate = 44100;
int readpos = 0;
int buffersize = 96000*MAX_DLY;
int blink = 0;
bool ledstate = true;

float interpolatesamples(float position, int channel) {
	return
		delay[(((int)floor(position))%buffersize)*2+channel]*(1-fmod(position,1.f))+
		delay[(((int)ceil (position))%buffersize)*2+channel]*   fmod(position,1.f);
}

void AudioCallback(	AudioHandle::InterleavingInputBuffer	in,
					AudioHandle::InterleavingOutputBuffer	out,
					size_t									size) {
	float index;
	float delout;
	for(size_t s = 0; s < size; s += 2) {
		delayamt.nextvalue();
		feedbackamt.nextvalue();
		mixamt.nextvalue();
		index = ((readpos-1)-samplerate*(delayamt.value*(MAX_DLY-MIN_DLY)+MIN_DLY))+buffersize*2;
		for(size_t c = 0; c < 2; ++c) {
			delout = interpolatesamples(index,c);
			delay[readpos*2+c] = (feedbackamt.value*delout)+in[s+c];
			out[s+c] = mixamt.value*delout+(1-mixamt.value)*in[s+c];
		}
		readpos = fmod(readpos+1,buffersize);

		if(++blink >= samplerate*(delayamt.value*(MAX_DLY-MIN_DLY)+MIN_DLY)) {
			hw.SetLed(ledstate);
			ledstate = !ledstate;
			blink = 0;
		}
	}
}

int main(void) {
	hw.Configure();
	hw.Init();
	hw.SetAudioBlockSize(4);
	hw.StartAudio(AudioCallback);
	samplerate = hw.AudioSampleRate();

	MidiUsbHandler::Config midi_cfg;
	midi.Init(midi_cfg);

	delayamt = dampendparameter(samplerate,1.f,.32f);
	feedbackamt = smoothedparameter(samplerate,.1f,.8f);
	mixamt = smoothedparameter(samplerate,.1f,.4f);
	for(int i = 0; i < buffersize*2; ++i) delay[i] = 0;

	while(true) {

		midi.Listen();

		while(midi.HasEvents()) {
			auto msg = midi.PopEvent();
			switch(msg.type) {
				case ControlChange: {
					ControlChangeEvent p = msg.AsControlChange();
					switch(p.control_number) {
						case 11:
							delayamt.target = p.value/127.f;
							break;
						case 12:
							feedbackamt.target = p.value/127.f;
							break;
						case 13:
							mixamt.target = p.value/127.f;
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
