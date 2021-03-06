#include "WundergroundConditions.h"
#include "WundergroundClient.h"
#include <JsonListener.h>
#include <Timezone.h>
#include <NTPClient.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Nextion.h>
#include <OpenWeatherMap.h>

#define nextion Serial1
time_t getNtpTime();
// WIFI
const char* WIFI_SSID     = "Parzival";
const char* WIFI_PASSWORD = "EF13FCBC9D";

static const char ntpServerName[] = "europe.pool.ntp.org";

// Setup
int UPDATE_INTERVAL_inSECS = 10 * 60; // Update every 10 minutes
float UTC_OFFSET = 2;                 // TimeClient settings
const String WG_API_KEY = "6fc2508ebadf7b1c";
const String WG_LANGUAGE = "EN";
const String WG_COUNTRY = "EE";
const String WG_CITY = "Viimsi";
const String WG_ZMW_CODE = "00000.55.26038";

WundergroundConditions wgc(true);                // True = use metric
WundergroundClient wg(true);

bool readyForWUpdate = false;
bool readyForCUpdate = true;
bool readyforConnect = false;
bool readyForWiFiCheck = false;
String lastUpdate = "-";
String months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
String dayIcons[] = {"chanceflurries", "chancerain", "chancesleet", "chancesnow", "chancetstorms", "clear", "cloudy", "flurries", "fog", "hazy", "mostlycloudy", "mostlysunny", "nt_chanceflurries", "nt_chancerain", "nt_chancesleet", "nt_chancesnow", "nt_chancetstorms", "nt_clear", "nt_cloudy", "nt_flurries", "nt_fog", "nt_hazy", "nt_mostlycloudy", "nt_mostlysunny", "nt_partlycloudy", "nt_partlysunny", "nt_rain", "nt_sleet", "nt_snow", "nt_sunny", "nt_tstorms", "nt_unknown", "partlycloudy", "partlysunny", "rain", "sleet", "snow", "sunny", "tstorms", "unknown"};
String icons[] = {"clear", "cloudy", "flurries", "fog", "hazy", "mostlycloudy", "moustlysunny", "rain", "sleet", "snow", "tstorms", "nt_clear", "nt_mostlycloudy", "nt_mostlysunny"};
String icons_alt[] = {"sunny", "nt_cloudy", "nt_flurries", "nt_fog", "nt_hazy", "partlysunny", "partlycloudy", "nt_rain", "nt_sleet", "nt_snow", "nt_sunny", "nt_partlysunny", "nt_partlycloudy"};

WiFiUDP ntpUDP;
uint16_t localPort;  // local port to listen for UDP packets


NTPClient timeClient(UTC_OFFSET);
//EET Tallinn/////////////////////////////////////////////////NOT WORKING
TimeChangeRule myDST = {"EEST", Fourth, Sun, Mar, 25, +180};    //Daylight time = UTC + 3 hours
TimeChangeRule mySTD = {"EET", Fourth, Sun, Oct, 29, +120};     //Standard time = UTC + 2 hours
Timezone myTZ(myDST, mySTD);

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev
time_t utc, local;


Ticker ticker;
Ticker ticker1;
Ticker ticker2;
Nextion nex(nextion, 9600); //create a Nextion object named myNextion using the nextion serial port @ 9600bps

/*===============================================================================
    |   Connecting to wifi. Restart when connection failed. On connection established
    |   enter the main screen.
    =================================================================================*/
void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  nex.setComponentText("textLoading", "Connecting to WiFi");
  String dots;
  long conStart = millis();
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    dots = dots + ".";
    nex.setComponentText("textLoadingBar", dots);
    if (millis() - conStart >= 10000) {                             //After 10 sec
      nex.setComponentText("textLoading", "Connection failed, restating");
      nex.setComponentText("textLoadingBar", "If this persists, try restarting manually");
      delay(1000);
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected!");
    Serial.println(WiFi.localIP());
    Serial.println();
    nex.setComponentText("textLoading", "WiFi connected!");
    nex.setComponentText("textLoadingBar", "");
    delay(1000);
    nex.sendCommand("page page0");

    setTime(myTZ.toUTC(compileTime()));
    localPort = random(1024, 65535);
    ntpUDP.begin(localPort);
    setSyncProvider(getNtpTime);
    setSyncInterval(5 * 60);
  }
}

/*===============================================================================
    |   Update weather conditions and forecast.
    =================================================================================*/
void updateData() {
  Serial.println("\n\nUpdating conditions...");
  WGConditions conditions;
  wgc.updateConditions(&conditions, WG_API_KEY, WG_LANGUAGE, WG_ZMW_CODE);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nNo connection");
    nex.sendCommand("p0.pic=14");                                           //Set the icon to 'unknown'
    nex.sendCommand("NexTemp.picc=14");
    nex.sendCommand("NexTimeSec.picc=14");
    nex.setComponentText("NexTemp", "");
    nex.setComponentText("NexHumid", "?");
    nex.setComponentText("NexFLike", "?");
    nex.setComponentText("NexWSpeed", "?");
    for (int i = 0; i < 3; i++) {
      String dayIconErased = "p" + String(i + 1) + ".pic=55";
      char *pointerString = new char[dayIconErased.length() + 1];
      strcpy(pointerString, dayIconErased.c_str());
      nex.sendCommand(pointerString);
      delete [] pointerString;
      nex.setComponentText("tagDay" + String(i + 1), "");
      nex.setComponentText("tagNight" + String(i + 1), "");
      nex.setComponentText("NexTemp" + String(i + 1), "");
    }
    UPDATE_INTERVAL_inSECS = 30;
    readyforConnect = true;
  }
  else
  {
    WGConditions conditions;
    wgc.updateConditions(&conditions, WG_API_KEY, WG_LANGUAGE, WG_ZMW_CODE);
    for (int i = 0; i < sizeof(icons) / sizeof(icons[0]); i++) {            //Loop through Icons array
      if (conditions.weatherIcon == icons[i] || conditions.weatherIcon == icons_alt[i]) {
        Serial.println("Icon index: " + String(i) + " | Icon: " + conditions.weatherIcon);
        String IconFinal = "p0.pic=" + String(i);  //forecast icons start from id 15 (on nextion)
        char *pointerString = new char[  IconFinal.length() + 1];                //Covert string to *char
        strcpy(pointerString, IconFinal.c_str());                              //
        nex.sendCommand(pointerString);                                           //Change the icon on screen
        delete [] pointerString;                                                  //Delete the *char to free up mem

        IconFinal = "NexTemp.picc=" + String(i);
        char *pointerStr = new char[  IconFinal.length() + 1];
        strcpy(pointerStr, IconFinal.c_str());
        nex.sendCommand(pointerStr);
        delete [] pointerStr;

        IconFinal = "NexTimeSec.picc=" + String(i);
        char *pointerSt = new char[  IconFinal.length() + 1];
        strcpy(pointerSt, IconFinal.c_str());
        nex.sendCommand(pointerSt);
        delete [] pointerSt;
        break;
      }
    }
  }

  //Refresh nextion elements, otherwise background image (p0) will apear on top.
  nex.sendCommand("ref p0");
  nex.sendCommand("ref NexTime");
  nex.sendCommand("ref NexTimeSec");
  nex.sendCommand("ref NexMonth");
  nex.sendCommand("ref NexDate");
  nex.sendCommand("ref NexTemp");
  nex.sendCommand("ref tagTemp");
  nex.sendCommand("ref NexHumid");
  nex.sendCommand("ref tagWSpeed");
  nex.sendCommand("ref NexWSpeed");
  nex.sendCommand("ref tagFlike");
  nex.sendCommand("ref NexFlike");
  nex.sendCommand("ref tagFlikeC");
  nex.sendCommand("ref p1");
  nex.sendCommand("ref tagDay1");
  nex.sendCommand("ref tagNight1");
  nex.sendCommand("ref NexTemp1");
  nex.sendCommand("ref p2");
  nex.sendCommand("ref tagDay2");
  nex.sendCommand("ref tagNight2");
  nex.sendCommand("ref NexTemp2");
  nex.sendCommand("ref p3");
  nex.sendCommand("ref tagDay3");
  nex.sendCommand("ref tagNight3");
  nex.sendCommand("ref NexTemp3");
  
  OWM_conditions *ow_cond = new OWM_conditions;
  owCC.updateConditions(ow_cond, ow_key, "ee", "Viimsi", "metric");
  Serial.print("Current: ");
  Serial.println("icon: " + ow_cond->icon + ", " + " temp.: " + ow_cond->temp + ", humid.: " + ow_cond->humidity) + ", wind.: " + ow_cond->speed);

  if (WiFi.status() == WL_CONNECTED) {
    nex.setComponentText("NexTemp", ow_cond->temp);
    nex.setComponentText("NexHumid", conditions.humidity);
    Serial.println("CurrentHumid: " + conditions.humidity);
    nex.setComponentText("NexFLike", conditions.feelslike);
    Serial.println("CurrentFeelsLike: " + conditions.feelslike);

    String windSpeed = conditions.windSpeed;
    windSpeed.remove(windSpeed.length() - 4);                                       //Remove 'km/h'
    float windSpeedFloat = windSpeed.toFloat() * 0.277778;                          //Convert to m/s
    float windSpeedFloatRounded = round(windSpeedFloat * 10) / 10.0;                //Round to the nearest tenth
    windSpeed = String(windSpeedFloatRounded, 1) + "m/s";                           //Leave 1 nr after decimal
    nex.setComponentText("NexWSpeed", windSpeed);
    Serial.println("CurrentWindSpeed: " + windSpeed);
    

    Serial.println("\nUpdating forecasts...");
    String dayIcon;
    String dayTitle;
    String dayTemp;
    wg.updateForecast(WG_API_KEY, WG_LANGUAGE, WG_COUNTRY, WG_CITY);                //Send HTTP request
    for (int dayIndex = 0; dayIndex < 3; dayIndex++) {                              //Loop through forecast replys[day]
      Serial.println("------------------------------------");
      dayIcon = wg.getForecastIcon(dayIndex);
      for (int i = 0; i < sizeof(dayIcons) / sizeof(dayIcons[0]); i++) {            //Loop through dayIcons array
        if (dayIcon == dayIcons[i]) {
          String dayIconFinal = "p" + String(dayIndex + 1) + ".pic=" + String(i + 16);  //forecast icons start from id 16 (on nextion)
          char *pointerString = new char[dayIconFinal.length() + 1];                //Covert string to *char
          strcpy(pointerString, dayIconFinal.c_str());                              //
          nex.sendCommand(pointerString);                                           //Change the icon on screen
          delete [] pointerString;                                                  //Delete the *char to free up mem
        }
      }
      Serial.println("getForecastIcon: " + dayIcon);
      dayTitle = wg.getForecastTitle(dayIndex);
      nex.setComponentText("tagDay" + String(dayIndex + 1), dayTitle.substring(0, 3));  //Set only the first 3 characters
      Serial.println("getForecastTitle: " + dayTitle);
      if (dayTitle.endsWith("Night")) {                                             //If we're dealing with a night reply
        nex.setComponentText("tagNight" + String(dayIndex + 1), "Night");
        dayTemp = wg.getForecastText(dayIndex);                                 //Then get the day description for temperature
        dayTemp = dayTemp.substring(dayTemp.indexOf("C.") - 3, dayTemp.indexOf("C."));      //Trim end: from 3 chars before C sign
        dayTemp = dayTemp.substring(dayTemp.indexOf(" ") + 1);                               //Trim start
      } else {                                                                      //If we're dealing with a day reply
        nex.setComponentText("tagNight" + String(dayIndex + 1), "");                //Clear night tag
        dayTemp = wg.getForecastLowTemp(dayIndex) + "|" + wg.getForecastHighTemp(dayIndex);
      }
      nex.setComponentText("NexTemp" + String(dayIndex + 1), dayTemp);
      Serial.println("ForecastTemp: " + dayTemp);
      //    Serial.println("getPoP: " + forecasts[i].PoP);

    }
    delete ow_cond;
  }
  readyForWUpdate = false;
  delay(1000);
}

//Function to return the compile date and time as a time_t value
time_t compileTime(void)  {
  tmElements_t tm;
  time_t t;

  breakTime(now(), tm);
  t = makeTime(tm);
  return t;
}

/*===============================================================================
    |   Update time, date, month
    =================================================================================*/
void updateClock() {
  timeClient.update();

  //String time = timeClient.getFormattedTime().substring(0,5);
  nex.setComponentText("NexTime", timeClient.getFormattedTime().substring(0, 5));         //Cut out seconds
  String secs;
  if (timeClient.getSeconds() < 10) {
    secs = "0" + String(timeClient.getSeconds());
  } else {
    secs = timeClient.getSeconds();
  }
  nex.setComponentText("NexTimeSec", ":" + secs);

  for (int i = 0; i < 12; i++) {                      //Loop through months
    if (timeClient.getMonth() == i + 1) {             //Add 1 becasue Month starts from 1
      nex.setComponentText("NexMonth", months[i]);
      break;
    }
  }

  int dateInt = timeClient.getDate();
  String date = String(dateInt);
  if (dateInt < 10) {
    date = 0 + date;
  }
  nex.setComponentText("NexDate", date);
  readyForCUpdate = false;
}

void connectBg() {
  Serial.println("\nConnecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  delay(25);
  int tim = 0;
  for (int i = 0; i < 20; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n\nConnected");
      updateData();
    }
    delay(500);
  }
  UPDATE_INTERVAL_inSECS = 10 * 60;
  readyforConnect = false;
}

void setGoodForWeatherUpdate() {
  readyForWUpdate = true;
  Serial.println("\n\nreadyForWUpdate = tru");
}

void setGoodForClockUpdate() {
  readyForCUpdate = true;
}

void setGoodForWiFiCheck() {
  readyForWiFiCheck = true;
  Serial.println("\n\nreadyForWiFiCheck = tru");
}

void setup() {
  Serial.begin(115200);
  nex.init();
  timeClient.begin();
  connectWifi();

  updateData();
  ticker.attach(UPDATE_INTERVAL_inSECS, setGoodForWeatherUpdate);        //Set a timer for updating weather info
  ticker1.attach(1, setGoodForClockUpdate);                              //Set a timer for updating clock
  ticker1.attach(60, setGoodForWiFiCheck);                                //Set a timer for checking the WiFi connection
}

void loop() {
  if (readyForWUpdate) {
    updateData();
  }
  if (readyForCUpdate) {
    updateClock();
  }
  if (readyForWiFiCheck) {
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }
  }


}
