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


const char* NDN_ROUTER_HOST_WAN = "titan.cs.memphis.edu";
// const char* NDN_ROUTER_HOST_WAN = "hobo.cs.arizona.edu";
// const char* NDN_ROUTER_HOST_WAN = "suns.cs.ucla.edu";

const char* NDN_ROUTER_HOST_LAN = "192.168.1.227";
// const char* NDN_ROUTER_HOST_LAN = "192.168.1.177";

// const char* PREFIX0 = "/ndn/edu/arizona/ping";
// const char* PREFIX1 = "/ndn/edu/memphis/ping";
const char *PREFIX0 = "/ndn/edu/nycu/ether/310505030/ping"; // 陽交 + studientID
const char *PREFIX1 = "/ndn/edu/nycu/udp/ping";             // 陽交
const char *PREFIX2 = "/ndn/edu/ucla/ping";                 // UCLA
const char *PREFIX3 = "/ndn/edu/nthu/udpm/ping";            // 清華

esp8266ndn::EthernetTransport transport_ether;
esp8266ndn::UdpTransport transport_udp;
esp8266ndn::UdpTransport transport_udpm;
esp8266ndn::UdpTransport transport_udp_WAN;
ndnph::Face face_ether(transport_ether);
ndnph::Face face_udp(transport_udp);
ndnph::Face face_udpm(transport_udpm);
ndnph::Face face_udp_WAN(transport_udp_WAN);
ndnph::StaticRegion<2048> region;
ndnph::PingClient client0(ndnph::Name::parse(region, PREFIX0), face_ether, 800);
ndnph::PingClient client1(ndnph::Name::parse(region, PREFIX1), face_udp, 500);
ndnph::PingClient client2(ndnph::Name::parse(region, PREFIX2), face_udp_WAN, 4000);
ndnph::PingClient client3(ndnph::Name::parse(region, PREFIX3), face_udpm, 700);

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
  Serial.println(F("WiFi connected!"));
#if defined(ARDUINO_ARCH_ESP8266)
#define TRIGGER_LED0 D5
#define TRIGGER_LED1 LED_BUILTIN      // gpio 2
#define TRIGGER_LED2 LED_BUILTIN_AUX  // gpio 16
#define TRIGGER_LED3 D6
  pinMode(TRIGGER_LED0, OUTPUT);      // ether-yello
  pinMode(TRIGGER_LED1, OUTPUT);      // udp-blue (onboard)
  pinMode(TRIGGER_LED2, OUTPUT);      // udp_WNA-blue[for UCLA NDN-router]
  pinMode(TRIGGER_LED3, OUTPUT);      // udpm-red
  blink_led(TRIGGER_LED0, 2, 200);
  blink_led(TRIGGER_LED1, 2, 200);
  blink_led(TRIGGER_LED2, 2, 200);
  blink_led(TRIGGER_LED3, 2, 200);
  BearSSL::WiFiClientSecure fchSocketClient;
  fchSocketClient.setInsecure();
#elif defined(ARDUINO_ARCH_ESP32)
  delay(500);
  WiFiClientSecure fchSocketClient;
  fchSocketClient.setInsecure();
#endif
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
  // auto fchResponse = esp8266ndn::fchQuery(fchSocketClient, "https://fch.ndn.today/"); // Query NDN-FCH service to find a nearby NDN-Router.
  // if (!fchResponse.ok){
  //   Serial.println(F("NDN-FCH service is unreachable!"));
  //   ESP.restart();
  //   }
  // transport_udp_WAN.beginTunnel(fchResponse.ip);

  uint16_t _remotePort = 6364;    // Since we built 2 UDP tunnels, each must use different port.
  uint16_t _localPort = 6364;
  // Transport for the "UDP" (Wide Area Network)
  transport_udp_WAN.beginTunnel(routerIp_WAN, 6363, 6363);
  // Transport for the "UDP"
  transport_udp.beginTunnel(routerIp, _remotePort, _localPort);
  // Transport for the "UDP multicast"
  transport_udpm.beginMulticast(); // default group IP=(224, 0, 23, 170)
  // Transport for the "Ethernet"
  transport_ether.begin();
}

uint32_t RxCounter(const ndnph::PingClient &client)
{
  auto cnt = client.readCounters();
  return cnt.nRxData;
}
void printCounters(const char *prefix, const ndnph::PingClient &client)
{
  auto cnt = client.readCounters();
  Serial.printf("[I/D]: %5d/%-5d %6.2f%%  %3s\n", 
                 static_cast<int>(cnt.nTxInterests),
                 static_cast<int>(cnt.nRxData),
                 100.0*cnt.nRxData / cnt.nTxInterests, prefix);
}

uint16_t led_ticks0, led_ticks1, led_ticks2, led_ticks3 = 0;
uint32_t nRxData_0, nRxData_1, nRxData_2, nRxData_3 = 0;
uint32_t temp_0, temp_1, temp_2, temp_3 = 0;
bool _nRxData_0, _nRxData_1, _nRxData_2, _nRxData_3 = true;
bool print_lock0, print_lock1, print_lock2, print_lock3 = true;

const uint32_t interval_trig = 1;     // interval at which to trig (milliseconds)
uint32_t previousMillis_trig = 0;     // will store the time of the last Trigger was updated
const uint32_t interval_print = 1020; // interval at which to print (milliseconds)
uint32_t previousMillis_print = 0;    // will store the time of the last print()


void CheckAndTrig(uint32_t &temp, uint32_t &nRxData, uint16_t &_led_ticks,
                  int TRIGGER_LED, bool &_print_lock, bool &_nRxData_x)
{
  // auto _LOW = LOW;
  // auto _HIGH = HIGH;
  auto _LOW = HIGH;
  auto _HIGH = LOW;
  if (++temp <= nRxData){           // check the change of the value
    temp = nRxData;
    nRxData = 1;
    if (_led_ticks <= 0)
      _led_ticks = 100;                            // register the trigger
  }else{
    nRxData = 0;
    temp--;
  }
  // if (TRIGGER_LED == LED_BUILTIN || TRIGGER_LED == LED_BUILTIN_AUX){
  //   _LOW = HIGH;
  //   _HIGH = LOW;
  //   }
  if (_led_ticks > 0){
    digitalWrite(TRIGGER_LED, _HIGH);   // turn on the LED
    if (_print_lock) {
      _nRxData_x = true;
      _print_lock = false;   // 上鎖，防止在print()之前就被改掉
      }
    _led_ticks--;
  }else{
    digitalWrite(TRIGGER_LED, _LOW);  // turn off the LED
    if (_print_lock) {
      _nRxData_x = false;
      _print_lock = false;   // 上鎖，防止在print()之前就被改掉
      }
  }
}

void loop()
{
  face_udp_WAN.loop();
  face_udp.loop();
  face_udpm.loop();
  face_ether.loop();

  uint32_t currentMillis = millis();  // current time spanned 

  if (currentMillis - previousMillis_trig >= interval_trig) 
  { previousMillis_trig = currentMillis;   // save the time of the last Trigger
    nRxData_0 = RxCounter(client0);
    nRxData_1 = RxCounter(client1);
    nRxData_2 = RxCounter(client2);
    nRxData_3 = RxCounter(client3);

    CheckAndTrig(temp_0, nRxData_0, led_ticks0, TRIGGER_LED0, print_lock0, _nRxData_0);
    CheckAndTrig(temp_1, nRxData_1, led_ticks1, TRIGGER_LED1, print_lock1, _nRxData_1);
    CheckAndTrig(temp_2, nRxData_2, led_ticks2, TRIGGER_LED2, print_lock2, _nRxData_2);
    CheckAndTrig(temp_3, nRxData_3, led_ticks3, TRIGGER_LED3, print_lock3, _nRxData_3);

    if (currentMillis - previousMillis_print >= interval_print) 
    {
      previousMillis_print = currentMillis;   // save the time of the last print
      printCounters(PREFIX0, client0);
      printCounters(PREFIX1, client1);
      printCounters(PREFIX2, client2);
      printCounters(PREFIX3, client3);
      Serial.print(F("Current Rx---------"));
      Serial.printf("[%d, %d, %d, %d]", _nRxData_0, _nRxData_1, _nRxData_2, _nRxData_3);
      Serial.println(F("----------------"));
      print_lock0 = true;  // 解鎖，print()之後就開放修改
      print_lock1 = true;  // 解鎖，print()之後就開放修改
      print_lock2 = true;  // 解鎖，print()之後就開放修改
      print_lock3 = true;  // 解鎖，print()之後就開放修改
    }
  }
}


    // if (++temp <= nRxData_1){
    //   temp = nRxData_1;
    //   nRxData_1 = 1;
    // }else{
    //   nRxData_1 = 0;
    //   temp--;
    // }
    // if (nRxData_1){
    //   ++led_ticks;                            // register the trigger
    // }
    // if (led_ticks > 0){
    //       digitalWrite(TRIGGER_LED, LOW);   // turn on the LED
    //       ++led_ticks;                          // release the trigger
    //     }else{
    //       digitalWrite(TRIGGER_LED, HIGH);  // turn off the LED
    //     }


      // Serial.print(F("Current Rx---------"));
      // Serial.print(F("["));   Serial.print(nRxData_0);
      // Serial.print(F(", "));  Serial.print(nRxData_1);
      // Serial.print(F(", "));  Serial.print(nRxData_2);
      // Serial.print(F(", "));  Serial.print(nRxData_3);
      // Serial.println(F("]----------------"));


// #################### local ####################
// 6565 [UdpTransport] connecting to 192.168.1.227:6363 from :6363
// 6565 [UdpTransport] joining group 224.0.23.170:56363 on (IP unset)
// 6566 [EthernetTransport] enabled on st0


// #################### Internet ####################
// 9264 [AutoConfig] https://fch.ndn.today/ body: ndn.ist.osaka-u.ac.jp
// 9288 [AutoConfig] DNS resolved to: 133.1.17.51
// 9289 [UdpTransport] connecting to 133.1.17.51:6363 from :6363
//        4I        0D 0.000% /ndn/edu/nycu/ether/310505030/ping
//        4I        0D 0.000% /ndn/edu/nycu/udp/ping
//        4I        0D 0.000% /ndn/edu/nthu/udpm/ping