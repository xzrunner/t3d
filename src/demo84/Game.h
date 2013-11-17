#include <windows.h>
#include <stdio.h>

#include "RenderObject.h"

namespace t3d {

class RenderList;
class Camera;

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
	// defines for the game universe
	static const int  UNIVERSE_RADIUS = 4000;

	static const int  POINT_SIZE = 200;
	static const int  NUM_POINTS_X = (2*UNIVERSE_RADIUS/POINT_SIZE);
	static const int  NUM_POINTS_Z = (2*UNIVERSE_RADIUS/POINT_SIZE);
	static const int  NUM_POINTS = (NUM_POINTS_X*NUM_POINTS_Z);

	// defines for objects
	static const int  NUM_TOWERS = 64;
	static const int  NUM_TANKS = 32;
	static const int  TANK_SPEED = 15;

private:
	Camera* _cam;

	RenderObject _obj_tower,    // used to hold the master tower
				 _obj_tank,     // used to hold the master tank
				 _obj_marker,   // the ground marker
				 _obj_player;   // the player object  
	vec4 _towers[NUM_TOWERS],
		 _tanks[NUM_TANKS];

	RenderList* _list;

	HWND main_window_handle; // save the window handle
	HINSTANCE main_instance; // save the instance
	char buffer[2048];        // used to print text

}; // Game

}