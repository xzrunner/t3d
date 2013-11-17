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

	// initialize a single polygon
	_poly.state  = POLY_STATE_ACTIVE;
	_poly.attr   =  0; 
	_poly.color  = Modules::GetGraphics().GetColor(0,255,0);

	_poly.vlist[0].x = 0;
	_poly.vlist[0].y = 50;
	_poly.vlist[0].z = 0;
	_poly.vlist[0].w = 1;

	_poly.vlist[1].x = 50;
	_poly.vlist[1].y = -50;
	_poly.vlist[1].z = 0;
	_poly.vlist[1].w = 1;

	_poly.vlist[2].x = -50;
	_poly.vlist[2].y = -50;
	_poly.vlist[2].z = 0;
	_poly.vlist[2].w = 1;

	_poly.next = _poly.prev = NULL;

	// initialize the camera with 90 FOV, normalized coordinates
	_cam = new Camera(CAM_MODEL_EULER,
					  vec4(0,0,-100,1),
					  vec4(0,0,0,1),
					  vec4(0,0,0,0),
					  50.0f,
					  500.0f,
					  90.0f,
					  WINDOW_WIDTH,
					  WINDOW_HEIGHT);
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
	static float ang_y = 0;      // rotation angle

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

	// insert polygon into the renderlist
	_list->Insert(_poly);

	// generate rotation matrix around y axis
	mrot = mat4::RotateY(ang_y);

	// rotate polygon slowly
	if (++ang_y >= 360.0) ang_y = 0;

	// rotate the local coords of single polygon in renderlist
	_list->Transform(mrot, TRANSFORM_LOCAL_ONLY);

	// perform local/model to world transform
	_list->ModelToWorld(vec4(0,0,100,1));

	// generate camera matrix
	_cam->BuildMatrixEuler(CAM_ROT_SEQ_ZYX);

	// apply world to camera transform
	_list->WorldToCamera(*_cam);

	// apply camera to perspective transformation
	_list->CameraToPerspective(*_cam);

	// apply screen transform
	_list->PerspectiveToScreen(*_cam);

	// draw instructions
	graphics.DrawTextGDI("Press ESC to exit.", 0, 30, RGB(0,255,0), graphics.GetBackSurface());

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