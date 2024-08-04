#include "HASwitchX.h"

HASwitchX::HASwitchX(const char *uniqueId, const char *name, byte pinId, bool isInverted)
    : HASwitch(uniqueId), _pinId(pinId), _isInverted(isInverted)
{
  setState(isInverted ? HIGH : LOW);
  setName(name);
  setIcon("mdi:pump");

  pinMode(pinId, OUTPUT);
	digitalWrite(pinId, HIGH);
}
