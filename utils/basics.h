struct smoothedparameter {
public:
	float value = 0;
	float target = 0;
	float speed = 1.f/9600.f;
	smoothedparameter(int sr = 96000, float s = 0.1f, float v = 0) {
		if(s < .001f) speed = 1.f;
		else speed = 1.f/(sr*s);
		value = v;
		target = v;
	};
	float nextvalue() {
		value += std::min(std::max(target-value,-speed),speed);
		return value;
	};
};

float maptolog10(float val, float min, float max) {
	float logmin = std::log10(min);
	float logmax = std::log10(max);
	return std::pow(10.f,val*(logmax-logmin)+logmin);
}
