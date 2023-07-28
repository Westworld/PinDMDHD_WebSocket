#include "rgb.h"


RGB::RGB(unsigned char r, unsigned char g, unsigned char b)
{
    R = r;
    G = g;
    B = b;
}

RGB::RGB(long rgb)
{
    unsigned char * longptr = (unsigned char *) &rgb;
    R = longptr[1];
    G = longptr[2];
    B = longptr[3];
}

bool RGB::Equals(RGB rgb)
{
    return (R == rgb.R) && (G == rgb.G) && (B == rgb.B);
}

void RGB::SetLong(long rgb) 
{
    unsigned char * longptr = (unsigned char *) &rgb;
    R = longptr[1];
    G = longptr[2];
    B = longptr[3];
}

void RGB::SetLong_grb(long rgb) 
{
    unsigned char * longptr = (unsigned char *) &rgb;
    G = longptr[1];
    R = longptr[2];
    B = longptr[3];
}

 float RGB::Min(float a, float b) {
	return a <= b ? a : b;
}

 float RGB::Max(float a, float b) {
	return a >= b ? a : b;
}

unsigned short RGB::toInt16() {
	return ((R & 0x00F8) << 8) | ((G & 0x00FC) << 3) | (B >> 3);
}


HSL RGB::toHSL()
{
	HSL hsl = HSL(0, 0, 0);

	float r = (R / 255.0f);
	float g = (G / 255.0f);
	float b = (B / 255.0f);

	float min = Min(Min(r, g), b);
	float max = Max(Max(r, g), b);
	float delta = max - min;

	hsl.L = (max + min) / 2;

	if (delta == 0)
	{
		hsl.H = 0;
		hsl.S = 0.0f;
	}
	else
	{
		hsl.S = (hsl.L <= 0.5) ? (delta / (max + min)) : (delta / (2 - max - min));

		float hue;

		if (r == max)
		{
			hue = ((g - b) / 6) / delta;
		}
		else if (g == max)
		{
			hue = (1.0f / 3) + ((b - r) / 6) / delta;
		}
		else
		{
			hue = (2.0f / 3) + ((r - g) / 6) / delta;
		}

		if (hue < 0)
			hue += 1;
		if (hue > 1)
			hue -= 1;

		hsl.H = (int)(hue * 360);
	}

	return hsl;
}