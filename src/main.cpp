#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Wire.h>
#include "secrets.h"
#include "connectionWifi.h"
#include <LiquidCrystal_I2C.h>
#include <iostream>
#include <string>
#include <Regexp.h>
#include <vector>
#include <cctype>
#define conectLed D0

// const int RS = D2, EN = D3, d4 = D5, d5 = D6, d6 = D7, d7 = D8;
// LiquidCrystal lcd(RS, EN, d4, d5, d6, d7);
LiquidCrystal_I2C lcd(0x27, 16, 2);

//WiFiClientSecure net;
WiFiClient wifiClient;

// BearSSL::X509List cert(cacert);
// BearSSL::X509List client_crt(client_cert);
// BearSSL::PrivateKey key(privkey);
//String thing_ref = "EmergencyCall";
int Posto_ref = 1;

PubSubClient client(wifiClient);

std::string globalPayload;
int count = 1;
byte NumFigure[] = {
    B01110,
    B10001,
    B10001,
    B01110,
    B00000,
    B11111,
    B00000,
    B00000};

unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
const long interval = 5000;
int TIME_ZONE = 4;
time_t now;
time_t nowish = 1510592825;

#define AWS_IOT_SUBSCRIBE_TOPIC "EmergencyCall_PostoEnfermagem/1"

//#define AWS_IOT_PUBLISH_TOPIC "EmergencyCall_RecoverState"

void NTPConnect()
{
    Serial.print("Setting time using SNTP");
    configTime(TIME_ZONE * 3600, -3, "pool.ntp.org", "time.nist.gov");

    now = time(nullptr);
    while (now < nowish)
    {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println(now);
    Serial.println("done!");
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("Current time: ");
    Serial.print(asctime(&timeinfo));
}

void messageReceived(char *topic, byte *payload, unsigned int length)
{
    std::string ids;
    for (int i = 0; i < (int)length; i++)
    {
        char c = (char)payload[i];

        if (c == '"')
            ids += "'";
        else
            ids += c;
    }
    globalPayload = ids;
    ids = "";
}
// void recoverState()
// {
//     StaticJsonDocument<512> doc;
//     doc["postoRef"] = Posto_ref;
//     doc["thingRef"] = thing_ref;
//     char jsonBuffer[700];
//     serializeJson(doc, jsonBuffer);
//     client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
//     Serial.println(jsonBuffer);
// }

void connectWifi()
{
    delay(3000);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID));

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }

    Serial.println("WIFI connected!!!");
}

void connectBroker()
{
    int TryCount = 0;
    Serial.println("MQTT connecting ");
    while (!client.connected())
    {
        TryCount++;
        if (client.connect(DEVICE_NAME))
        {   
            Serial.println("connected!");
            if (!client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC))
                Serial.println(client.state());
            //recoverState();
            //digitalWrite(conectLed, LOW);
           // delay(500);
        }
        else
        {
            //delay(500);
           // digitalWrite(conectLed, LOW);
            Serial.println("client foi desconectado!! ");
            Serial.print("failed, reason -> ");
            client.disconnect();
            WiFi.disconnect();
            Serial.println(client.state());
            delay(2000);
            if (TryCount == 10)
                ESP.restart();

            Serial.println(String(TryCount));
            break;
        }
    }
    client.setBufferSize(1024);
}

void checkConnection()
{
    if (!client.connected())
    {
        connectWifi();
        connectBroker();
    }
    else
    {
        client.loop();
        if (millis() - lastMillis > 5000)
        {
            lastMillis = millis();
        }
    }
    now = time(nullptr);
}

void get_match_leitos_callback(const char *match,
                               const unsigned int length,
                               const MatchState &ms)
{

    char *cap = new char[length];
    int leitoId;

    for (byte i = 0; i < ms.level; i++)
    {
        Serial.print(String(count));
        checkConnection();
        ms.GetCapture(cap, i);
        std::string toBeConverted(cap);
        leitoId = std::stoi(toBeConverted);
        if (count > 9)
        {
            lcd.setCursor(2, 0);
            lcd.write(byte(4));
        }
        lcd.setCursor(0, 0);
        lcd.print(String(count));
        lcd.write(byte(4));
        lcd.print(" ");
        lcd.setCursor(4, 0);
        lcd.print("Leito");
        lcd.setCursor(4, 1);
        lcd.print("CHAMANDO");
        lcd.setCursor(10, 0);
        lcd.print(String(leitoId));
        lcd.print(" ");
        delay(1000);
        count += 1;
    }
    delete[] cap;
}

void get_match_leitos(std::string buffer)
{
    char buf[900];
    strcpy(buf, buffer.c_str());

    MatchState ms(buf);
    ms.GlobalMatch("([0-9]+)", get_match_leitos_callback);
}

void lcdMessage()
{
    std::string buffer;
    buffer = globalPayload;

    get_match_leitos(buffer);
    count = 1;
}

void setup()
{
    Serial.begin(115200);
    client.setServer(MQTT_HOST, 1883);
    client.setCallback(messageReceived);
   // net.setTrustAnchors(&cert);
    //net.setClientRSACert(&client_crt, &key);
    connectWifi();
    NTPConnect();
    pinMode(conectLed, OUTPUT);
    Wire.begin();
    lcd.backlight();
    lcd.begin(16, 2);
    lcd.createChar(4, NumFigure);
    lcd.clear();
}

void loop()
{
    checkConnection();
    lcdMessage();
    lcd.clear();
}
