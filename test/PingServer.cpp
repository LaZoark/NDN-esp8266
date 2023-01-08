#if defined(ARDUINO_ARCH_ESP8266)
#include <AddrList.h>
#include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#endif
#include <esp8266ndn.h>
#include "myconfig.h"

const char* WIFI_SSID = __WIFI_SSID;
const char* WIFI_PASS = __WIFI_PASSWORD;

ndnph::StaticRegion<1024> region;

esp8266ndn::EthernetTransport transport0;
ndnph::Face face0(transport0);
const char* PREFIX0 = "/ndn/edu/nycu/ether/310505030/ping";
ndnph::PingServer server0(ndnph::Name::parse(region, PREFIX0), face0);

std::array<uint8_t, esp8266ndn::UdpTransport::DefaultMtu> udpBuffer;
esp8266ndn::UdpTransport transport1(udpBuffer);
ndnph::Face face1(transport1);
const char* PREFIX1 = "/ndn/edu/nycu/udp/ping";
ndnph::PingServer server1(ndnph::Name::parse(region, PREFIX1), face1);

esp8266ndn::UdpTransport transport2(udpBuffer);
ndnph::transport::ForceEndpointId transport2w(transport2);
ndnph::Face face2(transport2w);
const char* PREFIX2 = "/ndn/edu/nthu/udpm/ping";
ndnph::PingServer server2(ndnph::Name::parse(region, PREFIX2), face2);

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
#if defined(ARDUINO_ARCH_ESP8266) && LWIP_IPV6
  { // wait until no new address showing up in 1000ms
    size_t nAddrPrev = 0, nAddr = 0;
    do {
      delay(1000);
      nAddrPrev = nAddr;
      nAddr = 0;
      for (auto a : addrList) {
        (void)a;
        ++nAddr;
      }
    } while (nAddrPrev == nAddr);
  }
#else
  delay(1000);
#endif

  esp8266ndn::EthernetTransport::listNetifs(Serial);
  bool ok = transport0.begin(); // select any STA netif
  if (!ok) {
    Serial.println(F("Ethernet transport initialization failed"));
    ESP.restart();
  }

  ok = transport1.beginListen();
  if (!ok) {
    Serial.println(F("UDP unicast transport initialization failed"));
    ESP.restart();
  }

  ok = transport2.beginMulticast(WiFi.localIP());
  if (!ok) {
    Serial.println(F("UDP multicast transport initialization failed"));
    ESP.restart();
  }

  Serial.println(F("Please register prefixes on your router:"));
  Serial.println(F("nfdc route add /ndn/edu/nycu/ether/310505030 [ETHER-MCAST-FACEID]"));
#if defined(ARDUINO_ARCH_ESP8266)
  for (auto a : addrList) {
    if (a.isV4()) {
      Serial.print(F("nfdc face create udp4://"));
    } else {
      Serial.print(F("nfdc face create udp6://["));
    }
    Serial.print(a.addr());
    if (a.isV4()) {
      Serial.println(F(":6363"));
    } else {
      Serial.println(F("]:6363"));
    }
  }
#elif defined(ARDUINO_ARCH_ESP32)
  Serial.print(F("nfdc face create udp4://"));
  Serial.print(WiFi.localIP());
  Serial.println(F(":6363"));
#endif
  Serial.println(F("nfdc route add /ndn/edu/nycu/udp [UDP-UNICAST-FACEID]"));
  Serial.println(F("nfdc route add /ndn/edu/nthu/udpm [UDP-MCAST-FACEID]"));
  Serial.println();
  Serial.println(F("Then you can ping:"));
  Serial.println(F("ndnping /ndn/edu/nycu/ether/310505030"));
  Serial.println(F("ndnping /ndn/edu/nycu/udp"));
  Serial.println(F("ndnping /ndn/edu/nthu/udpm"));
  Serial.println();
}

void
loop()
{
  face0.loop();
  face1.loop();
  face2.loop();
  delay(1);
}


// ap1 (IP unset)
// st0 192.168.1.227
// 6846 [EthernetTransport] enabled on st0
// 6846 [UdpTransport] listening on 0.0.0.0:6363
// 6846 [UdpTransport] joining group 224.0.23.170:56363 on 192.168.1.227
// Please register prefixes on your router:
// nfdc route add /ndn/edu/nycu/ether/310505030 [ETHER-MCAST-FACEID]
// nfdc face create udp4://192.168.1.227:6363
// nfdc route add /ndn/edu/nycu/udp [UDP-UNICAST-FACEID]
// nfdc route add /ndn/edu/nthu/udpm [UDP-MCAST-FACEID]

// Then you can ping:
// ndnping /ndn/edu/nycu/ether/310505030
// ndnping /ndn/edu/nycu/udp
// ndnping /ndn/edu/nthu/udpm

