#ifndef _HASWITCHX_H
#define _HASWITCHX_H

#include <ArduinoHA.h>

class HASwitchX : public HASwitch
{
public:
    HASwitchX(const char *uniqueId, const char *name, byte pinId, bool isInverted);

    inline byte getPin() const
    {
        return _pinId;
    }

    inline bool IsInverted() const
    {
        return _isInverted;
    }

private:
    byte _pinId;
    bool _isInverted;
};

#endif
