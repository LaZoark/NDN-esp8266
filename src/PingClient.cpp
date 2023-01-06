#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif
#include <esp8266ndn.h>
#include "myconfig.h"

const char* WIFI_SSID = __WIFI_SSID;
const char* WIFI_PASS = __WIFI_PASSWORD;

// NDN ping-server, please refer the following links:
// https://gerrit.named-data.net/plugins/gitiles/ndn-tools/+/00aa181b4951fd5d9224e934ae08c95b1e167d37/tools/ping/README.md
// https://www.lists.cs.ucla.edu/pipermail/nfd-dev/2014-August/000361.html
// 
const char* PREFIX0 = "/ndn/edu/arizona/ping";   
const char* PREFIX1 = "/ndn/edu/memphis/ping";   
const char* PREFIX2 = "/ndn/edu/ucla/ping";     // UCLA
// const char* PREFIX0 = "/ndn/edu/nycu/ping";   // 陽交
// const char* PREFIX2 = "/ndn/edu/nthu/ping";   // 清華

esp8266ndn::UdpTransport transport;
ndnph::Face face(transport);

ndnph::StaticRegion<1024> region;
ndnph::PingClient client0(ndnph::Name::parse(region, PREFIX0), face);
ndnph::PingClient client1(ndnph::Name::parse(region, PREFIX1), face);
ndnph::PingClient client2(ndnph::Name::parse(region, PREFIX2), face);

void
setup()
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
  delay(1000);

#if defined(ARDUINO_ARCH_ESP8266)
  BearSSL::WiFiClientSecure fchSocketClient;
  fchSocketClient.setInsecure();
#elif defined(ARDUINO_ARCH_ESP32)
  WiFiClientSecure fchSocketClient;
  fchSocketClient.setInsecure();
#endif
  auto fchResponse = esp8266ndn::fchQuery(fchSocketClient);
  if (!fchResponse.ok) {
    ESP.restart();
  }
  transport.beginTunnel(fchResponse.ip);
}

void
printCounters(const char* prefix, const ndnph::PingClient& client)
{
  auto cnt = client.readCounters();
  Serial.printf("%8dI %8dD %3.3f%% %s\n", static_cast<int>(cnt.nTxInterests),
                static_cast<int>(cnt.nRxData), 100.0 * cnt.nRxData / cnt.nTxInterests, prefix);
}

void loop()
{
  face.loop();
  delay(1);

  static uint16_t i = 0;
  if (++i % 1024 == 0) {
    printCounters(PREFIX0, client0);
    printCounters(PREFIX1, client1);
    printCounters(PREFIX2, client2);
    Serial.println(F("-----------------------------"));
  }
}