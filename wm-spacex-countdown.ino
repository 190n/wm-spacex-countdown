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
bool timestampUnknown;
// webhook response will be put here as we get data
// stringstream partialResponse;
bool receivingData = false;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

char countdownText[14] = "T-00:00:00:00", oldCountdownText[14] = "xxxxxxxxxxxxx", *payloads;

bool payloadsChanged = false;

uint8_t loopCounter = 0;

timespan_t untilNextLaunch;

bool hasWifi = true, refreshing = false, refreshingChanged = false;

uint16_t progressCounter = 0;

unsigned long lastMillis = 0;

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
#define SCREEN_HEIGHT 320
// cursor location to draw countdown
// X is calculated
#define CURSOR_X 9
#define CURSOR_Y 20

#define PAYLOADS_Y 68
#define PAYLOADS_SIZE 2

// #999999
#define ICON_COLOR 0x9cd3
#define ICON_SIZE 24
// calculated
#define ICON_Y 296

// #ffc107
// material: amber 500
#define PROGRESS_COLOR 0xfe00
#define PROGRESS_HEIGHT 4

// from material icons
// posterized
// 24x24
const unsigned char noWifiIcon[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x18, 0xff, 0x00, 0x0c,
    0x7f, 0xe0, 0x1e, 0x3f, 0xf8, 0x7f, 0x1f, 0xfe, 0x7f, 0x8f, 0xfe, 0x3f, 0xc7,
    0xfc, 0x3f, 0xe3, 0xfc, 0x1f, 0xf1, 0xf8, 0x0f, 0xf8, 0xf0, 0x07, 0xfc, 0x60,
    0x03, 0xfe, 0x00, 0x03, 0xff, 0x00, 0x01, 0xff, 0x80, 0x00, 0xff, 0xc0, 0x00,
    0x7e, 0x60, 0x00, 0x3c, 0x30, 0x00, 0x3c, 0x10, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0
};

const unsigned char refreshingIcon [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x0e, 0x00, 0x00,
    0x7f, 0x00, 0x01, 0xff, 0x00, 0x03, 0xce, 0x00, 0x07, 0x0c, 0x00, 0x06, 0x08,
    0x20, 0x0e, 0x00, 0x70, 0x0c, 0x00, 0x30, 0x0c, 0x00, 0x30, 0x0c, 0x00, 0x30,
    0x0c, 0x00, 0x30, 0x0e, 0x00, 0x70, 0x04, 0x10, 0x60, 0x00, 0x30, 0xe0, 0x00,
    0x73, 0xc0, 0x00, 0xff, 0x80, 0x00, 0xfe, 0x00, 0x00, 0x70, 0x00, 0x00, 0x30,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0
};

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
    // delay(2000);
    requestData();
    startProgress();
}

void loop() {
    unsigned long nowMillis = millis();
    // wait 60 seconds between api hits
    if (nowMillis % 60000 < lastMillis % 60000) {
        requestData();
    }

    if (nowMillis % 1000 < lastMillis % 1000) {
        --untilNextLaunch;
        displayCountdown();
    }
    if (refreshingChanged) {
        refreshingChanged = false;
        if (refreshing) startProgress();
        else stopProgress();
    }
    if (refreshing && nowMillis % 25 < lastMillis % 25) iterProgress();
    lastMillis = nowMillis;
}

void requestData() {
    refreshing = true;
    refreshingChanged = true;
    Serial.println("requesting data");
    Particle.publish("upcomingLaunches");
}

void gotLaunchData(const char* name, const char* data) {
    refreshing = false;
    refreshingChanged = true;
    Serial.println("received launch data");
    Serial.println(data);
    if (strncmp(data, " &", 2) == 0) timestampUnknown = true;
    else {
        timestampUnknown = false;
        nextLaunchTimestamp = atoi(data);
    }
    // data will look like:
    // 1518468199 & Paz & Microsat-2a, -2b
    // \________/   \____________________/
    //  timestamp         payloads
    // so we look for the first ampersand, then go forward 2 characters to get the payloads
    const char* payloadStart = data;
    while (*payloadStart != '&') payloadStart++;
    payloadStart += 2;
    if (strcmp(payloadStart, payloads) != 0) payloadsChanged = true;
    // figure out how long the payload info is
    size_t payloadLen = strlen(data) - (payloadStart - data);
    free(payloads);
    payloads = (char*) malloc(payloadLen * sizeof(char));
    strcpy(payloads, payloadStart);
    timeUntil(nextLaunchTimestamp, untilNextLaunch);
}

void displayCountdown() {
    strcpy(oldCountdownText, countdownText);
    if (timestampUnknown) strcpy(countdownText, "Unknown       ");
    else sprintf(countdownText, "T-%02d:%02d:%02d:%02d", untilNextLaunch.days, untilNextLaunch.hours, untilNextLaunch.minutes, untilNextLaunch.seconds);
    Serial.println(countdownText);
    fastDrawCountdown();
}


// given two countdowns (13 chars each) returns bit mask of which chars differ
uint16_t diffCountdowns(char* c1, char* c2) {
    uint16_t result = 0;
    for (int i = 0; i < 14; i++) {
        if (c1[i] != c2[i]) {
            result += pow(2, i);
        }
    }
    return result;
}

void fastDrawCountdown() {
    uint16_t diff = diffCountdowns(oldCountdownText, countdownText);
    rect_t drawBox;
    for (int i = 0; i < 14; i++) {
        if (diff & (int)pow(2, i)) {
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
    // if (hasWifi != WiFi.ready()) {
    //     tft.drawBitmap(SCREEN_WIDTH - ICON_SIZE, ICON_Y, noWifiIcon, ICON_SIZE, ICON_SIZE, hasWifi ? HX8357_BLACK : ICON_COLOR);
    // }
    // if (refreshingChanged) {
    //     tft.drawBitmap(SCREEN_WIDTH - ICON_SIZE, ICON_Y, refreshingIcon, ICON_SIZE, ICON_SIZE, refreshing ? ICON_COLOR : HX8357_BLACK);
    // }
    if (!payloadsChanged) return;
    // not as optimized as it could be but I'm lazy
    tft.fillRect(CURSOR_X, PAYLOADS_Y, SCREEN_WIDTH - CURSOR_X, CHAR_HEIGHT * PAYLOADS_SIZE, HX8357_BLACK);
    tft.setTextColor(HX8357_WHITE);
    tft.setTextSize(PAYLOADS_SIZE);
    tft.setCursor(CURSOR_X, PAYLOADS_Y);
    tft.print(payloads);
    payloadsChanged = false;
}

void startProgress() {
    progressCounter = 0;
    tft.fillRect(0, SCREEN_HEIGHT - PROGRESS_HEIGHT, SCREEN_WIDTH, PROGRESS_HEIGHT, HX8357_BLACK);
    for (int i = 0; i < PROGRESS_HEIGHT; i++) {
        int y = (SCREEN_HEIGHT - PROGRESS_HEIGHT) + i;
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            if ((x + i) % 120 < 90) tft.drawPixel(x, y, PROGRESS_COLOR);
        }
    }
}

void iterProgress() {
    progressCounter += 2;
    if (progressCounter >= 120) progressCounter = 0;
    // add pixels to the right
    for (int row = 0; row < PROGRESS_HEIGHT; row++) {
        int y = (SCREEN_HEIGHT - PROGRESS_HEIGHT) + row;
        for (int x = (progressCounter - 31 - row); x < SCREEN_WIDTH; x += 120) {
            tft.drawPixel(x - 1, y, PROGRESS_COLOR);
            tft.drawPixel(x, y, PROGRESS_COLOR);
        }
    }
    // remove pixels from the left
    for (int row = 0; row < PROGRESS_HEIGHT; row++) {
        int y = (SCREEN_HEIGHT - PROGRESS_HEIGHT) + row;
        for (int x = (progressCounter - row - 1); x < SCREEN_WIDTH; x += 120) {
            tft.drawPixel(x - 1, y, HX8357_BLACK);
            tft.drawPixel(x, y, HX8357_BLACK);
        }
    }
}

void stopProgress() {
    tft.fillRect(0, SCREEN_HEIGHT - PROGRESS_HEIGHT, SCREEN_WIDTH, PROGRESS_HEIGHT, HX8357_BLACK);
}
