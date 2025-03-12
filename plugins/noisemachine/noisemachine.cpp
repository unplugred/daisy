#include "daisy_pod.h"
#include "../../utils/basics.h"
#include "../../utils/dampening.h"
#include "../../utils/dc_filter.h"

struct peakfilter {
	const float p4 = 1.0e-24;
	const float pi = 3.141592654;
	float r = 0;
	float f = 0;
	float a0 = 0;
	float b1 = 0;
	float b2 = 0;
	float outp [4] = {0,0,0,0};
	float outp1[4] = {0,0,0,0};
	float outp2[4] = {0,0,0,0};
	float mag = 0;

	void recalculate(float cutoff, float q) {
		r = q*0.99609375;
		f = cos(pi*cutoff);
		a0 = (1-r)*sqrt(r*(r-4*(f*f)+2)+1);
		b1 = 2*f*r;
		b2 = -(r*r);
	}

	float process(float input, int channel) {
		outp [channel] = a0*input+b1*outp1[channel]+b2*outp2[channel]+p4;
		outp2[channel] = outp1[channel];
		outp1[channel] = outp [channel];
		return outp[channel];
	}
};

#define MAX_DLY 6
#define MIN_DLY 0.f
#define DLINES 2

using namespace daisy;

dampendparameter delayamt[DLINES];
smoothedparameter feedbackamt;
smoothedparameter mixamt;
smoothedparameter bstdiramt;
smoothedparameter bstamt;
smoothedparameter stroamt;
smoothedparameter inputmute[2];
smoothedparameter effectamt;
smoothedparameter effectfrq;
peakfilter filter;
int effecttyp;

DaisyPod hw;

dc_filter dcfilter[2];
float DSY_SDRAM_BSS delay[96000*MAX_DLY*2];
float dindex[DLINES];
int blink[DLINES];
int samplerate = 96000;
int readpos = 0;
int buffersize = 96000*MAX_DLY;
bool ledstate[DLINES];
float effectcycle = 0;
float lastinput[4];

float interpolatesamples(float position, int channel) {
	return
		delay[(((int)floor(position))%buffersize)*2+channel]*(1-fmod(position,1.f))+
		delay[(((int)ceil (position))%buffersize)*2+channel]*   fmod(position,1.f);
}

float boost(float input) {
	if(bstamt.value <= 0) return input;
	if(bstdiramt.value > .5f) {
		return pow(fabs(input),1-bstamt.value*.99)*(input>0?1:-1);
	} else {
		if(bstamt.value >= 1) {
			return 0;
		} else {
			float cropmount = pow(bstamt.value,10);
			float output = (input-fmin(fmax(input,-cropmount),cropmount))/(1-cropmount);
			if(output > -1 && output < 1)
				output = output*pow(1-pow(1-fabs(output),12.5),(3/(1-bstamt.value*.98))-3);
			return output;
		}
	}
}

float effect(float input, int channel) {
	if(effecttyp < .5f) {
		return input*(sin(effectcycle*6.2831853072f)*effectamt.value+(1-effectamt.value));
	}

	if(effecttyp < 1.5f) {
		if(effectamt.value <= .001f) return 0;
		float out = lastinput[channel];
		if(effectfrq.value >= .999f) {
			out = input;
			lastinput[channel] = input;
		} else if(effectcycle < 1) {
			out = input*effectcycle+lastinput[channel]*(1-effectcycle);
			lastinput[channel] = input;
		}
		if(effectfrq.value <= .001f) return 0;
		if(effectamt.value >= .999f) return out;
		return round(out*(1.f/(1-effectamt.value)))*(1-effectamt.value);
	}

	return input*(1-effectamt.value)+filter.process(input,channel)*effectamt.value;
}

float stereowiden(float input, int channel) {
	if(stroamt.value <= 0) return input;
	return input*(stroamt.value*(channel-.5f)*2*(input>0?1:-1)+1);
}

void AudioCallback(	AudioHandle::InterleavingInputBuffer	in,
					AudioHandle::InterleavingOutputBuffer	out,
					size_t									size) {
	float delout;
	float feedbackout;
	float pannedout;
	float pan;
	float dlytime = 0;
	for(size_t s = 0; s < size; s += 2) {
		for(size_t d = 0; d < DLINES; ++d) {
			dlytime = samplerate*(powf(delayamt[d].nextvalue(),2)*(MAX_DLY-MIN_DLY)+MIN_DLY);
			dindex[d] = ((readpos-1)-dlytime)+buffersize*2;
			if(++blink[d] >= dlytime) {
				hw.led1.Set(ledstate[0]?.5:0,0,ledstate[1]?.5:0);
				hw.UpdateLeds();
				ledstate[d] = !ledstate[d];
				blink[d] = 0;
			}
		}

		feedbackamt.nextvalue();
		mixamt.nextvalue();
		bstamt.nextvalue();
		effectamt.nextvalue();
		effectfrq.nextvalue();
		bstdiramt.nextvalue();
		stroamt.nextvalue();
		if(effecttyp < .5f) {
			effectcycle = fmod(effectcycle+(maptolog10(effectfrq.value,20,20000)/samplerate),1);
		}
		if(effecttyp < 1.5f) {
			effectcycle = fmod(effectcycle+1,samplerate/maptolog10(effectfrq.value,20,20000));
		}
		for(size_t c = 0; c < 2; ++c) {
			feedbackout = 0;
			pannedout = 0;
			for(size_t d = 0; d < DLINES; ++d) {
				pan = ((float)d)/(DLINES-1);
				pan = c*(pan*2-1)+(1-pan);
				delout = interpolatesamples(dindex[d],c);
				feedbackout += delout;
				pannedout += delout*pan;
			}
			float stroin = stereowiden(in[s+c],c);
			delay[readpos*2+c] = fmax(-50.f,fmin(50.f,boost(effect(dcfilter[c].process((feedbackamt.value*feedbackout)+stroin*inputmute[c].nextvalue()),c))));
			out[s+c] = boost(mixamt.value*pannedout+(1-mixamt.value)*effect(stroin,c+2));
		}
		readpos = fmod(readpos+1,buffersize);
	}
}

int main(void) {
	hw.Init();
	hw.SetAudioBlockSize(512);
	hw.StartAudio(AudioCallback);
	hw.midi.StartReceive();
	samplerate = hw.AudioSampleRate();

	mixamt = smoothedparameter(samplerate,.1f,0.f);
	feedbackamt = smoothedparameter(samplerate,.1f,0.f);
	bstamt = smoothedparameter(samplerate,.1f,.8f);
	bstdiramt = smoothedparameter(samplerate,0,1.f);
	effectamt = smoothedparameter(samplerate,.1f,0.f);
	effectfrq = smoothedparameter(samplerate,.1f,.4f);
	effecttyp = 0;
	stroamt = smoothedparameter(samplerate,.1f,.8f);
	for(size_t d = 0; d < DLINES; ++d)
		delayamt[d] = dampendparameter(samplerate,1.f,.32f+d*.1f);
	for(size_t c = 0; c < 2; ++c) {
		inputmute[c] = smoothedparameter(samplerate,.1f,1.f);
		lastinput[c] = 0;
		lastinput[c+2] = 0;
	}

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

						// EFCT
						case 3: // AMT
							effectamt.target = p.value/127.f;
							break;
						case 9: // FRQ
							effectfrq.target = p.value/127.f;
							filter.recalculate(pow(p.value/127.f,2),.9f);
							break;
						case 12: // TYP
							effecttyp = p.value;
							break;

						case 20: // STRO
							stroamt.target = p.value/127.f;
							break;

						// TODO:
						// ----
						// TREM
						// RVRS
						// BEEP
						// CHOP

						// BST
						case 25: // DIR
							bstdiramt.target = p.value;
							break;
						case 26: // AMT
							bstamt.target = p.value/127.f;
							break;

						// DLY
						case 73: // MIX
							mixamt.target = p.value/127.f;
							break;
						case 75: // FDBK
							feedbackamt.target = p.value/127.f;
							break;
						case 27: // DLY1
							delayamt[0].target = p.value/127.f;
							break;
						case 72: // DLY2
							delayamt[1].target = p.value/127.f;
							break;
						case 93: // MUTE
							inputmute[0].target = 1-floor(p.value/2.f);
							inputmute[1].target = 1-fmod(p.value,2.f);
							break;

						default:
							break;
					}
					break;
				}
				default: break;
			}
		}
	}
}
