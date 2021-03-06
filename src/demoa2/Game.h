#include <windows.h>
#include <stdio.h>

namespace t3d {

class Camera;
class RenderList;
class RenderObject;
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

	static const int NUM_OBJECTS = 6; // number of objects system loads

	// terrain defines
	static const int TERRAIN_WIDTH		= 4000;
	static const int TERRAIN_HEIGHT		= 4000;
	static const int TERRAIN_SCALE		= 700;
	static const int MAX_SPEED			= 20;

private:
	Camera* _cam;

	HWND main_window_handle; // save the window handle
	HINSTANCE main_instance; // save the instance
	char buffer[256];        // used to print text

	RenderObject* obj_terrain;		// the terrain object

	RenderList* _list;

	// sounds
	int wind_sound_id, car_sound_id;

	// game imagery assets
	BOB* cockpit;               // the cockpit image

	// physical model defines play with these to change the feel of the vehicle
	float gravity;			// general gravity
	float vel_y;			// the y velocity of camera/jeep
	float cam_speed;		// speed of the camera/jeep
	float sea_level;		// sea level of the simulation
	float gclearance;		// clearance from the camera to the ground
	float neutral_pitch;	// the neutral pitch of the camera

}; // Game

}