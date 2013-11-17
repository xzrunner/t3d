#pragma once

namespace t3d {

class Color
{
public:
	Color() : rgba(0) {}

	Color(unsigned char r, unsigned char g,
		unsigned char b, unsigned char a)
		: r(r), g(g), b(b), a(a) {}

	Color(int c) : rgba(c) {}

public:
	union 
	{
		int rgba;								// compressed format
		unsigned char rgba_M[4];				// array format
		struct { unsigned char a,b,g,r; };		// explict name format
	};

}; // Color

}