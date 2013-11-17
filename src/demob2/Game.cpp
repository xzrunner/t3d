#include "Game.h"

#include "Modules.h"
#include "Log.h"
#include "Graphics.h"
#include "Input.h"
#include "Sound.h"
#include "Music.h"
#include "Timer.h"
#include "defines.h"
#include "Camera.h"
#include "PrimitiveDraw.h"
#include "Light.h"
#include "RenderList.h"
#include "RenderObject.h"
#include "BitmapFont.h"
#include "BmpFile.h"
#include "ZBuffer.h"
#include "BmpImg.h"
#include "BOB.h"

namespace t3d {

#define PITCH_RETURN_RATE (.50)   // how fast the pitch straightens out
#define PITCH_CHANGE_RATE (.045)  // the rate that pitch changes relative to height
#define CAM_HEIGHT_SCALER (.3)    // percentage cam height changes relative to height
#define VELOCITY_SCALER   (.03)   // rate velocity changes with height
#define CAM_DECEL         (.25)   // deceleration/friction
#define WATER_LEVEL         40    // level where wave will have effect
#define WAVE_HEIGHT         25    // amplitude of modulation
#define WAVE_RATE        (0.1f)   // how fast the wave propagates

#define MAX_JETSKI_TURN           25    // maximum turn angle
#define JETSKI_TURN_RETURN_RATE   .5    // rate of return for turn
#define JETSKI_TURN_RATE           2    // turn rate
#define JETSKI_YAW_RATE           .1    // yaw rates, return, and max
#define MAX_JETSKI_YAW            10 
#define JETSKI_YAW_RETURN_RATE    .02 

	// game states
#define GAME_STATE_INIT            0
#define GAME_STATE_RESTART         1
#define GAME_STATE_RUN             2
#define GAME_STATE_INTRO           3
#define GAME_STATE_PLAY            4
#define GAME_STATE_EXIT            5

	// delays for introductions
#define INTRO_STATE_COUNT1 (30*9)
#define INTRO_STATE_COUNT2 (30*11)
#define INTRO_STATE_COUNT3 (30*17)

	// player/jetski states
#define JETSKI_OFF                 0 
#define JETSKI_START               1 
#define JETSKI_IDLE                2
#define JETSKI_ACCEL               3 
#define JETSKI_FAST                4


Game::Game(HWND handle, HINSTANCE instance)
	: _cam(NULL)
	, main_window_handle(handle)
	, main_instance(instance)
{
	obj_terrain = new RenderObject;
	obj_player = new RenderObject;
	_list = new RenderList;

	intro_image = new BOB;
	ready_image = new BOB;
	nice_one_image = new BOB;

	zbuffer = new ZBuffer;

	game_state         = GAME_STATE_INIT;
	game_state_count1  = 0;

	player_state       = JETSKI_OFF;
	player_state_count1 = 0;
	enable_model_view   = 0;

	gravity       = -.30;    // general gravity
	vel_y         = 0;       // the y velocity of camera/jetski
	cam_speed     = 0;       // speed of the camera/jetski
	sea_level     = 30;      // sea level of the simulation
	gclearance    = 75;      // clearance from the camera to the ground/water
	neutral_pitch = 1;       // the neutral pitch of the camera
	turn_ang      = 0; 
	jetski_yaw    = 0;

	wave_count    = 0;       // used to track wave propagation
	scroll_count    = 0;
}

Game::~Game()
{
	delete zbuffer;

	delete background_bmp;

	delete intro_image;
	delete ready_image;
	delete nice_one_image;

	delete _list;
	delete obj_player;
	delete obj_terrain;

	delete _cam;
}

void Game::Init()
{
	Modules::Init();

	Modules::GetLog().Init();

	Modules::GetGraphics().Init(main_window_handle,
		WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP, WINDOWED_APP);

	Modules::GetInput().Init(main_instance);
	Modules::GetInput().InitKeyboard(main_window_handle);

	Modules::GetSound().Init(main_window_handle);
//	Modules::GetMusic().Init(main_window_handle);

	// hide the mouse
	if (!WINDOWED_APP)
		ShowCursor(FALSE);

	// seed random number generator
	srand(Modules::GetTimer().Start_Clock());

// 	// initialize math engine
// 	Build_Sin_Cos_Tables();

	// initialize the camera with 90 FOV, normalized coordinates
	_cam = new Camera(CAM_MODEL_EULER,		// the euler model
					  vec4(0,500,-200,1),	// initial camera position
					  vec4(0,0,0,1),		// initial camera angles
					  vec4(0,0,0,1),		// no target
					  10.0,					// near and far clipping planes
					  12000.0,
					  90.0,					// field of view in degrees
					  WINDOW_WIDTH,			// size of final screen viewport
					  WINDOW_HEIGHT);

	vec4 terrain_pos(0,0,0,0);
	obj_terrain->GenerateTerrain(TERRAIN_WIDTH,				// width in world coords on x-axis
								 TERRAIN_HEIGHT,			// height (length) in world coords on z-axis
								 TERRAIN_SCALE,				// vertical scale of terrain
								"../../assets/chap11/water_track_height_04.bmp",  // filename of height bitmap encoded in 256 colors
								"../../assets/chap11/water_track_color_03.bmp",   // filename of texture map
								_RGB32BIT(255,255,255,255), // color of terrain if no texture        
								&terrain_pos,				// initial position
								NULL,						// initial rotations
								POLY_ATTR_RGB16 | 
								POLY_ATTR_SHADE_MODE_FLAT | /* POLY4DV2_ATTR_SHADE_MODE_GOURAUD */
								POLY_ATTR_SHADE_MODE_TEXTURE);

	// we are getting divide by zero errors due to degenerate triangles
	// after projection, this is a problem with the rasterizer that we will
	// fix later, but if we add a random bias to all y components of each vertice
	// is fixes the problem, its a hack, but hey no one is perfect :)
	for (int v = 0; v < obj_terrain->VerticesNum(); v++)
		obj_terrain->GetTransVertexRef(v).y += ((float)RAND_RANGE(-5,5))/10;

	// load the jetski model for player
	obj_player->LoadCOB("../../assets/chap11/jetski05.cob", 
						&vec4(17.5,17.50,17.50, 1),
						&vec4(0,0,150,1),
						&vec4(0,0,0,1),
						VERTEX_FLAGS_SWAP_YZ  |
						VERTEX_FLAGS_TRANSFORM_LOCAL  | VERTEX_FLAGS_INVERT_TEXTURE_V
						/* VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD*/ );

	// set up lights
	LightsMgr& lights = Modules::GetGraphics().GetLights();
	lights.Reset();

	// create some working colors
	Color white(255,255,255,0),
		  gray(100,100,100,0),
		  black(0,0,0,0),
		  red(255,0,0,0),
		  green(0,255,0,0),
		  blue(0,0,255,0);

	// ambient light
	lights.Init(AMBIENT_LIGHT_INDEX,   
				LIGHT_STATE_ON,      // turn the light on
				LIGHT_ATTR_AMBIENT,  // ambient light type
				gray, black, black,    // color for ambient term only
				NULL, NULL,            // no need for pos or dir
				0,0,0,                 // no need for attenuation
				0,0,0);                // spotlight info NA

	vec4 dlight_dir(-1,1,0,1);

	// directional light
	lights.Init(INFINITE_LIGHT_INDEX,  
				LIGHT_STATE_ON,      // turn the light on
				LIGHT_ATTR_INFINITE, // infinite light type
				black, gray, black,    // color for diffuse term only
				NULL, &dlight_dir,     // need direction only
				0,0,0,                 // no need for attenuation
				0,0,0);                // spotlight info NA

	vec4 plight_pos(0,200,0,1);

	// point light
	lights.Init(POINT_LIGHT_INDEX,
				LIGHT_STATE_ON,      // turn the light on
				LIGHT_ATTR_POINT,    // pointlight type
				black, green, black,   // color for diffuse term only
				&plight_pos, NULL,     // need pos only
				0,.002,0,              // linear attenuation only
				0,0,1);                // spotlight info NA

// 	// create lookup for lighting engine
// 	RGB_16_8_IndexedRGB_Table_Builder(DD_PIXEL_FORMAT565,  // format we want to build table for
// 		palette,             // source palette
// 		rgblookup);          // lookup table

	// load background sounds
	Sound& sound = Modules::GetSound();
	wind_sound_id           = sound.LoadWAV("../../assets/chap11/WIND.WAV");
	water_light_sound_id    = sound.LoadWAV("../../assets/chap11/WATERLIGHT01.WAV");
	water_heavy_sound_id    = sound.LoadWAV("../../assets/chap11/WATERHEAVY01.WAV");
	water_splash_sound_id   = sound.LoadWAV("../../assets/chap11/SPLASH01.WAV");
	zbuffer_intro_sound_id  = sound.LoadWAV("../../assets/chap11/ZBUFFER02.WAV");
	zbuffer_instr_sound_id  = sound.LoadWAV("../../assets/chap11/ZINSTRUCTIONS02.WAV");
	jetski_start_sound_id   = sound.LoadWAV("../../assets/chap11/jetskistart04.WAV");
	jetski_idle_sound_id    = sound.LoadWAV("../../assets/chap11/jetskiidle01.WAV");
	jetski_fast_sound_id    = sound.LoadWAV("../../assets/chap11/jetskifast01.WAV");
	jetski_accel_sound_id   = sound.LoadWAV("../../assets/chap11/jetskiaccel01.WAV");
	jetski_splash_sound_id  = sound.LoadWAV("../../assets/chap11/splash01.WAV");

	// load background image
	BmpFile* bitmap = new BmpFile("../../assets/chap11/cloud03.BMP");
	background_bmp = new BmpImg(0,0,800,600,32);
	background_bmp->LoadImage32(*bitmap,0,0,BITMAP_EXTRACT_MODE_ABS);
	delete bitmap;

	// load in all the text images

	// intro
	bitmap = new BmpFile("../../assets/chap11/zbufferwr_intro01.BMP");
	intro_image->Create(WINDOW_WIDTH/2 - 560/2,40,560,160,1, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 32);
	intro_image->LoadFrame(bitmap, 0,0,0,BITMAP_EXTRACT_MODE_ABS);
	delete bitmap;

	// get ready
	bitmap = new BmpFile("../../assets/chap11/zbufferwr_ready01.BMP");
	ready_image->Create(WINDOW_WIDTH/2 - 596/2,40,596,244,1, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 32);
	ready_image->LoadFrame(bitmap, 0,0,0,BITMAP_EXTRACT_MODE_ABS);
	delete bitmap;

	// nice one
	bitmap = new BmpFile("../../assets/chap11/zbufferwr_nice01.BMP");
	nice_one_image->Create(WINDOW_WIDTH/2 - 588/2,40,588,92,1, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 32);
	nice_one_image->LoadFrame(bitmap, 0,0,0,BITMAP_EXTRACT_MODE_ABS);
	delete bitmap;

	// allocate memory for zbuffer
	zbuffer->Create(WINDOW_WIDTH, WINDOW_HEIGHT, ZBUFFER_ATTR_32BIT);
}

void Game::Shutdown()
{
	delete _cam;

	Modules::GetSound().StopAllSounds();
	Modules::GetSound().DeleteAllSounds();
	Modules::GetSound().Shutdown();

// 	Modules::GetMusic().DeleteAllMIDI();
// 	Modules::GetMusic().Shutdown();

	Modules::GetInput().ReleaseKeyboard();
	Modules::GetInput().Shutdown();

	Modules::GetGraphics().Shutdown();

	Modules::GetLog().Shutdown();
}

void Game::Step()
{
	// todo zz
	int debug_polys_rendered_per_frame = 0;
	int debug_polys_lit_per_frame = 0;

	static mat4 mrot;   // general rotation matrix

	// state variables for different rendering modes and help
	static bool wireframe_mode = false;
	static bool backface_mode  = true;
	static bool lighting_mode  = true;
	static bool help_mode      = true;
	static bool zsort_mode     = true;
	static bool x_clip_mode    = true;
	static bool y_clip_mode    = true;
	static bool z_clip_mode    = true;
	static bool z_buffer_mode  = true;
	static bool display_mode   = false;

	static int nice_one_on = 0; // used to display the nice one text
	static int nice_count1 = 0;

	char work_string[256]; // temp string

	Graphics& graphics = Modules::GetGraphics();
	Sound& sound = Modules::GetSound();

	// what state is the game in?
	switch(game_state)
	{
	case GAME_STATE_INIT:
		{
			// perform any important initializations

			// transition to restart
			game_state = GAME_STATE_RESTART;

		} break;

	case GAME_STATE_RESTART:
		{
			// reset all variables
			game_state_count1   = 0;
			player_state        = JETSKI_OFF;
			player_state_count1 = 0;
			gravity             = -.30;    
			vel_y               = 0;       
			cam_speed           = 0;       
			sea_level           = 30;      
			gclearance          = 75;      
			neutral_pitch       = 10;   
			turn_ang            = 0; 
			jetski_yaw          = 0;
			wave_count          = 0;
			scroll_count        = 0;
			enable_model_view   = 0;

			// set camera high atop mount aleph one
			_cam->PosRef().x = 1550;
			_cam->PosRef().y = 800;
			_cam->PosRef().z = 1950;
			_cam->DirRef().y = -140;      

			// turn on water sounds
			// start the sounds
// 			sound.Play(wind_sound_id, DSBPLAY_LOOPING);
// 			sound.SetVolume(wind_sound_id, 75);
// 
// 			sound.Play(water_light_sound_id, DSBPLAY_LOOPING);
// 			sound.SetVolume(water_light_sound_id, 50);
// 
// 			sound.Play(water_heavy_sound_id, DSBPLAY_LOOPING);
// 			sound.SetVolume(water_heavy_sound_id, 30);
// 
// 			sound.Play(zbuffer_intro_sound_id);
// 			sound.SetVolume(zbuffer_intro_sound_id, 100);

			// transition to introduction sub-state of run
			game_state = GAME_STATE_INTRO;

		} break;

		// in any of these cases enter into the main simulation loop
	case GAME_STATE_RUN:   
	case GAME_STATE_INTRO: 
	case GAME_STATE_PLAY:
		{  

			// perform sub-state transition logic here
			if (game_state == GAME_STATE_INTRO)
			{
				// update timer
				++game_state_count1;

				// test for first part of intro
				if (game_state_count1 == INTRO_STATE_COUNT1)
				{
					// change amplitude of water
// 					sound.SetVolume(water_light_sound_id, 50);
// 					sound.SetVolume(water_heavy_sound_id, 100);

					// reposition camera in water
					_cam->PosRef().x = 444; // 560;
					_cam->PosRef().y = 200;
					_cam->PosRef().z = -534; // -580;
					_cam->DirRef().y = 45;// (-100);  

					// enable model
					enable_model_view = 1;
				} // end if
				else if (game_state_count1 == INTRO_STATE_COUNT2)
				{
					// change amplitude of water
// 					sound.SetVolume(water_light_sound_id, 20);
// 					sound.SetVolume(water_heavy_sound_id, 50);

					// play the instructions
// 					sound.Play(zbuffer_instr_sound_id);
// 					sound.SetVolume(zbuffer_instr_sound_id, 100);

				} // end if
				else if (game_state_count1 == INTRO_STATE_COUNT3)
				{
					// reset counter for other use
					game_state_count1 = 0;

					// change amplitude of water
// 					sound.SetVolume(water_light_sound_id, 30);
// 					sound.SetVolume(water_heavy_sound_id, 70);

					// transition to play state
					game_state = GAME_STATE_PLAY;  

				} // end if

			} // end if

			// start the timing clock
			Modules::GetTimer().Start_Clock();

			// read keyboard and other devices here
			Modules::GetInput().ReadKeyboard();

			// game logic here...

			// reset the render list
			_list->Reset();

			// modes and lights

			// wireframe mode
			if (Modules::GetInput().KeyboardState()[DIK_W])
			{
				// toggle wireframe mode
				wireframe_mode = !wireframe_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// backface removal
			if (Modules::GetInput().KeyboardState()[DIK_B])
			{
				// toggle backface removal
				backface_mode = !backface_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// lighting
			if (Modules::GetInput().KeyboardState()[DIK_L])
			{
				// toggle lighting engine completely
				lighting_mode = !lighting_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			LightsMgr& lights = Modules::GetGraphics().GetLights();

			// toggle ambient light
			if (Modules::GetInput().KeyboardState()[DIK_A])
			{
				// toggle ambient light
				if (lights[AMBIENT_LIGHT_INDEX].state == LIGHT_STATE_ON)
					lights[AMBIENT_LIGHT_INDEX].state = LIGHT_STATE_OFF;
				else
					lights[AMBIENT_LIGHT_INDEX].state = LIGHT_STATE_ON;

				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// toggle infinite light
			if (Modules::GetInput().KeyboardState()[DIK_I])
			{
				// toggle ambient light
				if (lights[INFINITE_LIGHT_INDEX].state == LIGHT_STATE_ON)
					lights[INFINITE_LIGHT_INDEX].state = LIGHT_STATE_OFF;
				else
					lights[INFINITE_LIGHT_INDEX].state = LIGHT_STATE_ON;

				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// toggle point light
			if (Modules::GetInput().KeyboardState()[DIK_P])
			{
				// toggle point light
				if (lights[POINT_LIGHT_INDEX].state == LIGHT_STATE_ON)
					lights[POINT_LIGHT_INDEX].state = LIGHT_STATE_OFF;
				else
					lights[POINT_LIGHT_INDEX].state = LIGHT_STATE_ON;

				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// help menu
			if (Modules::GetInput().KeyboardState()[DIK_H])
			{
				// toggle help menu 
				help_mode = !help_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// z-sorting
			if (Modules::GetInput().KeyboardState()[DIK_S])
			{
				// toggle z sorting
				zsort_mode = !zsort_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// z-sorting
			if (Modules::GetInput().KeyboardState()[DIK_Z])
			{
				// toggle z sorting
				zsort_mode = !zsort_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// display mode
			if (Modules::GetInput().KeyboardState()[DIK_D])
			{
				// toggle display mode
				display_mode = !display_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// PLAYER CONTROL AREA ////////////////////////////////////////////////////////////

			// filter player control if not in PLAY state
			if (game_state==GAME_STATE_PLAY)
			{

				// forward/backward
				if (Modules::GetInput().KeyboardState()[DIK_UP] && player_state >= JETSKI_START)
				{

				     // test for acceleration 
				     if (cam_speed == 0)
				     {
// 					      sound.Play(jetski_accel_sound_id);
// 					      sound.SetVolume(jetski_accel_sound_id, 100);
				     } // end if
				      
				     // move forward
				     if ( (cam_speed+=1) > MAX_SPEED) 
						  cam_speed = MAX_SPEED;
				 } // end if

				/*
				else
				if (keyboard_state[DIK_DOWN])
				   {
				   // move backward
				   if ((cam_speed-=1) < -MAX_SPEED) cam_speed = -MAX_SPEED;
				   } // end if
				*/

				// rotate around y axis or yaw
				if (Modules::GetInput().KeyboardState()[DIK_RIGHT])
				{
					_cam->DirRef().y+=5;

				    if ((turn_ang+=JETSKI_TURN_RATE) > MAX_JETSKI_TURN)
					   turn_ang = MAX_JETSKI_TURN;

				    if ((jetski_yaw-=(JETSKI_YAW_RATE*cam_speed)) < -MAX_JETSKI_YAW)
					   jetski_yaw = -MAX_JETSKI_YAW;

				    // scroll the background
					background_bmp->Scroll(-10);
				} // end if

				if (Modules::GetInput().KeyboardState()[DIK_LEFT])
				{
					_cam->DirRef().y-=5;

				    if ((turn_ang-=JETSKI_TURN_RATE) < -MAX_JETSKI_TURN)
					   turn_ang = -MAX_JETSKI_TURN;

				    if ((jetski_yaw+=(JETSKI_YAW_RATE*cam_speed)) > MAX_JETSKI_YAW)
					   jetski_yaw = MAX_JETSKI_YAW;

				    // scroll the background
					background_bmp->Scroll(10);
				} // end if

				// is player trying to start jetski?
				if ( (player_state == JETSKI_OFF) && Modules::GetInput().KeyboardState()[DIK_RETURN])
				{
				    // start jetski
				    player_state = JETSKI_START;

				    // play start sound
// 				    sound.Play(jetski_start_sound_id);
// 				    sound.SetVolume(jetski_start_sound_id, 100);

				    // play idle in loop at 100%
// 				    sound.Play(jetski_idle_sound_id,DSBPLAY_LOOPING);
// 				    sound.SetVolume(jetski_idle_sound_id, 100);

				    // play fast sound in loop at 0%
// 				    sound.Play(jetski_fast_sound_id,DSBPLAY_LOOPING);
// 				    sound.SetVolume(jetski_fast_sound_id, 0);
				} // end if 

			} // end if controls enabled

			// turn JETSKI straight
			if (turn_ang > JETSKI_TURN_RETURN_RATE) turn_ang-=JETSKI_TURN_RETURN_RATE;
			else if (turn_ang < -JETSKI_TURN_RETURN_RATE) turn_ang+=JETSKI_TURN_RETURN_RATE;
			else turn_ang = 0;

			// turn JETSKI straight
			if (jetski_yaw > JETSKI_YAW_RETURN_RATE) jetski_yaw-=JETSKI_YAW_RETURN_RATE;
			else if (jetski_yaw < -JETSKI_YAW_RETURN_RATE) jetski_yaw+=JETSKI_YAW_RETURN_RATE;
			else jetski_yaw = 0;

			// reset the object (this only matters for backface and object removal)
			obj_terrain->Reset();

			// perform world transform to terrain now, so we can destroy the transformed
			// coordinates with the wave function
			obj_terrain->ModelToWorld(TRANSFORM_LOCAL_TO_TRANS);

			// apply wind effect ////////////////////////////////////////////////////

			// scroll the background
			if (++scroll_count > 5) 
			{
				background_bmp->Scroll(-1);
				scroll_count = 0;
			} // end if

			// wave generator ////////////////////////////////////////////////////////

			// for each vertex in the mesh apply wave modulation if height < water level
			for (int v = 0; v < obj_terrain->VerticesNum(); v++)
			{
				// wave modulate below water level only
				if (obj_terrain->GetTransVertex(v).y < WATER_LEVEL)
					obj_terrain->GetTransVertexRef(v).y+=WAVE_HEIGHT*sin(wave_count+(float)v/(2*(float)obj_terrain->GetIVar2()+0));
			} // end for v

			// increase the wave position in time
			wave_count+=WAVE_RATE;

			// motion section /////////////////////////////////////////////////////////

			// terrain following, simply find the current cell we are over and then
			// index into the vertex list and find the 4 vertices that make up the 
			// quad cell we are hovering over and then average the values, and based
			// on the current height and the height of the terrain push the player upward

			// the terrain generates and stores some results to help with terrain following
			//ivar1 = columns;
			//ivar2 = rows;
			//fvar1 = col_vstep;
			//fvar2 = row_vstep;

			int cell_x = (_cam->Pos().x  + TERRAIN_WIDTH/2) / obj_terrain->GetFVar1();
			int cell_y = (_cam->Pos().z  + TERRAIN_HEIGHT/2) / obj_terrain->GetFVar1();

			static float terrain_height, delta;

			// test if we are on terrain
			if ( (cell_x >=0) && (cell_x < obj_terrain->GetIVar1()) && (cell_y >=0) && (cell_y < obj_terrain->GetIVar2()) )
			{
				// compute vertex indices into vertex list of the current quad
				int v0 = cell_x + cell_y*obj_terrain->GetIVar2();
				int v1 = v0 + 1;
				int v2 = v1 + obj_terrain->GetIVar2();
				int v3 = v0 + obj_terrain->GetIVar2();   

				// now simply index into table 
				terrain_height = 0.25 * (obj_terrain->GetTransVertex(v0).y + obj_terrain->GetTransVertex(v1).y +
										 obj_terrain->GetTransVertex(v2).y + obj_terrain->GetTransVertex(v3).y);

				// compute height difference
				delta = terrain_height - (_cam->Pos().y - gclearance);

				// test for penetration
				if (delta > 0)
				{
					// apply force immediately to camera (this will give it a springy feel)
					vel_y+=(delta * (VELOCITY_SCALER));

					// test for pentration, if so move up immediately so we don't penetrate geometry
					_cam->PosRef().y += (delta*CAM_HEIGHT_SCALER);

					// now this is more of a hack than the physics model :) let move the front
					// up and down a bit based on the forward velocity and the gradient of the 
					// hill
					_cam->DirRef().x -= (delta*PITCH_CHANGE_RATE);

				} // end if

			} // end if

			// decelerate camera
			if (cam_speed > (CAM_DECEL) ) cam_speed-=CAM_DECEL;
			else if (cam_speed < (-CAM_DECEL) ) cam_speed+=CAM_DECEL;
			else cam_speed = 0;

			// force camera to seek a stable orientation
			if (_cam->Dir().x > (neutral_pitch+PITCH_RETURN_RATE)) _cam->DirRef().x -= (PITCH_RETURN_RATE);
			else if (_cam->Dir().x < (neutral_pitch-PITCH_RETURN_RATE)) _cam->DirRef().x += (PITCH_RETURN_RATE);
			else _cam->DirRef().x = neutral_pitch;

			// apply gravity
			vel_y+=gravity;

			// test for absolute sea level and push upward..
			if (_cam->Pos().y < sea_level)
			{ 
				vel_y = 0; 
				_cam->PosRef().y = sea_level;
			} // end if

			// move camera
			_cam->PosRef().x += cam_speed*sin(DEG_TO_RAD(_cam->Dir().y));
			_cam->PosRef().z += cam_speed*cos(DEG_TO_RAD(_cam->Dir().y));
			_cam->PosRef().y += vel_y;

			// move point light source in ellipse around game world
			lights[POINT_LIGHT_INDEX].pos.x = 130*cos(DEG_TO_RAD(_cam->Dir().y));
			lights[POINT_LIGHT_INDEX].pos.y = 50;
			lights[POINT_LIGHT_INDEX].pos.z = 130*sin(DEG_TO_RAD(_cam->Dir().y));

			// position the player object slighty in front of camera and in water
			obj_player->SetWorldPos(_cam->Pos().x + (130+cam_speed*1.75)*sin(DEG_TO_RAD(_cam->Dir().y)),
									_cam->Pos().y - 25 + 7.5*sin(wave_count),
									_cam->Pos().z + (130+cam_speed*1.75)*cos(DEG_TO_RAD(_cam->Dir().y)));

			// sound effect section ////////////////////////////////////////////////////////////

			// make engine sound if player is pressing gas
// 			sound.SetFreq(jetski_fast_sound_id,11000+fabs(cam_speed)*100+delta*8);
// 			sound.SetVolume(jetski_fast_sound_id, fabs(cam_speed)*5);

			// make splash sound if altitude changed enough
			if ( (fabs(delta) > 30) && (cam_speed >= (.75*MAX_SPEED)) && ((rand()%20)==1) ) 
			{
				// play sound
// 				sound.Play(jetski_splash_sound_id);
// 				sound.SetFreq(jetski_splash_sound_id,12000+fabs(cam_speed)*5+delta*2);
// 				sound.SetVolume(jetski_splash_sound_id, fabs(cam_speed)*5);

				// display nice one!
				nice_one_on = 1; 
				nice_count1 = 60;
			} // end if 

			// begin 3D rendering section ///////////////////////////////////////////////////////

			// generate camera matrix
			_cam->BuildMatrixEuler(CAM_ROT_SEQ_ZYX);

			// insert the object into render list
			_list->Insert(*obj_terrain, false);

#if 1
			//MAT_IDENTITY_4X4(&mrot);

			// generate rotation matrix around y axis
			mrot = mat4::RotateX(-_cam->Dir().x*1.5) * mat4::RotateY(_cam->Dir().y+turn_ang) * mat4::RotateZ(jetski_yaw);

			// rotate the local coords of the object
			obj_player->Transform(mrot, TRANSFORM_LOCAL_TO_TRANS,1);

			// perform world transform
			obj_player->ModelToWorld(TRANSFORM_TRANS_ONLY);

			// don't show model unless in play state
			if (enable_model_view)
			{
				// insert the object into render list
				_list->Insert(*obj_player, false);
			} // end if

#endif

			// reset number of polys rendered
			debug_polys_rendered_per_frame = 0;
			debug_polys_lit_per_frame = 0;

			// remove backfaces
			if (backface_mode)
				_list->RemoveBackfaces(*_cam);

			// apply world to camera transform
			_list->WorldToCamera(*_cam);

			// clip the polygons themselves now
			_list->ClipPolys(*_cam, (x_clip_mode ? CLIP_POLY_X_PLANE : 0) | 
									(y_clip_mode ? CLIP_POLY_Y_PLANE : 0) | 
									(z_clip_mode ? CLIP_POLY_Z_PLANE : 0) );

			// light scene all at once 
			if (lighting_mode)
			{
				lights.Transform(_cam->CameraMat(), TRANSFORM_LOCAL_TO_TRANS);
				_list->LightWorld32(*_cam);
			} // end if

			// sort the polygon list (hurry up!)
			if (zsort_mode)
				_list->Sort(SORT_POLYLIST_AVGZ);

			// apply camera to perspective transformation
			_list->CameraToPerspective(*_cam);

			// apply screen transform
			_list->PerspectiveToScreen(*_cam);

			// lock the back buffer
			graphics.LockBackSurface();

			// draw background
			background_bmp->Draw32(graphics.GetBackBuffer(), graphics.GetBackLinePitch(), 0);

			// render the object

			if (wireframe_mode)
			{
				zbuffer->Clear((32000 << FIXP16_SHIFT));
				_list->DrawSolidZB32(graphics.GetBackBuffer(), graphics.GetBackLinePitch(), zbuffer->Buffer(), WINDOW_WIDTH*4);
				_list->DrawWire32(graphics.GetBackBuffer(), graphics.GetBackLinePitch());
			}
			else
			{
				if (z_buffer_mode)
				{
					// initialize zbuffer to 32000 fixed point
					//Mem_Set_QUAD((void *)zbuffer, (32000 << FIXP16_SHIFT), (WINDOW_WIDTH*WINDOW_HEIGHT) ); 
					zbuffer->Clear((32000 << FIXP16_SHIFT));
					_list->DrawSolidZB32(graphics.GetBackBuffer(), graphics.GetBackLinePitch(), zbuffer->Buffer(), WINDOW_WIDTH*4);
				}
				else
				{
					// initialize zbuffer to 32000 fixed point
					_list->DrawSolid32(graphics.GetBackBuffer(), graphics.GetBackLinePitch());
				} 

			} // end if

			// dislay z buffer graphically (sorta)
			if (display_mode)
			{
				// use z buffer visualization mode
				// copy each line of the z buffer into the back buffer and translate each z pixel
				// into a color
				unsigned int* screen_ptr = (unsigned int*)graphics.GetBackBuffer();
				unsigned int* zb_ptr     =  (unsigned int*)zbuffer->Buffer();

				for (int y = 0; y < WINDOW_HEIGHT; y++)
				{
					for (int x = 0; x < WINDOW_WIDTH; x++)
					{
						// z buffer is 32 bit, so be carefull
						unsigned int zpixel = zb_ptr[x + y*WINDOW_WIDTH];
						zpixel = (zpixel/4096 & 0xffff);
						screen_ptr[x + y* (graphics.GetBackLinePitch() >> 2)] = (unsigned int)zpixel;
					} // end for
				} // end for y
			} // end if

			// unlock the back buffer
			graphics.UnlockBackSurface();

			// draw overlay text
			if (game_state == GAME_STATE_INTRO &&  game_state_count1 < INTRO_STATE_COUNT1)
			{
				intro_image->Draw(graphics.GetBackSurface());
			} // end if
			else if (game_state == GAME_STATE_INTRO &&  game_state_count1 > INTRO_STATE_COUNT2)
			{
				ready_image->Draw(graphics.GetBackSurface());
			} // end if

			// check for nice one display
			if (nice_one_on == 1 &&  game_state == GAME_STATE_PLAY)
			{
				// are we done displaying?
				if (--nice_count1 <=0)
					nice_one_on = 0;

				nice_one_image->Draw(graphics.GetBackSurface());
			} // end if draw nice one

			// draw statistics 
			sprintf(work_string,"Lighting [%s]: Ambient=%d, Infinite=%d, Point=%d, BckFceRM [%s], Zsort [%s], Zbuffer [%s]", 
				(lighting_mode ? "ON" : "OFF"),
				lights[AMBIENT_LIGHT_INDEX].state,
				lights[INFINITE_LIGHT_INDEX].state, 
				lights[POINT_LIGHT_INDEX].state,
				(backface_mode ? "ON" : "OFF"),
				(zsort_mode ? "ON" : "OFF"),
				(z_buffer_mode ? "ON" : "OFF"));

			graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16, RGB(0,255,0), graphics.GetBackSurface());

			// draw instructions
			graphics.DrawTextGDI("Press ESC to exit. Press <H> for Help.", 0, 0, RGB(0,255,0), graphics.GetBackSurface());

			// should we display help
			int text_y = 16;
			if (help_mode)
			{
				// draw help menu
				graphics.DrawTextGDI("<A>..............Toggle ambient light source.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<I>..............Toggle infinite light source.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<P>..............Toggle point light source.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<N>..............Next object.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<W>..............Toggle wire frame/solid mode.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<B>..............Toggle backface removal.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<S>..............Toggle Z sorting.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<Z>..............Toggle Z buffering.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<D>..............Toggle Normal 3D display / Z buffer visualization mode.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<H>..............Toggle Help.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<ESC>............Exit demo.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());

			} // end help

			sprintf(work_string,"Polys Rendered: %d, Polys lit: %d", debug_polys_rendered_per_frame, debug_polys_lit_per_frame);
			graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16-16, RGB(0,255,0), graphics.GetBackSurface());

			sprintf(work_string,"CAM [%5.2f, %5.2f, %5.2f @ %5.2f]",  _cam->Pos().x, _cam->Pos().y, _cam->Pos().z, _cam->Dir().y);
			graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16-16-16, RGB(0,255,0), graphics.GetBackSurface());

			// flip the surfaces
			graphics.Flip(main_window_handle);

			// sync to 30ish fps
			Modules::GetTimer().Wait_Clock(30);

			// check of user is trying to exit
			if (KEY_DOWN(VK_ESCAPE) || Modules::GetInput().KeyboardState()[DIK_ESCAPE])
			{
				// player is requesting an exit, fine!
				game_state = GAME_STATE_EXIT;
				PostMessage(main_window_handle, WM_DESTROY,0,0);
			} // end if

				// end main simulation loop
		} break;

	case GAME_STATE_EXIT:
		{
			// do any cleanup here, and exit

		} break;

	default: break;

} // end switch game_state

}

}