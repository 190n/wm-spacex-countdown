// This #include statement was automatically added by the Particle IDE.
#include <ArduinoJson.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_mfGFX.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_HX8357.h>

struct timespan_t {
    uint8_t seconds, minutes, hours;
    uint16_t days;
    timespan_t();
    void operator--();
};

// UNIX timestamp of next launch
int nextLaunchTimestamp;
// webhook response will be put here as we get data
// stringstream partialResponse;
bool receivingData = false;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

char countdownText[12];

uint8_t loopCounter = 0;

timespan_t untilNextLaunch;

Adafruit_HX8357 tft = Adafruit_HX8357(D6, D7, D5, D3, -1, D4);

timespan_t::timespan_t() {
    seconds = 0;
    minutes = 0;
    hours = 0;
    days = 0;
}

void timespan_t::operator--() {
    if (seconds == 0) {
        seconds = 59;
        if (minutes == 0) {
            minutes = 59;
            if (hours == 0) {
                hours = 23;
                if (days == 0) {
                    days = 0;
                    hours = 0;
                    minutes = 0;
                    seconds = 0;
                    return;
                } else days--;
            } else hours--;
        } else minutes--;
    } else seconds--;
}

// given a date in the future (as unix time), returns time until then (according
// to Time.now()
// saves values into dest
void timeUntil(int future, timespan_t& dest) {
    int seconds = future - Time.now();
    dest.seconds = seconds % 60;
    dest.minutes = (seconds / 60) % 60;
    dest.hours = (seconds / 3600) % 24;
    dest.days = seconds / 86400;
}

// logs an error to serial and the screen
void displayError(char* err) {
    Serial.print("[error] ");
    Serial.println(err);
}

void setup() {
    Serial.begin(9600);
    Particle.subscribe("hook-response/upcomingLaunches", gotLaunchData, MY_DEVICES);
    tft.begin(HX8357D);
    tft.setRotation(1);
    // so we can open the monitor
    delay(2000);
    requestData();
}

void loop() {
    unsigned long startTime = micros();
    // wait 60 seconds between api hits
    if (loopCounter >= 60) {
        requestData();
        loopCounter = 0;
    }

    --untilNextLaunch;
    displayCountdown();

    loopCounter++;
    // 1 second between loops, accounting for time it took to run loop
    delayMicroseconds(1000000 - (micros() - startTime));
}

void requestData() {
    Serial.println("requesting data");
    Particle.publish("upcomingLaunches");
}

void gotLaunchData(const char* name, const char* data) {
    Serial.println("received launch data");
    nextLaunchTimestamp = atoi(data);
    timeUntil(nextLaunchTimestamp, untilNextLaunch);
}

void displayCountdown() {
    sprintf(countdownText, "%02d:%02d:%02d:%02d", untilNextLaunch.days, untilNextLaunch.hours, untilNextLaunch.minutes, untilNextLaunch.seconds);
    Serial.println(countdownText);
    tft.fillScreen(HX8357_BLACK);
    tft.setTextColor(HX8357_WHITE);
    tft.setTextSize(6);
    tft.setCursor(0, 0);
    tft.println(countdownText);
}

// {
//     JsonArray& root = jsonBuffer.parseArray(data);
//     if (!root.success()) {
//         displayError("JSON parse failed");
//         return;
//     }
//     JsonObject& nextLaunch = root[0];
//     nextLaunchTimestamp = nextLaunch["launch_date_unix"];
//     timespan_t ts;
//     timeUntil(nextLaunchTimestamp, ts);
//     Serial.print(ts.days);
//     Serial.print(" ");
//     Serial.print(ts.hours);
//     Serial.print(" ");
//     Serial.print(ts.minutes);
//     Serial.print(" ");
//     Serial.println(ts.seconds);
// }
