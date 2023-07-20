#include "receiver.h"

extern void DrawOnePixel(int16_t x, int16_t y, uint16_t color);

RECEIVER::RECEIVER(bool left)
{
    LEFT = left;
    reset();
}

void RECEIVER::reset()
{
Serial.println("Before reset");

    gameName[0] = 0;
    width = 128;
    height = 32;
    color_rgb.SetLong(0x00EC843D);
    color_hsl = color_rgb.toHSL();

    if (colorpalette != NULL)
    {    delete[] colorpalette;
         colorpalette = NULL;
    }

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
        frame = (uint8_t *) malloc(256*64); // one byte for max 255 colors
    if (frame == NULL)
        Serial.printf("[error] not enough memory frame\n");


    if (drawRGB == NULL)
        drawRGB = (uint16_t *) malloc(128*64*2);  // only half for one panel
    if (drawRGB == NULL)
        Serial.printf("[error] not enough memory drawRGB\n");


Serial.println("after reset");        
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
    Serial.printf("Width: %u,  Height: %u\n", width, height);            
        }
        return;
    }

    if (strcmp(cur_job_in_progress, "color") == 0)
    {
        if (buffer_offset >= (package_header_end+4)) {
            //uint32_t newRGB;
            //memcpy(&newRGB, &buffer[package_header_end], 4);
            //color_rgb.SetLong(newRGB);
            color_rgb.R = buffer[package_header_end+2];
            color_rgb.G = buffer[package_header_end+1];
            color_rgb.B = buffer[package_header_end+3];
            color_hsl = color_rgb.toHSL(); 
            if (colorpalette != NULL)
            {    delete[] colorpalette;   
                 colorpalette = NULL;
            }      
        }
        return;
    }

    if (strcmp(cur_job_in_progress, "gray4Planes") == 0)
    {
        if (!init_done) return;
/*
        uint32_t newHash = getHash(16);
        if (newHash == lastHash) {
            Serial.println("same hash");
            return;
        }
        Serial.println("####### new hash");
        lastHash = newHash;  
*/          
        if (JoinPlane(16, 4)) return;
        graytoRgb16(16);
        drawFrame= true;      
        return;
    }

    if (strcmp(cur_job_in_progress, "gray2Planes") == 0)
    {
        if (!init_done) return;         
        if (JoinPlane(16, 2)) return;
        graytoRgb16(4);
        drawFrame= true;      
        return;
    }    

    if (strcmp(cur_job_in_progress, "coloredGray4") == 0)
    {
        if (!init_done) return;
        int16_t offset = 17;
        uint32_t countpalettes=0;
        memcpy(&countpalettes, &buffer[offset], 4); 
        offset+=4;
        if (colorpalette != NULL) delete colorpalette;

        colorpalette = new uint16_t[countpalettes];
Serial.printf("create color palette %u\n", countpalettes) ;
        
        for (int8_t i=0;i<countpalettes;i++) 
        {
            uint32_t newcolor=0;
            memcpy(&newcolor, &buffer[offset], 4); 
            offset+=4;
            colorpalette[i] = newcolor;
        }      
        
        if (JoinPlane(offset, 4)) return;
        graytoRgb16(16);
        drawFrame= true;      
        return;
    }


    if (strcmp(cur_job_in_progress, "coloredGray2") == 0)
    {
        if (!init_done) return;
        int16_t offset = 17;
        uint32_t countpalettes=0;
        memcpy(&countpalettes, &buffer[offset], 4); 
        offset+=4;
        if (colorpalette != NULL) delete colorpalette;

        colorpalette = new uint16_t[countpalettes];
Serial.printf("create color palette %u\n", countpalettes) ;
        
        for (int8_t i=0;i<countpalettes;i++) 
        {
            uint32_t newcolor=0;
            memcpy(&newcolor, &buffer[offset], 4); 
            offset+=4;
            colorpalette[i] = newcolor;
        }      
        
        if (JoinPlane(offset, 2)) return;
        graytoRgb16(4);
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

    memset ( frame, 0, 256*64 );    

    for (int bytePos=0; bytePos<(width*height/8); bytePos++) {
        for (int8_t bitPos=7; bitPos>=0; bitPos--) {
            for (int8_t planePos=0; planePos<bitlength;planePos++) {
                uint8_t * plane = &buffer[shouldbe * planePos];
                int8_t bit = isBitSet(plane[bytePos], bitPos) ? 1 : 0;
                uint8_t byte = frame[bytePos*8+bitPos];
                byte = byte | (bit << planePos);
                if (byte > 16)
                    Serial.println("wrong color");
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
        Serial.printf("create color palette %u\n", colors) ;
        
        for (int8_t i=0;i<colors;i++) 
        {
            float lum = i/(colors-1);
            HSL hslcolor =  HSL(color_hsl.H, color_hsl.S, lum*color_hsl.L);
            colorpalette[i] = hslcolor.HSLToRGB16();
        } 
    }

    for (short y=0; y<height;y++) 
    {
        for (short x=0; x<width; x++) 
        {
            long index = (y*width)+x;
            long test = frame[index];
            unsigned char test2 = frame[index];
            if (frame[index] >= colors)
                Serial.printf("invalid color: %u\n",frame[index]); 
            else    
                dotColor = colorpalette[frame[(y*width)+x]];
            if (drawRGB == NULL)  {
                Serial.printf("drawRGB == null\n"); return;
            }    

            if (LEFT)
            {
                if (x < 128)  
                {
                    //DrawOnePixel( x,  y,  dotColor);
                    drawRGB[(y*128)+x] = dotColor;
                }
            }
            else
            {  // RIGHT
                if (x >= 128)  
                    {
                        //DrawOnePixel( x,  y,  dotColor);
                        drawRGB[(y*128)+x-128] = dotColor;
                    }
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