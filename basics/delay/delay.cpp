#include "daisy_seed.h"

#define MAX_DLY 6.0
#define MIN_DLY 0.02

using namespace daisy;

static DaisySeed hw;
float DSY_SDRAM_BSS delay[96000*6*2];
int samplerate = 44100;
int readpos = 0;
int buffersize = 96000*6;
float delayamt = .75;
float feedbackamt = .8;
float mixamt = .4f;
int blink = 0;
bool ledstate = true;

static void AudioCallback(	AudioHandle::InterleavingInputBuffer	in,
							AudioHandle::InterleavingOutputBuffer	out,
							size_t									size) {
	int index;
	float delout;
	for(size_t s = 0; s < size; s += 2) {
		index = fmod(((readpos-1)-delayamt*samplerate)+buffersize*2,buffersize);
		for(size_t c = 0; c < 2; ++c) {
			delout = delay[index*2+c];
			delay[readpos*2+c] = (feedbackamt*delout)+in[s+c];
			out[s+c] = mixamt*delout+(1-mixamt)*in[s+c];
		}
		readpos = fmod(readpos+1,buffersize);

		if(++blink >= delayamt*samplerate) {
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
	for(int i = 0; i < buffersize*2; ++i) delay[i] = 0;
	hw.StartAudio(AudioCallback);
	while(true) {}
}
