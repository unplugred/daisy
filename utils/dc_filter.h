//adapted from https://musicdsp.org/en/latest/Filters/135-dc-filter.html

struct dc_filter {
	float R = 1-(31.5f/96000);
	float previn = 0;
	float prevout = 0;

	dc_filter(int samplerate = 96000) {
		R = 1-(31.5f/samplerate); //CUTOFF 5Hz
		previn = 0.f;
		prevout = 0.f;
	}

	float process(float in) {
		float out = in-previn+prevout*R;
		previn = in;
		prevout = out;
		return out;
	}

	void reset() {
		previn = 0;
		prevout = 0;
	}
};
