#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed hw;
static Oscillator osc;

static void AudioCallback(	AudioHandle::InterleavingInputBuffer	in,
							AudioHandle::InterleavingOutputBuffer	out,
							size_t									size) {
	float sig;
	for(size_t i = 0; i < size; i += 2) {
		sig = osc.Process();
		out[i] = sig;
		out[i+1] = sig;
	}
}

int main(void) {
	hw.Configure();
	hw.Init();
	hw.SetAudioBlockSize(4);

	osc.Init(hw.AudioSampleRate());
	osc.SetWaveform(osc.WAVE_SIN);
	osc.SetFreq(1000);
	osc.SetAmp(0.5);

	hw.StartAudio(AudioCallback);

	while(true) {}
}
