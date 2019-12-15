/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses ABP (Activation-by-personalisation), where a DevAddr and
 * Session keys are preconfigured (unlike OTAA, where a DevEUI and
 * application key is configured, while the DevAddr and session keys are
 * assigned/generated in the over-the-air-activation procedure).
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!
 *
 * To use this sketch, first register your application and device with
 * the things network, to set or generate a DevAddr, NwkSKey and
 * AppSKey. Each device should have their own unique values for these
 * fields.
 *
 * Do not forget to define the radio type correctly in config.h.
 *
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#include <SoftwareSerial.h>

// for temperature and humidity
#include <DHT.h>
#include <DHT_U.h>

// for fetching data from GPS module.
SoftwareSerial gpsSerial(3, 4); // RX, TX

// for fetching data from Bluetooth module.
SoftwareSerial bleSerial(8, 7);

// for fetching data from temperature/humidity module.
#define DHTTYPE DHT11
DHT dht(5, DHTTYPE);

// LoRaWAN NwkSKey, network session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const PROGMEM u1_t NWKSKEY[16] = { 0x8B, 0xF9, 0xDA, 0x6F, 0xC3, 0x51, 0x85, 0x32, 0xA9, 0xCB, 0xEA, 0xBD, 0x66, 0x2C, 0xFB, 0x28 };

// LoRaWAN AppSKey, application session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const u1_t PROGMEM APPSKEY[16] = { 0x01, 0xA7, 0x28, 0x95, 0xB1, 0xBC, 0x54, 0x5C, 0xAA, 0xB3, 0xEB, 0x16, 0x1D, 0x95, 0xF3, 0x51 };

// LoRaWAN end-device address (DevAddr)
// See http://thethingsnetwork.org/wiki/AddressSpace
static const u4_t DEVADDR = 0x26041811 ; // <-- Change this address for every node!

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

// send data
uint8_t senddata[25] = "\0";
// for Parsing GPS data
// Reference : ttp://www.technoblogy.com/show?SJ0
char fmt[]="$GPRMC,xxxxxx.xxx,A,eeee.eeee,l,eeeee.eeee,o,xxxx,xxxx,xxxxxx,,,xxxx";
int state = 0;
long Long = 0, Lat = 0, ltmp = 0;
char LongS = 'X', LatS = 'X';
// data buffer for DHT11
int temp, humid;
static osjob_t sendjob;
// timer
int timer = 0;
// BLE signal
String rcvBLES;
char rcvBLE[32] = "\0";

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 10;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 9,
  .dio = {2, 6, LMIC_UNUSED_PIN},
};

// Define the single channel and data rate (SF) to use
int channel = 0;
int dr = DR_SF9;
// Disables all channels, except for the one defined above, and sets the
// data rate (SF). This only affects uplinks; for downlinks the default
// channels or the configuration from the OTAA Join Accept are used.
//
// Not LoRaWAN compliant; FOR TESTING ONLY!
//
void forceTxSingleChannelDr() {
    for(int i=0; i<9; i++) { // For EU; for US use i<71
        if(i != channel) {
            LMIC_disableChannel(i);
        }
    }
    // Set data rate (SF) and transmit power for uplink
    LMIC_setDrTxpow(dr, 14);
}

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch(ev) {
    /*
    case EV_SCAN_TIMEOUT:
        Serial.println(F("EV_SCAN_TIMEOUT"));
        break;
    case EV_BEACON_FOUND:
        Serial.println(F("EV_BEACON_FOUND"));
        break;
    case EV_BEACON_MISSED:
        Serial.println(F("EV_BEACON_MISSED"));
        break;
    case EV_BEACON_TRACKED:
        Serial.println(F("EV_BEACON_TRACKED"));
        break;
    case EV_JOINING:
        Serial.println(F("EV_JOINING"));
        break;
    case EV_JOINED:
        Serial.println(F("EV_JOINED"));
        break;
    case EV_RFU1:
        Serial.println(F("EV_RFU1"));
        break;
    case EV_JOIN_FAILED:
        Serial.println(F("EV_JOIN_FAILED"));
        break;
    case EV_REJOIN_FAILED:
        Serial.println(F("EV_REJOIN_FAILED"));
        break;
    */
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.println(F("Received "));
        Serial.println(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
      }
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
        break;
    /*
    case EV_LOST_TSYNC:
        Serial.println(F("EV_LOST_TSYNC"));
        break;
    case EV_RESET:
        Serial.println(F("EV_RESET"));
        break;
    case EV_RXCOMPLETE:
        // data received in ping slot
        Serial.println(F("EV_RXCOMPLETE"));
        break;
    case EV_LINK_DEAD:
        Serial.println(F("EV_LINK_DEAD"));
        break;
    case EV_LINK_ALIVE:
        Serial.println(F("EV_LINK_ALIVE"));
        break;
    */
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

void do_send(osjob_t* j){
  sprintf(senddata, "%08ld%c%09ld%c%02d%02d", Lat, LatS, Long, LongS, temp, humid);
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
      Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
      // Prepare upstream data transmission at the next possible time.
      LMIC_setTxData2(1, senddata, sizeof(senddata)-1, 0);
      Serial.println(F("Packet queued"));
      Serial.print(F("Send data : ")); Serial.println((const char *)senddata);
      // Reset senddata
      senddata[0] = "\0";
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void sendBle(char *p,int l) {
  for(int i = 0; i < l; i++){
    bleSerial.write(p[i]);
    Serial.print(p[i]);
    delay(20);
  }
  Serial.println(F(" ...OK"));
  bleSerial.write('\r\n');
}

void getDHT11() {
  humid = dht.readHumidity();
  temp = dht.readTemperature();
}

void ParseGPS (char c) {
  if (c == '$')
  {
    state = 0;
    ltmp = 0;
  }
  char mode = fmt[state++];
  // If received character matches format string, or format is 'x' - return
  if ((mode == c) || (mode == 'x')) return;
  // d=decimal digit(number - '0' converts number to ascii code)
  char d = c - '0';
  // ltmp = ltmp*10 + d;
  if (mode == 'e')
  {
    ltmp = (ltmp<<3) + (ltmp<<1) + d;
  }
  // l=Latitude
  else if (mode == 'l')
  {
    LatS = c;
    Lat = ltmp;
    ltmp = 0;
  }
  // o=Longitude
  else if (mode == 'o')
  {
    LongS = c;
    Long = ltmp;
    ltmp = 0;
  }
  else state = 0;
}

void setup() {
  Serial.println(F("Starting"));
  Serial.begin(57600);
  
  for(int i = 0; i < 50; i++){
    delay(10);
    if(Serial.available() <= 54) continue;
    else break;
  }
  
  // begin communication with gps module
  gpsSerial.begin(9600);

  // begin communication with temperature/humidity sensor 
  dht.begin();
  
  // begin communication and setup BLE module 
  bleSerial.begin(115200);
  delay(500);
  sendBle("SB,3", 4);
  delay(100);
  sendBle("R,1", 3);
  delay(2000);
  sendBle("+", 1);
  delay(100);
  bleSerial.begin(38400);
  delay(100);
  sendBle("SF,1", 4);
  delay(100);

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // Set static session parameters. Instead of dynamically establishing a session
  // by joining the network, precomputed session parameters are be provided.
  #ifdef PROGMEM
  // On AVR, these values are stored in flash and only copied to RAM
  // once. Copy them to a temporary buffer here, LMIC_setSession will
  // copy them into a buffer of its own again.
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
  #else
  // If not running an AVR with PROGMEM, just use the arrays directly
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
  #endif

  // Set up the channels used by the Things Network
  LMIC_setupChannel(0, 923200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(1, 923400000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  //LMIC_setupChannel(2, 923600000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(3, 923800000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(4, 924000000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(5, 924200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(6, 924600000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(7, 924800000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  //LMIC_setupChannel(8, 925000000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  // LMIC_setDrTxpow(DR_SF7,14);
  // Only use one channel and SF
  forceTxSingleChannelDr();

  // Start job
  do_send(&sendjob);
}

void loop() {
  timer++;
  os_runloop_once();
  
  if (timer > 20000){
    getDHT11();
    timer = 0;
  }
  
  gpsSerial.listen();
  while (gpsSerial.available()) {
    ParseGPS(gpsSerial.read());
  }

  while(Serial.available()){
    rcvBLES = Serial.readString();
    
    if(rcvBLES.length() > 0){
      rcvBLES.toCharArray(rcvBLE, 32);
      
      rcvBLE[strlen(rcvBLE) - 1] = '\0';
      Serial.print(F("Sending : "));
      Serial.print(rcvBLE);
      Serial.print(F(" Length : "));
      Serial.println(strlen(rcvBLE));
      
      bleSerial.listen();
      sendBle(rcvBLE, strlen(rcvBLE));
    }
  }
  
  while(bleSerial.available()){
    Serial.print((char)bleSerial.read());
  }
}