// #ifndef Bounce_h
// #define Bounce_h

// #include <inttypes.h>

// class Bounce
// {

// public:
// 	// Initialize
// 	Bounce(uint8_t pin, bool isInverted, unsigned long interval_millis_rise, unsigned long interval_millis_fall);
// 	// Sets the debounce interval
// 	void interval(unsigned long interval_millis_rise, unsigned long interval_millis_fall);
// 	// Updates the pin
// 	// Returns 1 if the state changed
// 	// Returns 0 if the state did not change
// 	int update();
// 	// Forces the pin to signal a change (through update()) in X milliseconds
// 	// even if the state does not actually change
// 	// Example: press and hold a button and have it repeat every X milliseconds
// 	void rebounce(unsigned long interval);
// 	// Returns the updated pin state
// 	int read();
// 	// Sets the stored pin state
// 	void write(int new_state);
// 	// Returns the number of milliseconds the pin has been in the current state
// 	unsigned long duration();
// 	// The risingEdge method is true for one scan after the de-bounced input goes from off-to-on.
// 	bool risingEdge();
// 	// The fallingEdge  method it true for one scan after the de-bounced input goes from on-to-off.
// 	bool fallingEdge();

// protected:
// 	int debounce();
// 	unsigned long previous_millis, interval_millis_rise, interval_millis_fall, rebounce_millis;
// 	uint8_t state;
// 	uint8_t pin;
// 	uint8_t stateChanged;
// 	bool isInverted;
// };

// #endif
