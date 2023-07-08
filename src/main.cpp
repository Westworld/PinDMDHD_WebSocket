#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>

#include <WebSocketsClient.h>


#define WIFI_SSID "Thomas" // change with your own wifi ssid
#define WIFI_PASS "4239772726880285" // change with your own wifi password

// Server infos
char HOST[] = "192.168.0.91";
//#define HOST "http://localhost"
#define PORT 80
char PATH[] = "/ws";

long upperframe = 0, lowerframe = 0;



WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch(type) {
		case WStype_DISCONNECTED:
			USE_SERIAL.printf("[WSc] Disconnected!\n");
			break;
		case WStype_CONNECTED:
			USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);
      // 12500 byte works, 15000 fails

			// send message to server when Connected
			webSocket.sendTXT("Connected");
			break;
		case WStype_TEXT:
			//USE_SERIAL.printf("[WSc] get text: %s\n", payload);
      if (length == 12296) {
        //USE_SERIAL.printf("[WSc] 1st byte: %u  ", payload[0]);
        uint32_t * counter = (uint32_t *) payload;
        USE_SERIAL.printf("frame: %u\n", counter[1]);

        if (payload[0] == 0)  // upperframe
          upperframe = counter[1];
        else  // lowerframe
          if (upperframe == counter[1])   {
            USE_SERIAL.printf("frame: %u\n", upperframe);
            //jetzt malen!
          }
        // framebuffer füllen
        
      }
      else
      USE_SERIAL.printf("[WSc] length ERROR : %u", length);

			// send message to server
			// webSocket.sendTXT("message here");
			break;
		case WStype_BIN:
			USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
			//hexdump(payload, length);

			// send data to server
			// webSocket.sendBIN(payload, length);
			break;
		case WStype_ERROR:			
      USE_SERIAL.printf("[WSc] get error");
      break;
		case WStype_FRAGMENT_TEXT_START:
		case WStype_FRAGMENT_BIN_START:
      USE_SERIAL.printf("[WSc] get binary frag start");
      break;
		case WStype_FRAGMENT:
      USE_SERIAL.printf("[WSc] get binary frag");
      break;		
    case WStype_FRAGMENT_FIN:
			break;
	}

}

void setup() {
	// USE_SERIAL.begin(921600);
	USE_SERIAL.begin(115200);

	//Serial.setDebugOutput(true);
	USE_SERIAL.setDebugOutput(true);

	USE_SERIAL.println();
	USE_SERIAL.println();
	USE_SERIAL.println();

	for(uint8_t t = 4; t > 0; t--) {
		USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
		USE_SERIAL.flush();
		delay(1000);
	}

	WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);

	//WiFi.disconnect();
	while(WiFiMulti.run() != WL_CONNECTED) {
		delay(100);
	}

	// server address, port and URL
	webSocket.begin(HOST, PORT, PATH);

	// event handler
	webSocket.onEvent(webSocketEvent);

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);

}

void loop() {
	webSocket.loop();
}