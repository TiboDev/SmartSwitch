#ifndef Rotary_Encoder_h
#define Rotary_Encoder_h

#include "Arduino.h"
#include "principal.h"

class CRotaryEncoder{
  private:
    unsigned char encoderLastPosition = LOW;
    unsigned char encoderActualPosition;
    unsigned char encoderPosition = 0;
    unsigned char state = NEUTRAL;
    void setLED(unsigned char vred, unsigned char vgreen, unsigned char vblue);
    bool previousStat = LOW;
  
  public:
    void setLED(unsigned char);
    CRotaryEncoder();
    bool isButtonPressed();
    unsigned char getPosition();
    void updatePosition();
};

#endif //Rotary_Encoder_h

