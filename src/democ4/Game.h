#include <windows.h>
#include <stdio.h>

namespace t3d {

class Camera;
class RenderList;
class RenderObject;
class BOB;
class BmpImg;
class ZBuffer;

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

	static const int NUM_OBJECTS = 6; // number of objects system loads

	// terrain defines
	static const int TERRAIN_WIDTH		= 5000;
	static const int TERRAIN_HEIGHT		= 5000;
	static const int TERRAIN_SCALE		= 700;
	static const int MAX_SPEED			= 20;

private:
	Camera* _cam;

	HWND main_window_handle; // save the window handle
	HINSTANCE main_instance; // save the instance
	char buffer[256];        // used to print text

	int game_state;
	int game_state_count1;

	int player_state;
	int player_state_count1;
	int enable_model_view;

	RenderObject *obj_terrain, *obj_terrain2;		// the terrain object
	RenderObject* obj_player;

	RenderList* _list;

	// ambient and in game sounds
	int wind_sound_id          , 
		water_light_sound_id   ,
		water_heavy_sound_id   ,
		water_splash_sound_id  ,
		zbuffer_intro_sound_id ,
		zbuffer_instr_sound_id ,
		jetski_start_sound_id  ,
		jetski_idle_sound_id   ,
		jetski_fast_sound_id   ,
		jetski_accel_sound_id  ,
		jetski_splash_sound_id ;

	// game imagery assets
	BOB *intro_image, 
		*ready_image, 
		*nice_one_image;

	BmpImg* background_bmp;   // holds the background

	ZBuffer* zbuffer;             // out little z buffer

	// physical model defines play with these to change the feel of the vehicle
	float gravity;			// general gravity
	float vel_y;			// the y velocity of camera/jetski
	float cam_speed;		// speed of the camera/jetski
	float sea_level;		// sea level of the simulation
	float gclearance;		// clearance from the camera to the ground/water
	float neutral_pitch;    // the neutral pitch of the camera
	float turn_ang; 
	float jetski_yaw;

	float wave_count;       // used to track wave propagation
	int scroll_count;

}; // Game

}