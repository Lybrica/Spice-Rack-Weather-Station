#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <JsonListener.h>
#include <time.h>
#include "OpenWeatherMapCurrent.h"
#include "OpenWeatherMapForecast.h"
#include <Nextion.h>


// initiate the client
OpenWeatherMapCurrent clientC;
OpenWeatherMapForecast clientF;

// See https://docs.thingpulse.com/how-tos/openweathermap-key/
String OPEN_WEATHER_MAP_APP_ID = "6f290a66a22957dd3a122c9b99af4d8b";
/*
Go to https://openweathermap.org/find?q= and search for a location. Go through the
result set and select the entry closest to the actual location you want to display 
data for. It'll be a URL like https://openweathermap.org/city/2657896. The number
at the end is what you assign to the constant below.
 */
String OPEN_WEATHER_MAP_LOCATION_ID = "587629";
/*
Arabic - ar, Bulgarian - bg, Catalan - ca, Czech - cz, German - de, Greek - el,
English - en, Persian (Farsi) - fa, Finnish - fi, French - fr, Galician - gl,
Croatian - hr, Hungarian - hu, Italian - it, Japanese - ja, Korean - kr,
Latvian - la, Lithuanian - lt, Macedonian - mk, Dutch - nl, Polish - pl,
Portuguese - pt, Romanian - ro, Russian - ru, Swedish - se, Slovak - sk,
Slovenian - sl, Spanish - es, Turkish - tr, Ukrainian - ua, Vietnamese - vi,
Chinese Simplified - zh_cn, Chinese Traditional - zh_tw.
*/
String OPEN_WEATHER_MAP_LANGUAGE = "en";
boolean IS_METRIC = true;
uint8_t MAX_FORECASTS = 3;

/**
 * WiFi Settings
 */
const char* ESP_HOST_NAME = "esp-" + ESP.getFlashChipId();
const char* WIFI_SSID     = "Parzival";
const char* WIFI_PASSWORD = "EF13FCBC9D";

// initiate the WifiClient
WiFiClient wifiClient;

// initate Nextion
#define nextion Serial1
Nextion nex(nextion, 9600); //create a Nextion object named myNextion using the nextion serial port @ 9600bps

// decalare helpers
String icons[] = {"clear", "cloudy", "flurries", "fog", "hazy", "mostlycloudy", "moustlysunny", "rain", "sleet", "snow", "tstorms", "nt_clear", "nt_mostlycloudy", "nt_mostlysunny"};


/**
 * Helping funtions
 */
void connectWifi() {
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
  }
}

void loop() {
  nex.setComponentText("textLoading", "1");
  delay(500);
  nex.setComponentText("textLoading", "2");
  delay(500);
}

void updateData() {
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
    //UPDATE_INTERVAL_inSECS = 30;
    //readyforConnect = true;
  }
  else
  {
  OpenWeatherMapCurrentData data;
  clientC.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  clientC.setMetric(IS_METRIC);
  OpenWeatherMapForecastData dataF[MAX_FORECASTS];
  clientF.setMetric(IS_METRIC);
  clientF.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  uint8_t allowedHours[] = {6,12};
  clientF.setAllowedHours(allowedHours, 2);
    
  clientC.updateCurrentById(&data, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
  uint8_t foundForecasts = clientF.updateForecastsById(dataF, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);
//  for (int i = 0; i < sizeof(icons) / sizeof(icons[0]); i++) {            //Loop through Icons array
//    if (conditions.weatherIcon == icons[i] || conditions.weatherIcon == icons_alt[i]) {
//      Serial.println("Icon index: " + String(i) + " | Icon: " + conditions.weatherIcon);
//      String IconFinal = "p0.pic=" + String(i);  //forecast icons start from id 15 (on nextion)
//      char *pointerString = new char[  IconFinal.length() + 1];                //Covert string to *char
//      strcpy(pointerString, IconFinal.c_str());                              //
//      nex.sendCommand(pointerString);                                           //Change the icon on screen
//      delete [] pointerString;                                                  //Delete the *char to free up mem
//
//      IconFinal = "NexTemp.picc=" + String(i);
//      char *pointerStr = new char[  IconFinal.length() + 1];
//      strcpy(pointerStr, IconFinal.c_str());
//      nex.sendCommand(pointerStr);
//      delete [] pointerStr;
//
//      IconFinal = "NexTimeSec.picc=" + String(i);
//      char *pointerSt = new char[  IconFinal.length() + 1];
//      strcpy(pointerSt, IconFinal.c_str());
//      nex.sendCommand(pointerSt);
//      delete [] pointerSt;
//      break;
//    }
//  }
  
  
  
  Serial.println("------------------------------------");

  // "main": "Rain", String main;
  Serial.printf("main: %s\n", data.main.c_str());
  // "description": "shower rain", String description;
  Serial.printf("description: %s\n", data.description.c_str());
  // "icon": "09d" String icon; String iconMeteoCon;
  Serial.printf("icon: %s\n", data.icon.c_str());
  Serial.printf("iconMeteoCon: %s\n", data.iconMeteoCon.c_str());
  // "temp": 290.56, float temp;
  Serial.printf("temp: %f\n", data.temp);
  // "pressure": 1013, uint16_t pressure;
  Serial.printf("pressure: %d\n", data.pressure);
  // "humidity": 87, uint8_t humidity;
  Serial.printf("humidity: %d\n", data.humidity);
  // "temp_min": 289.15, float tempMin;
  Serial.printf("tempMin: %f\n", data.tempMin);
  // "temp_max": 292.15 float tempMax;
  Serial.printf("tempMax: %f\n", data.tempMax);
  // "wind": {"speed": 1.5}, float windSpeed;
  Serial.printf("windSpeed: %f\n", data.windSpeed);
  // "wind": {"deg": 1.5}, float windDeg;
  Serial.printf("windDeg: %f\n", data.windDeg);
  // "clouds": {"all": 90}, uint8_t clouds;
  Serial.printf("clouds: %d\n", data.clouds);
  // "dt": 1527015000, uint64_t observationTime;
  time_t time = data.observationTime;
  Serial.printf("observationTime: %d, full date: %s", data.observationTime, ctime(&time));
  // "country": "CH", String country;
  Serial.printf("country: %s\n", data.country.c_str());
  // "sunrise": 1526960448, uint32_t sunrise;
  time = data.sunrise;
  Serial.printf("sunrise: %d, full date: %s", data.sunrise, ctime(&time));
  // "sunset": 1527015901 uint32_t sunset;
  time = data.sunset;
  Serial.printf("sunset: %d, full date: %s", data.sunset, ctime(&time));

  Serial.println("---------------------------------------------------/\n");

  for (uint8_t i = 0; i < foundForecasts; i++) {
    Serial.printf("---\nForecast number: %d\n", i);
    // {"dt":1527066000, uint32_t observationTime;
    time = dataF[i].observationTime;
    Serial.printf("observationTime: %d, full date: %s", dataF[i].observationTime, ctime(&time));
    // "main":{
    //   "temp":17.35, float temp;
    Serial.printf("temp: %f\n", dataF[i].temp);
    //   "temp_min":16.89, float tempMin;
    Serial.printf("tempMin: %f\n", dataF[i].tempMin);
    //   "temp_max":17.35, float tempMax;
    Serial.printf("tempMax: %f\n", dataF[i].tempMax);
    //   "pressure":970.8, float pressure;
    Serial.printf("pressure: %f\n", dataF[i].pressure);
    //   "sea_level":1030.62, float pressureSeaLevel;
    Serial.printf("pressureSeaLevel: %f\n", dataF[i].pressureSeaLevel);
    //   "grnd_level":970.8, float pressureGroundLevel;
    Serial.printf("pressureGroundLevel: %f\n", dataF[i].pressureGroundLevel);
    //   "humidity":97, uint8_t humidity;
    Serial.printf("humidity: %d\n", dataF[i].humidity);
    //   "temp_kf":0.46
    // },"weather":[{
    //   "id":802, uint16_t weatherId;
    Serial.printf("weatherId: %d\n", dataF[i].weatherId);
    //   "main":"Clouds", String main;
    Serial.printf("main: %s\n", dataF[i].main.c_str());
    //   "description":"scattered clouds", String description;
    Serial.printf("description: %s\n", dataF[i].description.c_str());
    //   "icon":"03d" String icon; String iconMeteoCon;
    Serial.printf("icon: %s\n", dataF[i].icon.c_str());
    Serial.printf("iconMeteoCon: %s\n", dataF[i].iconMeteoCon.c_str());
    // }],"clouds":{"all":44}, uint8_t clouds;
    Serial.printf("clouds: %d\n", dataF[i].clouds);
    // "wind":{
    //   "speed":1.77, float windSpeed;
    Serial.printf("windSpeed: %f\n", dataF[i].windSpeed);
    //   "deg":207.501 float windDeg;
    Serial.printf("windDeg: %f\n", dataF[i].windDeg);
    // rain: {3h: 0.055}, float rain;
    Serial.printf("rain: %f\n", dataF[i].rain);
    // },"sys":{"pod":"d"}
    // dt_txt: "2018-05-23 09:00:00"   String observationTimeText;
    Serial.printf("observationTimeText: %s\n", dataF[i].observationTimeText.c_str());
  }
  }
  
}


void setup() {
  Serial.begin(115200);
  delay(500);
  connectWifi();
  updateData();
  }
