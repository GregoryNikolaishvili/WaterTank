#include "HASwitchX.h"

HASwitchX::HASwitchX(const char *uniqueId, const char *name, byte pinId)
    : HASwitch(uniqueId), _pinId(pinId)
{
  setState(LOW);
  setName(name);
  setIcon("mdi:pump");
}
