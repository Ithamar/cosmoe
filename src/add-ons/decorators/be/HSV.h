#ifndef HSV_H_
#define HSV_H_

#include <InterfaceDefs.h>
#include <math.h>

struct HSV
{
	float hue;
	float saturation;
	float value;

public:
	HSV(float h, float s, float v)
	{
		hue = h;
		while (hue < 0)
			hue += 360;

		while (hue > 360)
			hue -= 360;

		saturation = s;
		value = v;
	}

	HSV(rgb_color& c)
	{
		float min = (c.red < c.green)?((c.red < c.blue)? c.red : c.blue): (c.green < c.blue)? c.green: c.blue;
		float max = (c.red > c.green)?((c.red > c.blue)? c.red : c.blue): (c.green > c.blue)? c.green: c.blue;
		value = max;
		float delta = max - min;
		if (max != 0)
		saturation = delta/max;
		else
		{
			saturation = 0;
			hue = -1;
			return;
		}
		if (delta == 0)
		{
			hue = -1;
			return;
		}
		else if (c.red==max)
			hue = (c.green - c.blue) / delta;
		else if (c.green == max)
			hue = 2 + (c.blue - c.red) / delta;
		else
			hue = 4 + (c.red - c.green) / delta;
		hue *=60;
		if (hue < 0)
			hue +=360;
   }

	rgb_color GetRGB()
	{
		int i;
		float f,p,q,t;
		if (saturation == 0)
		{//achromatic
			rgb_color color = {value, value, value, 255};
			return color;
		}
		float h = hue/60;
		i = floor(h);
		f = h - i;
		p = value * (1 - saturation);
		q = value * (1 - saturation * f);
		t = value * (1 - saturation * (1 - f));
		switch(i)
		{
			case 0:
				{
				rgb_color color0 = {value,t,p,255};
				return color0;
				}
			case 1:
				{
				rgb_color color1 =  {q,value,p,255};
				return color1;
				}
			case 2:
				{
				rgb_color color2 =  {p,value,t,255};
				return color2;
				}
			case 3:
				{
				rgb_color color3 =  {p,q,value,255};
				return color3;
				}
			case 4:
				{
				rgb_color color4 =  {t,p,value,255};
				return color4;
				}
			default:
				{
				rgb_color color5 =  {value,p,q,255};
				return color5;
				}
		}
	}

};


#endif /* HSV_H_ */
