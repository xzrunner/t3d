#include <windows.h>
#include <stdio.h>

#include "Vector.h"

namespace t3d {

class BmpImg;

class Game
{
public:
	static const int WINDOW_WIDTH = 800;
	static const int WINDOW_HEIGHT = 600;
	static const int WINDOW_BPP = 32; // bitdepth of window (8,16,24 etc.)
	// note: if windowed and not
	// fullscreen then bitdepth must
	// be same as system bitdepth
	// also if 8-bit the a pallete
	// is created and attached
	static const int WINDOWED_APP = 1; // 0 not windowed, 1 windowed

public:
	Game(HWND handle, HINSTANCE instance);
	~Game();

	void Init();
	void Shutdown();
	void Step();

private:
	static const int TEXTSIZE = 128; // size of texture mxm
	static const int NUM_TEXT = 12;  // number of textures

private:
	HWND main_window_handle; // save the window handle
	HINSTANCE main_instance; // save the instance
	char buffer[80];         // used to print text

	BmpImg *textures[NUM_TEXT],
		   *temp_text;

	BmpImg* gmipmaps;

}; // Game

}