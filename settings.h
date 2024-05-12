// Set SSID and PASSWORD for WiFi connection

EspSimpleWifiHandler wifiHandler("WLAN", "PASSWORD");

// Set multicast mode.
//  If set to true, multicast messages will be used. Otherwise packets are send to
//  a fixed IP address defined below. Please note that sending an UDP packet to a 
//  fixed IP address performs better in wifi networks than sending multicast messages.
const bool USE_MULTICAST = true;

// Destination address for the packets.
//  (Only used, when multicast is set to false)
IPAddress destinationAddress(192, 168, 1, 100);

// Port used for energy meter packets
const uint16_t DESTINATION_PORT = 9522;

// Set serial number
//  This serial number will be used to identify this device.
const uint32_t SER_NO = 1901708212;

// local port to listen on
unsigned int localPort = 8888;  
