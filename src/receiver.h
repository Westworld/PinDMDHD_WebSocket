#ifndef RECEIVER_HPP_
#define RECEIVER_HPP_

#include <Arduino.h>
#include "rgb.h"
#include <esp32/rom/crc.h>

enum receiver_type { packet, fragmentstart, fragment, fragmentend };
#define maxreceiversize 49170

class RECEIVER
{
    private:
        bool LEFT; // true left, else right
        char gameName[32];
        int32_t width;
        int32_t height;
        int16_t xoffset=0;
        int16_t yoffset=0;
        int8_t scale=1;

        RGB color_rgb;
        HSL color_hsl;
        uint16_t * colorpalette = NULL;

        bool init_done;  // complete, we can process!
        int32_t init_last;

        uint8_t* buffer = NULL;
        uint8_t* frame = NULL;
        int32_t buffer_offset;
        bool buffer_complete;
        uint32_t lastHash;
        char cur_job_in_progress[32];  
        int32_t package_time;
        int8_t package_header_end;
       

        void reset();
        void AnalyzeHeader();
        void HandlePackage();
        uint32_t getHash(int offset);
        bool JoinPlane(short offset, short bits);
        bool isBitSet(uint8_t byte, uint8_t pos);
        void graytoRgb16(short colors);
        void graytoRgb16_setPixel(int16_t x, int16_t y, uint16_t dotColor);
        void Rgb24toRgb16(int32_t offsetstart);
        

    public:
        int8_t drawFrame=0;
        uint16_t* drawRGB = NULL;

        RECEIVER(bool left);
        void init();
        void ProcessPackage(uint8_t * payload, size_t length, receiver_type type);
        
};

#endif