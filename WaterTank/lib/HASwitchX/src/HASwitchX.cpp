#include "HASwitchX.h"

HASwitchX::HASwitchX(const char *uniqueId, const char *name, byte pinId, bool isInverted)
    : HASwitch(uniqueId), _pinId(pinId), _isInverted(isInverted)
{
  setCurrentState(false);
  setName(name);
  setIcon("mdi:water-pump");
  setDeviceClass("switch");

  pinMode(pinId, OUTPUT);
  digitalWrite(pinId, HIGH);
}
