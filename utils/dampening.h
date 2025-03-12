struct dampendparameter {
public:
	float value = 0;
	float target = 0;
	float speed = .1f;
	float velocity = 0;
	int samplerate = 96000;
	dampendparameter(int sr = 96000, float s = .1f, float v = 0) {
		samplerate = sr;
		speed = s;
		value = v;
		target = v;
		velocity = 0;
	};
	float nextvalue() {
		float s = fmax(.001f,speed);
		float num1 = 2.f/s;
		float num2 = num1/((float)samplerate);
		float num3 = 1.f/(1+num2+.48f*num2*num2+.235f*num2*num2*num2);
		float num4 = value-target;
		float num5 = (velocity+num1*num4)/((float)samplerate);
		float num6 = target+(num4+num5)*num3;
		if((target-value > 0) == (num6 > target)) {
			num6 = target;
			velocity = 0;
		} else velocity = (velocity-num1*num5)*num3;
		value = num6;
		return value;
	};
};
