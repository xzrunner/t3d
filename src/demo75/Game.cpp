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

namespace t3d {

Game::Game(HWND handle, HINSTANCE instance)
	: _cam(NULL)
	, main_window_handle(handle)
	, main_instance(instance)
{
	_list = new RenderList;
}

Game::~Game()
{
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
	_cam = new Camera(CAM_MODEL_UVN,
					  vec4(0,0,0,1),
					  vec4(0,0,0,1),
					  vec4(0,0,0,1),
					  50.0f,
					  8000.0f,
					  90.0f,
					  WINDOW_WIDTH,
					  WINDOW_HEIGHT);

	// all your initialization code goes here...
	vec4 vscale(1,1,1,1),		// scale of object
		vpos(0,0,0,1),        // position of object
		vrot(0,0,0,1);        // initial orientation of object

	// load the cube
	_obj.LoadPLG("../../assets/t3d/tank1.plg", &vscale, &vpos, &vrot);

	// set the position of the cube in the world
	_obj.SetWorldPos(0, 0, 0);
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
	static mat4 mrot; // general rotation matrix

	static float x_ang = 0,		// these track the rotation
             y_ang = 5,			// of the object
             z_ang = 0;

	// these are used to create a circling camera
	static float view_angle = 0; 
	static float camera_distance = 1750;
	static vec4 pos(0,0,0,0);

	char work_string[256];

	Graphics& graphics = Modules::GetGraphics();

	int index; // looping var

	// start the timing clock
	Modules::GetTimer().Start_Clock();

	// clear the drawing surface 
	graphics.FillSurface(graphics.GetBackSurface(), 0);

	// read keyboard and other devices here
	Modules::GetInput().ReadKeyboard();

	// game logic here...

	// initialize the renderlist
	_list->Reset();

	// reset angles
	x_ang = 0;
	y_ang = 1;
	z_ang = 0;

	// generate rotation matrix around y axis
	mrot = mat4::RotateX(x_ang) * mat4::RotateY(y_ang) * mat4::RotateZ(z_ang);

	// rotate the local coords of single polygon in renderlist
	_obj.Transform(mrot, TRANSFORM_LOCAL_ONLY, 1);

	// now cull the current object
	strcpy(buffer,"Objects Culled: ");

	for (int x=-NUM_OBJECTS/2; x < NUM_OBJECTS/2; x++)
		for (int z=-NUM_OBJECTS/2; z < NUM_OBJECTS/2; z++)
		{
			// reset the object (this only matters for backface and object removal)
			_obj.Reset();

			// set position of object
			_obj.SetWorldPos(x*OBJECT_SPACING+OBJECT_SPACING/2,
							 0,
							 500+z*OBJECT_SPACING+OBJECT_SPACING/2);

			// attempt to cull object   
			if (!_obj.Cull(*_cam, CULL_OBJECT_XYZ_PLANES))
			{
				// if we get here then the object is visible at this world position
				// so we can insert it into the rendering list
				// perform local/model to world transform
				_obj.ModelToWorld();

				// insert the object into render list
				_list->Insert(_obj);
			}
			else
			{
				sprintf(work_string, "[%d, %d] ", x,z);
				strcat(buffer, work_string);
			}
		}

	graphics.DrawTextGDI(buffer, 0, WINDOW_HEIGHT-20, RGB(0,255,0), graphics.GetBackSurface());

	// compute new camera position, rides on a circular path 
	// with a sinudoid modulation

	vec4& cpos = _cam->PosRef();
	cpos.x = camera_distance * cos(DEG_TO_RAD(view_angle));
	cpos.y = camera_distance * sin(DEG_TO_RAD(view_angle));
	cpos.z = 2*camera_distance * sin(DEG_TO_RAD(view_angle));

	// advance angle to circle around
	if ((view_angle += 1) >= 360)
		view_angle = 0;

	// generate camera matrix
	_cam->BuildMatrixUVN(UVN_MODE_SIMPLE);

	// remove backfaces
	_list->RemoveBackfaces(*_cam);

	// apply world to camera transform
	_list->WorldToCamera(*_cam);

	// apply camera to perspective transformation
	_list->CameraToPerspective(*_cam);

	// apply screen transform
	_list->PerspectiveToScreen(*_cam);

	// draw instructions
	graphics.DrawTextGDI("Press ESC to exit.", 0, 0, RGB(0,255,0), graphics.GetBackSurface());

	// lock the back buffer
	graphics.LockBackSurface();

	// render the polygon list
	_list->DrawWire32(graphics.GetBackBuffer(), graphics.GetBackLinePitch());

	// unlock the back buffer
	graphics.UnlockBackSurface();

	// flip the surfaces
	graphics.Flip(main_window_handle);

	// sync to 30ish fps
	Modules::GetTimer().Wait_Clock(30);

	// check of user is trying to exit
	if (KEY_DOWN(VK_ESCAPE) || 
		Modules::GetInput().KeyboardState()[DIK_ESCAPE])
	{
		PostMessage(main_window_handle, WM_DESTROY,0,0);
	}
}

}