#pragma once

#include <ddraw.h>

namespace t3d {

class BOB;

class BitmapFont
{
public:
	int Load(char *fontfile, BOB* font);

	int Print(BOB* font,  // pointer to bob containing font
			  int x, int y,  // screen position to render
			  char *string,  // string to render
			  int hpitch, int vpitch, // horizontal and vertical pitch
			  LPDIRECTDRAWSURFACE7 dest);

}; // BitmapFont

}