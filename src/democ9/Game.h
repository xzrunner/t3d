#include <windows.h>
#include <stdio.h>

#include "Vector.h"

namespace t3d {

class Camera;
class RenderList;
class RenderObject;
class ZBuffer;
class BOB;

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
	static const int AMBIENT_LIGHT_INDEX	= 0; // ambient light index
	static const int INFINITE_LIGHT_INDEX	= 1; // infinite light index
	static const int POINT_LIGHT_INDEX		= 2; // point light index
	static const int SPOT_LIGHT1_INDEX		= 4; // point light index
	static const int SPOT_LIGHT2_INDEX		= 3; // spot light index

	static const int NUM_OBJECTS = 4;			// number of objects system loads
	static const int NUM_SCENE_OBJECTS = 500;   // number of scenery objects
	static const int UNIVERSE_RADIUS = 2000;    // size of universe

private:
	Camera* _cam;

	HWND main_window_handle; // save the window handle
	HINSTANCE main_instance; // save the instance
	char buffer[256];        // used to print text

	BOB* background;

	RenderObject* obj_work;               // pointer to active working object
	RenderObject *obj_array[NUM_OBJECTS]; // array of objects 
	RenderList* _list;

	int curr_object;                  // currently active object index

	ZBuffer* zbuffer;

	float cam_speed;

	// why ?!
//	vec4 scene_objects[500]; // positions of scenery objects

}; // Game

}