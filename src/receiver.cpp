#include "receiver.h"

RECEIVER::RECEIVER()
{
    reset();
}

void RECEIVER::reset()
{
    gameName[0] = 0;
    width = 128;
    height = 32;
    color_rgb.SetLong(0x00EC843D);
    color_hsl = color_rgb.toHSL();

    if (colorpalette != NULL)
        delete[] colorpalette;

    init_done=false;
    init_last=0;
    drawFrame=false;

    buffer_offset=0;
    lastHash=0;
    buffer_complete=false;
    cur_job_in_progress[0] = 0;
    package_time = 0;
    package_header_end = 0;

    if (buffer == NULL)
        buffer = (uint8_t *) malloc(maxreceiversize);
    if (buffer == NULL)
        Serial.printf("[error] not enough memory\n");

    if (frame == NULL)
        frame = (uint8_t *) malloc(256*64);
    if (frame == NULL)
        Serial.printf("[error] not enough memory frame\n");

    if (drawRGB == NULL)
        drawRGB = (uint16_t *) malloc(128*64);  // only half for one panel
    if (drawRGB == NULL)
        Serial.printf("[error] not enough memory drawRGB\n");
}

void RECEIVER::init()
{
    reset();
    init_last=millis();
}

void RECEIVER::AnalyzeHeader() 
{
    // find first 0 to find headertext
    int16_t pos = 0;
    while ((pos < buffer_offset) && (buffer[pos] != 0))
        pos++;

    if ((pos < buffer_offset) && (pos<31))
    {
        // found headertext
        strncpy(cur_job_in_progress, (char *) buffer, pos);
        cur_job_in_progress[pos]=0;
        //memcpy(package_time, &buffer[pos], 4);
        package_header_end = pos+1;
    }
}

void RECEIVER::HandlePackage()
{
    if (strcmp(cur_job_in_progress, "gameName") == 0)
    {
        strncpy(gameName, (char *) &buffer[package_header_end],31);
        gameName[31]=0;
    Serial.print("GameName: ");
    Serial.println(gameName);    
        init_done = true;
        return;
    }

    if (strcmp(cur_job_in_progress, "dimensions") == 0)
    {
        if (buffer_offset >= (package_header_end+8)) {
            memcpy(&width, &buffer[package_header_end], 4);
            memcpy(&height, &buffer[package_header_end+4], 4);
    //Serial.printf("Width: %u,  Height: %u\n", width, height);            
        }
        return;
    }

    if (strcmp(cur_job_in_progress, "color") == 0)
    {
        if (buffer_offset >= (package_header_end+4)) {
            uint32_t newRGB;
            memcpy(&newRGB, &buffer[package_header_end], 4);
            color_rgb.SetLong(newRGB);
            color_hsl = color_rgb.toHSL(); 
            if (colorpalette != NULL)
                delete[] colorpalette;         
        }
        return;
    }

    if (strcmp(cur_job_in_progress, "gray4Planes") == 0)
    {
        if (!init_done) return;

        uint32_t newHash = getHash(16);
        if (newHash == lastHash) return;

        lastHash = newHash;
        
        if (JoinPlane(16, 4)) return;
        graytoRgb16(16);
        drawFrame= true;
        return;
    }

    Serial.print("package: ");
    Serial.println(cur_job_in_progress);
}

bool RECEIVER::JoinPlane(short offset, short bitlength)
{
    long planeSize = (buffer_offset-offset)/bitlength;
    long shouldbe = width * height / 8;
    if (planeSize != shouldbe) {
        Serial.printf("wrong plane size %u\n", planeSize); return true; }

    for (int bytePos=0; bytePos<(width*height/8); bytePos++) {
        for (int8_t bitPos=7; bitPos>=0; bitPos--) {
            for (int8_t planePos=0; planePos<bitlength;planePos++) {
                uint8_t * plane = &buffer[shouldbe * planePos];
                int8_t bit = isBitSet(plane[bytePos], bitPos) ? 1 : 0;
                uint8_t byte = frame[bytePos*8+bitPos];
                byte = byte | (bit << planePos);
                frame[bytePos*8+bitPos] = byte;
            }
        }
    }
    return false;
}

bool RECEIVER::isBitSet(uint8_t byte, uint8_t pos) {
    return  ((byte & (1 << pos)) != 0);
}

void RECEIVER::graytoRgb16(short colors)
{
    uint32_t pos=0;
    uint16_t dotColor=0;

    if (colorpalette == NULL)
    {    
        colorpalette = new uint16_t[colors];
        for (int8_t i=0;i<colors;i++) 
        {
            float lum = i/(colors-1);
            HSL * hslcolor = new HSL(color_hsl.H, color_hsl.S, lum*color_hsl.L);
            colorpalette[i] = hslcolor->HSLToRGB16();
            delete hslcolor;
        }
    }

    for (short y=0; y<height;y++) 
    {
        for (short x=0; x<width; x++) 
        {
            dotColor=colorpalette[frame[(y*width)+x]];
            if (x < 128)  
            {// LEFT
                drawRGB[(y*128)+x] = dotColor;
            }
        }
    }
}

uint32_t RECEIVER::getHash(int offset)
{
    return  (~crc32_le((uint32_t)~(0xffffffff), (const uint8_t*)&buffer[offset], buffer_offset-offset))^0xffffffFF;
   
}

void RECEIVER::ProcessPackage(uint8_t * payload, size_t length, receiver_type type)
{
    switch(type) {
        case packet: 
            // complete packet arrived
            if (length <= maxreceiversize)
            {    memcpy( buffer, payload, length );
                 buffer_offset = length;
                 buffer_complete=true;
            }
            break;
        case fragmentstart:
            if (length <= maxreceiversize)
            {    memcpy( buffer, payload, length );
                 buffer_offset = length;
            }
            buffer_complete=false;
            break;  
        case fragment:
            if (buffer_offset != 0) {
                if ((buffer_offset+length) <= maxreceiversize)
                {   memcpy( &buffer[buffer_offset], payload, length );
                    buffer_offset += length;
                }
                else
                    buffer_offset = 0;
            }
            break;              
        case fragmentend:
            if (buffer_offset != 0) {
                if ((buffer_offset+length) <= maxreceiversize)
                {   memcpy( &buffer[buffer_offset], payload, length );
                    buffer_offset += length;
                    buffer_complete=true;
                }
                else
                    buffer_offset = 0;
            }
            break;

        default:
            break;  // nothing
    }

    if (buffer_complete) {
        AnalyzeHeader();
        HandlePackage();
        buffer_complete = false;
        buffer_offset = 0;
    }
}