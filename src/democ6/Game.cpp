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
#include "ZBuffer.h"

namespace t3d {

// terrain defines
#define TERRAIN_WIDTH         4000
#define TERRAIN_HEIGHT        4000
#define TERRAIN_SCALE         700
#define MAX_SPEED             20

#define PITCH_RETURN_RATE (.33) // how fast the pitch straightens out
#define PITCH_CHANGE_RATE (.02) // the rate that pitch changes relative to height
#define CAM_HEIGHT_SCALER (.3)  // percentage cam height changes relative to height
#define VELOCITY_SCALER (.025)  // rate velocity changes with height
#define CAM_DECEL       (.25)   // deceleration/friction

// physical model defines play with these to change the feel of the vehicle
float gravity    = -.40;    // general gravity
float vel_y      = 0;       // the y velocity of camera/jeep
float cam_speed  = 0;       // speed of the camera/jeep
float sea_level  = 50;      // sea level of the simulation
float gclearance = 75;      // clearance from the camera to the ground
float neutral_pitch = 10;   // the neutral pitch of the camera

Game::Game(HWND handle, HINSTANCE instance)
	: _cam(NULL)
	, main_window_handle(handle)
	, main_instance(instance)
	, zbuffer(NULL)
{
	_list = new RenderList;

	obj_terrain = new RenderObject;

	zbuffer = new ZBuffer;

	cam_speed = 0;
}

Game::~Game()
{
	delete zbuffer;
	delete obj_terrain;
	delete _list;
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
					  40.0,					// near and far clipping planes
					  12000.0,
					  90.0,					// field of view in degrees
					  WINDOW_WIDTH,			// size of final screen viewport
					  WINDOW_HEIGHT);


	// filenames of objects to load
	char *object_filenames[NUM_OBJECTS] = { 
		"../../assets/chap12/earth01.cob",
		"../../assets/chap12/cube_flat_textured_01.cob",
		"../../assets/chap12/cube_flat_textured_02.cob", 
		"../../assets/chap12/cube_gouraud_textured_01.cob",  
		"../../assets/chap12/cube_gouraud_01.cob",
		"../../assets/chap12/sphere02.cob",
		"../../assets/chap12/sphere03.cob",
	};

	obj_terrain->GenerateTerrain(
		TERRAIN_WIDTH,           // width in world coords on x-axis
		TERRAIN_HEIGHT,          // height (length) in world coords on z-axis
		TERRAIN_SCALE,           // vertical scale of terrain
		"../../assets/chap12/checkerheight01.bmp",  // filename of height bitmap encoded in 256 colors
		"../../assets/chap12/checker256256.bmp",   // filename of texture map
		_RGB32BIT(255,255,255,255),  // color of terrain if no texture        
		&vec4(0, 0, 0, 0),           // initial position
		NULL,                   // initial rotations
		POLY_ATTR_RGB16  
		| POLY_ATTR_SHADE_MODE_FLAT 
		// | POLY_ATTR_SHADE_MODE_GOURAUD
		| POLY_ATTR_SHADE_MODE_TEXTURE);

	// set up lights
	LightsMgr& lights = Modules::GetGraphics().GetLights();
	lights.Reset();

	// create some working colors
	Color white(255,255,255,0),
		  gray(100,100,100,0),
		  black(0,0,0,0),
		  red(255,0,0,0),
		  green(0,255,0,0),
		  blue(0,0,255,0),
		  orange(255,128,0,0),
		  yellow(255,255,0,0);

	// ambient light
	lights.Init(AMBIENT_LIGHT_INDEX,   
				LIGHT_STATE_ON,      // turn the light on
				LIGHT_ATTR_AMBIENT,  // ambient light type
				gray, black, black,    // color for ambient term only
				NULL, NULL,            // no need for pos or dir
				0,0,0,                 // no need for attenuation
				0,0,0);                // spotlight info NA

	vec4 dlight_dir(-1,1,-1,1);

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
				0,.001,0,              // linear attenuation only
				0,0,1);                // spotlight info NA

	// point light 2
	lights.Init(POINT_LIGHT2_INDEX,
				LIGHT_STATE_ON,         // turn the light on
				LIGHT_ATTR_POINT,  // spot light type 2
				black, blue, black,      // color for diffuse term only
				&plight_pos, NULL, // need pos only
				0,.002,0,                 // linear attenuation only
				0,0,1);    


// 	// create lookup for lighting engine
// 	RGB_16_8_IndexedRGB_Table_Builder(DD_PIXEL_FORMAT565,  // format we want to build table for
// 		palette,             // source palette
// 		rgblookup);          // lookup table

	// create the z buffer
	zbuffer->Create(WINDOW_WIDTH, WINDOW_HEIGHT, ZBUFFER_ATTR_32BIT);

// 	// load background sounds
// 	wind_sound_id = DSound_Load_WAV("WIND.WAV");
// 
// 	// start the sounds
// 	DSound_Play(wind_sound_id, DSBPLAY_LOOPING);
// 	DSound_Set_Volume(wind_sound_id, 100);
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
	Modules::GetTimer().Step();

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
	static int perspective_mode = 0;

	static char  *perspective_modes[3] = {"AFFINE", "PERSPECTIVE CORRECT" };

	char work_string[256]; // temp string

	Graphics& graphics = Modules::GetGraphics();

	// start the timing clock
	Modules::GetTimer().Start_Clock();

	// clear the drawing surface 
	graphics.FillSurface(graphics.GetBackSurface(), 0);

 	// draw the sky
 	DrawRectangle(0,0, WINDOW_WIDTH, WINDOW_HEIGHT, graphics.GetColor(50,50,255), graphics.GetBackSurface());

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

		// toggle point light
		if (lights[POINT_LIGHT2_INDEX].state == LIGHT_STATE_ON)
			lights[POINT_LIGHT2_INDEX].state = LIGHT_STATE_OFF;
		else
			lights[POINT_LIGHT2_INDEX].state = LIGHT_STATE_ON;

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
	if (Modules::GetInput().KeyboardState()[DIK_Z])
	{
		// toggle z sorting
		zsort_mode = !zsort_mode;
		Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
	} // end if

	// perspective mode
	if (Modules::GetInput().KeyboardState()[DIK_T])
	{
		if (++perspective_mode > 1)
			perspective_mode=0;
		Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
	} // end if

	// object and camera movement

	// rotate around y axis or yaw
	if (Modules::GetInput().KeyboardState()[DIK_RIGHT])
	{
		_cam->DirRef().y+=5;
	} // end if
	else if (Modules::GetInput().KeyboardState()[DIK_LEFT])
	{
		_cam->DirRef().y-=5;
	} // end if

	if (Modules::GetInput().KeyboardState()[DIK_UP])
	{
		// move forward
		if ( (cam_speed+=1) > MAX_SPEED) cam_speed = MAX_SPEED;
	} // end if

	if (Modules::GetInput().KeyboardState()[DIK_DOWN])
	{
		// move backward
		if ((cam_speed-=1) < -MAX_SPEED) cam_speed = -MAX_SPEED;
	} // end if

	// motion section /////////////////////////////////////////////////////////

	// terrain following, simply find the current cell we are over and then
	// index into the vertex list and find the 4 vertices that make up the 
	// quad cell we are hovering over and then average the values, and based
	// on the current height and the height of the terrain push the player upward

	// the terrain generates and stores some results to help with terrain following
	//ivar1 = columns;
	//GetIVar2() = rows;
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
			_cam->PosRef().y+=(delta*CAM_HEIGHT_SCALER);

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

	static float plight_ang = 0; // angles for light motion

	// use these to rotate objects
	static float x_ang = 0, y_ang = 0, z_ang = 0;

	// move point light source in ellipse around game world
	lights[POINT_LIGHT_INDEX].pos.x = 1000*cos(DEG_TO_RAD(plight_ang));
	lights[POINT_LIGHT_INDEX].pos.y = 100;
	lights[POINT_LIGHT_INDEX].pos.z = 1000*sin(DEG_TO_RAD(plight_ang));

	if ((plight_ang+=3) > 360)
		plight_ang = 0;

	// move spot light source in ellipse around game world
	lights[POINT_LIGHT2_INDEX].pos.x = 500*cos(DEG_TO_RAD(-2*plight_ang));
	lights[POINT_LIGHT2_INDEX].pos.y = 200;
	lights[POINT_LIGHT2_INDEX].pos.z = 1000*sin(DEG_TO_RAD(-2*plight_ang));

	if ((plight_ang-=5) < 0)
		plight_ang = 360;

	// generate camera matrix
	_cam->BuildMatrixEuler(CAM_ROT_SEQ_ZYX);

	// reset the object (this only matters for backface and object removal)
	obj_terrain->Reset();

 	// generate rotation matrix around y axis
	mrot = mat4::Identity();
 
 	// rotate the local coords of the object
 	obj_terrain->Transform(mrot, TRANSFORM_LOCAL_TO_TRANS,true);

	// perform world transform
	obj_terrain->ModelToWorld(TRANSFORM_TRANS_ONLY);

	// insert the object into render list
	_list->Insert(*obj_terrain, false);

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
	}

	// sort the polygon list (hurry up!)
	if (zsort_mode)
		_list->Sort(SORT_POLYLIST_AVGZ);

	// apply camera to perspective transformation
	_list->CameraToPerspective(*_cam);

	// apply screen transform
	_list->PerspectiveToScreen(*_cam);

	// lock the back buffer
	graphics.LockBackSurface();

	// reset number of polys rendered
	debug_polys_rendered_per_frame = 0;

	if (wireframe_mode) {
		_list->DrawWire32(graphics.GetBackBuffer(), graphics.GetBackLinePitch());
	} else 
	{
		RenderContext rc;

		// perspective mode affine texturing
		if (perspective_mode == 0)
		{
			// set up rendering context
			rc.attr =    RENDER_ATTR_INVZBUFFER  
				// | RENDER_ATTR_ALPHA  
				// | RENDER_ATTR_MIPMAP  
				// | RENDER_ATTR_BILERP
				| RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE;

		} // end if
		else // linear piece wise texturing
			if (perspective_mode == 1)
			{
				// set up rendering context
				rc.attr =    RENDER_ATTR_INVZBUFFER  
					// | RENDER_ATTR_ALPHA  
					// | RENDER_ATTR_MIPMAP  
					// | RENDER_ATTR_BILERP
					| RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR;
			} // end if
			else // perspective correct texturing
				if (perspective_mode == 2)
				{
					// set up rendering context
					rc.attr =    RENDER_ATTR_INVZBUFFER  
						// | RENDER_ATTR_ALPHA  
						// | RENDER_ATTR_MIPMAP  
						// | RENDER_ATTR_BILERP
						| RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT;
				} // end if

				// initialize zbuffer to 0 fixed point
				zbuffer->Clear(0 << FIXP16_SHIFT);

				// set up remainder of rendering context
				rc.video_buffer   = graphics.GetBackBuffer();
				rc.lpitch         = graphics.GetBackLinePitch();
				rc.mip_dist       = 4500;
				rc.zbuffer        = (unsigned char*)zbuffer->Buffer();
				rc.zpitch         = WINDOW_WIDTH*4;
				rc.texture_dist   = 0;
				rc.alpha_override = -1;
	
		// render scene
		_list->DrawContext(rc);
	}

	// unlock the back buffer
	graphics.UnlockBackSurface();

	sprintf(work_string,"Lighting [%s]: Ambient=%d, Infinite=%d, Point=%d, BckFceRM [%s], Texture MODE [%s]", 
		(lighting_mode ? "ON" : "OFF"),
		lights[AMBIENT_LIGHT_INDEX].state,
		lights[INFINITE_LIGHT_INDEX].state, 
		lights[POINT_LIGHT_INDEX].state,
		(backface_mode ? "ON" : "OFF"),
		((perspective_modes[perspective_mode])));

	graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34, RGB(0,255,0), graphics.GetBackSurface());

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
		graphics.DrawTextGDI("<W>..............Toggle wire frame/solid mode.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<B>..............Toggle backface removal.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<T>..............Select thru texturing modes.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<H>..............Toggle Help.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<ESC>............Exit demo.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
	} // end help

	sprintf(work_string,"Polys Rendered: %d, Polys lit: %d", debug_polys_rendered_per_frame, debug_polys_lit_per_frame);
	graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16-16, RGB(0,255,0), graphics.GetBackSurface());

	sprintf(work_string,"CAM [%5.2f, %5.2f, %5.2f]",  _cam->Pos().x, _cam->Pos().y, _cam->Pos().z);
	graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16-16-16, RGB(0,255,0), graphics.GetBackSurface());

	sprintf(work_string,"FPS: %d", Modules::GetTimer().FPS());
	graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16-16-16-16, RGB(0,255,0), graphics.GetBackSurface());

	// flip the surfaces
	graphics.Flip(main_window_handle);

	// sync to 30ish fps
	Modules::GetTimer().Wait_Clock(30);

	// check of user is trying to exit
	if (KEY_DOWN(VK_ESCAPE) || Modules::GetInput().KeyboardState()[DIK_ESCAPE])
	{
		PostMessage(main_window_handle, WM_DESTROY,0,0);
	} // end if

}

}
