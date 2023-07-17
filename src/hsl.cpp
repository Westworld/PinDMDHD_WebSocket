#include "hsl.h"



HSL::HSL(int h, float s, float l)
{
    H = h;
    S = s;
    L = l;
}

bool HSL::Equals(HSL hsl)
{
    return (H == hsl.H) && (S == hsl.S) && (L == hsl.L);
}


float HSL::HueToRGB(float v1, float v2, float vH) {
	if (vH < 0)
		vH += 1;

	if (vH > 1)
		vH -= 1;

	if ((6 * vH) < 1)
		return (v1 + (v2 - v1) * 6 * vH);

	if ((2 * vH) < 1)
		return v2;

	if ((3 * vH) < 2)
		return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);

	return v1;
}

long HSL::HSLToRGB32() {
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;

	if (S == 0)
	{
		r = g = b = (unsigned char)(L * 255);
	}
	else
	{
		float v1, v2;
		float hue = (float)H / 360;

		v2 = (L < 0.5) ? (L * (1 + S)) : ((L + S) - (L * S));
		v1 = 2 * L - v2;

		r = (unsigned char)(255 * HueToRGB(v1, v2, hue + (1.0f / 3)));
		g = (unsigned char)(255 * HueToRGB(v1, v2, hue));
		b = (unsigned char)(255 * HueToRGB(v1, v2, hue - (1.0f / 3)));
	}

    long result = (r << 16) + (g <<8 )+b;

	return result;
}

unsigned short HSL::HSLToRGB16() {
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;

	if (S == 0)
	{
		r = g = b = (unsigned char)(L * 255);
	}
	else
	{
		float v1, v2;
		float hue = (float)H / 360;

		v2 = (L < 0.5) ? (L * (1 + S)) : ((L + S) - (L * S));
		v1 = 2 * L - v2;

		r = (unsigned char)(255 * HueToRGB(v1, v2, hue + (1.0f / 3)));
		g = (unsigned char)(255 * HueToRGB(v1, v2, hue));
		b = (unsigned char)(255 * HueToRGB(v1, v2, hue - (1.0f / 3)));
	}
    return ((r & 0x00F8) << 8) | ((g & 0x00FC) << 3) | (b >> 3);
}