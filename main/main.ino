//define blynk
#define BLYNK_TEMPLATE_ID "TMPL6qIHiFlyL"
#define BLYNK_TEMPLATE_NAME "Gas detector"
#define BLYNK_AUTH_TOKEN "6jPXx5C9nGfTjljoRc_pFUTT1gfjXGrS"

//gas sensor
#define gas_sensor 34//A0

//fire sensor
#define fire_sensor 15//D0

//led
#define LED_2 2

//temperature sensor
#define data_temperature_sensor 4
#define DHTTYPE DHT11

//include libraries
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include <DHT11.h>
#include <ESP_Mail_Client.h>

BlynkTimer timer;
SMTPSession smtp;
//blynk variable
char auth[] = "6jPXx5C9nGfTjljoRc_pFUTT1gfjXGrS";
char ssid[] = ".";
char pass[] = "tumotden8";
// WidgetLED LED1(V1); //gas
// WidgetLED LED2(V2); //fire

#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

//define cac chan cua esp32
#define PIN_SG90 17 //pin servo
#define SS_PIN 5  // save slave in rfid
#define RST_PIN 0 // reset in rfid
#define LED 13
#define BUZZER 16 //buzzer pin

extern unsigned char index_t;

extern char password[6];
extern char pass_default[6];

extern unsigned char in_num, time_error, isMode;
extern LiquidCrystal_I2C lcd;

//rfid
extern MFRC522 rfid;

//servo
extern Servo sg90;

void writeEpprom(char data[]);
void readEpprom();
void checkPass();
void rfidCheck();
void changePass();
void resetPass();
void openDoor();
void errored3Times();
void errored5Times();
void addRFID();
void delRFID();
void delAllRFID();

//temperature variable
DHT11 dht(data_temperature_sensor);

BLYNK_WRITE(V6) {
  digitalWrite(LED_2, param.asInt() );
}

//gas function
void GASLevel() {
  int value = analogRead(gas_sensor);
  value = map(value, 0, 4095, 0, 100);
   Blynk.virtualWrite(V5, value);

  if (value >= 50) {
    digitalWrite(BUZZER, HIGH);
    delay(1000);
    WidgetLED LED1(V3);
    LED1.on();

  } else {
    digitalWrite(BUZZER,LOW);
    WidgetLED LED1(V3);
    LED1.off();
  }
}

//fire function
void fireSensor(){
  int flame_state = digitalRead(fire_sensor);

  if (flame_state == HIGH) {
      digitalWrite(BUZZER, HIGH);
      WidgetLED LED2(V4);
      LED2.on();
  }

  else {
     digitalWrite(BUZZER,LOW);
     WidgetLED LED2(V4);
     LED2.off();
  }
}

//temperature function
void temperatureSensor(){
  float Celsius_degree = dht.readTemperature();
  int value1 = int(Celsius_degree);
  Blynk.virtualWrite(V1, value1);
  if(Celsius_degree > 28){
    digitalWrite(LED, HIGH);
  }
  else {
    digitalWrite(LED, LOW);
    }
  Serial.printf("Temperature: %f°C\n", Celsius_degree);
}

//lock door function
void lock_door(){
  while (index_t == 1)
    {
        changePass();
    }

    while (index_t == 2)
    {
        resetPass();
    }

    while (index_t == 3)
    {
        openDoor();
        time_error = 0;
    }

    if (index_t == 4)
    {
        errored3Times();
        //time_error = 0;
    }

    while(index_t == 5){
        errored5Times();
        time_error = 0;
    }

    while (index_t == 8)
    {
        addRFID();
    }

    while (index_t == 9)
    {
        delRFID();
    }

    while (index_t == 10)
    {
        delAllRFID();
    }
}

void setup()
{
    Serial.begin(9600);
    EEPROM.begin(512);
    //Wifi.begin()
//init servo
    sg90.setPeriodHertz(50);
    sg90.attach(PIN_SG90, 500, 2400);
//init SPI
    SPI.begin();
//init RFID
    rfid.PCD_Init();
//init temperature sensor
    //dht.begin();
//init lcd to print
    lcd.init();
    lcd.backlight();
    lcd.print("   SYSTEM INIT   ");
    readEpprom();
    delay(2000);
    lcd.clear();
    Serial.print("PASSWORD: ");
    Serial.println(password);
//pin mode
    pinMode(LED, OUTPUT);
    pinMode(fire_sensor, INPUT);
    pinMode(gas_sensor, INPUT);
    //pinMode(D0_light_sensor, INPUT);
    //pinMode(data_temperature_sensor, INPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(LED_2, OUTPUT);

// Blynk init
    Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);

     smtp.debug(1);
     ESP_Mail_Session session;
     session.server.host_name = SMTP_server;
     session.server.port = SMTP_Port;
     session.login.email = sender_email;
     session.login.password = sender_password;
     session.login.user_domain ="";
 }
  static unsigned long lastSensorRead1 = 0;
  static unsigned long lastSensorRead = 0;
  
void loop()
{
    Blynk.run();
    lcd.setCursor(1, 0);
    lcd.print("Enter Password");
    checkPass();
    rfidCheck();
    lock_door();
  const unsigned long SENSOR_INTERVAL1 = 2000; // Read every 2 seconds
  if (millis() - lastSensorRead1 > SENSOR_INTERVAL1) {
    lastSensorRead1 = millis();
    GASLevel();
  }

  const unsigned long SENSOR_INTERVAL = 10000; // Read every 2 seconds
  if (millis() - lastSensorRead > SENSOR_INTERVAL) {
    lastSensorRead = millis();
    temperatureSensor();
  }
    fireSensor();    
}
