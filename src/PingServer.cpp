/* This is a PingServer, which plays the role of the "data producer" and 
 * "NDN-router" in NDN.
 * 
 *
 * 
*/


#if defined(ARDUINO_ARCH_ESP8266)
#include <AddrList.h>
#include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#endif
#include <esp8266ndn.h>
#include "myconfig.h"

#if defined(USE_AS_SERVER) 

const char* WIFI_SSID = __WIFI_SSID;
const char* WIFI_PASS = __WIFI_PASSWORD;

// NDN router for WAN. 
// Due to geography and connectivity, the connection quality will vary with a different server.
const char* NDN_ROUTER_HOST_WAN = "titan.cs.memphis.edu";
// NDN router for LAN, should be the server's IP
const char* NDN_ROUTER_HOST_LAN = "192.168.1.227";  // server-esp8266

const uint16_t LISTEN_PORT = 6364;
ndnph::StaticRegion<4096> region;

// Ethernet unicast
esp8266ndn::EthernetTransport transport_ether;
ndnph::Face face_ether(transport_ether);
const char* PREFIX0 = "/ndn/edu/nycu/ether/310505030/ping";
ndnph::PingServer server_ether(ndnph::Name::parse(region, PREFIX0), face_ether);

// UDP/IPv4 unicast
std::array<uint8_t, esp8266ndn::UdpTransport::DefaultMtu> udpBuffer;
esp8266ndn::UdpTransport transport_udp(udpBuffer);
ndnph::Face face_udp(transport_udp);
const char* PREFIX1 = "/ndn/edu/nycu/udp/ping";
ndnph::PingServer server_udp(ndnph::Name::parse(region, PREFIX1), face_udp);

// UDP/IPv4 multicast
esp8266ndn::UdpTransport transport2(udpBuffer);
ndnph::transport::ForceEndpointId transport2w(transport2);
ndnph::Face face_udpm(transport2w);
const char* PREFIX2 = "/ndn/edu/nthu/udpm/ping";
ndnph::PingServer server_udpm(ndnph::Name::parse(region, PREFIX2), face_udpm);

/// @brief Quick way to play with a LED.
/// @param led_pin Specify the pin of the LED.
/// @param times The amount of time of the flicking.
/// @param milli_seconds Expected time to finish the whole action. (10e-3)
void blink_led(uint8_t led_pin, int8_t times=3, int16_t milli_seconds=40)
{
  for(int i=0; i<times; i++){
    digitalWrite(led_pin, LOW);
    delay(milli_seconds/2);
    digitalWrite(led_pin, HIGH);
    delay(milli_seconds/2);
  }
}


void setup()
{
  Serial.begin(115200);
  Serial.println();
  esp8266ndn::setLogOutput(Serial);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println(F("WiFi connect failed"));
    ESP.restart();
  }

#if defined(ARDUINO_ARCH_ESP8266)
// #define TRIGGER_LED0 LED_BUILTIN_AUX    // gpio 16
#define TRIGGER_LED0 D3             // yellow
#define TRIGGER_LED1 LED_BUILTIN    // blue (onboard)
#define TRIGGER_LED3 D6             // red
    pinMode(TRIGGER_LED0, OUTPUT);  // GPIO 0
    pinMode(TRIGGER_LED1, OUTPUT);  // GPIO 2
    pinMode(TRIGGER_LED3, OUTPUT);  // GPIO 12
    blink_led(TRIGGER_LED0, 2, 200);
    blink_led(TRIGGER_LED1, 2, 200);
    blink_led(TRIGGER_LED3, 2, 200);
#define BTN_PIN0 D5                 // GPIO 14
#define BTN_PIN1 D1                 // GPIO 5
#define BTN_PIN3 D2                 // GPIO 4
    pinMode(BTN_PIN0, INPUT);       // for yellow
    pinMode(BTN_PIN1, INPUT);       // for blue (onboard)
    pinMode(BTN_PIN3, INPUT);       // for red

#if defined(LWIP_IPV6)
{
  size_t nAddrPrev = 0, nAddr = 0;
  do{
    delay(1000);
    nAddrPrev = nAddr;
    nAddr = 0;
    for (auto a : addrList){
      (void)a;
      ++nAddr;
      }
    } while (nAddrPrev == nAddr);
}
#endif
#else // wait until no new address showing up in 1000ms
  delay(1000);
#endif

  esp8266ndn::EthernetTransport::listNetifs(Serial);
  bool ok = transport_ether.begin(); // select any STA netif
  if (!ok) {
    Serial.println(F("Ethernet transport initialization failed"));
    ESP.restart();
  }

  IPAddress routerIp;
  if (!WiFi.hostByName(NDN_ROUTER_HOST_LAN, routerIp)) {
    Serial.println("[LAN] cannot resolve router IP");
    ESP.restart();
  }
  IPAddress routerIp_WAN;
  if (!WiFi.hostByName(NDN_ROUTER_HOST_WAN, routerIp_WAN)) {
    Serial.println("[WAN] cannot resolve router IP");
    ESP.restart();
  }

  // ok = transport_udp.beginListen(LISTEN_PORT, routerIp);
  ok = transport_udp.beginListen(LISTEN_PORT, routerIp_WAN);
  if (!ok) {
    Serial.println(F("UDP unicast transport initialization failed"));
    ESP.restart();
  }

  // ok = transport2.beginMulticast(WiFi.localIP());
  ok = transport2.beginMulticast(routerIp);
  if (!ok) {
    Serial.println(F("UDP multicast transport initialization failed"));
    ESP.restart();
  }

  Serial.println(F("Please register prefixes on your router:"));
#if defined(ARDUINO_ARCH_ESP8266)
  for (auto a : addrList) {
    if (a.isV4()) {
      Serial.print(F("nfdc face create udp4://"));
    } else {
      Serial.print(F("nfdc face create udp6://["));
    }
    Serial.print(a.addr());
    if (a.isV4()) {
      Serial.printf(":%d", LISTEN_PORT);
    } else {
      Serial.printf("]:%d", LISTEN_PORT);
    }
  }
#elif defined(ARDUINO_ARCH_ESP32)
  Serial.print(F("nfdc face create udp4://"));
  Serial.print(WiFi.localIP());
  Serial.printf(":%d", LISTEN_PORT);
#endif
  Serial.println(F("\nnfdc route add /ndn/edu/nycu/ether/310505030 [ETHER-MCAST-FACEID]"));
  Serial.println(F("nfdc route add /ndn/edu/nycu/udp [UDP-UNICAST-FACEID]"));
  Serial.println(F("nfdc route add /ndn/edu/nthu/udpm [UDP-MCAST-FACEID] \n"));
  Serial.println(F("Then you can ping:"));
  Serial.println(F("ndnping /ndn/edu/nycu/ether/310505030"));
  Serial.println(F("ndnping /ndn/edu/nycu/udp"));
  Serial.println(F("ndnping /ndn/edu/nthu/udpm \n"));
}

uint32_t btn_ticks0, btn_ticks1, btn_ticks3 = 0;

void loop()
{
  if (digitalRead(BTN_PIN0) == true){
    ++btn_ticks0;                       // register the trigger
  }
  if (btn_ticks0 > 0){
    digitalWrite(TRIGGER_LED0, LOW);    // turn on the LED
    face_ether.loop();
    btn_ticks0 = 0;                     // release the trigger
  }else{
    digitalWrite(TRIGGER_LED0, HIGH);   // turn off the LED
  }

  if (digitalRead(BTN_PIN1) == true){
    ++btn_ticks1;                       // register the trigger
  }
  if (btn_ticks1 > 0){
    digitalWrite(TRIGGER_LED1, LOW);    // turn on the LED
    face_udp.loop();
    btn_ticks1 = 0;                     // release the trigger
  }else{
    digitalWrite(TRIGGER_LED1, HIGH);   // turn off the LED
  }

  if (digitalRead(BTN_PIN3) == true){
    ++btn_ticks3;                       // register the trigger
  }
  if (btn_ticks3 > 0){
    digitalWrite(TRIGGER_LED3, LOW);    // turn on the LED
    face_udpm.loop();
    btn_ticks3 = 0;                     // release the trigger
  }else{
    digitalWrite(TRIGGER_LED3, HIGH);   // turn off the LED
  }

}

#endif

  // delayMicroseconds(900000); // too slow
  // delay(1);

//////////////// ESP8266 ////////////////

// ap1 (IP unset)
// st0 192.168.1.227
// 6846 [EthernetTransport] enabled on st0
// 6846 [UdpTransport] listening on 0.0.0.0:6364
// 6846 [UdpTransport] joining group 224.0.23.170:56363 on 192.168.1.227
// Please register prefixes on your router:
// nfdc route add /ndn/edu/nycu/ether/310505030 [ETHER-MCAST-FACEID]
// nfdc face create udp4://192.168.1.227:6364
// nfdc route add /ndn/edu/nycu/udp [UDP-UNICAST-FACEID]
// nfdc route add /ndn/edu/nthu/udpm [UDP-MCAST-FACEID]

// Then you can ping:
// ndnping /ndn/edu/nycu/ether/310505030
// ndnping /ndn/edu/nycu/udp
// ndnping /ndn/edu/nthu/udpm

//////////////// ESP32 ////////////////

// st1 192.168.1.177
// lo0 127.0.0.1
// 2637 [EthernetTransport] enabled on st1
// 2647 [UdpTransport] listening on 141.225.11.173:6363
// 2648 [UdpTransport] joining group 224.0.23.170:56363
// Please register prefixes on your router:
// nfdc route add /ndn/edu/nycu/ether/310505030 [ETHER-MCAST-FACEID]
// nfdc face create udp4://192.168.1.177:6363
// nfdc route add /ndn/edu/nycu/udp [UDP-UNICAST-FACEID]
// nfdc route add /ndn/edu/nthu/udpm [UDP-MCAST-FACEID]

// Then you can ping:
// ndnping /ndn/edu/nycu/ether/310505030
// ndnping /ndn/edu/nycu/udp
// ndnping /ndn/edu/nthu/udpm