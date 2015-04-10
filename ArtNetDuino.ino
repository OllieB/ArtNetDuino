#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>

#define short_get_high_byte(x) ((HIGH_BYTE & x) >> 8)
#define short_get_low_byte(x)  (LOW_BYTE & x)
#define bytes_to_short(h,l) ( ((h << 8) & 0xff00) | (l & 0x00FF) );

// ** BEGIN CONFIG **
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Arduino Ethernet MAC Address

const int numberOfChannels = 4; // How many channels do you need to monitor?
const int inputChannels[numberOfChannels] = {1, 2, 3, 4}; // DMX input channels to map to outputs
const int outputPins[numberOfChannels] = {2, 3, 4, 5}; // Output pins of the arduino used
const int outputStartSpool[numberOfChannels] = {130, 130, 255, 255}; // On start-up send this level for 3 seconds (useful for motors or fans)
const int outputMapMin[numberOfChannels] = {0, 0, 254, 254}; // Add a floor/pre-heat level to the outputs
const int outputMapMax[numberOfChannels] = {255, 255, 1, 1}; // Add a floor/pre-heat level to the outputs

// ** END CONFIG **

//buffers
#define UDP_TX_PACKET_MAX_SIZE 768 // Set max buffer size
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to store incoming data
byte ArtNetDMXData[numberOfChannels]; //buffer to store returned

// ArtNet Protocol Configuration
#define ArtNetPort 6454 // ArtNet port
#define ArtNetDMXHeaderLength 17 // Length of standard ArtNetDMX packet header (hopefully to be removed later, messy.)
#define ArtNetHead "Art-Net" // Used to check if packet has an ArtNet header

// Global Vars
boolean packetIsArtNet; // Set to true if incoming packet is ArtNet
short Opcode; // Determines packet type (may be deprecated by a switch statement)

byte remoteIp[4];        // holds received packet's originating IP
unsigned int remotePort; // holds received packet's originating port

EthernetUDP Udp;

void setup() {
  // Fix PWM pins - maybe?
  bitSet(TCCR1B, WGM12);
  
  Ethernet.begin(mac);
  Udp.begin(ArtNetPort);
  //Serial.begin(9600);
  //Serial.println("Ready!");
  
  // Set pin outputs from config
  for(int i = 0; i < numberOfChannels; i++) {
    pinMode(outputPins[i], OUTPUT);
    
    // Send spool level
    analogWrite(outputPins[i], outputStartSpool[i]);
  }
  delay(3000);
  for(int i = 0; i < numberOfChannels; i++) {
    analogWrite(outputPins[i], map(0, 0, 255, outputMapMin[i], outputMapMax[i]));
  }
}

void loop() {
  int packetSize = Udp.parsePacket();
  
  if(packetSize) {
    IPAddress remoteIP = Udp.remoteIP();    
    remotePort = Udp.remotePort();
    
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    
    //read header
    packetIsArtNet = 1;
    for (int i=0;i<7;i++) {
      //if not corresponding, this is not an artnet packet, so we stop reading
      if(char(packetBuffer[i]) != ArtNetHead[i]) {
        packetIsArtNet = 0;
        break;
      }
    }
    
    if(packetIsArtNet==1) { 
      //sequence of data, to avoid lost packets on routeurs
      //if(
      //seq_artnet=packetBuffer[12];*/
      
      Opcode = bytes_to_short(packetBuffer[9], packetBuffer[8]);
      
      if(Opcode==0x2000) {
        Serial.println("ArtPoll Request");
        // Do ArtPollReply here
      }
      else if(Opcode==0x5000) {
        for(int i = 0; i < numberOfChannels; i++) {
          analogWrite(outputPins[i], map(byte(packetBuffer[inputChannels[i] + ArtNetDMXHeaderLength]), 0, 255, outputMapMin[i], outputMapMax[i]));
        }
      }
    }
  }  
}
