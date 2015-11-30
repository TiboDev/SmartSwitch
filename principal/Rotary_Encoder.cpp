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
    case WHITE:
      setLED(HIGH, HIGH, HIGH);
      break;
    case NONE:
      setLED(LOW, LOW, LOW);
      break;
  }
}

bool CRotaryEncoder::isButtonPressed()
{
  if (digitalRead(ENCODER_BUTTON) == HIGH)
    return true;
  else
    return false;
}

unsigned char CRotaryEncoder::sendPosition()
{
  encoderActualPosition = digitalRead(ENCODER_A);
  if ((encoderLastPosition == LOW) && (encoderActualPosition == HIGH)) {
    if (digitalRead(ENCODER_B) == LOW)
      encoderPosition--;
    else
      encoderPosition++;

    Serial.print(encoderPosition);
    Serial.print("/");
  }
  encoderLastPosition = encoderActualPosition;
  return encoderPosition;
}

