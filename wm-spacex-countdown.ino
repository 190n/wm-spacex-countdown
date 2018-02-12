// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_mfGFX.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_HX8357.h>

#include <math.h>
#include <string.h>
#include <malloc.h>

struct timespan_t {
    uint8_t seconds, minutes, hours;
    uint16_t days;
    timespan_t();
    void operator--();
};

struct rect_t {
    uint16_t x, y, w, h;
};

// UNIX timestamp of next launch
int nextLaunchTimestamp;
// webhook response will be put here as we get data
// stringstream partialResponse;
bool receivingData = false;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

char countdownText[12] = "00:00:00:00", oldCountdownText[12] = "__ __ __ __", *payloads;

bool payloadsChanged = false;

uint8_t loopCounter = 0;

timespan_t untilNextLaunch;

Adafruit_HX8357 tft = Adafruit_HX8357(D6, D7, D5, D3, -1, D4);

#define TEXT_SIZE 6
// dimensions of single character of text
// measured in "blocks" that are TEXT_SIZE by TEXT_SIZE pixels
#define CHAR_WIDTH 5
#define CHAR_HEIGHT 7
// horizontal gap between chars
#define CHAR_GAP 1
// screen dimensions in pixels
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 480
// cursor location to draw countdown
// X is calculated
#define CURSOR_X 45
#define CURSOR_Y 45

#define PAYLOADS_Y 93
#define PAYLOADS_SIZE 2

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

// get rectangle taken up by character in countdown, store in result
void getDrawBox(uint8_t c, rect_t& result) {
    result.x = CURSOR_X + c * TEXT_SIZE * (CHAR_WIDTH + CHAR_GAP);
    result.y = CURSOR_Y;
    result.w = TEXT_SIZE * CHAR_WIDTH;
    result.h = TEXT_SIZE * CHAR_HEIGHT;
}

void setup() {
    Serial.begin(9600);
    Particle.subscribe("hook-response/upcomingLaunches", gotLaunchData, MY_DEVICES);
    tft.begin(HX8357D);
    tft.setRotation(1);
    tft.fillScreen(HX8357_BLACK);
    tft.setTextSize(TEXT_SIZE);
    tft.setTextColor(HX8357_WHITE);
    tft.setCursor(CURSOR_X, CURSOR_Y);
    tft.println(countdownText);
    // so we can open the monitor
    delay(2000);
    rect_t r;
    getDrawBox(1, r);
    char t[100];
    sprintf(t, "%d %d %d %d", r.x, r.y, r.w, r.h);
    Serial.println(t);
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
    Serial.println(data);
    nextLaunchTimestamp = atoi(data);
    // data will look like:
    // 1518468199 & Paz & Microsat-2a, -2b
    // \________/   \____________________/
    //  timestamp         payloads
    // so we look for the first ampersand, then go forward 2 characters to get the payloads
    char* payloadsStart = data;
    while (*payloadStart != '&') payloadStart++;
    payloadStart += 2;
    if (strcmp(payloadStart, payloads) != 0) payloadsChanged = true;
    // figure out how long the payload info is
    size_t payloadLen = strlen(data) - (payloadStart - data);
    payloads = (char*) malloc(payloadLen * sizeof(char));
    strcpy(payloads, payloadStart);
    timeUntil(nextLaunchTimestamp, untilNextLaunch);
}

void displayCountdown() {
    strcpy(oldCountdownText, countdownText);
    sprintf(countdownText, "%02d:%02d:%02d:%02d", untilNextLaunch.days, untilNextLaunch.hours, untilNextLaunch.minutes, untilNextLaunch.seconds);
    Serial.println(countdownText);
    fastDrawCountdown();
}


// given two countdowns (12 chars each) returns bit mask of which chars differ
uint16_t diffCountdowns(char* c1, char* c2) {
    uint16_t result = 0;
    for (int i = 0; i < 12; i++) {
        if (c1[i] != c2[i]) {
            result += pow(2, i);
        }
    }
    return result;
}

void fastDrawCountdown() {
    uint16_t diff = diffCountdowns(oldCountdownText, countdownText);
    rect_t drawBox;
    for (int i = 0; i < 12; i++) {
        if (diff & (int)pow(2, i)) {
            Serial.print("replacing char ");
            Serial.println(oldCountdownText[i]);
            getDrawBox(i, drawBox);
            tft.setTextColor(HX8357_BLACK);
            tft.setTextSize(TEXT_SIZE);
            tft.setCursor(drawBox.x, drawBox.y);
            tft.print(oldCountdownText[i]);
            tft.setTextColor(HX8357_WHITE);
            tft.setTextSize(TEXT_SIZE);
            tft.setCursor(drawBox.x, drawBox.y);
            tft.print(countdownText[i]);
        }
    }
    if (!payloadsChanged) return;
    // not as optimized as it could be but I'm lazy
    tft.fillRect(CURSOR_X, PAYLOADS_Y, SCREEN_WIDTH - CURSOR_X, CHAR_HEIGHT * PAYLOADS_SIZE, HX8357_BLACK);
    tft.setTextColor(HX8357_WHITE);
    tft.setTextSize(PAYLOADS_SIZE);
    tft.setCursor(CURSOR_X, PAYLOADS_Y);
    tft.print(payloads);
}
