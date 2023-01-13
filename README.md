# NDN-esp8266

This project uses four esp8266 Dev Boards (NodeMCUs) to construct a Named Data Network (NDN) and demonstrates the communication of packets and data between a producer and a consumer based on the Ping mechanism.  
There are 3 clients and 1 server in this project. (both can be expanded)  

Here comes the code!  
* [Client.cpp](https://github.com/LaZoark/NDN-esp8266/blob/master/src/PingClient.cpp)  
* [Server.cpp](https://github.com/LaZoark/NDN-esp8266/blob/master/src/PingServer.cpp)  

DEMO ▶ <https://youtu.be/02WIjpYO2hk>  
SHORTS ▶ [very simple demo](https://www.youtube.com/shorts/ip4JU2zqrJk)

## What is NDN?

Named Data Networking (NDN) is a new networking architecture that aims to address the limitations of the current Internet architecture, which is based on IP addresses. **In NDN, data is the primary focus, rather than devices or hosts.** Each piece of data is given a unique name, and communication is based on requesting and sending data by name, rather than by IP address.

One of the key features of NDN is that it uses a technique called in-network caching, which **allows data to be stored and retrieved from multiple nodes** within the network. `This improves the efficiency and speed of data transfer, as well as reducing the load on the network.`

NDN also includes security features such as digital signature validation and encryption, which help to ensure the authenticity and integrity of data. Additionally, NDN supports multiple types of data, including text, video, and audio, and can be used for a variety of applications, including content distribution, telemetry, and IoT.

Overall, NDN aims to provide a more flexible, efficient, and secure networking architecture that can adapt to the changing needs of the Internet.

## Using library

- [esp8266ndn](https://github.com/yoursunny/esp8266ndn)  
- [NDNph](https://github.com/yoursunny/NDNph)  

### Papers for refer

<https://named-data.net/publications/?limit=1&tgid=&yr=&type=&usr=&auth=&tsr=#tppubs>

---

### Which PIN should I use?

Here's a worth-look blog <https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/>.
