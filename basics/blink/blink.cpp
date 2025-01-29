#include "daisy_seed.h"

using namespace daisy;

DaisySeed hw;
bool ledstate = true;
int blink = 0;
int samplerate = 44100;

void AudioCallback(	AudioHandle::InterleavingInputBuffer	in,
					AudioHandle::InterleavingOutputBuffer	out,
					size_t									size) {
	for(size_t s = 0; s < size; s += 2) {
		out[s] = in[s];
		out[s+1] = in[s+1];
		if(++blink >= samplerate/2) {
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
	samplerate = hw.AudioSampleRate();
	hw.StartAudio(AudioCallback);
	while(true) {}
}
