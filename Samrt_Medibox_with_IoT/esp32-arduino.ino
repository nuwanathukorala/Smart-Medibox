#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

Servo myServo;
int p = 0;

#define DHT_PIN 15
#define BUZZER 12
#define LDR_1 32
#define LDR_2 33

float angle = 30;
float D = 0;
float contFact = 0.75;
float intensity = 0;
float rotateAngle = 0;
float calculatedAng = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

char tempAr[6];
char LDRAr_1[6];
char LDRAr_2[6];
DHTesp dhtSensor;

bool isScheduledON = false;
unsigned long scheduledOnTime;

void setup();
void loop();
void buzzerOn(bool on);
void setupMqtt();
void receiveCallback(char* topic, byte* payload, unsigned int length);
void connectToBroker();
void updateTemperature();
void setupWifi();

void setup() {
  Serial.begin(115200);
  myServo.attach(4);

  setupWifi();

  setupMqtt();

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  timeClient.begin();
  timeClient.setTimeOffset(5.5*3600);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  pinMode(LDR_1,INPUT);
  pinMode(LDR_2,INPUT);

}

void loop(){
  if (!mqttClient.connected()) {
    connectToBroker();
  }
  mqttClient.loop();

  updateTemperature();
  Serial.print("TEMP: ");
  Serial.println(tempAr);
  mqttClient.publish("EE-ADMIN-TEMP", tempAr);

  checkSchedule();

  float LDR1_OUT = analogRead(LDR_1);
  String(LDR1_OUT, 2).toCharArray(LDRAr_1, 6);
  Serial.print("LDR_1: ");
  Serial.println(LDRAr_1);
  mqttClient.publish("EE-ADMIN-LDR_1", LDRAr_1);

  float LDR2_OUT = analogRead(LDR_2);
  String(LDR2_OUT, 2).toCharArray(LDRAr_2, 6);
  Serial.print("LDR_2: ");
  Serial.println(LDRAr_2);
  mqttClient.publish("EE-ADMIN-LDR_2", LDRAr_2);

  //Check whether which one has the highest intensity
  if(LDR2_OUT > LDR1_OUT){
    //Left LDR has high intensity
    D = 1.5;
    intensity = (4063-LDR1_OUT)/4031;
  }
  else{
    //Right LDR has high intensity
    D = 0.5;
    intensity = (4063-LDR2_OUT)/4031;
  }
  Serial.print("Mninimum angle: ");
  Serial.println(angle);

  Serial.print("Maximum intensity: ");
  Serial.println(intensity);

  Serial.print("Control Factor: ");
  Serial.println(contFact);

  Serial.print("D: ");
  Serial.println(D);

  Serial.print("Motor angle: ");
  angleCalculate(angle,contFact,intensity,D);

  rotate();

  delay(1000);
}

void buzzerOn(bool on){
  if(on){
    tone(BUZZER, 256);
  }else{
    noTone(BUZZER);
  }
}

void setupMqtt(){
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(receiveCallback);
}

void receiveCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]");

  char payloadCharAr[length];

  for(int i = 0; i < length; i++){
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }

  if (strcmp(topic, "EE-ADMIN-MAIN-ON-OFF") == 0) {
    buzzerOn(payloadCharAr[0] == '1');
  }
  else if(strcmp(topic, "EE-ADMIN-SCH-ON") == 0){
    if (payloadCharAr[0] == 'N') {
      isScheduledON = false;
    }else{
      isScheduledON = true;
      scheduledOnTime = atol(payloadCharAr);
    }
  }
  //Fetch the set value for the minimun angle in node red
  else if(strcmp(topic, "MIN-ANG") == 0){
    angle = atof(payloadCharAr);
  }

  //Fetch the set value for the control factor in node red
  else if(strcmp(topic, "CONT-FACT") == 0){
    contFact = atof(payloadCharAr);
  }
}

void connectToBroker() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32-2323232323")) {
      Serial.print("connected");
      mqttClient.subscribe("EE-ADMIN-MAIN-ON-OFF");
      mqttClient.subscribe("EE-ADMIN-SCH-ON");
      mqttClient.subscribe("MIN-ANG");
      mqttClient.subscribe("CONT-FACT");
    }
    else{
      Serial.print("failed");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

//Calculate the motor angle
void angleCalculate(float angle,float contFact,float intensity,float D){
  calculatedAng = angle * D + (180 - angle) * contFact * intensity;
  if(calculatedAng < 180){
    Serial.println(calculatedAng);
  }
  else{
    Serial.println(180);
  }
}

//Setup sevo motor rotation according to the calculated angle
void rotate(){
  if (rotateAngle<calculatedAng){
    for (p=int(rotateAngle);p<int(calculatedAng);p++){
      myServo.write(p);
      delay(50);
      rotateAngle = calculatedAng;
    }
  }
  else{
    for (p=int(rotateAngle);p>int(calculatedAng);p--){
      myServo.write(p);
      delay(50);
      rotateAngle = calculatedAng;
   }
  } 
}

void updateTemperature(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature, 2).toCharArray(tempAr, 6);
}

void setupWifi() {
  Serial.println();
  Serial.println("Connecting to ");
  Serial.println("Wokwi-GUEST");
  WiFi.begin("Wokwi-GUEST", "");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

unsigned long getTime() {
  timeClient.update();
  return timeClient.getEpochTime();
}

void checkSchedule() {
  if (isScheduledON) {
    unsigned long currentTime = getTime();
    if (currentTime > scheduledOnTime){
      buzzerOn(true);
      isScheduledON=false;
      mqttClient.publish("EE-ADMIN-MAIN-ON-OFF-ESP", "1");
      Serial.println("Scheduled ON");
    }
  }
}