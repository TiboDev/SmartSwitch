#include "Rotary_Encoder.h"
#include "principal.h"

CRotaryEncoder::CRotaryEncoder()
{
  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);
  pinMode(ENCODER_RED, OUTPUT);
  pinMode(ENCODER_GREEN, OUTPUT);
  pinMode(ENCODER_BLUE, OUTPUT);
  pinMode(ENCODER_BUTTON, INPUT);
  state = NEUTRAL;

};

void CRotaryEncoder::setLED(unsigned char vred, unsigned char vgreen, unsigned char vblue)
{
  digitalWrite(ENCODER_RED, vred);
  digitalWrite(ENCODER_GREEN, vgreen);
  digitalWrite(ENCODER_BLUE, vblue);
}

void CRotaryEncoder::setLED(unsigned char color)
{
  switch (color)
  {
    case RED:
      setLED(HIGH, LOW, LOW);
      break;
    case GREEN:
      setLED(LOW, HIGH, LOW);
      break;
    case BLUE:
      setLED(LOW, LOW, HIGH);
      break;
    case NONE:
      setLED(LOW, LOW, LOW);
      break;
  }
}

bool CRotaryEncoder::isButtonPressed()
{
  if (digitalRead(ENCODER_BUTTON) == HIGH)
  {
    if (previousStat == LOW)
    {
      Serial.println("button pressed");
      previousStat = HIGH;
      return true;
    }
    else
      return false;
  }
  else
  {
    previousStat = LOW;
    return false;
  }
}

unsigned char CRotaryEncoder::getPosition()
{
  unsigned stateReturn = state;
  state = NEUTRAL;
  return stateReturn;
}

void CRotaryEncoder::updatePosition()
{
  if (state == NEUTRAL)
  {
    encoderActualPosition = digitalRead(ENCODER_A);
    if ((encoderLastPosition == LOW) && (encoderActualPosition == HIGH)) {
      if (digitalRead(ENCODER_B) == LOW)
      {
        encoderPosition--;
        state = DECREASE;
      }
      else
      {
        encoderPosition++;
        state = INCREASE;
      }
    }
    encoderLastPosition = encoderActualPosition;
  }
}


