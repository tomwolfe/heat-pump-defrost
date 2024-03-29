#ifdef ESP32   
       #include <WiFi.h>
#endif
#include <Arduino.h>

// SinricPro - Version: Latest 
#include <SinricPro.h>
#include <SinricProThermostat.h>

// from github: https://github.com/ddebasilio/esp8266-google-home-notifier
#include <esp8266-google-home-notifier.h>

// LinearRegression - Version:
// from github: https://github.com/akkoyun/LinearRegression not in arduino library yet
#include <LinearRegression.h>

// OneButton - Version: Latest 
#include <OneButton.h>

// DHT sensor library - Version: 1.4.2
#include <DHT.h>
#include <DHT_U.h>

#include <LiquidCrystal_I2C.h>

#define DHTTYPE DHT11
#define APP_KEY "7d7b05d9-924d-4a2d-bbf5-240a6469ae56"
#define APP_SECRET SECRET_APP_SECRET
#define THERMOSTAT_ID "6350bb30134b2df11cd08698"

const unsigned int COMPRESSOR = 2;
const unsigned int TEMP_SENSOR_EXHAUST = 25;
const unsigned int TEMP_SENSOR_AMBIENT = 33;
const unsigned int TEMP_SENSOR_OUTSIDE = 32;
const unsigned int BUTTON = 13;
OneButton btn = OneButton(BUTTON, true, true);
// LCD interface pins
// i2c now... const int rs = 12, en = 11, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
float temp_exhaust;
float temp_ambient;
//float temp_outside;
float humidity_exhaust;
float humidity_ambient;
float last_humidity_ambient;
float humidity_outside;
float heat_index_exhaust;
float heat_index_ambient;
float last_heat_index_ambient;
float heat_index_outside;
//float target=90.0;
const float SETBACK=2.0; // dht11 +-1 degree of accuracy
const float MIN_DIFF=13.0; // depends on fan speed/etc
const float MIN_DIFF_LAST=2.0;
float max_diff=0.0;
//float max_diff_cycle=0.0;
//float curr_diff=0.0;
float last_diff=0.0;
unsigned long cycle=0;
unsigned long defrost_fails=0;
unsigned long total_defrost_fails=0;
unsigned long turned_on_from_fails=1;
unsigned long total_turned_on_from_fails=0;
unsigned long previous_millis_compressor=0;
unsigned long previous_millis_dht=0;
unsigned long previous_millis_lcd = 0;
const unsigned int COMPRESSOR_INTERVAL=60000;
unsigned long current_millis_compressor=0;
unsigned long current_millis_dht=0;
unsigned long current_millis_lcd=0;
//bool compressor_state=false; //true low/on : false high/off //
const unsigned int TONE_FREQ[] = {330, 294, 262};
const unsigned int TONE_COUNT = sizeof(TONE_FREQ)/sizeof(TONE_FREQ[0]);
int tones=0;
const unsigned int PIEZO_PIN=13;
bool sound_state=true;
unsigned int page = 1;
const unsigned int DISPLAY_INTERVAL=1700;
unsigned long last_interrupt_time = 0;
LinearRegression lr;
bool sufficient_training=false;
unsigned long running_millis_start=0;
unsigned long running_millis_end=0;
unsigned long defrost_millis_start=0;
unsigned long defrost_millis_end=0;
unsigned long running_millis_total=0;
unsigned long defrost_millis_total=0;
unsigned long reset_display_millis=0;
unsigned long undertemp_millis_start=0;
unsigned long target_millis_start=0;
const unsigned int RESET_DISPLAY_DELAY=3000;
unsigned long sound_millis=0;
bool bypass=false; // bypass learning mostly when undertemp/targethit
const float UNDERTEMP=34.0;
bool undertemp_state=false;
bool target_reached=false;
float def_mins=0.0;
unsigned long sinric_millis=0;

DHT dht_exhaust(TEMP_SENSOR_EXHAUST, DHTTYPE);
DHT dht_ambient(TEMP_SENSOR_AMBIENT, DHTTYPE);
DHT dht_outside(TEMP_SENSOR_OUTSIDE, DHTTYPE);

// i2c... LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* 
  Sketch generated by the Arduino IoT Cloud Thing "Untitled"
  https://create.arduino.cc/cloud/things/d42034ea-8a25-44b1-841f-852efa429831 

  Arduino IoT Cloud Variables description

  The following variables are automatically generated and updated when changes are made to the Thing

  float curr_diff;
  float max_diff_cycle;
  float target;
  float temp_outside;
  bool compressor_state;

  Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
  which are called when their values are changed from the Dashboard.
  These functions are generated with the Thing and added at the end of this sketch.
*/

#include "thingProperties.h"
GoogleHomeNotifier ghn;
GoogleHomeNotifier living_room;

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500);
  pinMode(COMPRESSOR, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  dht_exhaust.begin();
  dht_ambient.begin();
  dht_outside.begin();
  lcd.init();
  lcd.backlight();
  btn.attachClick(click);
  btn.attachDoubleClick(doubleClick);
  btn.attachLongPressStart(longPress);
  btn.setPressTicks(2000); // long press duration

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  
  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you’ll get.
     The default is 0 (only errors).
     Maximum is 4
 */
  setDebugMessageLevel(4);
  ArduinoCloud.printDebugInfo();
  setupWiFi();
  
  // google home notifier stuff
  IPAddress ip(192, 168, 1, 118); // ip address of your google home mini nest
  IPAddress ip_living_room(192, 168, 1, 152);
  Serial.println("connecting to Google Home...");
  if (ghn.ip(ip, "EN", 8009) != true) { // this uses the IP address and language english
    Serial.println(ghn.getLastError());
  }
  if (living_room.ip(ip_living_room, "EN", 8009) != true) { // this uses the IP address and language english
    Serial.println(living_room.getLastError());
  }
  setupSinricPro();
}

void loop() {
  ArduinoCloud.update();
  // Your code here 
  SinricPro.handle();
  if (target==NULL) {
    target=90;
  }
  if (compressor_state==NULL) {
    compressor_state = false;
  }
  current_millis_compressor=millis();
  current_millis_dht=millis();
  current_millis_lcd=millis();
  Serial.print(F("cycle: "));
  Serial.println(cycle);
  if (checkTimeDHT()) {
    if (measuretemps()) {
      lcdLogic();
    }
  }
  btn.tick();
  playTones();
  sinricUpdate();
}


bool measuretemps() {
  temp_exhaust= dht_exhaust.readTemperature(true); // true=Fahrenheit, blank Celsius.
  temp_ambient= dht_ambient.readTemperature(true);
  temp_outside= dht_outside.readTemperature(true);
  humidity_exhaust= dht_exhaust.readHumidity();
  humidity_ambient= dht_ambient.readHumidity();
  humidity_outside= dht_outside.readHumidity();

  if (sensorCheck()) {
    return false;
  }
  else {
    heatIndex();
    Serial.print(F("heat_index_ambient: "));
    Serial.println(heat_index_ambient);
    Serial.print(F("heat_index_exhaust: "));
    Serial.println(heat_index_exhaust);
    Serial.print(F("temp_exhaust: "));
    Serial.println(temp_exhaust);
    Serial.print(F("temp_ambient: "));
    Serial.println(temp_ambient);
    Serial.print(F("temp_outside: "));
    Serial.println(temp_outside);
    Serial.print(F("humidity_exhaust: "));
    Serial.println(humidity_exhaust);
    Serial.print(F("humidity_ambient: "));
    Serial.println(humidity_ambient);
    Serial.print(F("humidity_outside: "));
    Serial.println(humidity_outside);
    targetLogic();
    outsideLogic();
    difference();
    return true;
  }
}

bool sensorCheck() {
  bool flag=false;
  if (isnan(temp_exhaust) || isnan(humidity_exhaust)) {
    Serial.println(F("Failed to read from exhaust DHT sensor!"));
    flag=true;
  }
  else {
    Serial.println(F("exhaust readings"));
    Serial.println(temp_exhaust);
    Serial.println(humidity_exhaust);
  }
  if (isnan(temp_ambient) || isnan(humidity_ambient)) {
    Serial.println(F("Failed to read from ambient DHT sensor!"));
    flag=true;
  }
  else {
    Serial.println(F("ambient readings"));
    Serial.println(temp_ambient);
    Serial.println(humidity_ambient);
  }
  if (isnan(temp_outside) || isnan(humidity_outside)) {
    Serial.println(F("Failed to read from outside DHT sensor!"));
    flag=true;
  }
  else {
    Serial.println(F("outside readings"));
    Serial.println(temp_outside);
    Serial.println(humidity_outside);
  }
  return flag;
}

void heatIndex() {
  heat_index_exhaust= dht_exhaust.computeHeatIndex(temp_exhaust, humidity_exhaust);
  heat_index_ambient= dht_ambient.computeHeatIndex(temp_ambient, humidity_ambient);
  heat_index_outside= dht_outside.computeHeatIndex(temp_outside, humidity_outside);
}

void difference() {
  if (checkTimeCompressor()) {
    last_diff=curr_diff;
    curr_diff=temp_exhaust-temp_ambient;
    Serial.print(F("current difference exhaust-ambient: "));
    Serial.println(curr_diff);
    Serial.print(F("current difference - last difference: "));
    Serial.println(curr_diff-last_diff);
    if (curr_diff>max_diff) {
      max_diff=curr_diff;
      Serial.print(F("new max diff: "));
      Serial.println(max_diff);
    }
    if (curr_diff>max_diff_cycle) {
      max_diff_cycle=curr_diff;
    }
    defrostLogic();
  }
}

void defrostLogic() {
  if (((curr_diff-last_diff)>MIN_DIFF_LAST) || (curr_diff>(max_diff*0.98)) || ((curr_diff>(max_diff_cycle*0.98))&&(max_diff_cycle>MIN_DIFF))  || (cycle==0) || (bypass==true)) {
    defrostSuccess();
  }
  else {
    defrostFailed();
  }
}

void targetLogic() {  // Idea/TODO: maybe merge targetLogic() and outsideLogic()
  if (heat_index_ambient < target) {
    if (target_reached && ((millis()-target_millis_start)>(60000*3)) && (heat_index_ambient < (target-SETBACK))) {
      Serial.println(F("heat_index_ambient < (target-SETBACK)"));
      target_reached=false;
    }
  }
  else {
    targetHit();
    playTone(TONE_FREQ[2], 2000);
    displayInterrupt(14);
  }
}

void targetHit() {
  if (!target_reached) {
    Serial.println(F("heat_index_ambient > (target-SETBACK)"));
    turnOff();
    target_millis_start=millis();
    bypass=true;
    target_reached=true;
    String s = "Heat pump target reached. It's currently " + String(heat_index_ambient, 1) + " degrees inside with a target of " + String(target, 1) + " degrees.";
    if (ghn.notify(s.c_str()) != true) {
      Serial.println(ghn.getLastError());
      return;
    }
    if (living_room.notify(s.c_str()) != true) {
      Serial.println(living_room.getLastError());
      return;
    }
  }
}

void outsideLogic() {
  if (temp_outside > UNDERTEMP) {
    if ( ((undertemp_state) && ((millis()-undertemp_millis_start)>(60000*3)) && (temp_outside > (UNDERTEMP+SETBACK))) ) {
      undertemp_state=false;
    }
  }
  else {
    undertempHit();
    displayInterrupt(14);
    playTone(TONE_FREQ[0], 2000);
  }
}

void undertempHit() {
  if (!undertemp_state) {
    Serial.print(F("temp_outside =< UNDERTEMP, UNDERTEMP: "));
    Serial.println(UNDERTEMP);
    turnOff();
    undertemp_millis_start=millis();
    bypass=true;
    undertemp_state=true;
    String s = "It's currently  " + String(temp_outside, 1) + " degrees outside. Consider turning off the heat pump.";
    if (ghn.notify(s.c_str()) != true) {
      Serial.println(ghn.getLastError());
      return;
    }
    if (living_room.notify(s.c_str()) != true) {
      Serial.println(living_room.getLastError());
      return;
    }
  }
}

void turnOn() {
  if (!compressor_state && !undertemp_state && !target_reached) {
    Serial.println(F("turning on"));
    digitalWrite(COMPRESSOR, HIGH);
    compressor_state=true;
    defrost_millis_end=millis()-defrost_millis_start;
    defrost_millis_total+=defrost_millis_end;
    running_millis_start=millis();
    resetDisplay();
  }
  if (cycle==0) {
      cycle++;
  }
}

void turnOff() {
  if (compressor_state) {
    Serial.println(F("turning off"));
    digitalWrite(COMPRESSOR, LOW);
    compressor_state=false;
    cycle++;
    max_diff_cycle=last_diff;
    running_millis_end=millis()-running_millis_start;
    running_millis_total+=running_millis_end;
    defrost_millis_start=millis();
    resetDisplay();
  }
}

bool checkTimeCompressor() {
  if (cycle==0) {
    return true;
  }
  if ((current_millis_compressor-previous_millis_compressor)>=COMPRESSOR_INTERVAL) {
    previous_millis_compressor=current_millis_compressor;
    return true;
  }
  else {
    return false;
  }
}

bool checkTimeDHT() {
  // dht min delay 2000
  if ((current_millis_dht-previous_millis_dht)>=3000) {
    previous_millis_dht=current_millis_dht;
    return true;
  }
  else {
    return false;
  }
}

void defrostFailed() {
  if (sound_state && defrost_fails==0 && turned_on_from_fails >=2) {
    tones=TONE_COUNT*turned_on_from_fails;
  }
  defrost_fails++;
  total_defrost_fails++;
  Serial.print(F("defrost check failed. Current defrost_fails: "));
  Serial.print(defrost_fails);
  Serial.print(F("\t total_defrost_fails: "));
  Serial.print(total_defrost_fails);
  Serial.print(F("\t turned_on_from_fails: "));
  Serial.println(turned_on_from_fails);
  def_mins=lr.Calculate(temp_outside);
  if (sufficient_training && def_mins>=3.0 && def_mins<=10.0) {  // failsafe if prediction is way off
    if ((turned_on_from_fails>2) || (max_diff_cycle<MIN_DIFF)) { // if failed before, don't 2x time. smaller steps.
      def_mins++;
    }
    else if (def_mins >= 4.0) { // try to pull down training defrost time
      def_mins--;
    }
    else {
    }
    tryStartCond(def_mins);
  }
  else {
    tryStartCond((float)(3*turned_on_from_fails));
  }
}

void tryStartCond(float mins) {
  if ((float)defrost_fails>=mins && curr_diff<=4.1) {
    tryStart();
  }
  else {
    turnOff();
  }
}

void playTones() {
  if (tones>0) {
    if (millis() - sound_millis >= 500) {
      playTone(TONE_FREQ[tones%TONE_COUNT], 500);
      tones--;
      sound_millis=millis();
    }
  }
}

void tryStart() {
  Serial.print(F("defrost failed: "));
  Serial.print(3*turned_on_from_fails);
  Serial.println(F(" times, trying to restart"));
  turnOn();
  turned_on_from_fails++;
  if (turned_on_from_fails>2) {
    total_turned_on_from_fails++;
  }
}

void defrostSuccess() {
  if (turned_on_from_fails==2) {
    if (!sufficient_training) {
      sufficient_training=true;
      Serial.print(F("sufficient training. samples: "));
      Serial.println(lr.Samples());
      displayInterrupt(11);
    }
  }
  if (turned_on_from_fails>=2 && bypass==false) {
    lr.Data(temp_outside, (float)defrost_fails); // minimum regression learning
  }
  Serial.println(F("defrost check successful"));
  Serial.println(F("resetting turnedonfromfail and defrost_fails"));
  turned_on_from_fails=1;
  defrost_fails=0;
  bypass=false;
  turnOn();
}

void displayInterrupt(int a) {
  page=a;
  previous_millis_lcd = current_millis_lcd; // give time to display
}

void lcdLogic() {
  String lineOne, lineTwo;
  switch (page) {
    case 1:
      lineOne = "Target: ";
      lineOne=lineOne+target+"F";
      lineTwo = "Curr diff: ";
      lineTwo=lineTwo+curr_diff;
      lcdPrint(lineOne,lineTwo);
      page++;
      break;
    case 2:
      if (current_millis_lcd - previous_millis_lcd >= DISPLAY_INTERVAL) {
        lineOne = "Ambient: ";
        lineOne=lineOne+temp_ambient+"F";
        lineTwo = "Exhaust: ";
        lineTwo=lineTwo+temp_exhaust+"F";
        lcdPrint(lineOne,lineTwo);
        page++;
      }
      break;
    case 3:
      if (current_millis_lcd - previous_millis_lcd >= DISPLAY_INTERVAL) {
        lineOne = "Outside: ";
        lineOne=lineOne+temp_outside+"F";
        lineTwo = "Ambient: ";
        lineTwo=lineTwo+humidity_ambient+"%";
        lcdPrint(lineOne,lineTwo);
        page++;
      }
      break;
    case 4:
      if (current_millis_lcd - previous_millis_lcd >= DISPLAY_INTERVAL) {
        lineOne = "Exhaust: ";
        lineOne=lineOne+humidity_exhaust+"%";
        lineTwo = "Outside: ";
        lineTwo=lineTwo+humidity_outside+"%";
        lcdPrint(lineOne,lineTwo);
        page++;
      }
      break;
    case 5:
      if (current_millis_lcd - previous_millis_lcd >= DISPLAY_INTERVAL) {
        lineOne = "HIdx Amb: ";
        lineOne=lineOne+String(heat_index_ambient,2)+"F"; // 2 decimal place
        lineTwo = "LastDiff: ";
        lineTwo=lineTwo+last_diff+"F";
        lcdPrint(lineOne,lineTwo);
        page++;
      }
      break;
    case 6:
      if (current_millis_lcd - previous_millis_lcd >= DISPLAY_INTERVAL) {
        lineOne = "Cycles: ";
        lineOne=lineOne+cycle;
        lineTwo = "DfrstFails: ";
        lineTwo=lineTwo+defrost_fails;
        lcdPrint(lineOne,lineTwo);
        page++;
      }
      break;
    case 7:
      if (current_millis_lcd - previous_millis_lcd >= DISPLAY_INTERVAL) {
        lineOne = "TotDefFail: ";
        lineOne=lineOne+total_defrost_fails;
        lineTwo = "OnFromFail: ";
        lineTwo=lineTwo+turned_on_from_fails;
        lcdPrint(lineOne,lineTwo);
        page++;
      }
      break;
    case 8:
      if (current_millis_lcd - previous_millis_lcd >= DISPLAY_INTERVAL) {
        lineOne = "SoundState: ";
        lineOne=lineOne+sound_state;
        lineTwo = "compress: ";
        lineTwo=lineTwo+compressor_state;
        lcdPrint(lineOne,lineTwo);
        page++;
      }
      break;
    case 9:
      if (current_millis_lcd - previous_millis_lcd >= DISPLAY_INTERVAL) {
        if (compressor_state) {
          lineOne = "CurRunMin: ";
          lineOne=lineOne+int((millis()-running_millis_start)/1000/60);
        }
        else {
          lineOne = "CurDfstMin: ";
          lineOne=lineOne+int((millis()-defrost_millis_start)/1000/60);
        }
        lineTwo = "TotDfstMin: ";
        lineTwo=lineTwo+int((defrost_millis_total)/1000/60);
        lcdPrint(lineOne,lineTwo);
        page++;
      }
      break;
    case 10:
      if (current_millis_lcd - previous_millis_lcd >= DISPLAY_INTERVAL) {
        float mb[2];
        lr.Parameters(mb);
        lineOne = "TotRunMin: ";
        lineOne=lineOne+int((running_millis_total)/1000/60);
        lineTwo = "m: ";
        lineTwo=lineTwo+String(mb[0],1)+" b: "+String(mb[1],1); //1 decimal place
        lcdPrint(lineOne,lineTwo);
        page++;
      }
      break;
    case 11:
      if (current_millis_lcd - previous_millis_lcd >= DISPLAY_INTERVAL) {
        lineOne = "SufTrn:";
        lineOne=lineOne+sufficient_training;
        lineTwo = "PredMinDef";
        lineTwo=lineTwo+String(lr.Calculate(temp_outside),1); //1 decimal place
        lcdPrint(lineOne,lineTwo);
        page++;
      }
      break;
      case 12:
      if (current_millis_lcd - previous_millis_lcd >= DISPLAY_INTERVAL) {
        lineOne = "Samples: ";
        lineOne=lineOne+lr.Samples();
        lineTwo = "Max_diff";
        lineTwo=lineTwo+max_diff;
        lcdPrint(lineOne,lineTwo);
        page++;
      }
      break;
      case 13:
      if (current_millis_lcd - previous_millis_lcd >= DISPLAY_INTERVAL) {
        lineOne = "maxdiffcy: ";
        lineOne=lineOne+max_diff_cycle;
        lineTwo = "totOnFrFail: ";
        lineTwo=lineTwo+total_turned_on_from_fails;
        lcdPrint(lineOne,lineTwo);
        page++;
      }
      break;
      case 14:
      if (current_millis_lcd - previous_millis_lcd >= DISPLAY_INTERVAL) {
        lineOne = "undertemp: ";
        lineOne=lineOne+undertemp_state;
        lineTwo = "target_reached: ";
        lineTwo=lineTwo+target_reached;
        lcdPrint(lineOne,lineTwo);
        page=1;
      }
      break;
  }
}

void lcdPrint(String one, String two) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(one);
  lcd.setCursor(0, 1);
  lcd.print(two);
  previous_millis_lcd = current_millis_lcd;
}

void doubleClick() {
  target=55.0;
  displayInterrupt(1);
}

void click() {
  target+=5.0;
  displayInterrupt(1);
}

void longPress() {
  sound_state=!sound_state;
  displayInterrupt(8);
}

void resetDisplay() {
  // EMF corruption switching relay
  if(millis() >= reset_display_millis + RESET_DISPLAY_DELAY){
    reset_display_millis=millis();
    lcd.init();
    lcd.backlight();
    lcd.clear();
  }
}

void playTone(int a_tone, int duration) {
  if (sound_state) {
    tone(PIEZO_PIN, a_tone, duration);
  }
}


/*
  Since Target is READ_WRITE variable, onTargetChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onTargetChange()  {
  // Add your code here to act upon Target change
}


// Sinric pro functions
bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("Thermostat %s turned %s\r\n", deviceId.c_str(), state?"on":"off");
  return true; // request handled properly
}

bool onTargetTemperature(const String &deviceId, float &temperature) {
  Serial.printf("Thermostat %s set temperature to %f\r\n", deviceId.c_str(), temperature);
  target = ctof(temperature);
  return true;
}

bool onAdjustTargetTemperature(const String & deviceId, float &temperatureDelta) {
  target += ctof(temperatureDelta);  // calculate absolut temperature
  Serial.printf("Thermostat %s changed temperature about %f to %f", deviceId.c_str(), temperatureDelta, target);
  temperatureDelta = ftoc(target); // return absolut temperature
  return true;
}

bool onThermostatMode(const String &deviceId, String &mode) {
  Serial.printf("Thermostat %s set to mode %s\r\n", deviceId.c_str(), mode.c_str());
  return true;
}

void setupSinricPro() {
  Serial.println("setting up sinric");
  SinricProThermostat &myThermostat = SinricPro[THERMOSTAT_ID];
  myThermostat.onPowerState(onPowerState);
  myThermostat.onTargetTemperature(onTargetTemperature);
  myThermostat.onAdjustTargetTemperature(onAdjustTargetTemperature);
  myThermostat.onThermostatMode(onThermostatMode);

  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  IPAddress localIP = WiFi.localIP();
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %d.%d.%d.%d\r\n", localIP[0], localIP[1], localIP[2], localIP[3]);
}

void sinricUpdate() {
  unsigned long actualMillis = millis();
  if (actualMillis - sinric_millis < 60000) return; //only check every EVENT_WAIT_TIME milliseconds
  if (heat_index_ambient == last_heat_index_ambient && humidity_ambient == last_humidity_ambient) return; // if no values changed do nothing.
  SinricProThermostat &myThermostat = SinricPro[THERMOSTAT_ID];  // get temperaturesensor device
  bool success = myThermostat.sendTemperatureEvent(ftoc(heat_index_ambient), humidity_ambient); // send event
  if (success) {  // if event was sent successfuly, print temperature and humidity to serial
    Serial.println("Temperature humidity sent");
    Serial.println(heat_index_ambient);
  } else {  // if sending event failed, print error message
    Serial.printf("Something went wrong...could not send Event to server!\r\n");
  }

  last_heat_index_ambient = heat_index_ambient;  // save actual temperature for next compare
  last_humidity_ambient = humidity_ambient;        // save actual humidity for next compare
  sinric_millis = actualMillis;       // save actual time for next compare
}

float ftoc(float f) {
  return (f-32)*0.5556;
}

float ctof(float c) {
  return 1.8 * c + 32.0;
}
