#ifndef _HAVALVEX_H
#define _HAVALVEX_H

#include <ArduinoHA.h>
//#include "HABaseDeviceType.h"

#ifndef EX_ARDUINOHA_VALVEX

#define HAVALVEX_CALLBACK(name) void (*name)(ValveCommand cmd, HAValveX* sender)

/**
 * HAValveX allows to control a valve.
 *
 * @note
 * You can find more information about this entity in the Home Assistant documentation:
 * https://www.home-assistant.io/integrations/valve.mqtt/
 */
class HAValveX : public HABaseDeviceType
{
public:
    static const int16_t DefaultPosition = -32768;

    enum ValveState {
        StateUnknown = 0,
        StateClosed,
        StateClosing,
        StateOpen,
        StateOpening,
        StateStopped
    };

    enum ValveCommand {
        CommandOpen,
        CommandClose,
        CommandStop
    };

    enum Features {
        DefaultFeatures = 0,
        PositionFeature = 1
    };

    /**
     * @param uniqueId The unique ID of the valve. It needs to be unique in a scope of your device.
     * @param features Features that should be enabled for the fan.
     */
    HAValveX(const char* uniqueId, const Features features = DefaultFeatures);

    /**
     * Changes state of the valve and publishes MQTT message.
     * Please note that if a new value is the same as previous one,
     * the MQTT message won't be published.
     *
     * @param state New state of the valve.
     * @param force Forces to update state without comparing it to previous known state.
     * @returns Returns true if MQTT message has been published successfully.
     */
    bool setState(const ValveState state, const bool force = false);

    /**
     * Changes the position of the valve and publishes MQTT message.
     * Please note that if a new value is the same as previous one,
     * the MQTT message won't be published.
     *
     * @param position The new position of the valve (0-100).
     * @param force Forces to update the state without comparing it to a previous known state.
     * @returns Returns `true` if MQTT message has been published successfully.
     */
    bool setPosition(const int16_t position, const bool force = false);

    /**
     * Sets the current state of the valce without publishing it to Home Assistant.
     * This method may be useful if you want to change the state before the connection
     * with the MQTT broker is acquired.
     *
     * @param state The new state of the valve.
     */
    inline void setCurrentState(const ValveState state)
        { _currentState = state; }

    /**
     * Returns last known state of the valve.
     * By default the state is set to ValveState::StateUnknown
     */
    inline ValveState getCurrentState() const
        { return _currentState; }

    /**
     * Sets the current position of the valve without pushing the value to Home Assistant.
     * This method may be useful if you want to change the position before the connection
     * with the MQTT broker is acquired.
     *
     * @param position The new position of the valve (0-100).
     */
    inline void setCurrentPosition(const int16_t position)
        { _currentPosition = position; }

    /**
     * Returns the last known position of the valve.
     * By default position is set to HAValveX::DefaultPosition
     */
    inline int16_t getCurrentPosition() const
        { return _currentPosition; }

    /**
     * Sets class of the device.
     * You can find list of available values here: https://www.home-assistant.io/integrations/valve/
     *
     * @param deviceClass The class name.
     */
    inline void setDeviceClass(const char* deviceClass)
        { _class = deviceClass; }

    /**
     * Sets icon of the valve.
     * Any icon from MaterialDesignIcons.com (for example: `mdi:home`).
     *
     * @param icon The icon name.
     */
    inline void setIcon(const char* icon)
        { _icon = icon; }

    /**
     * Sets retain flag for the valve's command.
     * If set to `true` the command produced by Home Assistant will be retained.
     *
     * @param retain
     */
    inline void setRetain(const bool retain)
        { _retain = retain; }

    /**
     * Sets optimistic flag for the valve state.
     * In this mode the valve state doesn't need to be reported back to the HA panel when a command is received.
     * By default the optimistic mode is disabled.
     *
     * @param optimistic The optimistic mode (`true` - enabled, `false` - disabled).
     */
    inline void setOptimistic(const bool optimistic)
        { _optimistic = optimistic; }

    /**
     * Registers callback that will be called each time the command from HA is received.
     * Please note that it's not possible to register multiple callbacks for the same valve.
     *
     * @param callback
     */
    inline void onCommand(HAVALVEX_CALLBACK(callback))
        { _commandCallback = callback; }

protected:
    virtual void buildSerializer() override;
    virtual void onMqttConnected() override;
    virtual void onMqttMessage(
        const char* topic,
        const uint8_t* payload,
        const uint16_t length
    ) override;

private:
    /**
     * Publishes the MQTT message with the given state.
     *
     * @param state The state to publish.
     * @returns Returns `true` if the MQTT message has been published successfully.
     */
    bool publishState(const ValveState state);

    /**
     * Publishes the MQTT message with the given position.
     *
     * @param position The position to publish.
     * @returns Returns `true` if the MQTT message has been published successfully.
     */
    bool publishPosition(const int16_t position);

    /**
     * Parses the given command and executes the valve's callback with proper enum's property.
     *
     * @param cmd The data of the command.
     * @param length Length of the command.
     */
    void handleCommand(const uint8_t* cmd, const uint16_t length);

    /// Features enabled for the valve.
    const uint8_t _features;

    /// The current state of the valve. By default it's `HAValveX::StateUnknown`.
    ValveState _currentState;

    /// The current position of the valve. By default it's `HAValveX::DefaultPosition`.
    int16_t _currentPosition;

    /// The device class. It can be nullptr.
    const char* _class;

    /// The icon of the button. It can be nullptr.
    const char* _icon;

    /// The retain flag for the HA commands.
    bool _retain;

    /// The optimistic mode of the valve (`true` - enabled, `false` - disabled).
    bool _optimistic;

    /// The command callback that will be called when clicking the valve's button in the HA panel.
    HAVALVEX_CALLBACK(_commandCallback);
};

#endif
#endif
