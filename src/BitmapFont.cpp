#include "BitmapFont.h"

#include "BOB.h"
#include "BmpFile.h"
#include "BmpImg.h"

namespace t3d {

int BitmapFont::Load(char *fontfile, BOB* font)
{
	// this is a semi generic font loader...
	// expects the file name of a font in a template that is 
	// 4 rows of 16 cells, each character is 16x14, and cell 0 is the space character
	// characters from 32 to 95, " " to "-", suffice for 90% of text work

	// load the font bitmap template
	BmpFile* bmpfile = new BmpFile(fontfile);

	// create the bob that will hold the font, use a bob for speed, we can use the 
	// hardware blitter
	font->Create(0,0,16,14,64, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 16); 

	// load all the frames
	for (int index=0; index < 64; index++)
		font->LoadFrame(bmpfile,index,index%16,index/16,BITMAP_EXTRACT_MODE_CELL);

	// unload the bitmap file
	delete bmpfile;

	// return success
	return(1);
}

int BitmapFont::Print(BOB* font,  // pointer to bob containing font
					  int x, int y,  // screen position to render
					  char *string,  // string to render
					  int hpitch, int vpitch, // horizontal and vertical pitch
					  LPDIRECTDRAWSURFACE7 dest)
{
	// this function draws a string based on a 64 character font sent in as a bob
	// the string will be drawn at the given x,y position with intercharacter spacing
	// if hpitch and a interline spacing of vpitch

	// are things semi valid?
	if (!string || !dest)
		return(0);

	// loop and render
	for (int index = 0; index < strlen(string); index++)
	{
		// set the position and character
		font->x = x;
		font->y = y;
		font->curr_frame = string[index] - 32;
		// test for overflow set to space
		if (font->curr_frame > 63 || font->curr_frame < 0) font->curr_frame = 0;

		// render character (i hate making a function call!)
		font->Draw(dest);

		// move position
		x+=hpitch;

	} // end for index

	// return success
	return(1);
}

}