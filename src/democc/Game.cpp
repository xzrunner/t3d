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
#include "BmpImg.h"
#include "ZBuffer.h"
#include "BOB.h"

namespace t3d {


vec4 scene_objects[500]; // positions of scenery objects
vec4 scene_objects_vel[500]; // velocities of scenery objects

#define CAM_DECEL            (.25)   // deceleration/friction
#define MAX_SPEED             20

static int alpha_override = 0,
		alpha_inc = 10;

Game::Game(HWND handle, HINSTANCE instance)
	: _cam(NULL)
	, main_window_handle(handle)
	, main_instance(instance)
	, zbuffer(NULL)
{
	background = new BOB;

	_list = new RenderList;

	for (size_t i = 0; i < NUM_OBJECTS; ++i)
		obj_array[i] = new RenderObject;
	obj_terrain = new RenderObject;

	zbuffer = new ZBuffer;

	curr_object = 2;

	cam_speed = 0;
}

Game::~Game()
{
	delete zbuffer;
	delete obj_terrain;
	for (size_t i = 0; i < NUM_OBJECTS; ++i)
		delete obj_array[i];
	delete _list;
	delete background;
}

void Game::Init()
{
	Modules::Init();

	Modules::GetLog().Init();

	Modules::GetGraphics().Init(main_window_handle,
		WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP, WINDOWED_APP, false);

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
					  vec4(0,0,0,1),	// initial camera position
					  vec4(0,0,0,1),		// initial camera angles
					  vec4(0,0,0,1),		// no target
					  10.0,					// near and far clipping planes
					  12000.0,
					  120.0,					// field of view in degrees
					  WINDOW_WIDTH,			// size of final screen viewport
					  WINDOW_HEIGHT);

#if 0
VECTOR4D terrain_pos = {0,0,0,0}; 
Generate_Terrain_OBJECT4DV2(&obj_terrain,            // pointer to object
                            TERRAIN_WIDTH,           // width in world coords on x-axis
                            TERRAIN_HEIGHT,          // height (length) in world coords on z-axis
                            TERRAIN_SCALE,           // vertical scale of terrain
                            "checkerheight05.bmp",  // filename of height bitmap encoded in 256 colors
                            "checker256256.bmp",   // filename of texture map
                             RGB16Bit(255,255,255),  // color of terrain if no texture        
                             &terrain_pos,           // initial position
                             NULL,                   // initial rotations
                             POLY4DV2_ATTR_RGB16  
                             | POLY4DV2_ATTR_SHADE_MODE_CONSTANT
                             // | POLY4DV2_ATTR_SHADE_MODE_FLAT 
                             // | POLY4DV2_ATTR_SHADE_MODE_GOURAUD
                             | POLY4DV2_ATTR_SHADE_MODE_TEXTURE);

#endif

	// filenames of objects to load
	char *object_filenames[NUM_OBJECTS] = { "../../assets/chap12/sphere_bw01.cob",
											"../../assets/chap12/cube_constant_textured_01.cob",
											"../../assets/chap12/cube_constant_textured_02.cob",
											"../../assets/chap12/fire_constant_cube01.cob",
											"../../assets/chap12/borg_cube_constant01.cob",
											"../../assets/chap12/sphere_constant_textured_02.cob",
											"../../assets/chap12/earth01.cob",
	};

	// load in default object
	vec4 vscale(TERRAIN_WIDTH,1.00,TERRAIN_HEIGHT,1),
		vpos(0,0,0,1), 
		vrot(0,0,0,1);

	obj_terrain->LoadCOB("../../assets/chap12/plane01.cob",  
							&vscale, &vpos, &vrot, VERTEX_FLAGS_SWAP_YZ  | 
												   VERTEX_FLAGS_TRANSFORM_LOCAL 
												   /* VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD*/,0 );

	vscale.Assign(60.00,60.00,60.00);
	vpos.Assign(0,0,150);

	// load all the objects in
	for (int index_obj=0; index_obj < NUM_OBJECTS; index_obj++)
	{
		obj_array[index_obj]->LoadCOB(object_filenames[index_obj],
									  &vscale, &vpos, &vrot, 
									  VERTEX_FLAGS_SWAP_YZ  | 
									  VERTEX_FLAGS_TRANSFORM_LOCAL 
									  /* VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD*/ );

	} // end for index_obj

	// position the scenery objects randomly
	for (int index = 0; index < NUM_SCENE_OBJECTS; index++)
	{
		// randomly position object
		scene_objects[index].x = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
		scene_objects[index].y = 75; // RAND_RANGE(-(UNIVERSE_RADIUS/2), (UNIVERSE_RADIUS/2));
		scene_objects[index].z = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);

		// select random object, use w to store value
		scene_objects[index].w = RAND_RANGE(0,NUM_OBJECTS-1);
	} // end for

	// select random velocities
	for (int index = 0; index < NUM_SCENE_OBJECTS; index++)
	{
		// randomly position object
		scene_objects_vel[index].x = RAND_RANGE(-MAX_VEL, MAX_VEL);
		scene_objects_vel[index].y = 0; //RAND_RANGE(-MAX_VEL, MAX_VEL);
		scene_objects_vel[index].z = RAND_RANGE(-MAX_VEL, MAX_VEL);
	} // end for

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
				0,.002,0,              // linear attenuation only
				0,0,1);                // spotlight info NA

	vec4 slight2_pos(0,1000,0,1);
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

	// create the z buffer
	zbuffer->Create(WINDOW_WIDTH, WINDOW_HEIGHT, ZBUFFER_ATTR_32BIT);

	// load in the background
	background->Create(0,0,800,600,1, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 32);
	BmpFile* bitmap = new BmpFile("../../assets/chap12/cloud03.bmp");
	background->LoadFrame(bitmap, 0,0,0,BITMAP_EXTRACT_MODE_ABS);
	delete bitmap;
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
	static bool z_buffer_mode   = true;
	static bool display_mode    = true;
	static float turning      = 0;
	static int pass_mode      = 2;

	char work_string[256]; // temp string

	Graphics& graphics = Modules::GetGraphics();

	// start the timing clock
	Modules::GetTimer().Start_Clock();

	// clear the drawing surface 
	graphics.FillSurface(graphics.GetBackSurface(), 0);

 	// draw the sky
 //	DrawRectangle(0,0, WINDOW_WIDTH, WINDOW_HEIGHT*.5, graphics.GetColor(180,180,220), graphics.GetBackSurface());
	graphics.GetBackSurface()->Blt(NULL, background->GetImage(), NULL, DDBLT_WAIT, NULL); 
 
	// read keyboard and other devices here
	Modules::GetInput().ReadKeyboard();

	// game logic here...

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

	// reflection pass mode
	if (Modules::GetInput().KeyboardState()[DIK_N])
	{
		// toggle help menu 
		if (++pass_mode > 2) pass_mode = 0;
		Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
	} // end if

	// z-sorting
	if (Modules::GetInput().KeyboardState()[DIK_S])
	{
		// toggle z sorting
		zsort_mode = !zsort_mode;
		Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
	} // end if

	// z buffer
	if (Modules::GetInput().KeyboardState()[DIK_Z])
	{
		// toggle z buffer
		z_buffer_mode = !z_buffer_mode;
		Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
	} // end if

	// object and camera movement

	// rotate around y axis or yaw
	if (Modules::GetInput().KeyboardState()[DIK_RIGHT])
	{
		_cam->DirRef().y+=3;
		// add a little turn to object
		if ((turning+=2) > 25)
			turning=25;
	} // end if
	else if (Modules::GetInput().KeyboardState()[DIK_LEFT])
	{
		_cam->DirRef().y-=3;
		// add a little turn to object
		if ((turning-=2) < -25)
			turning=-25;
	} // end if
	else // center heading again
	{
		if (turning > 0)
			turning-=1;
		else if (turning < 0)
			turning+=1;
	} // end else

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

	// move to next object
	if (Modules::GetInput().KeyboardState()[DIK_N])
	{
		if (++curr_object >= NUM_OBJECTS)
			curr_object = 0;

		// update pointer
		obj_work = obj_array[curr_object];
		Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
	} // end if

	// decelerate camera
	if (cam_speed > (CAM_DECEL) ) cam_speed-=CAM_DECEL;
	else if (cam_speed < (-CAM_DECEL) ) cam_speed+=CAM_DECEL;
	else cam_speed = 0;

	// move camera
	_cam->PosRef().x += cam_speed*sin(DEG_TO_RAD(_cam->Dir().y));
	_cam->PosRef().y = 200;
	_cam->PosRef().z += cam_speed*cos(DEG_TO_RAD(_cam->Dir().y));

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

	// lock the back buffer
	graphics.LockBackSurface();

	// render the terrain first /////////////////////////////////
	if (pass_mode >= 0)
	{
		// reset the render list
		_list->Reset();

		// rotate the local coords of the object
		mrot = mat4::Identity();
		obj_terrain->Transform(mrot, TRANSFORM_LOCAL_TO_TRANS, 1);

		// perform world transform
		obj_terrain->ModelToWorld(TRANSFORM_TRANS_ONLY);

		// insert the object into render list
		_list->Insert(*obj_terrain, false);

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
			_list->Sort();

		//   // get an identity matrix
		//   MAT_IDENTITY_4X4(&mrot); 
		//   mrot.M11 = -1;

		// transform the rendering list by x-z plane reflection matrix
		_list->Transform(mrot, TRANSFORM_TRANS_ONLY);  

		// apply camera to perspective transformation
		_list->CameraToPerspective(*_cam);

		// apply screen transform
		_list->PerspectiveToScreen(*_cam);

		// reset number of polys rendered
		debug_polys_rendered_per_frame = 0;

		// render the renderinglist
		if (wireframe_mode)
			_list->DrawWire32(graphics.GetBackBuffer(), graphics.GetBackLinePitch());
		else {
			// initialize zbuffer to 16000 fixed point
			zbuffer->Clear(16000 << FIXP16_SHIFT);

			// set up rendering context
			RenderContext rc;
			rc.attr         = RENDER_ATTR_ZBUFFER  
							| RENDER_ATTR_ALPHA  
							//| RENDER_ATTR_MIPMAP  
							//| RENDER_ATTR_BILERP
							| RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE;

			rc.video_buffer   = graphics.GetBackBuffer();
			rc.lpitch         = graphics.GetBackLinePitch();
			rc.mip_dist       = 3500;
			rc.zbuffer        = (unsigned char*)zbuffer->Buffer();
			rc.zpitch         = WINDOW_WIDTH*4;
			rc.texture_dist   = 0;
			rc.alpha_override = -1;

			// render scene
			_list->DrawContext(rc);
		} // end if

	} // end if
	//////////////////////////////////////////////////////////

	if (pass_mode >= 1)
	{
		// render the reflection of the objects

		// reset the render list
		_list->Reset();

		// insert the scenery into universe
		for (int index = 0; index < NUM_SCENE_OBJECTS; index++)
		{
			// select proper object first
			obj_work = obj_array[(int)scene_objects[index].w];

			// reset the object (this only matters for backface and object removal)
			obj_work->Reset();

			// set position of tower
			obj_work->SetWorldPos(scene_objects[index].x, scene_objects[index].y, scene_objects[index].z);

			// move objects
			scene_objects[index].x+=scene_objects_vel[index].x;
			scene_objects[index].y+=scene_objects_vel[index].y;
			scene_objects[index].z+=scene_objects_vel[index].z;

			// test for out of bounds
			if (scene_objects[index].x >= UNIVERSE_RADIUS || scene_objects[index].x <= -UNIVERSE_RADIUS)
			{ 
				scene_objects_vel[index].x=-scene_objects_vel[index].x;
				scene_objects[index].x+=scene_objects_vel[index].x;
			} // end if

			if (scene_objects[index].y >= (UNIVERSE_RADIUS/2) || scene_objects[index].y <= -(UNIVERSE_RADIUS/2))
			{ 
				scene_objects_vel[index].y=-scene_objects_vel[index].y;
				scene_objects[index].y+=scene_objects_vel[index].y;
			} // end if

			if (scene_objects[index].z >= UNIVERSE_RADIUS  || scene_objects[index].z <= -UNIVERSE_RADIUS)
			{ 
				scene_objects_vel[index].z=-scene_objects_vel[index].z;
				scene_objects[index].z+=scene_objects_vel[index].z;
			} // end if

			// attempt to cull object   
			if (!obj_work->Cull(*_cam, CULL_OBJECT_XYZ_PLANES))
			{
				mrot = mat4::Identity();

				// rotate the local coords of the object
				obj_work->Transform(mrot, TRANSFORM_LOCAL_TO_TRANS,1);

				// perform world transform
				obj_work->ModelToWorld(TRANSFORM_TRANS_ONLY);

				// insert the object into render list
				_list->Insert(*obj_work,0);

			} // end if

		} // end for

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

		// get an identity matrix
		mrot = mat4::Identity();
		mrot.c[1][1] = -1; mrot.c[3][1] = -450;

		// transform the rendering list by x-z plane reflection matrix
		_list->Transform(mrot, TRANSFORM_TRANS_ONLY);  

		// apply camera to perspective transformation
		_list->CameraToPerspective(*_cam);

		// apply screen transform
		_list->PerspectiveToScreen(*_cam);

		// reset number of polys rendered
		debug_polys_rendered_per_frame = 0;

		// render the renderinglist
		if (wireframe_mode)
			_list->DrawWire32(graphics.GetBackBuffer(), graphics.GetBackLinePitch());
		else {

			// set up rendering context
			RenderContext rc;

			rc.attr         = RENDER_ATTR_NOBUFFER  
							| RENDER_ATTR_ALPHA  
							//| RENDER_ATTR_MIPMAP  
							//| RENDER_ATTR_BILERP
							| RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE;

			rc.video_buffer   = graphics.GetBackBuffer();
			rc.lpitch         = graphics.GetBackLinePitch();
			rc.mip_dist       = 3500;
			rc.zbuffer        = (unsigned char*)zbuffer->Buffer();
			rc.zpitch         = WINDOW_WIDTH*4;
			rc.texture_dist   = 0;
			rc.alpha_override = 45;

			// render scene
			_list->DrawContext(rc);
		} // end if

	} // end if

	//////////////////////////////////////////////////////////

	if (pass_mode >= 2)
	{
		// render the objects now

		// reset the render list
		_list->Reset();

		// insert the scenery into universe
		for (int index = 0; index < NUM_SCENE_OBJECTS; index++)
		{
			// select proper object first
			obj_work = obj_array[(int)scene_objects[index].w];

			// reset the object (this only matters for backface and object removal)
			obj_work->Reset();

			// set position of tower
			obj_work->SetWorldPos(scene_objects[index].x, scene_objects[index].y, scene_objects[index].z);

			// move objects
			scene_objects[index].x+=scene_objects_vel[index].x;
			scene_objects[index].y+=scene_objects_vel[index].y;
			scene_objects[index].z+=scene_objects_vel[index].z;

			// test for out of bounds
			if (scene_objects[index].x >= UNIVERSE_RADIUS || scene_objects[index].x <= -UNIVERSE_RADIUS)
			{ 
				scene_objects_vel[index].x=-scene_objects_vel[index].x;
				scene_objects[index].x+=scene_objects_vel[index].x;
			} // end if

			if (scene_objects[index].y >= (UNIVERSE_RADIUS/2) || scene_objects[index].y <= -(UNIVERSE_RADIUS/2))
			{ 
				scene_objects_vel[index].y=-scene_objects_vel[index].y;
				scene_objects[index].y+=scene_objects_vel[index].y;
			} // end if

			if (scene_objects[index].z >= UNIVERSE_RADIUS  || scene_objects[index].z <= -UNIVERSE_RADIUS)
			{ 
				scene_objects_vel[index].z=-scene_objects_vel[index].z;
				scene_objects[index].z+=scene_objects_vel[index].z;
			} // end if

			// attempt to cull object   
			if (!obj_work->Cull(*_cam, CULL_OBJECT_XYZ_PLANES))
			{
				mrot = mat4::Identity();

				// rotate the local coords of the object
				obj_work->Transform(mrot, TRANSFORM_LOCAL_TO_TRANS,1);

				// perform world transform
				obj_work->ModelToWorld(TRANSFORM_TRANS_ONLY);

				// insert the object into render list
				_list->Insert(*obj_work,0);

			} // end if

		} // end for

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

		// get an identity matrix
		//MAT_IDENTITY_4X4(&mrot); 
		//mrot.M11 = -1; mrot.M31 = -400;

		// transform the rendering list by x-z plane reflection matrix
		_list->Transform(mrot, TRANSFORM_TRANS_ONLY);  

		// apply camera to perspective transformation
		_list->CameraToPerspective(*_cam);

		// apply screen transform
		_list->PerspectiveToScreen(*_cam);

		// reset number of polys rendered
		debug_polys_rendered_per_frame = 0;

		// render the renderinglist
		if (wireframe_mode)
			_list->DrawWire32(graphics.GetBackBuffer(), graphics.GetBackLinePitch());
		else {

			// set up rendering context
			RenderContext rc;
			rc.attr         = RENDER_ATTR_ZBUFFER  
				| RENDER_ATTR_ALPHA  
				//| RENDER_ATTR_MIPMAP  
				//| RENDER_ATTR_BILERP
				| RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE;

			rc.video_buffer   = graphics.GetBackBuffer();
			rc.lpitch         = graphics.GetBackLinePitch();
			rc.mip_dist       = 3500;
			rc.zbuffer        = (unsigned char*)zbuffer->Buffer();
			rc.zpitch         = WINDOW_WIDTH*4;
			rc.texture_dist   = 0;
			rc.alpha_override = -1;

			// render scene
			_list->DrawContext(rc);
		} // end if

	} // end if

	// unlock the back buffer
	graphics.UnlockBackSurface();

	sprintf(work_string,"Lighting [%s]: Ambient=%d, Infinite=%d, Point=%d, Spot=%d, BckFceRM [%s], Zsort [%s]", 
																					 (lighting_mode ? "ON" : "OFF"),
																					 lights[AMBIENT_LIGHT_INDEX].state,
																					 lights[INFINITE_LIGHT_INDEX].state, 
																					 lights[POINT_LIGHT_INDEX].state,
																					 lights[SPOT_LIGHT2_INDEX].state,
																					 (backface_mode ? "ON" : "OFF"),
																					 (zsort_mode ? "ON" : "OFF"));


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
		graphics.DrawTextGDI("<S>..............Toggle spot light source.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<N>..............Enable next rendering pass.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<W>..............Toggle wire frame/solid mode.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<B>..............Toggle backface removal.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<S>..............Toggle Z sorting.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<H>..............Toggle Help.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<ESC>............Exit demo.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());

		} // end help

	sprintf(work_string,"Polys Rendered: %d, Polys lit: %d", debug_polys_rendered_per_frame, debug_polys_lit_per_frame);
	graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16-16, RGB(0,255,0), graphics.GetBackSurface());

	sprintf(work_string,"CAM [%5.2f, %5.2f, %5.2f]",  _cam->Pos().x, _cam->Pos().y, _cam->Pos().z);

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