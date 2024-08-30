#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C //OLED display address

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET_DST 0
int UTC_OFFSET = 0;

#define BUZZER 12
#define PB_UP 34
#define PB_DOWN 33
#define PB_OK 32
#define PB_CANCEL 14
#define DHT_PIN 13
#define LED 27

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int hours = 0;
int minutes = 0;
int seconds = 0;
unsigned int timeNow = 0;
unsigned int timeLast = 0;

bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {11,11,11};
int alarm_minutes[] = {45,46,47};
bool alarm_triggered[] = {false,false,false};

int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 - Set Time","2 - Set Alarm 1","3 - Set Alarm 2","4 - Set Alarm 3","5 - Disable Alarms"};

DHTesp dhtSensor;

int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C,D,E,F,G,A,B,C_H};

void setup() {
  Serial.begin(115200);
  dhtSensor.setup(DHT_PIN,DHTesp::DHT22);
  
  pinMode(BUZZER,OUTPUT);
  pinMode(PB_UP,INPUT);
  pinMode(PB_DOWN,INPUT);
  pinMode(PB_OK,INPUT);
  pinMode(PB_CANCEL,INPUT);
  pinMode(LED,OUTPUT);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC,SCREEN_ADDRESS)){
    Serial.println(F("SSD_1306 allocation failed!"));
    for(;;);
  }
  display.display();
  delay(2000);
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line(0,0,2,"Connecting to WIFI");
  }
  display.clearDisplay();
  print_line(0,0,2,"Connected to WIFI");
  delay(50);
  display.clearDisplay();

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

}

void loop() {
  delay(10); // this speeds up the simulation
  update_time_with_check_alarm();
  if (digitalRead(PB_OK) == LOW) {
    delay(200); // This delay is added to debounce the push button
    go_to_menu();
  }
  tempAndHumidity();
}


void tempAndHumidity(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  float temperature = data.temperature;
  float humidity = data.humidity;

  if(isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  }
  else{
    if (temperature < 26 || temperature > 32 || humidity < 60 || humidity > 80) {
      print_line(0,20,2,"Warning:Unhealthy Conditions");
      digitalWrite(LED, HIGH);
      delay(300); 
      digitalWrite(LED, LOW);
      delay(300);
    } 
    else {
  
    }
  }
}

void print_time_now(){

  display.clearDisplay();
  print_line(0,0,2,String(hours)+":"+String(minutes)+":"+String(seconds));

}

void update_time(){
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute,3, "%M", &timeinfo);
  minutes = atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond,3, "%S", &timeinfo);
  seconds = atoi(timeSecond);

}
void update_time_with_check_alarm(void){
  update_time();
  print_time_now();
  if(alarm_enabled == true){
    for(int i = 0;i < n_alarms;i++){
      if(alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes){
        ring_alarm();
        alarm_triggered[i] == true;
      }
    }
  }
}

void ring_alarm() {
  display.clearDisplay();
  print_line(0, 0, 2, "MEDICINE TIME!");

  bool break_happened = false;

  // Ring the buzzer
  while (break_happened == false && digitalRead(PB_CANCEL) == HIGH) {
    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(1200); // This is to prevent the bouncing of the push button
        break_happened = true;
        break;
      }
      tone(BUZZER, notes[i]);
      delay(200);
      noTone(BUZZER);
      delay(2);
    }
  }
}

void print_line(int column,int row,int text_size,String text){

  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column,row);
  display.println(text);

  display.display();
}

int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(200);
      return PB_UP;
    }

    else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return PB_DOWN;
    }

    else if (digitalRead(PB_OK) == LOW) {
      delay(200);
      return PB_OK;
    }

    else if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return PB_CANCEL;
    }
    update_time();
  }
}

void go_to_menu() {
  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(0, 0, 2, modes[current_mode]);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode = max_modes - 1;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      Serial.println(current_mode);
      run_mode(current_mode);
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
}

void set_alarm(int alarm) {

  int temp_hour = alarm_hours[alarm];
  while (true) {
    display.clearDisplay();
    print_line(0, 0, 2, "Enter hour: " + String(temp_hour));

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line(0, 0, 2, "Enter minute: " + String(temp_minute));

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line(0, 0, 2, "Alarm is set");
  delay(300);
}

void run_mode(int mode){
  if (mode == 0){
    setTimeZone();
  }

  else if (mode == 1 || mode == 2 || mode == 3){
    set_alarm(mode - 1);
  }

  else if (mode == 4){
    alarm_enabled = false;
    display.clearDisplay();
    print_line(0, 0, 2, "Alarms disabled");
    delay(300);
  }
}

// selecting the time zone and configure time
void setTimeZone() {
  
  int temp_hour = hours;

  while (true){
    display.clearDisplay();
    print_line(0, 0, 2, "Enter hours offset: " + String(temp_hour));

    int pressed = wait_for_button_press();

    if (pressed == PB_UP){
      delay(200);
      temp_hour += 1;
      if (temp_hour > 14) {     //-12 <= UTC Offset <= 14
        temp_hour = -12;
      }
    }

    else if (pressed == PB_DOWN){
       delay(200);
      temp_hour -= 1;
      if (temp_hour < -12) {
        temp_hour = 14;
      }
    }

    else if (pressed == PB_OK){
      delay(200);
      //hours =  temp_hour; 
      int offset_hours = temp_hour;
      break;
    }

    else if (pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }

  int temp_minute = minutes;
  while (true){
    display.clearDisplay();
    print_line(0, 0, 2,"Enter minutes offset: " + String(temp_minute));

    int pressed = wait_for_button_press();
    if (pressed == PB_UP){
      delay(200);
      temp_minute += 1;
      if (temp_minute > 59) {
        temp_minute = 0;
      }
    } 

    else if (pressed == PB_DOWN){
      delay(200);
      temp_minute -= 1;
      if (temp_minute < -59) {
        temp_minute = 0;
      }
    }

    else if (pressed = PB_OK){
      delay(200);
      //minutes =  temp_minute;
      int offset_minutes = temp_minute;
      break;
    }

    else if (pressed = PB_CANCEL){
      delay(200);
      break;
    }
  }

  display.clearDisplay();

  int offset_hours = temp_hour;
  int offset_minutes = temp_minute;

  hours += offset_hours;      
  minutes += offset_minutes;
  
  UTC_OFFSET = offset_hours * 3600 + offset_minutes * 60; //Stting the offset
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
  print_line(0, 0, 2, "Timezone is set");
  delay(200);
  display.clearDisplay();
}



