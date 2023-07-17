#ifndef RECEIVER_HPP_
#define RECEIVER_HPP_

#include <Arduino.h>
#include "rgb.h"
#include <esp32/rom/crc.h>

enum receiver_type { packet, fragmentstart, fragment, fragmentend };
#define maxreceiversize 16384

class RECEIVER
{
    private:
        char gameName[32];
        int32_t width;
        int32_t height;
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

    public:
        bool drawFrame=false;
        uint16_t* drawRGB = NULL;

        RECEIVER();
        void init();
        void ProcessPackage(uint8_t * payload, size_t length, receiver_type type);
        
};

#endif