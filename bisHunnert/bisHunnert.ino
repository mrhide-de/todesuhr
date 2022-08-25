/*
KUBIS TODESUHR

COUNT DOWN CLOCK TILL THE END

*/

#define DEBUG 0
/*  DEBUG 0 - uses e-paper display as output
    DEBUG 1 - uses serial monitor as output */

#define RESET 0
/* CAREFULL, deletes wifi settings */

/* default event date */
#define DEF_YEAR "2082"
#define DEF_MONTH "7"
#define DEF_DAY "8"
#define DEF_HOUR "0"
#define DEF_MIN "0"

/* # # # # # # # # */

#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include "meine_zeichen.h"
#include "GxEPD2_display_selection_new_style.h"

#include <NTPClient.h>
#include <ESP8266WiFi.h>
// #include <WiFiUdp.h>
#include <WiFiManager.h>


// const long utcOffsetInSeconds = 3600; // DE, FR, ES, PL, IT, NO, SE, DK, ...
const long utcOffsetInSeconds = 7200; // light saving


// TIME
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// TIME CALCULATIONS
#include <TimeLib.h>
int dieZeit[8] = {0, 0, 0, 6, 9, 5, 2, 5};
int dieZeitTemp[8] = {8, 7, 9, 5, 9, 5, 2, 5};
// int dieZeitTempUp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
long hunnertJahre = 52596000;
long hunnertJahreTemp = 52596000;
// für die Lücke zwischen den 3ern
int mutZL[8] = {0,0,0,8,8,8,16,16};
time_t event_minutes;
time_t this_minute;
// Timer
unsigned long startMS;
unsigned long currentMS;
int period = 1000;
// FRAME
const byte p_x = 11;
const byte p_y = 10;

// parameter and defaults
WiFiManagerParameter wmp_user_year("event_year", "year in the future or past", DEF_YEAR, 4);
WiFiManagerParameter wmp_user_mon("event_mon", "month", DEF_MONTH, 2);
WiFiManagerParameter wmp_user_day("event_day", "day", DEF_DAY, 2);
WiFiManagerParameter wmp_user_hour("event_hour", "hour", DEF_HOUR, 2);
WiFiManagerParameter wmp_user_min("event_min", "minute", DEF_MIN, 2);
int event_year = 0;
int event_mon = 0;
int event_day = 0;
int event_hour = 0;
int event_min = 0;
String event_year_char;
String event_mon_char;
String event_day_char;
String event_hour_char;
String event_min_char;

byte today = false;

#if DEBUG == 1
#define sp(x) Serial.print(x)
#define spl(x) Serial.println(x)
#else
#define sp(x)
#define spl(x)
#endif


void setup() {
  #if DEBUG == 0
  display.init(115200);
  #else
  Serial.begin(9600);
  spl(); spl(); spl("Los geht's!"); spl();
  #endif

  // set timer
  startMS = millis();

  timeClient.begin();

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  WiFiManager wm;
  wm.setDebugOutput(DEBUG);
  #if RESET == 1
  wm.resetSettings();
  #endif
  wm.setConfigPortalTimeout(60);
  std::vector<const char *> menu = {"param","wifi","exit"};
  wm.setMenu(menu);
  wm.addParameter(&wmp_user_year);
  wm.addParameter(&wmp_user_mon);
  wm.addParameter(&wmp_user_day);
  wm.addParameter(&wmp_user_hour);
  wm.addParameter(&wmp_user_min);

  spl("CONNECTING");

  if (!wm.autoConnect("kubistodesuhr", "wartenurbalde")) {
    spl("no configuration");
    error_message(1);
    error_message(2);
    delay(2000);
    wm.setConfigPortalTimeout(180);
    spl("CONNECTING 2nd TIME");
    if (wm.autoConnect("kubistodesuhr", "wartenurbalde")) { // second try
      // ESP.restart();
      // delay(3000);
    }
  } else {
    spl("connected to wifi");
  }

  if (WiFi.status() == WL_CONNECTED) {
    event_year_char = wmp_user_year.getValue();
    event_mon_char = wmp_user_mon.getValue();
    event_day_char = wmp_user_day.getValue();
    event_hour_char = wmp_user_hour.getValue();
    event_min_char = wmp_user_min.getValue();

    event_year = event_year_char.toInt();
    event_mon = event_mon_char.toInt();
    event_day = event_day_char.toInt();
    event_hour = event_hour_char.toInt();
    event_min = event_min_char.toInt();

    tmElements_t event_date;
    event_date.Second = 0;
    event_date.Hour = event_hour;
    event_date.Minute = event_min;
    event_date.Day = event_day;
    event_date.Month = event_mon;      // months start from 0, so deduct 1 ???
    event_date.Year = event_year - 1970; // years since 1970, so deduct 1970
    event_minutes = makeTime(event_date);

    timeClient.update();
    getTime();
    check_direction();
    fullDisplayRefresh();
    startup_animation();
    countDown();
  } else {
    getTimeToHundred();
    fullDisplayRefresh();
    startup_animation();
  }

}


void loop() {
  currentMS = millis();
  if (currentMS - startMS >= period) {
    if (WiFi.status() == WL_CONNECTED) {
      if (period == 1000) { // first time, sync to millis
        while (timeClient.getSeconds() != 59) {
          spl(timeClient.getSeconds());
          delay(1000);
        }
        spl("set interval to 1 min");
        period = 60000;
      }
      if (timeClient.getMinutes() == 0) {
        spl("cleanup and start again");
        fullDisplayRefresh();
        timeClient.update();
      }
      getTime();
    } else { // offline mode
      if (period == 1000) {
        spl("set interval to 1 min");
        period = 60000;
      }
      getTimeToHundred();
    }
    write_time_to_display();
    startMS = millis();
  }
}

void check_direction() {
  if (event_minutes < this_minute) {
    spl("counting up");
    dieZeitTemp[0] = 0;
    dieZeitTemp[1] = 0;
    dieZeitTemp[2] = 0;
    dieZeitTemp[3] = 0;
    dieZeitTemp[4] = 0;
    dieZeitTemp[5] = 0;
    dieZeitTemp[6] = 0;
    dieZeitTemp[7] = 0;
  } else {
    spl("counting down");
  }
}

void getTime() {
  this_minute = timeClient.getEpochTime();
  long sum_minutes;

  if (event_minutes < this_minute) { // past
    sum_minutes = (this_minute-event_minutes)/60;
  } else { // future
    sum_minutes = (event_minutes-this_minute)/60;
  }

  struct tm *t_now = gmtime ((time_t *)&this_minute);
  int now_year = t_now->tm_year;
  int now_month = t_now->tm_mon;
  int now_day = t_now->tm_mday;
  struct tm *t_then = gmtime ((time_t *)&event_minutes);
  int then_year = t_then->tm_year;
  int then_month = t_then->tm_mon;
  int then_day = t_then->tm_mday;
  int then_hour = t_then->tm_hour;
  int then_min = t_then->tm_min;
  if (now_day == then_day && now_month == then_month) {
    today = true;
    spl("TODAY");
  } else {
    today = false;
  }

  sp("EPOCHE TIME: "); spl(this_minute);
  sp("NOW: "); sp(timeClient.getFormattedTime()); sp(" ("); sp(now_day); sp("."); sp(now_month+1); sp("."); sp(now_year+1900); spl(")");
  sp("THEN: "); sp(then_hour); sp(":"); sp(then_min); sp(" ("); sp(then_day); sp("."); sp(then_month+1); sp("."); sp(then_year+1900); spl(")");
  sp("MINUTES TILL THEN: "); spl(sum_minutes);

  long sum_minutes_temp;
  int i = 0;
  // cut to digits
  while ( sum_minutes >= 1 ) {
    sum_minutes_temp = sum_minutes / 10;
    dieZeit[i] = sum_minutes-(10 * sum_minutes_temp);
    sum_minutes = sum_minutes_temp;
    i++;
  }
  while (i < 8) {
    dieZeit[i] = 0;
    i++;
  }
}

void getTimeToHundred() {
  hunnertJahre--;
  sp("MINUTES TO 100: "); spl(hunnertJahre);
  hunnertJahreTemp = hunnertJahre;
  int i = 0;
  long tempMin;
  // cut to digits
  while ( hunnertJahreTemp >= 1 ) {
    tempMin = hunnertJahreTemp / 10;
    dieZeit[i] = hunnertJahreTemp-(10 * tempMin);
    hunnertJahreTemp=tempMin;
    i++;
  }
}


void fullDisplayRefresh() {
  #if DEBUG == 0
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
  }
  while (display.nextPage());
  #endif
}


void write_time_to_display() {
  #if DEBUG == 0

  byte leading_zeros = 0;
  for (uint16_t digit = 0; digit < 8; digit++) { // all digits
    if (dieZeit[digit] == 0) {
      leading_zeros++;
    } else {
      leading_zeros = 0;
    }
  }
  if (leading_zeros > 7) {
    leading_zeros = 7;
  }

  display.setRotation(1);
  display.setPartialWindow(0, 0, 296, 128);

  if (today) {

    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      display.drawBitmap(257+p_x, 27+p_y, epd_bitmap_allArray[10], 35, 60, GxEPD_BLACK); // "min"
      for (uint16_t digit = 0; digit < 8-leading_zeros; digit++) { // all digits
        display.drawBitmap(233+p_x - 30 * digit, 28+p_y, epd_bitmap_allArray[dieZeit[digit]], 35, 60, GxEPD_BLACK);
      }
      display.drawBitmap(0+p_x, 28+p_y, epd_bitmap_allArray[11], 35, 60, GxEPD_BLACK);
    }
    while (display.nextPage());
    display.hibernate();

  } else {

    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      display.drawBitmap(257+p_x, 27+p_y, epd_bitmap_allArray[10], 35, 60, GxEPD_BLACK); // "min"
      for (uint16_t digit = 0; digit < 8-leading_zeros; digit++) { // all digits
        display.drawBitmap(233+p_x - 30 * digit - mutZL[digit], 28+p_y, epd_bitmap_allArray[dieZeit[digit]], 35, 60, GxEPD_BLACK);
      }
    }
    while (display.nextPage());
    display.hibernate();

  }

  #endif
}


void countDown() {
  #if DEBUG == 0

    byte leading_zeros = 0;
    for (uint16_t digit = 0; digit < 8; digit++) { // all digits
      if (dieZeit[digit] == 0) {
        leading_zeros++;
      } else {
        leading_zeros = 0;
      }
    }
    if (leading_zeros > 7) {
      leading_zeros = 7;
    }

    display.setRotation(1);
    display.setPartialWindow(0, 0, 296, 128);
    int count_d = 7; // digit

    while (count_d >= 0) {

      display.firstPage();
      do {
        display.fillScreen(GxEPD_WHITE);
        // "min"
        display.drawBitmap(257+p_x, 27+p_y, epd_bitmap_allArray[10], 35, 60, GxEPD_BLACK);
        // all digits
        for (uint16_t digit = 0; digit < 8-leading_zeros; digit++) {
          display.drawBitmap(233+p_x - 30 * digit - mutZL[digit], 28+p_y, epd_bitmap_allArray[dieZeitTemp[digit]], 35, 60, GxEPD_BLACK);
        }
      }
      while (display.nextPage());

      // counting
      if (count_d != 0) {
        if (dieZeit[count_d-1] < dieZeitTemp[count_d-1] ) {
          dieZeitTemp[count_d-1]--;
        } else if (dieZeit[count_d-1] > dieZeitTemp[count_d-1] ) {
          dieZeitTemp[count_d-1]++;
        }
      }
      if (count_d > 1) {
        if (dieZeit[count_d-2] < dieZeitTemp[count_d-2] - 1 ) {
          dieZeitTemp[count_d-2]--;
        } else if (dieZeit[count_d-2] > dieZeitTemp[count_d-2] + 1 ) {
          dieZeitTemp[count_d-2] += 2;
        }
      }
      if (count_d > 2 && dieZeitTemp[count_d-3] < 8 ) {
        dieZeitTemp[count_d-3] += 2;
      }
      if (count_d > 3 && dieZeitTemp[count_d-4] < 7 ) {
        dieZeitTemp[count_d-4] += 3;
      }
      // last digit
      if (dieZeitTemp[count_d] > dieZeit[count_d]) {
        dieZeitTemp[count_d]--;
      } else if (dieZeitTemp[count_d] < dieZeit[count_d]) {
        dieZeitTemp[count_d]++;
      } else if (dieZeitTemp[count_d] == dieZeit[count_d]) {
        count_d--;
      }
    }

  #endif
}


void startup_animation() {
  #if DEBUG == 0

  display.setRotation(1);
  display.setPartialWindow(0, 0, 296, 128);
  for (size_t lC = 1; lC <= 7; lC++) {
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      // hourglas 1-7
      display.drawBitmap(130+p_x, 42+p_y, logoAnim_all[lC], 17, 24, GxEPD_BLACK);
      // logo name
      display.drawBitmap(47+p_x, 49+p_y, logoAnim_all[8], 186, 42, GxEPD_BLACK);
    }
    while (display.nextPage());
    delay(200);
  }
  delay(5000);

  #endif
}

void error_message(int nr) {
  if (nr == 1) {
    display.setRotation(1);
    display.setPartialWindow(0, 0, 296, 128);
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      display.drawBitmap(42+p_x, 42+p_y, error_messages[0], 186, 42, GxEPD_BLACK);
    }
    while (display.nextPage());
    delay(400);
  }
  if (nr == 2) {
    display.setRotation(1);
    display.setPartialWindow(0, 0, 296, 128);
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      display.drawBitmap(42+p_x, 42+p_y, error_messages[1], 186, 42, GxEPD_BLACK);
    }
    while (display.nextPage());
  }
  delay(1000);
}
