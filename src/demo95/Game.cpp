#include "Game.h"

#include "Modules.h"
#include "Log.h"
#include "Graphics.h"
#include "RenderList.h"
#include "Input.h"
#include "Sound.h"
#include "Music.h"
#include "Timer.h"
#include "defines.h"
#include "Camera.h"
#include "tmath.h"
#include "PrimitiveDraw.h"
#include "Light.h"
#include "BmpFile.h"
#include "Camera.h"
#include "BmpImg.h"

#include "context.h"
#include "Starfield.h"
#include "Alien.h"

using namespace t3d;

namespace raider3d {

float cross_x = 0, // cross hairs
	  cross_y = 0; 

int cross_x_screen  = WINDOW_WIDTH/2,   // used for cross hair
	cross_y_screen  = WINDOW_HEIGHT/2, 
	target_x_screen = WINDOW_WIDTH/2,   // used for targeter
	target_y_screen = WINDOW_HEIGHT/2;

float diff_level = 1;	// difficulty level used in velocity and probability calcs

int escaped = 0;			// tracks number of missed ships
int hits = 0;			// tracks number of hits
int score = 0;
int weapon_energy  = 100;// weapon energy
bool weapon_active = false;

Game::Game(HWND handle, HINSTANCE instance)
	: _cam(NULL)
	, main_window_handle(handle)
	, main_instance(instance)
{
	font = new t3d::BitmapFont;
	_cam = NULL;
	_list = new t3d::RenderList;

	star_field = new Starfield;
	aliens = new AlienList;

	game_state = GAME_STATE_INIT;
	game_state_count1    = 0;               // general counters
	game_state_count2    = 0;
	restart_state        = 0;               // state when game is restarting
	restart_state_count1 = 0;               // general counter
}

Game::~Game()
{
	delete _list;
	delete _cam;
	delete font;

	delete aliens;
	delete star_field;
}

void Game::Init()
{
	Modules::Init();

	Modules::GetLog().Init();

	Modules::GetGraphics().Init(main_window_handle,
		WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP, WINDOWED_APP);

	Modules::GetInput().Init(main_instance);
	Modules::GetInput().InitKeyboard(main_window_handle);
	Modules::GetInput().InitMouse(main_window_handle);

	Modules::GetSound().Init(main_window_handle);
//	Modules::GetMusic().Init(main_window_handle);

	explosion_id = Modules::GetSound().LoadWAV("../../assets/chap09/exp1.wav");
	laser_id = Modules::GetSound().LoadWAV("../../assets/chap09/shocker.wav");
	game_over_id = Modules::GetSound().LoadWAV("../../assets/chap09/gameover.wav");
	ambient_systems_id = Modules::GetSound().LoadWAV("../../assets/chap09/stationthrob2.wav");

	// load main music track
//	main_track_id = DMusic_Load_MIDI("midifile2.mid");

	// hide the mouse
//	if (!WINDOWED_APP)
		ShowCursor(FALSE);

	// seed random number generator
	srand(Modules::GetTimer().Start_Clock());

// 	// initialize math engine
// 	Build_Sin_Cos_Tables();

	// initialize the camera with 90 FOV, normalized coordinates
	_cam = new t3d::Camera(CAM_MODEL_EULER,
					  vec4(0,40,0,1),
					  vec4(0,0,0,1),
					  vec4(0,0,0,1),
					  150.0f,
					  20000.0f,
					  120.0f,
					  WINDOW_WIDTH,
					  WINDOW_HEIGHT);

	// set up lights
	LightsMgr& lights = Modules::GetGraphics().GetLights();
	lights.Reset();

	// create some working colors
	Color white(255,255,255,0),
				  light_gray(100,100,100,0),
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

	vec4 dlight_dir(-1,-1,1,0);

	// directional light
	lights.Init(INFINITE_LIGHT_INDEX,  
		LIGHT_STATE_ON,      // turn the light on
		LIGHT_ATTR_INFINITE, // infinite light type
		black, gray, black,    // color for diffuse term only
		NULL, &dlight_dir,     // need direction only
		0,0,0,                 // no need for attenuation
		0,0,0);                // spotlight info NA

	vec4 plight_pos(-2700,-1600,8000,0);

	// point light
	lights.Init(POINT_LIGHT_INDEX,
		LIGHT_STATE_ON,      // turn the light on
		LIGHT_ATTR_POINT,    // pointlight type
		black, green, black,   // color for diffuse term only
		&plight_pos, NULL,     // need pos only
		0,.001,0,              // linear attenuation only
		0,0,1);                // spotlight info NA

	vec4 plight2_pos(0,0,200,0);

	// point light
	lights.Init(POINT_LIGHT2_INDEX,
		LIGHT_STATE_OFF,      // turn the light on
		LIGHT_ATTR_POINT,    // pointlight type
		black, green, black,   // color for diffuse term only
		&plight2_pos, NULL,     // need pos only
		0,.005,0,              // linear attenuation only
		0,0,1);                // spotlight info NA

// 	// create lookup for lighting engine
// 	RGB_16_8_IndexedRGB_Table_Builder(DD_PIXEL_FORMAT565,  // format we want to build table for
// 		palette,             // source palette
// 		rgblookup);          // lookup table

	// load in the cockpit image
	BmpFile* bmpfile;

	cockpit.Create(0,0,800,600,2, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 32);

	bmpfile = new BmpFile("../../assets/chap09/cockpit03.BMP");
	cockpit.LoadFrame(bmpfile, 0,0,0,BITMAP_EXTRACT_MODE_ABS);
	delete bmpfile, bmpfile = NULL;

	bmpfile = new BmpFile("../../assets/chap09/cockpit03b.BMP");
	cockpit.LoadFrame(bmpfile, 1,0,0,BITMAP_EXTRACT_MODE_ABS);
	delete bmpfile, bmpfile = NULL;

	// load in the background starscape(s)
	starscape.Create(0,0,800,600,3, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 32);

	bmpfile = new BmpFile("../../assets/chap09/nebblue01.BMP");
	starscape.LoadFrame(bmpfile, 0,0,0,BITMAP_EXTRACT_MODE_ABS);
	delete bmpfile, bmpfile = NULL;

	bmpfile = new BmpFile("../../assets/chap09/nebred01.BMP");
	starscape.LoadFrame(bmpfile, 1,0,0,BITMAP_EXTRACT_MODE_ABS);
	delete bmpfile, bmpfile = NULL;

	bmpfile = new BmpFile("../../assets/chap09/nebgreen03.BMP");
	starscape.LoadFrame(bmpfile, 2,0,0,BITMAP_EXTRACT_MODE_ABS);
	delete bmpfile, bmpfile = NULL;

	// load the crosshair image
	bmpfile = new BmpFile("../../assets/chap09/crosshair01.BMP");
	crosshair.Create(0,0,CROSS_WIDTH, CROSS_HEIGHT,1, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 32);
	crosshair.LoadFrame(bmpfile, 0,0,0,BITMAP_EXTRACT_MODE_ABS);
	delete bmpfile, bmpfile = NULL;

	// set single precission
	//_control87( _PC_24, MCW_PC );
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
	Modules::GetInput().ReleaseMouse();
	Modules::GetInput().Shutdown();

	Modules::GetGraphics().Shutdown();

	Modules::GetLog().Shutdown();
}

void Game::Step()
{
	// this is the workhorse of your game it will be called
	// continuously in real-time this is like main() in C
	// all the calls for you game go here!

	static mat4 mrot;   // general rotation matrix

	// these are used to create a circling camera
	static float view_angle = 0; 
	static float camera_distance = 6000;
	static vec4 pos = vec4(0,0,0,0);
	static float tank_speed;
	static float turning = 0;
	// state variables for different rendering modes and help
	static bool wireframe_mode = false;
	static bool backface_mode  = true;
	static bool lighting_mode  = true;
	static bool zsort_mode     = true;

	char work_string[256]; // temp string
	int index; // looping var

	Graphics& graphics = Modules::GetGraphics();

	// what state is game in?
	switch(game_state)
	{
	case GAME_STATE_INIT:
		{
			// load the font
			font->Load("../../assets/chap09/tech_char_set_01.bmp", &tech_font);
			// transition to un state
			game_state = GAME_STATE_RESTART;
		} break;

	case GAME_STATE_RESTART:
		{
			// start music
		//	DMusic_Play(main_track_id);
//			Modules::GetSound().Play(ambient_systems_id, DSBPLAY_LOOPING);

			// initialize the stars
			star_field->Init();   

			// initialize all the aliens
			aliens->Init();

			// reset all vars
//			player_z_vel = 4; // virtual speed of viewpoint/ship

			cross_x = CROSS_START_X; // cross hairs
			cross_y = CROSS_START_Y;

			cross_x_screen  = CROSS_START_X;   // used for cross hair
			cross_y_screen  = CROSS_START_Y; 
			target_x_screen = CROSS_START_X;   // used for targeter
			target_y_screen = CROSS_START_Y;

			escaped       = 0;   // tracks number of missed ships
			hits          = 0;   // tracks number of hits
			score         = 0;   // take a guess :)
			weapon_energy = 100; // weapon energy
			weapon_active = false;   // weapons are off

			game_state_count1    = 0;    // general counters
			game_state_count2    = 0;
			restart_state        = 0;    // state when game is restarting
			restart_state_count1 = 0;    // general counter

			// difficulty
			diff_level           = 1;

			// transition to run state
			game_state = GAME_STATE_RUN;

		} break;

	case GAME_STATE_RUN:
	case GAME_STATE_OVER: // keep running sim, but kill diff_level, and player input
		{
			// start the clock
			Modules::GetTimer().Start_Clock();

			// clear the drawing surface 
			//DDraw_Fill_Surface(graphics.GetBackSurface(), 0);

			// blt to destination surface
			graphics.GetBackSurface()->Blt(
				NULL, starscape.GetImage(), NULL, DDBLT_WAIT, NULL); 

			// reset the render list
			_list->Reset();

			// read keyboard and other devices here
			Modules::GetInput().ReadKeyboard();
			Modules::GetInput().ReadMouse();

			// game logic begins here...


#ifdef DEBUG_ON      
			if (Modules::GetInput().KeyboardState()[DIK_UP])
			{
				py+=10;        

			} // end if

			if (Modules::GetInput().KeyboardState()[DIK_DOWN])
			{
				py-=10;     
			} // end if

			if (Modules::GetInput().KeyboardState()[DIK_RIGHT])
			{
				px+=10;
			} // end if

			if (Modules::GetInput().KeyboardState()[DIK_LEFT])
			{
				px-=10;
			} // end if

			if (Modules::GetInput().KeyboardState()[DIK_1])
			{
				pz+=20;

			} // end if

			if (Modules::GetInput().KeyboardState()[DIK_2])
			{
				pz-=20;

			} // end if
#endif

			// modes and lights

			// wireframe mode
			if (Modules::GetInput().KeyboardState()[DIK_W])
			{
				// toggle wireframe mode
				wireframe_mode = !wireframe_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// change nebula
			if (Modules::GetInput().KeyboardState()[DIK_N])
			{
				starscape.Step();
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

			LightsMgr& lights = graphics.GetLights();

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

			// toggle spot light
			if (Modules::GetInput().KeyboardState()[DIK_S])
			{
				// toggle spot light
				if (lights[SPOT_LIGHT2_INDEX].state == LIGHT_STATE_ON)
					lights[SPOT_LIGHT2_INDEX].state = LIGHT_STATE_OFF;
				else
					lights[SPOT_LIGHT2_INDEX].state = LIGHT_STATE_ON;

				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// z-sorting
			if (Modules::GetInput().KeyboardState()[DIK_Z])
			{
				// toggle z sorting
				zsort_mode = !zsort_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// track cross hair
			cross_x+=(Modules::GetInput().MouseState().lX);
			cross_y-=(Modules::GetInput().MouseState().lY); 

			// check bounds (x,y) are in 2D space coords, not screen coords
			if (cross_x >= (WINDOW_WIDTH/2-CROSS_WIDTH/2))
				cross_x = (WINDOW_WIDTH/2-CROSS_WIDTH/2) - 1;
			else
				if (cross_x <= -(WINDOW_WIDTH/2-CROSS_WIDTH/2))
					cross_x = -(WINDOW_WIDTH/2-CROSS_WIDTH/2) + 1;

			if (cross_y >= (WINDOW_HEIGHT/2-CROSS_HEIGHT/2))
				cross_y = (WINDOW_HEIGHT/2-CROSS_HEIGHT/2) - 1;
			else
				if (cross_y <= -(WINDOW_HEIGHT/2-CROSS_HEIGHT/2))
					cross_y = -(WINDOW_HEIGHT/2-CROSS_HEIGHT/2) + 1;

			// player is done moving create camera matrix //////////////////////////
			_cam->BuildMatrixEuler(CAM_ROT_SEQ_ZYX);

			// perform all non-player game AI and motion here /////////////////////

			// move starfield
			star_field->Move();

			// move aliens
			aliens->Update(*_cam);

			// update difficulty of game
			if ((diff_level+=DIFF_RATE) > DIFF_PMAX)
				diff_level = DIFF_PMAX;

			// start a random alien as a function of game difficulty
			if ( (rand()%(DIFF_PMAX - (int)diff_level+2)) == 1)
				aliens->Start();

			// perform animation and transforms on lights //////////////////////////

			// lock the back buffer
			graphics.LockBackSurface();

			// draw all 3D entities
			aliens->DrawAliens(*_cam, *_list);

			// draw mesh explosions
			aliens->DrawExplosions(*_list);

			// entire into final 3D pipeline /////////////////////////////////////// 

			// remove backfaces
			if (backface_mode)
				_list->RemoveBackfaces(*_cam);

			// light scene all at once 
			if (lighting_mode)
				_list->LightWorld32(*_cam);

			// apply world to camera transform
			_list->WorldToCamera(*_cam);

			// sort the polygon list (hurry up!)
			if (zsort_mode)
				_list->Sort(SORT_POLYLIST_AVGZ);

			// apply camera to perspective transformation
			_list->CameraToPerspective(*_cam);

			// apply screen transform
			_list->PerspectiveToScreen(*_cam);

			// draw the starfield now
			star_field->Draw();

			// reset number of polys rendered
//			debug_polys_rendered_per_frame = 0;

			// render the list
			if (!wireframe_mode)
				_list->DrawSolid32(graphics.GetBackBuffer(), graphics.GetBackLinePitch());
			else
				_list->DrawWire32(graphics.GetBackBuffer(), graphics.GetBackLinePitch());

			// draw energy bindings
			RenderEnergyBindings(energy_bindings, 1, 8, 16, graphics.GetColor(0,255,0), 
				graphics.GetBackBuffer(), graphics.GetBackLinePitch());

			// fire the weapon
			if (game_state == GAME_STATE_RUN && Modules::GetInput().MouseState().rgbButtons[0] && weapon_energy > 20)
			{
				// set endpoints of energy bolts
				weapon_bursts[1].x = cross_x_screen + RAND_RANGE(-4,4);
				weapon_bursts[1].y = cross_y_screen + RAND_RANGE(-4,4);

				weapon_bursts[3].x = cross_x_screen + RAND_RANGE(-4,4);
				weapon_bursts[3].y = cross_y_screen + RAND_RANGE(-4,4);

				// draw the weapons firing
				RenderWeaponBursts(weapon_bursts, 2, 8, 16, graphics.GetColor(0,255,0), 
					graphics.GetBackBuffer(), graphics.GetBackLinePitch());

				// decrease weapon energy
				if ((weapon_energy-=5) < 0)
					weapon_energy = 0;

				// make sound
//				Modules::GetSound().Play(laser_id);

				// energize weapons
				weapon_active  = true; 

				// turn the lights on!
				lights[POINT_LIGHT2_INDEX].state = LIGHT_STATE_ON;
				cockpit.SetCurrFrame(1);

			} // end if
			else
			{
				weapon_active  = false; 

				// turn the lights off!
				lights[POINT_LIGHT2_INDEX].state = LIGHT_STATE_OFF;
				cockpit.SetCurrFrame(0);
			} // end else

			if ((weapon_energy+=1) > 100)
				weapon_energy = 100;

			// draw any overlays ///////////////////////////////////////////////////

			// unlock the back buffer
			graphics.UnlockBackSurface();

			// draw cockpit
			cockpit.Draw(graphics.GetBackSurface());

			// draw information
			sprintf(work_string, "SCORE:%d", score);  
			font->Print(&tech_font, TPOS_SCORE_X,TPOS_SCORE_Y, work_string, FONT_HPITCH, FONT_VPITCH, graphics.GetBackSurface());

			sprintf(work_string, "HITS:%d", hits);  
			font->Print(&tech_font, TPOS_HITS_X,TPOS_HITS_Y, work_string, FONT_HPITCH, FONT_VPITCH, graphics.GetBackSurface());

			sprintf(work_string, "ESCAPED:%d", escaped);  
			font->Print(&tech_font, TPOS_ESCAPED_X,TPOS_ESCAPED_Y, work_string, FONT_HPITCH, FONT_VPITCH, graphics.GetBackSurface());

			sprintf(work_string, "LEVEL:%2.4f", diff_level);  
			font->Print(&tech_font, TPOS_GLEVEL_X,TPOS_GLEVEL_Y, work_string, FONT_HPITCH, FONT_VPITCH, graphics.GetBackSurface());

			sprintf(work_string, "VEL:%3.2fK/S", player_z_vel+diff_level);  
			font->Print(&tech_font, TPOS_SPEED_X,TPOS_SPEED_Y, work_string, FONT_HPITCH, FONT_VPITCH, graphics.GetBackSurface());

			sprintf(work_string, "NRG:%d", weapon_energy);  
			font->Print(&tech_font, TPOS_ENERGY_X,TPOS_ENERGY_Y, work_string, FONT_HPITCH, FONT_VPITCH, graphics.GetBackSurface());

			// process game over stuff
			if (game_state == GAME_STATE_OVER)
			{
				// do rendering
				sprintf(work_string, "GAME OVER");  
				font->Print(&tech_font, TPOS_GINFO_X-(FONT_HPITCH/2)*strlen(work_string),TPOS_GINFO_Y, work_string, FONT_HPITCH, FONT_VPITCH, graphics.GetBackSurface());

				sprintf(work_string, "PRESS ENTER TO PLAY AGAIN");  
				font->Print(&tech_font, TPOS_GINFO_X-(FONT_HPITCH/2)*strlen(work_string),TPOS_GINFO_Y + 2*FONT_VPITCH, work_string, FONT_HPITCH, FONT_VPITCH, graphics.GetBackSurface());

				// logic...
				if (Modules::GetInput().KeyboardState()[DIK_RETURN])
				{
					game_state = GAME_STATE_RESTART;
				} // end if

			} // end if

#ifdef DEBUG_ON
			// display diagnostics
			sprintf(work_string,"LE[%s]:AMB=%d,INFL=%d,PNTL=%d,SPTL=%d,ZS[%s],BFRM[%s], WF=%s", 
				((lighting_mode) ? "ON" : "OFF"),
				lights[AMBIENT_LIGHT_INDEX].state,
				lights[INFINITE_LIGHT_INDEX].state, 
				lights[POINT_LIGHT_INDEX].state,
				lights[SPOT_LIGHT2_INDEX].state,
				((zsort_mode) ? "ON" : "OFF"),
				((backface_mode) ? "ON" : "OFF"),
				((wireframe_mode) ? "ON" : "OFF"));

			font->Print(&tech_font, 4,WINDOW_HEIGHT-16, work_string, FONT_HPITCH, FONT_VPITCH, graphics.GetBackSurface());
#endif

			// draw startup information for first few seconds of restart
			if (restart_state == 0)
			{
				sprintf(work_string, "GET READY!");  
				font->Print(&tech_font, TPOS_GINFO_X-(FONT_HPITCH/2)*strlen(work_string),TPOS_GINFO_Y, work_string, FONT_HPITCH, FONT_VPITCH, graphics.GetBackSurface());

				// update counter
				if (++restart_state_count1 > 100)
					restart_state = 1; 
			} // end if

#ifdef DEBUG_ON
			sprintf(work_string, "p=[%d, %d, %d]", px,py,pz);  
			font->Print(&tech_font, TPOS_GINFO_X-(FONT_HPITCH/2)*strlen(work_string),TPOS_GINFO_Y+24, work_string, FONT_HPITCH, FONT_VPITCH, graphics.GetBackSurface());
#endif

			if (game_state == GAME_STATE_RUN)
			{
				// draw cross on top of everything, it's holographic :)
				cross_x_screen = WINDOW_WIDTH/2  + cross_x;
				cross_y_screen = WINDOW_HEIGHT/2 - cross_y;

				crosshair.SetPosition(cross_x_screen - CROSS_WIDTH/2+1, 
					cross_y_screen - CROSS_HEIGHT/2);

				crosshair.Draw(graphics.GetBackSurface());
			} // end if

			// flip the surfaces
			graphics.Flip(main_window_handle);

			// sync to 30ish fps
			Modules::GetTimer().Wait_Clock(30);

			// check if player has lost?
			if (escaped == MAX_MISSES_GAME_OVER)
			{
				game_state = GAME_STATE_OVER;
//				Modules::GetSound().Play(game_over_id);
				escaped++;
			} // end if

			// check of user is trying to exit
			if (KEY_DOWN(VK_ESCAPE) || Modules::GetInput().KeyboardState()[DIK_ESCAPE])
			{
				PostMessage(main_window_handle, WM_DESTROY,0,0);
				game_state = GAME_STATE_EXIT;
			} // end if      


		} break;

	case GAME_STATE_DEMO:
		{


		} break;

	case GAME_STATE_EXIT:
		{


		} break;


	default: break;

	} // end switch game state
}

void Game::RenderEnergyBindings(vec2* bindings, int num_bindings, int num_segments, int amplitude, 
								int color, unsigned char* video_buffer, int lpitch)
{
	// this functions renders the energy bindings across the main exterior energy
	// transmission emitters :) Basically, the function takes two points in 2d then
	// it anchors a line at the two ends and randomly walks from end point to the
	// other by breaking the line into segments and then randomly modulating the y position
	// and amount amplitude +-, maximum number of segments 16
	vec2 segments[17]; // to hold constructed segments after construction

	// render each binding
	for (int index = 0; index < num_bindings; index++)
	{
		// store starting and ending positions

		// starting position
		segments[0] = bindings[index*2];

		// ending position
		segments[num_segments] = bindings[index*2+1];

		// compute vertical gradient, so if y positions of endpoints are
		// greatly different bindings will be modulated using the straight line
		// as a basis
		float dyds = (segments[num_segments].y - segments[0].y) / (float)num_segments;
		float dxds = (segments[num_segments].x - segments[0].x) / (float)num_segments;

		// now build up segments
		for (int sindex = 1; sindex < num_segments; sindex++)
		{
			segments[sindex].x = segments[sindex-1].x + dxds;
			segments[sindex].y = segments[0].y + sindex*dyds + RAND_RANGE(-amplitude, amplitude);
		} // end for segment            

		// draw binding
		for (int sindex = 0; sindex < num_segments; sindex++)
			DrawLine32(segments[sindex].x, segments[sindex].y, 
					   segments[sindex+1].x, segments[sindex+1].y,
					   color, video_buffer, lpitch); 

	} // end for index

} // end Render_Energy_Binding

void Game::RenderWeaponBursts(vec2* burstpoints, int num_bursts, int num_segments, 
							  int amplitude, int color, UCHAR *video_buffer, int lpitch)
{
	// this functions renders the weapon energy bursts from the weapon emitter or wherever
	// function derived from Render_Energy_Binding, but generalized 
	// Basically, the function takes two points in 2d then
	// it anchors a line at the two ends and randomly walks from end point to the
	// other by breaking the line into segments and then randomly modulating the x,y position
	// and amount amplitude +-, maximum number of segments 16

	vec2 segments[17]; // to hold constructed segments after construction

	// render each energy burst
	for (int index = 0; index < num_bursts; index++)
	{
		// store starting and ending positions

		// starting position
		segments[0] = burstpoints[index*2];

		// ending position
		segments[num_segments] = burstpoints[index*2+1];

		// compute horizontal/vertical gradients, so we can modulate the lines 
		// on the proper trajectory
		float dyds = (segments[num_segments].y - segments[0].y) / (float)num_segments;
		float dxds = (segments[num_segments].x - segments[0].x) / (float)num_segments;

		// now build up segments
		for (int sindex = 1; sindex < num_segments; sindex++)
		{
			segments[sindex].x = segments[0].x + sindex*dxds + RAND_RANGE(-amplitude, amplitude);
			segments[sindex].y = segments[0].y + sindex*dyds + RAND_RANGE(-amplitude, amplitude);
		} // end for segment            

		// draw binding
		for (int sindex = 0; sindex < num_segments; sindex++)
			DrawLine32(segments[sindex].x, segments[sindex].y, 
					   segments[sindex+1].x, segments[sindex+1].y,
					   color, video_buffer, lpitch); 

	} // end for index

} // end Render_Weapons_Bursts


}