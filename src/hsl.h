#ifndef HSL_HPP_
#define HSL_HPP_  

class HSL
{
public:
	int H;
	float S;
	float L;

    HSL() {};
    HSL(int h, float s, float l);
    void operator = (const HSL &D ) { 
         H = D.H;
         S = D.S;
         L = D.L;
      }
    long HSLToRGB32(void);
    unsigned short HSLToRGB16(void);

private:
    bool Equals(HSL hsl) ; 
    float HueToRGB(float v1, float v2, float vH);  
};

#endif