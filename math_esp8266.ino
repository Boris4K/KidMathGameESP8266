/**********************************************************************

  == Display Connection ==

  USB TTL     Node      Nokia 5110  Description
            MCU     PCD8544

            GND         GND         Ground
            3V          VCC         3.3V from NodeMCU to display
            D5          CLK         Output from ESP SPI clock
            D7          DIN         Output from ESP SPI MOSI to display data input
            D6          D/C         Output from display data/command to ESP
            D0 GPIO16   CS          Output from ESP to chip select/enable display
            D4 GPIO2    RST         Output from ESP to reset display
            15          LED         3.3V to turn backlight on, GND off

  GND (blk)   GND                     Ground
  5V  (red)   V+                      5V power from PC or charger
  TX  (green) RX                      Serial data from IDE to ESP
  RX  (white) TX                      Serial data to ESP from IDE


  ==I2C Expander pinout
  Nodemcu   Expander
  D1        SCL
  D2        SDA
  Digitran keypad, bit numbers of PCF8574 i/o port

*********************************************************************/

#define BUF_LEN 20

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "WifiCredentials.h"

// Replace with your network credentials

int connect_status = WL_DISCONNECTED;

#include <Wire.h>
#include "Keypad_I2C.h"
#include <Keypad.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#define I2CADDR 0x20

#include <EEPROM.h>

//  Adafruit_PCD8544(int8_t DC, int8_t CS, int8_t RST);
Adafruit_PCD8544 display = Adafruit_PCD8544(12, 16, 2);

int contrast;
#define MAX_CONTRAST 127
#define MIN_CONTRAST 0

const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {0, 1, 2, 3}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 5, 6, 7}; //connect to the column pinouts of the keypad

Keypad_I2C kpd( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR, PCF8574 );


void setup()
{
  EEPROM.begin(4);
  Wire.begin();
  kpd.begin(makeKeymap(keys));
  
  Serial.begin(115200);
  Serial.println("start");

  display.begin();
  display.clearDisplay(); //
  display.display();

  display.setCursor(0, 0);

  //suffle random number generator
  randomSeed(analogRead(0));

  contrast = EEPROM.read(0);
  display.setContrast(contrast);
}

void loop()
{
  if (WL_CONNECTED == connect_status)
  {
    ArduinoOTA.handle();
  }

  char inChar;
  int inVal;
  char buf[BUF_LEN];
  int renderFlag = 0;

  //font data
  int font_height = 10;
  int font_width = 6;
  int font_base = 0;
  int line_margin = 1;
  int strWidth;

  //question data
  static int randNumber1;
  static int randNumber2;
  static int result;
  static int operand;
  static char nextQuestion = 1;
  static String questionStr;
  static String inString;
  static String strOperand;
  static String resString = "V: 0, X: 0";
  static int rightAnswers = 0;
  static int wrongAnswers = 0;

  if (nextQuestion == 1)
  {
    operand = random(1, 3);
    questionStr = "";
    inString = "";
    strOperand = "";
    //resString = "";

    switch (operand)
    {
      case 1: //"+"
        randNumber1 = random(1, 50);
        randNumber2 = random(1, 50);
        result = randNumber1 + randNumber2;
        strOperand = "+";
        break;
      case 2: //"x"
      default:
        randNumber1 = random(2, 11);
        randNumber2 = random(2, 11);
        result = randNumber1 * randNumber2;
        strOperand = "x";
        break;
    }
    //sprintf(questionStr,"%d%c%d",randNumber1,strOperand
    questionStr = String(randNumber1) + strOperand + String(randNumber2) + "=";
    nextQuestion = 0;
    renderFlag = 1;

    Serial.print(questionStr);
  }

  // Read serial input:
  inChar = kpd.getKey();
  if (isDigit(inChar))
  {
    Serial.print(inChar);
    if ((inString == "" && inChar != '0') || (inString != ""))
    {
      inString += inChar;
    }
  }
  else if (inChar == 'A')
  {
    Serial.print('A');
    if (inString.length() > 0)
    {
      inString.remove(inString.length() - 1);
    }
  }
  else if (inChar == '#' && inString != "")
  {
    inVal = inString.toInt();

    if (inVal == result)
    {
      resString = "RIGHT";
      rightAnswers++;
    }
    else
    {
      resString = "WRONG";
      wrongAnswers++;
    }
    nextQuestion = 1;
    Serial.println(resString);
    resString = "V: " + String(rightAnswers) + ", X: " + String(wrongAnswers);
    Serial.println("Right: " + String(rightAnswers) + ", Wrong: " + String(wrongAnswers));
  }
  else if (inChar == '*')
  {
    if (WL_CONNECTED != connect_status)
    {
      //display.clearDisplay();
      display.setCursor(0, 30);
      display.print("Connecting");
      display.display();
      Serial.println("Connecting");
      if (WL_CONNECTED == StartOTA())
      {
        display.setCursor(0, 40);
        display.print(WiFi.localIP().toString());
        display.display();
        delay(1000);
      }
    }
    else if (WL_CONNECTED == connect_status)
    {
      display.setCursor(0, 40);
      display.print(WiFi.localIP().toString());
      display.display();
      delay(1000);
    }
  }
  else if (inChar == 'B')
  {
    display.setCursor(0, 30);
    display.print(contrast);
    display.display();
    contrast = contrast <= MAX_CONTRAST ? contrast + 1 : contrast;
    display.setContrast(contrast);
    EEPROM.write(0, contrast);
    EEPROM.commit();
    delay(10);
  }
  else if (inChar == 'C')
  {
    display.setCursor(0, 30);
    display.print(contrast);
    display.display();
    contrast = contrast >= MIN_CONTRAST ? contrast - 1 : contrast;
    display.setContrast(contrast);
    EEPROM.write(0, contrast);
    EEPROM.commit();
    delay(10);
  }

  //print data to display
  display.clearDisplay();
  questionStr.toCharArray(buf, BUF_LEN);
  display.setCursor(0, font_base);
  display.print(buf);
  strWidth = questionStr.length() * font_width;

  inString.toCharArray(buf, BUF_LEN);
  display.setCursor(strWidth + 2, font_base);
  display.print(buf);

  resString.toCharArray(buf, BUF_LEN);
  display.setCursor(0, font_base + font_height);
  display.print(buf);

  display.display();
}

int StartOTA()
{
  String ip;
  int retry_num = 0;
  WiFi.mode(WIFI_STA);

  while (retry_num < 100)
  {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(100);
    ip = WiFi.localIP().toString();
    if (ip == "0.0.0.0")
    {
      Serial.println("Not Connected");
      connect_status = WL_DISCONNECTED;
    }
    else
    {
      connect_status = WL_CONNECTED;
      Serial.println(ip);
      break;
    }
    retry_num++;
  }
  if (connect_status == WL_DISCONNECTED)
  {
    return connect_status;
  }

  //connect_status = WiFi.waitForConnectResult();
  Serial.println(connect_status);

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  return connect_status;
}
