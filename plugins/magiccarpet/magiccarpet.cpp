#include "daisy_pod.h"
#include "../../utils/basics.h"
#include "../../utils/dampening.h"
#include "../../utils/dc_filter.h"

#define MAX_DLY 6
#define MIN_DLY 0.f
#define DLINES 3

using namespace daisy;

dampendparameter delayamt[DLINES];
smoothedparameter feedbackamt;
smoothedparameter mixamt;

DaisyPod hw;

dc_filter dcfilter[2];
float DSY_SDRAM_BSS delay[96000*MAX_DLY*2];
float dindex[DLINES];
int blink[DLINES];
int samplerate = 96000;
int readpos = 0;
int buffersize = 96000*MAX_DLY;
bool ledstate = true;

float interpolatesamples(float position, int channel) {
	return
		delay[(((int)floor(position))%buffersize)*2+channel]*(1-fmod(position,1.f))+
		delay[(((int)ceil (position))%buffersize)*2+channel]*   fmod(position,1.f);
}

void AudioCallback(	AudioHandle::InterleavingInputBuffer	in,
					AudioHandle::InterleavingOutputBuffer	out,
					size_t									size) {
	float delout;
	float feedbackout;
	float pannedout;
	float pan;
	int linecount;
	for(size_t s = 0; s < size; s += 2) {
		linecount = 0;
		for(size_t d = 0; d < DLINES; ++d) {
			if(delayamt[d].target >= .999f) continue;
			++linecount;

			dindex[d] = ((readpos-1)-samplerate*(powf(delayamt[d].nextvalue(),2)*(MAX_DLY-MIN_DLY)+MIN_DLY))+buffersize*2;
			if(++blink[d] >= samplerate*(delayamt[d].value*(MAX_DLY-MIN_DLY)+MIN_DLY)) {
				hw.led1.Set(ledstate?1:0,0,0);
				hw.UpdateLeds();
				ledstate = !ledstate;
				blink[d] = 0;
			}
		}

		feedbackamt.nextvalue();
		mixamt.nextvalue();
		for(size_t c = 0; c < 2; ++c) {
			feedbackout = 0;
			pannedout = 0;
			if(linecount > 0) {
				for(size_t d = 0; d < DLINES; ++d) {
					if(delayamt[d].target >= .999f) continue;
					pan = ((float)d)/(DLINES-1);
					pan = c*(pan*2-1)+(1-pan);
					delout = interpolatesamples(dindex[d],c);
					feedbackout += delout;
					pannedout += delout*pan;
				}
				//feedbackout /= linecount;
				//pannedout /= linecount;
			}
			delay[readpos*2+c] = fmax(-50.f,fmin(50.f,dcfilter[c].process((feedbackamt.value*feedbackout)+in[s+c])));
			out[s+c] = mixamt.value*pannedout+(1-mixamt.value)*in[s+c];
		}
		readpos = fmod(readpos+1,buffersize);
	}
}

int main(void) {
	hw.Init();
	hw.SetAudioBlockSize(64);
	hw.StartAudio(AudioCallback);
	hw.midi.StartReceive();
	samplerate = hw.AudioSampleRate();

	mixamt = smoothedparameter(samplerate,.1f,.4f);
	feedbackamt = smoothedparameter(samplerate,.1f,.8f);
	for(size_t d = 0; d < DLINES; ++d)
		delayamt[d] = dampendparameter(samplerate,1.f,.32f+d*.1f);

	dcfilter[0] = dc_filter(samplerate);
	dcfilter[1] = dc_filter(samplerate);
	for(int i = 0; i < buffersize*2; ++i) delay[i] = 0;

	while(true) {
		hw.midi.Listen();
		while(hw.midi.HasEvents()) {
			auto msg = hw.midi.PopEvent();
			switch(msg.type) {
				case ControlChange: {
					ControlChangeEvent p = msg.AsControlChange();
					switch(p.control_number) {
						case 74:
							mixamt.target = p.value/127.f;
							break;
						case 71:
							feedbackamt.target = p.value/127.f;
							break;
						default:
							for(size_t d = 0; d < DLINES; ++d) {
								if((d+22) != p.control_number) continue;
								delayamt[d].target = p.value/127.f;
								break;
							}
							break;
					}
					break;
				}
				default: break;
			}
		}
	}
}
