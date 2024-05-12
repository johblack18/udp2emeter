/**
* ESP8266 udp to SMA energy meter converter
*
* ﻿This sketch may be used to read udp telegrams from an loxone, convert it to SMA energy-meter 
* telegrams.
* 
* Dependencies:
* EspSimpleWifiHandler
*
* Configuration:
change settings.h
*/

//#include "util/IotWebConf.h"
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include "util/sml_testpacket.h"
#include "smlparser.h"
#include "emeterpacket.h"
#include "EspSimpleWifiHandler.h"






// ----------------------------------------------------------------------------
// Settings
// ----------------------------------------------------------------------------

#include "settings.h"
// ----------------------------------------------------------------------------
// Compile time settings
// ----------------------------------------------------------------------------


// Timeout for reading SML packets
const int SERIAL_TIMEOUT_MS = 100;


// Time to wait for demo-data
const int TEST_PACKET_RECEIVE_TIME_MS = SERIAL_TIMEOUT_MS + SML_TEST_PACKET_LENGTH / 12;

// Default multicast address for energy meter packets
const IPAddress MCAST_ADDRESS = IPAddress(239, 12, 255, 254);

// ----------------------------------------------------------------------------
// Constants for IotWebConf
// ----------------------------------------------------------------------------


const int STRING_LEN = 128;
const int NUMBER_LEN = 32;




// ----------------------------------------------------------------------------
// Global variables
// ----------------------------------------------------------------------------


// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1];  // buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged\r\n";        // a string to send back

uint32_t powerout1 = 0;
uint32_t powerin1 = 0;





// Buffer for serial reading
const int SML_PACKET_SIZE = 1000;
uint8_t smlPacket[SML_PACKET_SIZE];

// Parser for SML packets
SmlParser smlParser;

// Class for generating e-meter packets
EmeterPacket emeterPacket;



// Destination addresses for sending meter packets
const int DEST_ADRESSES_SIZE = 2;
IPAddress destAddresses[DEST_ADRESSES_SIZE];
uint8_t numDestAddresses = 0;

// Destination port
uint16_t port = DESTINATION_PORT;

// UDP instance for sending packets
WiFiUDP Udp;


void onWifiConnected() {
  Serial.println("Wifi is connnected !");
}


/**
 * @brief Wait the given time in ms
 */
void delayMs(unsigned long delayMs) {
  unsigned long start = millis();
  while (millis() - start < delayMs) {
    delay(1);
  }
}

/**
* @brief Update the energy meter packet
*/
void updateEmeterPacket() {
  emeterPacket.begin(millis());

  int pinno = atoi((char *)packetBuffer);
  if (pinno >= 0) {
    powerin1 = pinno;
    powerout1 = 0;
  } else {
    pinno = pinno * -1;
    powerout1 = pinno;
    powerin1 = 0;
  }




  // Store active and reactive power (convert from centi-W to deci-W)
  emeterPacket.addMeasurementValue(EmeterPacket::SMA_POSITIVE_ACTIVE_POWER, powerin1);
  emeterPacket.addMeasurementValue(EmeterPacket::SMA_NEGATIVE_ACTIVE_POWER, powerout1);
  emeterPacket.addMeasurementValue(EmeterPacket::SMA_POSITIVE_REACTIVE_POWER, 0);
  emeterPacket.addMeasurementValue(EmeterPacket::SMA_NEGATIVE_REACTIVE_POWER, 0);

  // Store energy (convert from centi-Wh to Ws)
  emeterPacket.addCounterValue(EmeterPacket::SMA_POSITIVE_ENERGY, smlParser.getEnergyInWh() * 36UL);
  emeterPacket.addCounterValue(EmeterPacket::SMA_NEGATIVE_ENERGY, smlParser.getEnergyOutWh() * 36UL);

  emeterPacket.end();
}



/**
* @brief Read test packet
*/
int readTestPacket() {
  Serial.print("w");

  for (int i = 0; i < 1000 - TEST_PACKET_RECEIVE_TIME_MS; i += 5) {
    delayMs(5);
  }
  Serial.print("r");

  delay(TEST_PACKET_RECEIVE_TIME_MS);
  memcpy(smlPacket, SML_TEST_PACKET, SML_TEST_PACKET_LENGTH);

  return SML_TEST_PACKET_LENGTH;
}






/**
* @brief Setup the sketch
*/
void setup() {

  // Start WiFi
  wifiHandler.enableDebuggingMessages();
  wifiHandler.onConnectionEstablished(onWifiConnected);

  Udp.begin(localPort);
  // Open serial communications and wait for port to open
  Serial.begin(9600);
  while (!Serial)
    ;

  Serial.println();
  Serial.print("ESP8266-D0 to SMA Energy Meter ");
  Serial.print("Chip-ID: ");
  Serial.println(ESP.getChipId());
  Serial.print("Flash-chip-ID: ");
  Serial.println(ESP.getFlashChipId());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());


  emeterPacket.init(SER_NO);
}

/**
* @brief Main loop
*/
void loop() {
  Serial.print("_");

  //if (wifiHandler.isConnected()) {
  //}

  int packetSize = Udp.parsePacket();
  if (packetSize) {
    // read the packet into packetBufffer
    int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    packetBuffer[n] = 0;
    Serial.println("Contents:");
    Serial.println(packetBuffer);
  }
  // Read the next packet
  int smlPacketLength;
  smlPacketLength = readTestPacket();


  // Send the packet if a valid telegram was received
  if (smlPacketLength <= SML_PACKET_SIZE) {
    if (smlParser.parsePacket(smlPacket, smlPacketLength)) {
      if (port > 0) {
        updateEmeterPacket();
        int i = 0;
        do {
          Serial.print("S");
          if (numDestAddresses == 0) {
            Udp.beginPacketMulticast(MCAST_ADDRESS, port, WiFi.localIP(), 1);
          } else {
            Udp.beginPacket(destAddresses[i], port);
          }

          if (port == DESTINATION_PORT) {
            Udp.write(emeterPacket.getData(), emeterPacket.getLength());
          } else {
            Udp.write(smlPacket, smlPacketLength);
          }

          // Send paket
          Udp.endPacket();
          ++i;
        } while (i < numDestAddresses);
      }
    } else {
      Serial.print("E");
    }
  } else {
    // Overflow error
    Serial.print("O");
  }
  Serial.println(".");
}
