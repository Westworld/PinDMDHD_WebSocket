#include <Arduino.h>

#include <WiFiManager.h> 
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>


#include <WebSocketsClient.h>

#ifndef NoDMD
#include "ESP32-HUB75-MatrixPanel-DMA.h"
#endif

#include <receiver.h>


bool IsLeft=true;

// Server infos direkt
/*
char HOST[] = "192.168.0.103";
#define PORT 9090
char PATH[] = "/dmd";
*/
//proxy

char HOST[] = "192.168.0.91";
#define PORT 80
char PATH[] = "/ws";


#ifndef NoDMD
MatrixPanel_DMA *dma_display = nullptr;
#endif
RECEIVER * receiver = nullptr;

bool DelayedRedrawNeeded=false;

// direct connection // as used with ZeDMD

    #define PIN_R1  32 //25  
    #define PIN_G1  22 // 26  ((23 has issues?))
    #define PIN_B1  33 // 27
    #define PIN_R2  25 // 18  
    #define PIN_G2  19 // 21 
    #define PIN_B2  26 // 2  

    #define PIN_A   27 //23
    #define PIN_B   18 // 19
    #define PIN_C   14 // 5
    #define PIN_D   17 // 17
    #define PIN_E   22  // nc
              
    #define PIN_LE 16 // 4   stb
    #define PIN_OE  13 // 15
    #define PIN_CLK 12 // 16
  #define PANEL_WIDTH 128
  #define PANEL_HEIGHT 64
  #define PANEL_WIDTH_CNT 1
  #define PANEL_HEIGHT_CNT 1
  #define PANE_WIDTH PANEL_WIDTH*PANEL_WIDTH_CNT
  #define PANE_HEIGHT PANEL_HEIGHT*PANEL_HEIGHT_CNT

WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial

void decodePacket(uint8_t * payload, size_t length) {
  short pos=0;

  while ((pos<20) && (pos<length) && (payload[pos]!=0))
    pos++;

  if (pos>0) {
    char buffer[21];
    strncpy(buffer, (char *) payload, 21);
    USE_SERIAL.printf("[dmd] found: %s\n", buffer);

  }  
}

#ifndef NoDMD
void drawHalfFrameFast(uint8_t * payload, uint8_t half) {
    uint8_t * startpayload = &payload[8];

    uint16_t * buffer = (uint16_t *) startpayload;

		if (half == 0)
			{
        dma_display->fillScreenRGB888(0, 0, 0);
        dma_display->CopyBuffer(0,  31, buffer);
      }
    else {
      dma_display->CopyBuffer(32,  63, buffer);
      dma_display->flipDMABuffer();

    }  
}

void drawHalfFrame(uint8_t * payload, uint8_t half) {
		if (half == 0)
			dma_display->fillScreenRGB888(0, 0, 0);

    uint16_t * buffer = (uint16_t *) payload;

	  for (int x = 0; x < 128; x++) {
      for (int y = 0; y <  32; y++) {
				long offset = 4 + ((x + (y*128)));  // offset 4 weil 4 * 2
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

#endif

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch(type) {
		case WStype_DISCONNECTED:
			USE_SERIAL.printf("[WSc] Disconnected!\n");
			break;
		case WStype_CONNECTED:
			USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);
			webSocket.sendTXT("init");
      receiver->init();
			break;
		case WStype_TEXT:
		//	USE_SERIAL.printf("[WSc] get text: %s\n", payload);
      receiver->ProcessPackage( payload, length, packet) ;

			break;
		case WStype_BIN:
		//	USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
      receiver->ProcessPackage( payload, length, packet) ;
			break;
		case WStype_ERROR:			
      USE_SERIAL.printf("[WSc] get error");
      break;
		case WStype_FRAGMENT_TEXT_START:
    //USE_SERIAL.printf("[WSc] get text frag start, len: %u\n", length);
    receiver->ProcessPackage( payload, length, fragmentstart) ;
      break;
		case WStype_FRAGMENT_BIN_START:
     // USE_SERIAL.printf("[WSc] get binary frag start, len: %u\n", length);
      receiver->ProcessPackage( payload, length, fragmentstart) ;
      break;
		case WStype_FRAGMENT:
     // USE_SERIAL.printf("[WSc] get binary frag, len: %u\n", length);
      receiver->ProcessPackage( payload, length, fragment) ;
      break;		
    case WStype_FRAGMENT_FIN:
    // USE_SERIAL.printf("[WSc]  binary frag fin: %u\n", length);
     receiver->ProcessPackage( payload, length, fragmentend) ;
		break;
	}

}


void DrawFrame() 
{
#ifndef NoDMD
  if (IsLeft) {
    dma_display->setColor(255, 0 , 0);
    dma_display->drawFastHLine(0, 0, 128);
    dma_display->drawFastHLine(1, 1, 127);
    dma_display->drawFastHLine(1, 62, 127);
    dma_display->drawFastHLine(0, 63, 128);
    dma_display->drawFastVLine(0, 0, 64);
    dma_display->drawFastVLine(1, 0, 64);

    dma_display->setColor(0, 255 , 0);
    dma_display->drawFastHLine(2, 2, 126);
    dma_display->drawFastHLine(3, 3, 125);    
    dma_display->drawFastHLine(2, 61, 126);
    dma_display->drawFastHLine(3, 60, 125);
    dma_display->drawFastVLine(2, 2, 59);    
    dma_display->drawFastVLine(3, 3, 59); 

    dma_display->setColor(0, 0 , 255);
    dma_display->drawFastHLine(4, 4, 124);
    dma_display->drawFastHLine(5, 5, 123);
    dma_display->drawFastHLine(4, 59, 124);
    dma_display->drawFastHLine(5, 58, 123);
    dma_display->drawFastVLine(4, 4, 55);  
    dma_display->drawFastVLine(5, 5, 55); 

  }
  else {  // Rechts
   dma_display->setColor(255, 0 , 0);
    dma_display->drawFastHLine(0, 0, 128);
    dma_display->drawFastHLine(1, 1, 127);
    dma_display->drawFastHLine(1, 62, 127);
    dma_display->drawFastHLine(0, 63, 128);
    dma_display->drawFastVLine(63, 0, 64);
    dma_display->drawFastVLine(62, 0, 64);

    dma_display->setColor(0, 255 , 0);
    dma_display->drawFastHLine(2, 2, 126);
    dma_display->drawFastHLine(3, 3, 125);    
    dma_display->drawFastHLine(2, 61, 126);
    dma_display->drawFastHLine(3, 60, 125);
    dma_display->drawFastVLine(61, 3, 59);    
    dma_display->drawFastVLine(60, 3, 59); 

    dma_display->setColor(0, 0 , 255);
    dma_display->drawFastHLine(4, 4, 124);
    dma_display->drawFastHLine(5, 5, 123);
    dma_display->drawFastHLine(4, 59, 124);
    dma_display->drawFastHLine(5, 58, 123);
    dma_display->drawFastVLine(59, 6, 55);  
    dma_display->drawFastVLine(58, 6, 55); 
 
  }
#endif  
}  

void setup() {
	// USE_SERIAL.begin(921600);
	USE_SERIAL.begin(115200);

	//Serial.setDebugOutput(true);
	USE_SERIAL.setDebugOutput(true);
	USE_SERIAL.println();

#ifndef NoDMD
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

  DrawFrame();

  dma_display->setTextColorRGB(255, 0, 0);
  dma_display->setCursor(20, 20);
  dma_display->setTextSize(1);
  dma_display->println("Connect Wifi");
  //dma_display->drawChar(10, 50, 'a', 0xE120, 0x0, 18);
 
 dma_display->flipDMABuffer();
 delay(1000);
#endif

	WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);

	//WiFi.disconnect();
	while(WiFiMulti.run() != WL_CONNECTED) {
		delay(100);
	}

  receiver = new RECEIVER(IsLeft);

	// server address, port and URL
	webSocket.begin(HOST, PORT, PATH);

	// event handler
	webSocket.onEvent(webSocketEvent);

	// try ever 1000 again if connection has failed
	webSocket.setReconnectInterval(500);

      #ifndef NoDMD
   DrawFrame();
  dma_display->setTextColorRGB(255, 0, 0);
  dma_display->setCursor(20, 20);
  dma_display->setTextSize(1);
  dma_display->println("Connected");
  //dma_display->drawChar(10, 50, 'a', 0xE120, 0x0, 18);
 
 dma_display->flipDMABuffer();
 #endif

}

void loop() {
	webSocket.loop();
  if (receiver->drawFrame != 0)
  {
      #ifndef NoDMD
      if (receiver->drawFrame == 1) {
       // Serial.println("Before draw");
        //dma_display->fillScreenRGB888(0, 0, 0);
        dma_display->CopyBuffer(0,  63, receiver->drawRGB);
      }

      receiver->drawFrame=0;
      DelayedRedrawNeeded = dma_display->flipDMABufferIfReady();
      if (!DelayedRedrawNeeded)  Serial.print("+");
//Serial.println("after draw");
      #endif
  }

  // retry in loop. If next frame already displayed, flag is reset
  if (DelayedRedrawNeeded) {
    #ifndef NoDMD
    Serial.print(".");
    DelayedRedrawNeeded = dma_display->flipDMABufferIfReady();
    #endif
  }  
  
}

void DrawOnePixel(int16_t x, int16_t y, uint16_t color)
{
#ifndef NoDMD
  dma_display->drawPixel(x, y, color);	
#endif
}

void DrawOnePixe24(int16_t x, int16_t y, uint8_t R, uint8_t G, uint8_t B)
{
#ifndef NoDMD
  dma_display->drawPixelRGB888(x, y, R, G, B);	
#endif
}