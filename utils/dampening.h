struct dampendparameter {
public:
	float value = 0;
	float target = 0;
	float speed = .1f;
	float velocity = 0;
	int samplerate = 44100;
	dampendparameter(int sr = 44100, float s = .1f, float v = 0) {
		samplerate = sr;
		speed = s;
		value = v;
		target = v;
		velocity = 0;
	};
	float nextvalue() {
		float s = fmax(.0001,speed);
		float num1 = 2./s;
		float num2 = num1/((float)samplerate);
		float num3 = 1./(1+num2+.48*num2*num2+.235*num2*num2*num2);
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

