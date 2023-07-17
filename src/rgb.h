#ifndef RGB_HPP_
#define RGB_HPP_

#include "hsl.h"

class RGB
{
public:
	unsigned char R;
	unsigned char G;
	unsigned char B;

    RGB() {}; 
    RGB(unsigned char r, unsigned char g, unsigned char b);
    void SetLong(long rgb);
    HSL toHSL();
    unsigned short toInt16();

private:
    bool Equals(RGB rgb);
    float Min(float a, float b);
    float Max(float a, float b);
};    

#endif