# 各プログラムの説明
single_chan_pkt_fwd-master : Program for Raspberry Pi  
LoRa_test_LMIC_monochannel : Program(sketch) for Arduino + Dragino LoRa Shield  
LoRa_GPS_temp_BLE_test_monochannel : Program(sketch) for Arduino + Dragino LoRa Shield + BLE + temperature sensor + GPS
arduino-lmic-monochannel.zip : library for Arduino sketch  

# オリジナルからの変更点(change from original program)  

* 1. `lorabase.h`を以下のように変更

before change

```c:lorabase.h(before change)
// Default frequency plan for EU 868MHz ISM band
// Bands:
//  g1 :   1%  14dBm
//  g2 : 0.1%  14dBm
//  g3 :  10%  27dBm
//                 freq             band     datarates
enum { EU868_F1 = 868100000,      // g1   SF7-12
       EU868_F2 = 868300000,      // g1   SF7-12 FSK SF7/250
       EU868_F3 = 868500000,      // g1   SF7-12
       EU868_F4 = 868850000,      // g2   SF7-12
       EU868_F5 = 869050000,      // g2   SF7-12
       EU868_F6 = 869525000,      // g3   SF7-12
       EU868_J4 = 864100000,      // g2   SF7-12  used during join
       EU868_J5 = 864300000,      // g2   SF7-12   ditto
       EU868_J6 = 864500000,      // g2   SF7-12   ditto
};
enum { EU868_FREQ_MIN = 863000000,
       EU868_FREQ_MAX = 870000000 };
```

after

```c:lorabase.h(after)
// Default frequency plan for EU 868MHz ISM band
// Bands:
//  g1 :   1%  14dBm
//  g2 : 0.1%  14dBm
//  g3 :  10%  27dBm
//                 freq             band     datarates
enum { EU868_F1 = 923200000,      // g1   SF7-12
       EU868_F2 = 923400000,      // g1   SF7-12 FSK SF7/250
       EU868_F3 = 923600000,      // g1   SF7-12
       EU868_F4 = 923800000,      // g2   SF7-12
       EU868_F5 = 924000000,      // g2   SF7-12
       EU868_F6 = 924200000,      // g3   SF7-12
       EU868_J4 = 924600000,      // g2   SF7-12  used during join
       EU868_J5 = 924800000,      // g2   SF7-12   ditto
       EU868_J6 = 925000000,      // g2   SF7-12   ditto
};
enum { EU868_FREQ_MIN = 920600000,
       EU868_FREQ_MAX = 928000000 };
```

* 2. Arduinoスケッチに以下を追記

```Arduino:LoRa_test_LMIC_monochannel.ino
// Define the single channel and data rate (SF) to use
int channel = 0;
int dr = DR_SF7;
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
```

* 3. Arduinoスケッチの周波数設定を以下のように変更  

before change

```Arduino:LoRa_test_LMIC_monochannel.ino
LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
```

after

```Arduino:LoRa_test_LMIC_monochannel.ino
LMIC_setupChannel(0, 923200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(1, 923400000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
LMIC_setupChannel(2, 923600000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(3, 923800000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(4, 924000000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(5, 924200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(6, 924600000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(7, 924800000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
LMIC_setupChannel(8, 925000000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
```

* 4. Arduinoスケッチの`void setup()`内を以下のように変更  

before change

```Arduino:LoRa_test_LMIC_monochannel.ino(before change)
// Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
LMIC_setDrTxpow(DR_SF7,14);
```

after

```Arduino:LoRa_test_LMIC_monochannel.ino(after)
// Only use one channel and SF
forceTxSingleChannelDr();
```

* 5. Arduinoスケッチのピンマッピングを以下のように変更

```Arduino:LoRa_test_LMIC_monochannel.ino
// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 6, 7},
};
```

* 6. Arduinoスケッチの`NWKSKEY`、`APPSKEY`、`DEVADDR`をTTNで設定したとおりに変更

* 7. Raspberry Pi側、`main.cpp`を開き、以下のように変更

before change

```cpp:main.cpp(before change)
// Set center frequency
uint32_t  freq = 868100000; // in Mhz! (868.1)
```

after

```cpp:main.cpp(after)
// Set center frequency
uint32_t  freq = 923200000; // in Mhz! (923.2)
```

* 8. Raspberry Pi側、`main.cpp`を開き、以下のように変更

before change

```cpp:main.cpp(before change)
// define servers
// TODO: use host names and dns
#define SERVER1 "54.72.145.119"    // The Things Network: croft.thethings.girovito.nl
//#define SERVER2 "192.168.1.10"      // local
#define PORT 1700                   // The port on which to send data
```

after

```cpp:main.cpp(after)
// define servers
// TODO: use host names and dns
#define SERVER1 "13.115.14.75"
#define PORT 1700                   // The port on which to send data
```

## Reference
* [LoraWAN-in-C library, adapted to run under the Arduino environment](https://github.com/matthijskooijman/arduino-lmic "LoraWAN-in-C library, adapted to run under the Arduino environment")
* [LoRaWANでIoTプラットフォーム（The Things Network）にデータを上げる方法](https://qiita.com/openwave-co-jp/items/7edb3661ab5703e38e7c "LoRaWANでIoTプラットフォーム（The Things Network）にデータを上げる方法")
* [Can LMIC 1.6 be set up to use a single channel and SF?](https://www.thethingsnetwork.org/forum/t/can-lmic-1-6-be-set-up-to-use-a-single-channel-and-sf/5207/ "Can LMIC 1.6 be set up to use a single channel and SF?")
* [Single Channel LoRaWAN Gateway](https://github.com/tftelkamp/single_chan_pkt_fwd/ "Single Channel LoRaWAN Gateway")
