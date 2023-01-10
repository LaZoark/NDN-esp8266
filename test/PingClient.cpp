#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif
#include <esp8266ndn.h>
#include "myconfig.h"

const char *WIFI_SSID = __WIFI_SSID;
const char *WIFI_PASS = __WIFI_PASSWORD;

// NDN ping-server, please refer the following links:
// https://gerrit.named-data.net/plugins/gitiles/ndn-tools/+/00aa181b4951fd5d9224e934ae08c95b1e167d37/tools/ping/README.md
// https://www.lists.cs.ucla.edu/pipermail/nfd-dev/2014-August/000361.html
//
// const char* PREFIX0 = "/ndn/edu/arizona/ping";
// const char* PREFIX1 = "/ndn/edu/memphis/ping";
const char *PREFIX0 = "/ndn/edu/nycu/ether/310505030/ping"; // 陽交 + studientID
const char *PREFIX1 = "/ndn/edu/nycu/udp/ping";             // 陽交
const char *PREFIX2 = "/ndn/edu/ucla/ping";                 // UCLA
const char *PREFIX3 = "/ndn/edu/nthu/udpm/ping";            // 清華

// const char* NDN_ROUTER_HOST = "141.225.11.173";
const char* NDN_ROUTER_HOST = "192.168.1.227";
// const char* NDN_ROUTER_HOST = "192.168.1.177";

esp8266ndn::UdpTransport transport;
ndnph::Face face(transport);

ndnph::StaticRegion<1024> region;
ndnph::PingClient client0(ndnph::Name::parse(region, PREFIX0), face);
ndnph::PingClient client1(ndnph::Name::parse(region, PREFIX1), face);
ndnph::PingClient client2(ndnph::Name::parse(region, PREFIX2), face);
ndnph::PingClient client3(ndnph::Name::parse(region, PREFIX3), face);

void blink_led(uint8_t led, int8_t times=3, int16_t miniseconds=40)
{
  for(int i=0; i<times; i++){
    digitalWrite(led, LOW);
    delay(miniseconds/2);
    digitalWrite(led, HIGH);
    delay(miniseconds/2);
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
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println(F("WiFi connect failed"));
    ESP.restart();
  }
  // delay(1000);

#if defined(ARDUINO_ARCH_ESP8266)
  pinMode(LED_BUILTIN_AUX, OUTPUT);     // gpio 16
  pinMode(LED_BUILTIN, OUTPUT);         // gpio 2
  blink_led(LED_BUILTIN, 2, 100);
  blink_led(LED_BUILTIN_AUX, 1, 150);
  blink_led(LED_BUILTIN, 1, 100);
  blink_led(LED_BUILTIN_AUX, 2, 150);
  BearSSL::WiFiClientSecure fchSocketClient;
  fchSocketClient.setInsecure();
#elif defined(ARDUINO_ARCH_ESP32)
  WiFiClientSecure fchSocketClient;
  fchSocketClient.setInsecure();
#endif
  // auto fchResponse = esp8266ndn::fchQuery(fchSocketClient);
  // auto fchResponse = esp8266ndn::fchQuery(fchSocketClient, "http://hobo.cs.arizona.edu/"); // NG
  // auto fchResponse = esp8266ndn::fchQuery(fchSocketClient, "https://suns.cs.ucla.edu/");
  // auto fchResponse = esp8266ndn::fchQuery(fchSocketClient, "http://ndn-fch.named-data.net/"); // NG
  // auto fchResponse = esp8266ndn::fchQuery(fchSocketClient, "https://fch.ndn.today/"); // 會幫忙找到最佳的 NDN-Router
  // if (!fchResponse.ok)
  // {
  //   ESP.restart();
  // }
  // transport.beginTunnel(fchResponse.ip);
  
  // const char* NDN_ROUTER_HOST = "titan.cs.memphis.edu";
  uint16_t _remotePort = 6363;
  uint16_t _localPort = 6363;
  IPAddress routerIp;
  if (!WiFi.hostByName(NDN_ROUTER_HOST, routerIp)) {
    Serial.println("cannot resolve router IP");
    ESP.restart();
  }
  transport.beginTunnel(routerIp, _remotePort, _localPort);
}

uint32_t printCounters(const char *prefix, const ndnph::PingClient &client)
{
  auto cnt = client.readCounters();
  // Serial.printf("%6dI %6dD %3.2f%% %s\n", static_cast<int>(cnt.nTxInterests),
  Serial.printf("[I/D]: %5d/%-5d %3.2f%% %s\n", static_cast<int>(cnt.nTxInterests),
      static_cast<int>(cnt.nRxData), 100.0 * cnt.nRxData / cnt.nTxInterests, prefix);

  return cnt.nRxData;
}

uint32_t nRxData_0 = 0;
uint32_t nRxData_1 = 0;
uint32_t nRxData_2 = 0;
uint32_t nRxData_3 = 0;
uint32_t temp = 0;

void loop()
{
  face.loop();
  delayMicroseconds(1000);

  static uint16_t i = 0;
  if (++i % 1024 == 0)
  {
    nRxData_0 = printCounters(PREFIX0, client0);
    nRxData_1 = printCounters(PREFIX1, client1);
    nRxData_2 = printCounters(PREFIX2, client2);
    nRxData_3 = printCounters(PREFIX3, client3);
    if (++temp <= nRxData_1){
      temp = nRxData_1;
      nRxData_1 = 1;
    }else{
      nRxData_1 = 0;
      temp--;
    }
    Serial.print(F("----------------"));
    Serial.print(F("["));    Serial.print(nRxData_0);
    Serial.print(F(", "));  Serial.print(nRxData_1);
    Serial.print(F(", "));  Serial.print(nRxData_2);
    Serial.print(F(", "));  Serial.print(nRxData_3);
    Serial.println(F("]----------------"));
    // Serial.print(F(", temp="));  Serial.print(temp);


#if defined(ARDUINO_ARCH_ESP8266)
    if (nRxData_1)
    {
      blink_led(LED_BUILTIN, 2, 50);
      blink_led(LED_BUILTIN_AUX, 1, 20);
    }
#endif
  }
}


// 9264 [AutoConfig] https://fch.ndn.today/ body: ndn.ist.osaka-u.ac.jp
// 9288 [AutoConfig] DNS resolved to: 133.1.17.51
// 9289 [UdpTransport] connecting to 133.1.17.51:6363 from :6363
//        4I        0D 0.000% /ndn/edu/nycu/ether/310505030/ping
//        4I        0D 0.000% /ndn/edu/nycu/udp/ping
//        4I        0D 0.000% /ndn/edu/nthu/udpm/ping