#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>

#include <WebSocketsClient.h>

#include "ESP32-HUB75-MatrixPanel-DMA.h"

// Server infos
char HOST[] = "192.168.0.91";
//#define HOST "http://localhost"
#define PORT 80
char PATH[] = "/ws";

long upperframe = 0, lowerframe = 0;

MatrixPanel_DMA *dma_display = nullptr;


// check

    #define PIN_R1  32 //25  
    #define PIN_G1  23 // 26
    #define PIN_B1  33 // 27
    #define PIN_R2  25 // 18  
    #define PIN_G2  19 // 21 
    #define PIN_B2  26 // 2  

    #define PIN_A   27 //23
    #define PIN_B   18 // 19
    #define PIN_C   14 // 5
    #define PIN_D   17 // 17
    #define PIN_E   22  // nc
              
    #define PIN_LE 16 // 4
    #define PIN_OE  13 // 15
    #define PIN_CLK 12 // 16


WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial

void drawHalfFrame(uint8_t * payload, uint8_t half) {
		if (half == 0)
			dma_display->clearScreen();

    uint16_t * buffer = (uint16_t *) payload;

	  for (int x = 0; x < 128; x++) {
      for (int y = 0; y <  32; y++) {
				long offset = 8 + ((x + (y*128)) * 2);
				if (half == 0) {
                	dma_display->drawPixel(x, y, buffer[offset]);
				}	
				else
                	dma_display->drawPixel(x, y+32, buffer[offset]);				
            }
    }
	if (half == 1)
    	dma_display->flipDMABuffer();
}

void drawHalfFrameRGB(uint8_t * payload, uint8_t half) {
		if (half == 0)
			dma_display->fillScreenRGB888(0, 0, 0);
	    for (int x = 0; x < 128; x++) {
            for (int y = 0; y <  32; y++) {
				long offset = 8 + ((x + (y*128)) * 3);
				if (half == 0) {
                	dma_display->drawPixelRGB888(x, y, payload[offset], payload[offset+1], payload[offset+2]);
				}	
				else
                	dma_display->drawPixelRGB888(x, y+32, payload[offset], payload[offset+1], payload[offset+2]);				
            }
    }
	if (half == 1)
    	dma_display->flipDMABuffer();
}

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
      if (length == (8192+8)) { // (length == 12296) {
		    uint32_t * counter = (uint32_t *) payload;

        //USE_SERIAL.printf("[WSc] 1st byte: %u  frame: %u\n", payload[0], counter[1]);

        if (payload[0] == 0)  // upperframe
        {
			  if ((counter[1] > upperframe) |(counter[1] < (upperframe-50))){  // bei falscher Reihenfolge ignorieren
				upperframe = counter[1];
				drawHalfFrame(payload, 0);
			  }
		}	  
        else  // lowerframe
          if (upperframe == counter[1])   {
			USE_SERIAL.printf("[WSc] frame: %u\n", upperframe);
			drawHalfFrame(payload, 1);
          }//
        
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
		delay(100);
	}

	WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);

	//WiFi.disconnect();
	while(WiFiMulti.run() != WL_CONNECTED) {
		delay(100);
	}

	
  #define PANEL_WIDTH 128
  #define PANEL_HEIGHT 64
  #define PANEL_WIDTH_CNT 1
  #define PANEL_HEIGHT_CNT 1
  #define PANE_WIDTH PANEL_WIDTH*PANEL_WIDTH_CNT
  #define PANE_HEIGHT PANEL_HEIGHT*PANEL_HEIGHT_CNT

  hub75_cfg_t mxconfig = {
    .mx_width = PANEL_WIDTH,
    .mx_height = PANEL_HEIGHT,
    .mx_count_width = PANEL_WIDTH_CNT,
    .mx_count_height = PANEL_HEIGHT_CNT,
    .gpio = {
    .r1 = PIN_R1,
    .g1 = PIN_G1,
    .b1 = PIN_B1,
    .r2 = PIN_R2,
    .g2 = PIN_G2,
    .b2 = PIN_B2,
    .a = PIN_A,
    .b = PIN_B,
    .c = PIN_C,
    .d = PIN_D,
    .e = PIN_D,
    .lat = PIN_LE,
    .oe = PIN_OE,
    .clk = PIN_CLK,
    },
    .driver = ICN2053,
    .clk_freq = HZ_13M, 
    .clk_phase = CLK_POZITIVE,
    .color_depth = COLORx16,//PIXEL_COLOR_DEPTH
    .double_buff = DOUBLE_BUFF_ON,
    .double_dma_buff = DOUBLE_BUFF_ON,
    .decoder_INT595 = true,
  };

  dma_display = new MatrixPanel_DMA(mxconfig);


  // Allocate memory and start DMA display
  if( not dma_display->begin() )
      Serial.println("****** !KABOOM! I2S memory allocation failed ***********");

  // let's adjust default brightness to about 75%
  dma_display->setBrightness8(192);    // range is 0-255, 0 - 0%, 255 - 100%
  //set rotate for new drawing
  //dma_display->setRotate(ROTATE_90); //ROTATE_0, ROTATE_90, ROTATE_180, ROTATE_270
  //set mirror for new drawing
  //dma_display->setMirrorX(true);
  //dma_display->setMirrorY(true);
 
  // well, hope we are OK, let's draw some colors first :)
  Serial.println("Fill screen: RED");
  dma_display->fillScreenRGB888(255, 0, 0);
  dma_display->flipDMABuffer();
  delay(1000);
    Serial.println("draw line: green");
  for (int i=0;i<64;i++)
  	dma_display->drawPixelRGB888(i, i, 0, 255, 0);
  dma_display->flipDMABuffer();
  delay(1000);
   Serial.println("Fill screen: black");
  dma_display->fillScreenRGB888(0, 0, 0);
  dma_display->flipDMABuffer();

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