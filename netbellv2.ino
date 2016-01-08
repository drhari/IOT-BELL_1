#include <Wire.h>
#include <EEPROM.h>
#include <UIPEthernet.h>
#include <String.h>
#include <Time.h>
#include <DS1307RTC.h>

byte mac[] = { 0x98, 0x4F, 0xEE, 0x00, 0x50, 0xCC };
IPAddress ip(172,17,5,113);
IPAddress dnsServer(172,16,0,5);
IPAddress gateway(172,17,5,1);
IPAddress subnet(255,255,255,0);
IPAddress google(173,194,33,104);

EthernetServer server(80);
EthernetClient client;
String readString;
tmElements_t tm;
void setup() {
  Serial.begin(9600);
  pinMode(7,OUTPUT);
  digitalWrite(7,LOW);
  Ethernet.begin(mac, ip, dnsServer, gateway, subnet);
  server.begin();
  if (client.connect(google, 80)) {
    Serial.println(F("DNS success"));
  }
  tm.Hour = 13;
  tm.Minute = 15;
  tm.Second = 0;
  tm.Day = 8;
  tm.Month = 1;
  tm.Year = 2016;
  RTC.write(tm);
  Serial.println(F("Intialized"));
}

//Looping function where we serve the page.
void loop() {
  client = server.available();
  readData();
  readString = "";
  checkTime();
}

// To read request data and store it in the variable readstring
void readData() {
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (readString.length() < 100) {
          readString += c;
        }
        if (c == '\n') {
          processData();
        }
      }
    }
  }
}

// function to parse request data and process it
void processData() {
  Serial.println(readString);
  sendHeader();
  if(readString.indexOf(F("/login?password=test")) >= 0) {
    client.println(F("<form action=\"/edit\" method=\"get\"><input type=\"text\" name=\"time\" placeholder=\"time1\"><input type=\"hidden\" name=\"end\"><input type=\"submit\" value=\"Submit\"></form></body></html>"));
  } else if (readString.indexOf(F("/edit")) >= 0) {
    int ind1, ind2;
    if(readString.indexOf("=*") < 0) {
      ind1 = readString.indexOf("=");
      ind2 = readString.indexOf('&');
      char times[50];
      String parsedData = readString.substring(ind1+1, ind2);
      parsedData.replace("%2C",",");
      parsedData.toCharArray(times, 50);
      char *p = times;
      char *str;
      int count = 1;
      int timeInt;
      while((str=strtok_r(p,",",&p)) != NULL) {
        String timestr = str;
        timeInt = timestr.substring(0,2).toInt();
        EEPROM.write(count, timeInt);
        count ++;
        timeInt = timestr.substring(3,5).toInt();
        EEPROM.write(count, timeInt);
        count ++;
      }
      EEPROM.write(0,count);
      for (int i=1;i<EEPROM.read(0);i=i+2) {
        client.print(EEPROM.read(i));
        client.print(":");
        client.print(EEPROM.read(i+1));
        client.print("</br>");
      }
      client.print("</body></html>");
    } else {
      ind1 = readString.indexOf("=");
      ind2 = readString.indexOf('&');
      String timeData = readString.substring(ind1+1, ind2);
      ind2 = timeData.indexOf(".");
      tm.Hour = timeData.substring(1,ind2).toInt();
      ind1 = timeData.indexOf(".",ind2+1);
      tm.Minute = timeData.substring(ind2+1,ind1).toInt();
      tm.Second = timeData.substring(ind1+1,timeData.length()).toInt();
      RTC.write(tm);
    }
  } else {
    client.println(F("<form action=\"/login\" method=\"get\"><input type=\"password\" name=\"password\" placeholder=\"Password\"><input type=\"submit\" value=\"Submit\"></form></body></html>"));
  }
  delay(1);
  client.stop();
  readString = "";
}

void sendHeader() {
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println();
  client.print(F("<!DOCTYPE html><html><head><title>NETBELL</title></head><body>"));
}

void checkTime() {
  int prevSecond = tm.Second;
  RTC.read(tm);
  for (int i=1;i<EEPROM.read(0);i=i+2) {
    int rhours = EEPROM.read(i);
    int rminutes = EEPROM.read(i+1);
    int hours = tm.Hour;
    int minutes = tm.Minute;
    int seconds = tm.Second;
    if (prevSecond != tm.Second){
      Serial.print(rhours);
      Serial.print(":");
      Serial.print(rminutes);
      Serial.print(":");
      Serial.print(hours);
      Serial.print(":");
      Serial.print(minutes);
      Serial.print(":");
      Serial.print(seconds);
      Serial.println("");
    }
    if (rhours == hours && rminutes==minutes && seconds < 9) {
      ringBell();
    }
  }  
}

void ringBell() {
  digitalWrite(7,HIGH);
  Serial.println("Bell Ringing");
  delay(10000);
  digitalWrite(7,LOW);
}

