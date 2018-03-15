#include "WundergroundConditions.h"
#include "WundergroundClient.h"
#include <JsonListener.h>
#include <NTPClient.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Nextion.h>

#define nextion Serial1

// WIFI
const char* WIFI_SSID     = "Demo";
const char* WIFI_PASSWORD = "miksonvaja";

// Setup
const int UPDATE_INTERVAL_inSECS = 10 * 60; // Update every 10 minutes
const int CLOCK_UPDATE_INTERVAL = 60;
const float UTC_OFFSET = 2;                 // TimeClient settings
const String WG_API_KEY = "6fc2508ebadf7b1c";
const String WG_LANGUAGE = "EN";
const String WG_COUNTRY = "EE";
const String WG_CITY = "Tallinn";
const String WG_ZMW_CODE = "00000.55.26038";

WundergroundConditions wgc(true);                // True = use metric
WundergroundClient wg(true);

bool readyForWUpdate = false;
bool readyForCUpdate = true;
String dots;
String lastUpdate = "-";
String months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
String dayIcons[] = {"chanceflurries", "chancerain", "chancesleet", "chancesnow", "chancetstorms", "clear", "cloudy", "flurries", "fog", "hazy", "mostlycloudy", "mostlysunny", "nt_chanceflurries", "nt_chancerain", "nt_chancesleet", "nt_chancesnow", "nt_chancetstorms", "nt_clear", "nt_cloudy", "nt_flurries", "nt_fog", "nt_hazy", "nt_mostlycloudy", "nt_mostlysunny", "nt_partlycloudy", "nt_partlysunny", "nt_rain", "nt_sleet", "nt_snow", "nt_sunny", "nt_tstorms", "nt_unknown", "partlycloudy", "partlysunny", "rain", "sleet", "snow", "sunny", "tstorms", "unknown"};
String icons[] = {"clear", "cloudy", "flurries", "fog", "hazy", "mostlycloudy", "moustlysunny", "rain", "sleet", "snow", "tstorms", "nt_clear", "nt_mostlycloudy", "nt_mostlysunny"};
String icons_alt[] = {"sunny", "nt_cloudy", "nt_flurries", "nt_fog", "nt_hazy", "partlysunny", "partlycloudy", "nt_rain", "nt_sleet", "nt_snow", "nt_sunny", "nt_partlysunny", "nt_partlycloudy"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600 * UTC_OFFSET, 60000);


Ticker ticker;
Nextion nex(nextion, 9600); //create a Nextion object named myNextion using the nextion serial port @ 9600bps

/*===============================================================================
    |   Connecting to wifi. Restart when connection failed. On connection established
    |   enter the main screen.
    =================================================================================*/
void connectWifi() {
  //WiFi.hostname(ESP_HOST_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  delay(25);
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  nex.setComponentText("textLoading", "Connecting to WiFi");
  int tim = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    dots = dots + ".";
    nex.setComponentText("textLoadingBar", dots);
    if (tim == 20) {                                                    //After 10 sec restart
      nex.setComponentText("textLoading", "Connection failed, restating");
      nex.setComponentText("textLoadingBar", "If this persists, try restarting manually");
      delay(1000);
      ESP.restart();
    }
    tim++;
  }
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.println(WiFi.localIP());
  Serial.println();
  nex.setComponentText("textLoading", "WiFi connected!");
  nex.setComponentText("textLoadingBar", "");
  delay(1000);
  nex.sendCommand("page page0");
}

/*===============================================================================
    |   Update weather conditions and forecast.
    =================================================================================*/
void updateData() {
  Serial.println("\n\nUpdating conditions...");
  WGConditions conditions;
  wgc.updateConditions(&conditions, WG_API_KEY, WG_LANGUAGE, WG_ZMW_CODE);
  for (int i = 0; i < sizeof(icons) / sizeof(icons[0]); i++) {            //Loop through Icons array
      if (conditions.weatherIcon == icons[i] || icons_alt[i]) {
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
        break();
      }
    }
   
 //Refresh nextion elements, otherwise background image (p0) will apear on top.    
  nex.sendCommand("ref p0");
  nex.sendCommand("ref NexTime");
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

  nex.setComponentText("NexTemp", conditions.currentTemp);
  Serial.println("CurrentTemp: " + conditions.currentTemp + "°C");
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
  readyForWUpdate = false;
  delay(1000);
}

/*===============================================================================
    |   Update time, date, month
    =================================================================================*/
void updateClock() {
  timeClient.update();

  //String time = timeClient.getFormattedTime().substring(0,5);
  nex.setComponentText("NexTime", timeClient.getFormattedTime().substring(0, 5));         //Cut out seconds

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

void setGoodForWeatherUpdate() {
  readyForWUpdate = true;
}

void setGoodForClockUpdate() {
  readyForCUpdate = true;
}

void setup() {
  Serial.begin(115200);
  nex.init();
  timeClient.begin();
  connectWifi();

  updateData();
  ticker.attach(UPDATE_INTERVAL_inSECS, setGoodForWeatherUpdate);       //Set a timer for updating weather info
  ticker.attach(1, setGoodForClockUpdate);                              //Set a timer for updating clock  
}

void loop() {
  if (readyForWUpdate) {
    updateData();
  }
  if (readyForCUpdate) {
    updateClock();
  }
  
  
}
