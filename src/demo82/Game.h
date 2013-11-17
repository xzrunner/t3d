#include <windows.h>
#include <stdio.h>

namespace t3d {

class BmpFile;
class BmpImg;

class Game
{
public:
	static const int WINDOW_WIDTH = 640;
	static const int WINDOW_HEIGHT = 480;
	static const int WINDOW_BPP = 32; // bitdepth of window (8,16,24 etc.)
	// note: if windowed and not
	// fullscreen then bitdepth must
	// be same as system bitdepth
	// also if 8-bit the a pallete
	// is created and attached
	static const int WINDOWED_APP = 1; // 0 not windowed, 1 windowed

public:
	Game(HWND handle, HINSTANCE instance);

	void Init();
	void Shutdown();
	void Step();

private:
	static const int TEXTSIZE = 128; // size of texture mxm
	static const int NUM_TEXT = 12;  // number of textures

	static const int NUM_LMAP = 4;   // number of light map textures

private:
	BmpImg *_textures[NUM_TEXT],	// holds our texture library 
		   *_lightmaps[NUM_LMAP],	// holds our light map textures
		   *_temp_text;				// temporary working texture

	HWND main_window_handle; // save the window handle
	HINSTANCE main_instance; // save the instance
	char buffer[256];        // used to print text

}; // Game

}