#include <Wire.h>
#include "rgb_lcd.h"
#include <SPI.h>
#include <SparkFunLSM6DS3.h>
#include "WiFiEsp.h"

#define BUZZER 8
#define TOUCH 3

#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(10, 11); // RX, TX
#endif

LSM6DS3 myIMU;
rgb_lcd lcd;
WiFiEspClient client;


char ssid[] = "ASUS"; // your network SSID (name)
char pass[] = "aquaticviolin678"; // your network password
int status = WL_IDLE_STATUS; // the Wifi radio's status
char server[] = "3.135.9.11";

const int colorR = 155;
const int colorG = 155;
const int colorB = 155;

unsigned char counter;
unsigned long temp[21];
unsigned long sub;
bool data_effect=true;
int heart_rate=0;//the measurement result of heart rate
float fall = 0.1;
bool updated = false;
bool hasFallen = false;
bool changed = false;


const int max_heartpluse_duty = 2000;//you can change it follow your system's request.
                        //2000 meams 2 seconds. System return error 
                        //if the duty overtrip 2 second.
void printWifiStatus()
{
    // print the SSID of the network you're attached to
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    // print your WiFi shield's IP address
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
    // print the received signal strength
    long rssi = WiFi.RSSI();
    Serial.print("Signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}
void interrupt()
{
    temp[counter]=millis(); 
    switch(counter)
    {
        case 0:
            sub=temp[counter]-temp[20];
            break;
        default:
            sub=temp[counter]-temp[counter-1];
            break;
    }
    if(sub>max_heartpluse_duty)//set 2 seconds as max heart pluse duty
    {
        data_effect=0;//sign bit
        counter=0;
        Serial.println("Heart rate measure error,test will restart!" );
        arrayInit();
    }
    if (counter==20&&data_effect)
    {
        counter=0;
        sum();
    }
    else if(counter!=20&&data_effect){
      counter++;
    }
    
    else 
    {
        counter=0;
        data_effect=1;
    }
    Serial.println(counter);
}

void sum()
{
 if(data_effect)
    {
      heart_rate=1200000/(temp[20]-temp[0]);//60*20*1000/20_total_time 
      
      Serial.println(heart_rate);
      updated = true;
    }
   data_effect=1;//sign bit
}

/*Function: Initialization for the array(temp)*/
void arrayInit()
{
    for(unsigned char i=0;i < 20;i ++)
    {
        temp[i]=0;
    }
    temp[20]=millis();
}

bool touched() {
  int sensorValue = digitalRead(TOUCH);
  return sensorValue == 1;
}

void turnOffAlarm(){
    digitalWrite(BUZZER,LOW);
    hasFallen = false;
    changed = true;
    Serial.println("turned off");
}

void setup()
{
    Serial.begin(115200);
    Serial1.begin(115200);
    pinMode(BUZZER, OUTPUT);
    pinMode(TOUCH, INPUT);
    lcd.begin(16, 2);
    lcd.setRGB(colorR, colorG, colorB);
    myIMU.begin();
    arrayInit();
    attachInterrupt(0, interrupt, RISING);//set interrupt 0,digital port 2
    attachInterrupt(digitalPinToInterrupt(3), turnOffAlarm, RISING);
    lcd.print("Initializing...");
    
    WiFi.init(&Serial1);
    if (WiFi.status() == WL_NO_SHIELD) {
      Serial.println("WiFi shield not present");
      // don't continue
      while (true);
    }

    while ( status != WL_CONNECTED) {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network
        status = WiFi.begin(ssid, pass);
    }

    Serial.println("You're connected to the network");
    printWifiStatus();

    
    
    
}
void loop()
{
  if (heart_rate > 0 && updated){
    lcd.clear();
    lcd.print("Heart rate: ");
    lcd.print(heart_rate);
    updated = false;

    if (!client.connected()){
        Serial.println("Starting connection to server...");
        client.connect(server, 5000);
    }
    
    String get_request = "GET /RecordHeartRate?heartRate="+ String(heart_rate) + " HTTP/1.1\r\nHost: 18.221.147.67\r\nConnection: close\r\n\r\n";
    Serial.println(get_request);
    client.print(get_request);
    delay(500);
      while (client.available()) {
        char c = client.read();
        Serial.write(c);
      }
  }

  float zval = myIMU.readFloatAccelZ();
  float xval = myIMU.readFloatAccelX();
  float yval = myIMU.readFloatAccelY();

  if (zval < fall && xval < fall && yval < fall){
    hasFallen = true; 
    changed = true;
    Serial.println("fallen");
  }

  if (hasFallen && changed){
    lcd.setCursor(0, 1);
    lcd.print("FALL DETECTED");
    digitalWrite(BUZZER, HIGH);
    changed = false;

    if (!client.connected()){
        Serial.println("Starting connection to server...");
        client.connect(server, 5000);
    }
    
    String get_request = "GET /RecordFall HTTP/1.1\r\nHost: 18.221.147.67\r\nConnection: close\r\n\r\n";
    client.print(get_request);
    delay(500);
      while (client.available()) {
        char c = client.read();
        Serial.write(c);
      }
  }
  else if (changed) {
    lcd.setCursor(0,1);
    lcd.print("                    ");
    digitalWrite(BUZZER,LOW);
  }
  
}
