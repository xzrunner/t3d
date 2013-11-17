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

namespace t3d {

Game::Game(HWND handle, HINSTANCE instance)
	: _cam(NULL)
	, main_window_handle(handle)
	, main_instance(instance)
{
	for (size_t i = 0; i < NUM_OBJECTS; ++i)
		obj_array[i] = new RenderObject;
	_list = new RenderList;

	curr_object = 0;
}

Game::~Game()
{
	delete _list;
	for (size_t i = 0; i < NUM_OBJECTS; ++i)
		delete obj_array[i];
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
	_cam = new Camera(CAM_MODEL_EULER, // the euler model
					  vec4(0,0,0,1),  // initial camera position
					  vec4(0,0,0,1),  // initial camera angles
					  vec4(0,0,0,1),      // no target
					  100.0,      // near and far clipping planes
					  1000.0,
					  120.0,      // field of view in degrees
					  WINDOW_WIDTH,   // size of final screen viewport
					  WINDOW_HEIGHT);

	// load in default object
	vec4 vscale(20.00,20.00,20.00,1),
		 vpos(0,0,150,1), 
		 vrot(0,0,0,1);

	// filenames of objects to load
	char *object_filenames[NUM_OBJECTS] = { "../../assets/chap10/cube_flat_01.cob",
											"../../assets/chap10/cube_gouraud_01.cob",
											"../../assets/chap10/cube_flat_textured_01.cob",
											"../../assets/chap10/sphere02.cob",
											"../../assets/chap10/sphere03.cob",
											"../../assets/chap10/hammer03.cob",
	};

	// load all the objects in
	for (int index_obj=0; index_obj < NUM_OBJECTS; index_obj++)
	{
		obj_array[index_obj]->LoadCOB(object_filenames[index_obj],  
									  &vscale, &vpos, &vrot, VERTEX_FLAGS_SWAP_YZ  |
									  VERTEX_FLAGS_TRANSFORM_LOCAL 
									  /* VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD*/ );

	} // end for index_obj

	// set current object
	curr_object = 0;
	obj_work = obj_array[curr_object];

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

	vec4 dlight_dir(-1,0,-1,1);

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

	vec4 slight2_pos(0,200,0,1);
	vec4 slight2_dir(-1,0,-1,1);

	// spot light2
	lights.Init(SPOT_LIGHT2_INDEX,
				LIGHT_STATE_ON,         // turn the light on
				LIGHT_ATTR_SPOTLIGHT2,  // spot light type 2
				black, red, black,      // color for diffuse term only
				&slight2_pos, &slight2_dir, // need pos only
				0,.001,0,                 // linear attenuation only
				0,0,1);    


// 	// create lookup for lighting engine
// 	RGB_16_8_IndexedRGB_Table_Builder(DD_PIXEL_FORMAT565,  // format we want to build table for
// 		palette,             // source palette
// 		rgblookup);          // lookup table
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

	// these are used to create a circling camera
	static float view_angle = 0; 
	static float camera_distance = 6000;
	static vec4 pos(0,0,0,0);
	static float tank_speed;
	static float turning = 0;
	// state variables for different rendering modes and help
	static bool wireframe_mode = false;
	static bool backface_mode  = true;
	static bool lighting_mode  = true;
	static bool help_mode      = true;
	static bool zsort_mode     = true;
	static bool x_clip_mode    = true;
	static bool y_clip_mode    = true;
	static bool z_clip_mode    = true;

	char work_string[256]; // temp string

	Graphics& graphics = Modules::GetGraphics();

	// start the timing clock
	Modules::GetTimer().Start_Clock();

	// clear the drawing surface 
	graphics.FillSurface(graphics.GetBackSurface(), 0);

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

	// clipping
	if (Modules::GetInput().KeyboardState()[DIK_X])
	{
		// toggle x clipping
		x_clip_mode = !x_clip_mode;
		Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
	} // end if

	if (Modules::GetInput().KeyboardState()[DIK_Y])
	{
		// toggle y clipping
		y_clip_mode = !y_clip_mode;
		Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
	} // end if

	if (Modules::GetInput().KeyboardState()[DIK_Z])
	{
		// toggle z clipping
		z_clip_mode = !z_clip_mode;
		Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
	} // end if

	// object and camera movement

	// rotate around y axis or yaw
	if (Modules::GetInput().KeyboardState()[DIK_RIGHT])
	{
		_cam->DirRef().y+=4;
	} // end if

	if (Modules::GetInput().KeyboardState()[DIK_LEFT])
	{
		_cam->DirRef().y-=4;
	} // end if

	if (Modules::GetInput().KeyboardState()[DIK_UP])
	{
		vec4 pos = obj_work->GetWorldPos();
		pos.z += 5;
		obj_work->SetWorldPos(pos.x, pos.y, pos.z);
	} // end if

	if (Modules::GetInput().KeyboardState()[DIK_DOWN])
	{
		vec4 pos = obj_work->GetWorldPos();
		pos.z-=5;
		obj_work->SetWorldPos(pos.x, pos.y, pos.z);
	} // end if

	// move to next object
	if (Modules::GetInput().KeyboardState()[DIK_O])
	{
		if (++curr_object >= NUM_OBJECTS)
			curr_object = 0;

		// update pointer
		obj_work = obj_array[curr_object];
		Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
	} // end if

	static float plight_ang = 0, slight_ang = 0; // angles for light motion

	// move point light source in ellipse around game world
	lights[POINT_LIGHT_INDEX].pos.x = 1000*cos(DEG_TO_RAD(plight_ang));
	lights[POINT_LIGHT_INDEX].pos.y = 100;
	lights[POINT_LIGHT_INDEX].pos.z = 1000*sin(DEG_TO_RAD(plight_ang));

	if ((plight_ang+=3) > 360)
		plight_ang = 0;

	// move spot light source in ellipse around game world
	lights[SPOT_LIGHT2_INDEX].pos.x = 1000*cos(DEG_TO_RAD(slight_ang));
	lights[SPOT_LIGHT2_INDEX].pos.y = 200;
	lights[SPOT_LIGHT2_INDEX].pos.z = 1000*sin(DEG_TO_RAD(slight_ang));

	if ((slight_ang-=5) < 0)
		slight_ang = 360;

	// generate camera matrix
	_cam->BuildMatrixEuler(CAM_ROT_SEQ_ZYX);

	// use these to rotate objects
	static float x_ang = 0, y_ang = 0, z_ang = 0;

	//////////////////////////////////////////////////////////////////////////
	// constant shaded water

	// reset the object (this only matters for backface and object removal)
	obj_work->Reset();

	// generate rotation matrix around y axis
	mrot = mat4::RotateX(x_ang) * mat4::RotateY(y_ang) * mat4::RotateZ(z_ang);

	// rotate the local coords of the object
	obj_work->Transform(mrot, TRANSFORM_LOCAL_TO_TRANS,true);

	// perform world transform
	obj_work->ModelToWorld(TRANSFORM_TRANS_ONLY);

	// insert the object into render list
	_list->Insert(*obj_work, false);

	// reset number of polys rendered
	debug_polys_rendered_per_frame = 0;
	debug_polys_lit_per_frame = 0;

	// update rotation angles
	if ((x_ang+=.2) > 360) x_ang = 0;
	if ((y_ang+=.4) > 360) y_ang = 0;
	if ((z_ang+=.8) > 360) z_ang = 0;

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

	if (wireframe_mode)
		_list->DrawWire32(graphics.GetBackBuffer(), graphics.GetBackLinePitch());
	else
		_list->DrawSolid32(graphics.GetBackBuffer(), graphics.GetBackLinePitch());

	// unlock the back buffer
	graphics.UnlockBackSurface();

	sprintf(work_string,"Lighting [%s]: Ambient=%d, Infinite=%d, Point=%d, Spot=%d | Zsort [%s], BckFceRM [%s]", 
		(lighting_mode ? "ON" : "OFF"),
		lights[AMBIENT_LIGHT_INDEX].state,
		lights[INFINITE_LIGHT_INDEX].state, 
		lights[POINT_LIGHT_INDEX].state,
		lights[SPOT_LIGHT2_INDEX].state,
		(zsort_mode ? "ON" : "OFF"),
		(backface_mode ? "ON" : "OFF"));

	graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34, RGB(0,255,0), graphics.GetBackSurface());

	// draw instructions
	graphics.DrawTextGDI("Press ESC to exit. Press <H> for Help.", 0, 0, RGB(0,255,0), graphics.GetBackSurface());

	sprintf(work_string,"X Clipping [%s], Y Clipping [%s], Z Clipping [%s],  ",
		(x_clip_mode ? "ON" : "OFF") ,
		(y_clip_mode ? "ON" : "OFF") ,
		(z_clip_mode ? "ON" : "OFF") );

	graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16, RGB(0,255,0), graphics.GetBackSurface());

	// should we display help
	int text_y = 16;
	if (help_mode)
	{
		// draw help menu
		graphics.DrawTextGDI("<A>..............Toggle ambient light source.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<I>..............Toggle infinite light source.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<P>..............Toggle point light source.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<S>..............Toggle spot light source.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<W>..............Toggle wire frame/solid mode.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<B>..............Toggle backface removal.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<X>..............Toggle X axis clipping.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<Y>..............Toggle Y axis clipping.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<Z>..............Toggle Z axis clipping.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<O>..............Select next object.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<H>..............Toggle Help.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<ESC>............Exit demo.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());

	} // end help

	sprintf(work_string,"Polys Rendered: %d, Polys lit: %d", debug_polys_rendered_per_frame, debug_polys_lit_per_frame);
	graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16-16, RGB(0,255,0), graphics.GetBackSurface());

	sprintf(work_string,"CAM [%5.2f, %5.2f, %5.2f], OBJ [%5.2f, %5.2f, %5.2f], Near_Z = %5.2f, Far_Z = %5.2f", 
		_cam->Pos().x, _cam->Pos().y, _cam->Pos().z, 
		obj_work->GetWorldPos().x,  obj_work->GetWorldPos().y,  obj_work->GetWorldPos().z,  
		_cam->NearClipZ(), _cam->FarClipZ());

	graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16-16-16, RGB(0,255,0), graphics.GetBackSurface());

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