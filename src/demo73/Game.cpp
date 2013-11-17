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

namespace t3d {

Game::Game(HWND handle, HINSTANCE instance)
	: _cam(NULL)
	, main_window_handle(handle)
	, main_instance(instance)
{
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
	_cam = new Camera(CAM_MODEL_EULER,
					  vec4(0,0,0,1),
					  vec4(0,0,0,1),
					  vec4(0,0,0,0),
					  50.0f,
					  500.0f,
					  90.0f,
					  WINDOW_WIDTH,
					  WINDOW_HEIGHT);

	// all your initialization code goes here...
	vec4 vscale(5.0,5.0,5.0,1),  // scale of object
		vpos(0,0,0,1),        // position of object
		vrot(0,0,0,1);        // initial orientation of object

	// load the cube
	_obj.LoadPLG("../../assets/t3d/cube2.plg", &vscale, &vpos, &vrot);

	// set the position of the cube in the world
	_obj.SetWorldPos(0, 0, 100);
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

	static float x_ang = 0, // these track the rotation
             y_ang = 2, // of the object
             z_ang = 0;

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
	_obj.Reset();

	// reset angles
	x_ang = z_ang = 0;

	// is user trying to rotate
	if (KEY_DOWN(VK_DOWN))
		x_ang = 1;
	else
		if (KEY_DOWN(VK_UP))
			x_ang = -1; 
	mrot = mat4::RotateX(x_ang) * mat4::RotateY(y_ang) * mat4::RotateZ(z_ang);

	// rotate the local coords of single polygon in renderlist
	_obj.Transform(mrot, TRANSFORM_LOCAL_ONLY, 1);

	// perform local/model to world transform
	_obj.ModelToWorld();

	// generate camera matrix
	_cam->BuildMatrixEuler(CAM_ROT_SEQ_ZYX);

	// remove backfaces
	_obj.RemoveBackfaces(*_cam);

	// apply world to camera transform
	_obj.WorldToCamera(*_cam);

	// apply camera to perspective transformation
	_obj.CameraToPerspective(*_cam);

	// apply screen transform
	_obj.PerspectiveToScreen(*_cam);

	// draw instructions
	graphics.DrawTextGDI("Press ESC to exit. Use UP ARROW and DOWN ARROW to rotate.", 0, 0, RGB(0,255,0), graphics.GetBackSurface());

	// lock the back buffer
	graphics.LockBackSurface();

	// render the polygon list
	_obj.DrawWire32(graphics.GetBackBuffer(), graphics.GetBackLinePitch());

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