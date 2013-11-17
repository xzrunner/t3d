#include <windows.h>
#include <stdio.h>

#include "Vector.h"
#include "BitmapFont.h"
#include "BOB.h"

namespace t3d { class RenderList; class Camera; class Sound; }

namespace raider3d {

class Starfield;
class AlienList;

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
	void RenderEnergyBindings(vec2* bindings,		// array containing binding positions in the form
													// start point, end point, start point, end point...
							  int num_bindings,     // number of energy bindings to render 1-3
							  int num_segments,     // number of segments to randomize bindings into
							  int amplitude,        // amplitude of energy binding
							  int color,            // color of bindings
							  unsigned char* video_buffer,  // video buffer to render
							  int lpitch);

	void RenderWeaponBursts(vec2* burstpoints,    // array containing energy burst positions in the form
												  // start point, end point, start point, end point...
							int num_bursts,
							int num_segments,     // number of segments to randomize bindings into
							int amplitude,        // amplitude of energy binding
							int color,            // color of bindings
							UCHAR *video_buffer,  // video buffer to render
							int lpitch);          // memory pitch of buffer

private:
	// defines for the game universe
	static const int  UNIVERSE_RADIUS = 4000;

	static const int  POINT_SIZE = 100;
	static const int  NUM_POINTS_X = (2*UNIVERSE_RADIUS/POINT_SIZE);
	static const int  NUM_POINTS_Z = (2*UNIVERSE_RADIUS/POINT_SIZE);
	static const int  NUM_POINTS = (NUM_POINTS_X*NUM_POINTS_Z);

	// defines for objects
	static const int  NUM_TOWERS = 64;
	static const int  NUM_TANKS = 32;
	static const int  TANK_SPEED = 15;

	// create some constants for ease of access
	static const int AMBIENT_LIGHT_INDEX	= 0; // ambient light index
	static const int INFINITE_LIGHT_INDEX	= 1; // infinite light index
	static const int POINT_LIGHT_INDEX		= 2; // point light index
	static const int POINT_LIGHT2_INDEX		= 3; // point light index
	static const int SPOT_LIGHT2_INDEX		= 4; // spot light index

	// game state defines
	static const int GAME_STATE_INIT		= 0;  // game initializing for first time
	static const int GAME_STATE_RESTART		= 1;  // game restarting after game over
	static const int GAME_STATE_RUN			= 2;  // game running normally
	static const int GAME_STATE_DEMO		= 3;  // game in demo mode
	static const int GAME_STATE_OVER		= 4;  // game over
	static const int GAME_STATE_EXIT		= 5;  // game state exit

	static const int MAX_MISSES_GAME_OVER   = 25; // miss this many and it's game over man

private:
	// music and sound stuff
	int main_track_id,		// main music track id
		laser_id,			// sound of laser pulse
		explosion_id,		// sound of explosion
		flyby_id,			// sound of ALIEN fighter flying by
		game_over_id,		// game over
		ambient_systems_id; // ambient sounds

	// game imagery assets
	t3d::BOB cockpit;               // the cockpit image
	t3d::BOB starscape;             // the background starscape
	t3d::BOB tech_font;		       // the bitmap font for the engine
	t3d::BOB crosshair;             // the player's cross hair

	t3d::BitmapFont* font;

	Starfield* star_field;

	AlienList* aliens;

	t3d::Camera* _cam;

	t3d::RenderList* _list;

	int game_state;
	int game_state_count1;         // general counters
	int game_state_count2;
	int restart_state;             // state when game is restarting
	int restart_state_count1;      // general counter

	HWND main_window_handle; // save the window handle
	HINSTANCE main_instance; // save the instance
	char buffer[2048];        // used to print text

}; // Game

}