// OneButton - Version: Latest 
#include <OneButton.h>

// DHT sensor library - Version: 1.4.2
#include <DHT.h>
#include <DHT_U.h>

#include <LiquidCrystal.h>

#define DHTTYPE DHT11
const unsigned int COMPRESSOR = 7;
const unsigned int TEMP_SENSOR_EXHAUST = 9;
const unsigned int TEMP_SENSOR_AMBIENT = 10;
const unsigned int TEMP_SENSOR_OUTSIDE = 8;
const unsigned int BUTTON = 2;
OneButton btn = OneButton(BUTTON, true, true);
// LCD interface pins
const int rs = 12, en = 11, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
float temp_exhaust;
float temp_ambient;
float temp_outside;
float humidity_exhaust;
float humidity_ambient;
float humidity_outside;
float heat_index_exhaust;
float heat_index_ambient;
float heat_index_outside;
float target=55.0;
const float SETBACK=1.0;
const float MIN_DIFF=10.0;
const float MIN_DIFF_LAST=2.0;
float max_diff=0.0;
float curr_diff=0.0;
float last_diff=0.0;
unsigned long cycle=0;
unsigned long defrost_fails=0;
unsigned long total_defrost_fails=0;
unsigned long turned_on_from_fails=1;
unsigned long previous_millis_compressor=0;
unsigned long previous_millis_dht=0;
unsigned long previous_millis_lcd = 0;
const unsigned int COMPRESSOR_INTERVAL=60000;
unsigned long current_millis_compressor=0;
unsigned long current_millis_dht=0;
unsigned long current_millis_lcd=0;
bool compressor_state=false; //true low/on : false high/off
const unsigned int TONE_FREQ[] = {330, 294, 262};
const unsigned int TONE_COUNT = sizeof(TONE_FREQ)/sizeof(TONE_FREQ[0]);
const unsigned int PIEZO_PIN=13;
bool sound_state=true;
unsigned int page = 1;
const unsigned int DISPLAY_INTERVAL=3200;
unsigned long last_interrupt_time = 0;

DHT dht_exhaust(TEMP_SENSOR_EXHAUST, DHTTYPE);
DHT dht_ambient(TEMP_SENSOR_AMBIENT, DHTTYPE);
DHT dht_outside(TEMP_SENSOR_OUTSIDE, DHTTYPE);

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  Serial.begin(9600);
  pinMode(COMPRESSOR, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  //turnOff();
  dht_exhaust.begin();
  dht_ambient.begin();
  dht_outside.begin();
  lcd.begin(16, 2);
  btn.attachClick(click);
  btn.attachDoubleClick(doubleClick);
  btn.attachLongPressStart(longPress);
  btn.setPressTicks(2000);
}

void loop() {
  current_millis_compressor=millis();
  current_millis_dht=millis();
  current_millis_lcd=millis();
  Serial.print(F("cycle: "));
  Serial.println(cycle);
  if (checkTimeDHT()) {
    if (measuretemps()) {
      //templogic();
      lcdLogic();
    }
  }
  btn.tick();
}

bool measuretemps() {
  temp_exhaust= dht_exhaust.readTemperature(true); //true=Fahrenheit, blank Celsius.
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
  if (isnan(temp_ambient) || isnan(humidity_ambient)) {
    Serial.println(F("Failed to read from ambient DHT sensor!"));
    flag=true;
  }
  if (isnan(temp_outside) || isnan(humidity_outside)) {
    Serial.println(F("Failed to read from outside DHT sensor!"));
    flag=true;
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
    templogic();
  }
}

void templogic() {
  if (heat_index_ambient < (target-SETBACK)) {
    Serial.println(F("heat_index_ambient < (target-SETBACK)"));
    if (temp_outside > 35) {
      Serial.println(F("temp_outside > 35"));
      if ((curr_diff>MIN_DIFF) || ((curr_diff-last_diff)>MIN_DIFF_LAST) || (curr_diff>(max_diff*0.75)) || (cycle==0)) {
        defrostsuccess();
      }
      else {
        defrostfailed();
      }
    }
    else {
      Serial.println(F("temp_outside < 35"));
      turnOff();
      if (sound_state) {
        tone(PIEZO_PIN, TONE_FREQ[0], 2000);
      }
    }
  }
  else {
    Serial.println(F("heat_index_ambient > (target-SETBACK)"));
    turnOff();
  }
}

void turnOn() {
  if (!compressor_state) {
    Serial.println(F("turning on"));
    digitalWrite(COMPRESSOR, LOW);
    compressor_state=true;
    if (cycle==0) {
      cycle++;
    }
  }
}

void turnOff() {
  if (compressor_state) {
   Serial.println(F("turning off"));
    digitalWrite(COMPRESSOR, HIGH);
    compressor_state=false;
    cycle++;
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

void defrostfailed() {
  defrost_fails++;
  total_defrost_fails++;
  Serial.print(F("defrost check failed. Current defrost_fails: "));
  Serial.print(defrost_fails);
  Serial.print(F("\t total_defrost_fails: "));
  Serial.print(total_defrost_fails);
  Serial.print(F("\t turned_on_from_fails: "));
  Serial.println(turned_on_from_fails);
  if (defrost_fails==5*turned_on_from_fails) {
    if (sound_state) {
      for (int j=0; j < turned_on_from_fails; ++j) {
        for (int i=0; i < TONE_COUNT; ++i) {
          tone(PIEZO_PIN, TONE_FREQ[i], 450);
        }
      }
    }
    Serial.print(F("defrost failed: "));
    Serial.print(5*turned_on_from_fails);
    Serial.println(F(" times, trying to restart"));
    turnOn();
    turned_on_from_fails++;
  }
  else {
    turnOff();
  }
}

void defrostsuccess() {
  Serial.println(F("defrost check successful"));
  Serial.println(F("resetting turnedonfromfail and defrost_fails"));
  turned_on_from_fails=1;
  defrost_fails=0;
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
        page = 1;
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
