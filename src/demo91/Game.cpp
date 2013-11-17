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

	// clear the drawing surface 
	Modules::GetGraphics().FillSurface(Modules::GetGraphics().GetBackSurface(), 0);
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
	char work_string[256]; // temp string

	int index; // looping var

	// start the timing clock
	Modules::GetTimer().Start_Clock();

	// read keyboard and other devices here
	Modules::GetInput().ReadKeyboard();

	// game logic here...

	Graphics& graphics = Modules::GetGraphics();

	// lock the back buffer
	graphics.LockBackSurface();

	// draw a randomly positioned gouraud triangle with 3 random vertex colors
	PolygonF face;

	// set the vertices
	face.tvlist[0].x  = (int)RAND_RANGE(0, WINDOW_WIDTH - 1);
	face.tvlist[0].y  = (int)RAND_RANGE(0, WINDOW_HEIGHT - 1);
	face.lit_color[0] = graphics.GetColor(RAND_RANGE(0,255), RAND_RANGE(0,255), RAND_RANGE(0,255));

	face.tvlist[1].x  = (int)RAND_RANGE(0, WINDOW_WIDTH - 1);
	face.tvlist[1].y  = (int)RAND_RANGE(0, WINDOW_HEIGHT - 1);
	face.lit_color[1] = graphics.GetColor(RAND_RANGE(0,255), RAND_RANGE(0,255), RAND_RANGE(0,255));

	face.tvlist[2].x  = (int)(int)RAND_RANGE(0, WINDOW_WIDTH - 1);
	face.tvlist[2].y  = (int)(int)RAND_RANGE(0, WINDOW_HEIGHT - 1);
	face.lit_color[2] = graphics.GetColor(RAND_RANGE(0,255), RAND_RANGE(0,255), RAND_RANGE(0,255));

	// draw the gouraud shaded triangle
	DrawGouraudTriangle32(&face, graphics.GetBackBuffer(), graphics.GetBackLinePitch());
// 	DrawTriangle32(face.tvlist[0].x, face.tvlist[0].y, face.tvlist[1].x, face.tvlist[1].y, face.tvlist[2].x, face.tvlist[2].y,
// 		graphics.GetColor(RAND_RANGE(0,255), RAND_RANGE(0,255), RAND_RANGE(0,255)), graphics.GetBackBuffer(), graphics.GetBackLinePitch());

	// unlock the back buffer
	graphics.UnlockBackSurface();

	// draw instructions
	Modules::GetLog().WriteError("Press ESC to exit.", 0, 0, RGB(0,255,0), graphics.GetBackSurface());

	// flip the surfaces
	graphics.Flip(main_window_handle);

	// sync to 30ish fps
	Modules::GetTimer().Wait_Clock(100);

	// check of user is trying to exit
	if (KEY_DOWN(VK_ESCAPE) || 
		Modules::GetInput().KeyboardState()[DIK_ESCAPE])
	{
		PostMessage(main_window_handle, WM_DESTROY,0,0);
	}
}

}