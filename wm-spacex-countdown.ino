// This #include statement was automatically added by the Particle IDE.
#include <ArduinoJson.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_mfGFX.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_HX8357.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

struct timespan_t {
    uint8_t seconds, minutes, hours;
    uint16_t days;
    timespan_t();
};

timespan_t::timespan_t() {
    seconds = 0;
    minutes = 0;
    hours = 0;
    days = 0;
}

// given a date in the future (as unix time), returns time until then (according
// to Time.now()
timespan_t& timeUntil(int future) {
    int seconds = future - Time.now();
    timespan_t* resultp = new timespan_t;
    timespan_t& result = *resultp;
    result.seconds = seconds % 60;
    result.minutes = (seconds / 60) % 60;
    result.hours = (seconds / 3600) % 24;
    result.days = seconds / 86400;
    return result;
}

void setup() {
    Serial.begin(9600);
    int future = 1517352748;
    timespan_t& countdown = timeUntil(future);
    Serial.print(countdown.days);
    Serial.print(":");
    Serial.print(countdown.hours);
    Serial.print(":");
    Serial.print(countdown.minutes);
    Serial.print(":");
    Serial.println(countdown.seconds);
}

void loop() {

}
