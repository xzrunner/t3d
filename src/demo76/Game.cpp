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
	_cam = new Camera(CAM_MODEL_EULER,
					  vec4(0,40,0,1),
					  vec4(0,0,0,1),
					  vec4(0,0,0,1),
					  200.0f,
					  12000.0f,
					  120.0f,
					  WINDOW_WIDTH,
					  WINDOW_HEIGHT);

	// all your initialization code goes here...
	vec4 vscale(0.75f,0.75f,0.75f,1),		// scale of object
		vpos(0,0,0,1),        // position of object
		vrot(0,0,0,1);        // initial orientation of object

	// load the master tank object
	vscale.Assign(0.75f,0.75f,0.75f);
	_obj_tank.LoadPLG("../../assets/t3d/tank2.plg", &vscale, &vpos, &vrot);

	// load player object for 3rd person view
	_obj_player.LoadPLG("../../assets/t3d/tank3.plg", &vscale, &vpos, &vrot);

	// load the master tower object
	vscale.Assign(1, 2, 1);
	_obj_tower.LoadPLG("../../assets/t3d/tower1.plg", &vscale, &vpos, &vrot);

	// load the master ground marker
	vscale.Assign(3, 3, 3);
	_obj_marker.LoadPLG("../../assets/t3d/marker1.plg", &vscale, &vpos, &vrot);

	// position the tanks
	for (int index = 0; index < NUM_TANKS; index++)
	{
		// randomly position the tanks
		_tanks[index].x = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
		_tanks[index].y = 0; // obj_tank.max_radius;
		_tanks[index].z = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
		_tanks[index].w = RAND_RANGE(0,360);
	}
	// position the towers
	for (int index = 0; index < NUM_TOWERS; index++)
	{
		// randomly position the tower
		_towers[index].x = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
		_towers[index].y = 0; // obj_tower.max_radius;
		_towers[index].z = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
	}
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

	// these are used to create a circling camera
	static float view_angle = 0; 
	static float camera_distance = 6000;
	static vec4 pos(0,0,0,0);
	static float tank_speed;
	static float turning = 0;

	char work_string[256];

	Graphics& graphics = Modules::GetGraphics();

	int index; // looping var

	// start the timing clock
	Modules::GetTimer().Start_Clock();

	// clear the drawing surface 
	graphics.FillSurface(graphics.GetBackSurface(), 0);

	// draw the sky
	DrawRectangle(0,0, WINDOW_WIDTH-1, WINDOW_HEIGHT/2, graphics.GetColor(0,140,192), graphics.GetBackSurface());

	// draw the ground
	DrawRectangle(0,WINDOW_HEIGHT/2, WINDOW_WIDTH-1, WINDOW_HEIGHT-1, graphics.GetColor(103,62,3), graphics.GetBackSurface());

	// read keyboard and other devices here
	Modules::GetInput().ReadKeyboard();

	// game logic here...

	// initialize the renderlist
	_list->Reset();

	// allow user to move camera

	// turbo
	if (Modules::GetInput().KeyboardState()[DIK_SPACE])
		tank_speed = 5*TANK_SPEED;
	else
		tank_speed = TANK_SPEED;

	// forward/backward
	if (Modules::GetInput().KeyboardState()[DIK_UP])
	{
		// move forward
		_cam->PosRef().x += tank_speed*sin(DEG_TO_RAD(_cam->Dir().y));
		_cam->PosRef().z += tank_speed*cos(DEG_TO_RAD(_cam->Dir().y));
	}

	if (Modules::GetInput().KeyboardState()[DIK_DOWN])
	{
		// move backward
		_cam->PosRef().x -= tank_speed*sin(DEG_TO_RAD(_cam->Dir().y));
		_cam->PosRef().z -= tank_speed*cos(DEG_TO_RAD(_cam->Dir().y));
	}

	// rotate
	if (Modules::GetInput().KeyboardState()[DIK_RIGHT])
	{
		_cam->DirRef().y+=3;

		// add a little turn to object
		if ((turning+=2) > 15)
			turning=15;

	}
	else if (Modules::GetInput().KeyboardState()[DIK_LEFT])
	{
		_cam->DirRef().y-=3;

		// add a little turn to object
		if ((turning-=2) < -15)
			turning=-15;

	}
	else
	{
		if (turning > 0)
			turning-=1;
		else if (turning < 0)
			turning+=1;
	}

	// generate camera matrix
	_cam->BuildMatrixEuler(CAM_ROT_SEQ_ZYX);

	// insert the tanks in the world
	for (index = 0; index < NUM_TANKS; index++)
	{
		// reset the object (this only matters for backface and object removal)
		_obj_tank.Reset();

		// generate rotation matrix around y axis
		mrot = mat4::RotateY(_tanks[index].w);

		// rotate the local coords of the object
		_obj_tank.Transform(mrot, TRANSFORM_LOCAL_TO_TRANS, 1);

		// set position of tank
		_obj_tank.SetWorldPos(_tanks[index].x,
							  _tanks[index].y,
							  _tanks[index].z);

		// attempt to cull object  
		if (!_obj_tank.Cull(*_cam, CULL_OBJECT_XYZ_PLANES))
		{
			// if we get here then the object is visible at this world position
			// so we can insert it into the rendering list
			// perform local/model to world transform
			_obj_tank.ModelToWorld(TRANSFORM_TRANS_ONLY);

			// insert the object into render list
			_list->Insert(_obj_tank);
		}
	}

	// insert the player into the world
	// reset the object (this only matters for backface and object removal)
	_obj_player.Reset();

	// set position of tank
 	_obj_player.SetWorldPos(_cam->Pos().x+300*sin(DEG_TO_RAD(_cam->Dir().y)),
 							_cam->Pos().y-70,
 							_cam->Pos().z+300*cos(DEG_TO_RAD(_cam->Dir().y)));

	// generate rotation matrix around y axis
	mrot = mat4::RotateY(_cam->Dir().y+turning);

	// rotate the local coords of the object
	_obj_player.Transform(mrot, TRANSFORM_LOCAL_TO_TRANS,0);

	// perform world transform
	_obj_player.ModelToWorld(TRANSFORM_TRANS_ONLY);

	// insert the object into render list
	_list->Insert(_obj_player);

	// insert the towers in the world
	for (int index = 0; index < NUM_TOWERS; index++)
	{
		// reset the object (this only matters for backface and object removal)
		_obj_tower.Reset();

		// set position of tower
		_obj_tower.SetWorldPos(_towers[index].x,
			_towers[index].y, _towers[index].z);

		// attempt to cull object
		if (!_obj_tower.Cull(*_cam, CULL_OBJECT_XYZ_PLANES))
		{
			// if we get here then the object is visible at this world position
			// so we can insert it into the rendering list
			// perform local/model to world transform
			_obj_tower.ModelToWorld();

			// insert the object into render list
			_list->Insert(_obj_tower);
		}
	}

	// seed number generator so that modulation of markers is always the same
	srand(13);

	// insert the ground markers into the world
	for (int index_x = 0; index_x < NUM_POINTS_X; index_x++)
		for (int index_z = 0; index_z < NUM_POINTS_Z; index_z++)
		{
			// reset the object (this only matters for backface and object removal)
			_obj_marker.Reset();

			// set position of tower
			_obj_marker.SetWorldPos(RAND_RANGE(-100,100)-UNIVERSE_RADIUS+index_x*POINT_SIZE,
									_obj_marker.GetMaxRadius(),
									RAND_RANGE(-100,100)-UNIVERSE_RADIUS+index_z*POINT_SIZE);

			// attempt to cull object   
			if (!_obj_marker.Cull(*_cam, CULL_OBJECT_XYZ_PLANES))
			{
				// if we get here then the object is visible at this world position
				// so we can insert it into the rendering list
				// perform local/model to world transform
				_obj_marker.ModelToWorld();

				// insert the object into render list
				_list->Insert(_obj_marker);
			}
		}

	// remove backfaces
	_list->RemoveBackfaces(*_cam);

	// apply world to camera transform
	_list->WorldToCamera(*_cam);

	// apply camera to perspective transformation
	_list->CameraToPerspective(*_cam);

	// apply screen transform
	_list->PerspectiveToScreen(*_cam);

	sprintf(work_string,"pos:[%f, %f, %f] heading:[%f] elev:[%f]", 
		_cam->Pos().x, _cam->Pos().y, _cam->Pos().z, _cam->Dir().y, _cam->Dir().x); 

	graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-20, RGB(0,255,0), graphics.GetBackSurface());

	// draw instructions
	graphics.DrawTextGDI("Press ESC to exit. Press Arrow Keys to Move. Space for TURBO.", 0, 0, RGB(0,255,0), graphics.GetBackSurface());

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