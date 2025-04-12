#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <EEPROM.h> 

#define SS 5       
#define RST 14     
#define DIO0 26   


// Set your network credentials
const char* ssid = "realme 12 5G";
const char* password = "king..77";


const unsigned long channelID = 2652374;
const char* writeAPIKey = "IRTPWG5Y9XDPVLHV";

WiFiClient client;


float readCumulativePower() {
  float cumulativePower;
  EEPROM.get(0, cumulativePower);
  return cumulativePower;
}

void writeCumulativePower(float cumulativePower) {
  EEPROM.put(0, cumulativePower);
  EEPROM.commit(); 
}

void setup() {
  Serial.begin(9600); 
  while (!Serial);


  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) { 
    Serial.println("LoRa initialization failed!");
    while (1);
  }
  Serial.println("LoRa initialization successful!");


  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  
  ThingSpeak.begin(client);

  EEPROM.begin(512);
}

void loop() {
  // Try to parse a packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.print("Received packet with size: ");
    Serial.println(packetSize);

    // Read packet
    String receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    Serial.print("Received Data: ");
    Serial.println(receivedData);

    

    ThingSpeak.setField(1, receivedData); 
    ThingSpeak.writeFields(channelID, writeAPIKey);

    Serial.print("Data sent to ThingSpeak: ");
    Serial.println(receivedData);
  }

  delay(5000); 
}