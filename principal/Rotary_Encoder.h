#ifndef Rotary_Encoder_h
#define Rotary_Encoder_h

#include "Arduino.h";

class CRotaryEncoder{
	private:
		unsigned char encoderLastPosition = LOW;
		unsigned char encoderActualPosition;
		unsigned char encoderPosition = 0;
    void setLED(unsigned char vred, unsigned char vgreen, unsigned char vblue);
	
	public:
    void setLED(unsigned char);
		CRotaryEncoder();
		bool isButtonPressed();
		unsigned char sendPosition();
};

#endif //Rotary_Encoder_h
