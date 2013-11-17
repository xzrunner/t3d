#include "Game.h"

#include "Modules.h"
#include "Log.h"
#include "Input.h"
#include "Music.h"
#include "Sound.h"
#include "Graphics.h"
#include "Timer.h"
#include "BOB.h"
#include "BmpFile.h"
#include "BmpImg.h"
#include "ZBuffer.h"
#include "Camera.h"
#include "Light.h"
#include "RenderList.h"
#include "RenderObject.h"
#include "BSP.h"

#include "Gadgets.h"
#include "Context.h"
#include "EditTask.h"
#include "BSPFile.h"

namespace t3d {

// hacking stuff...
#define NUM_SCENE_OBJECTS          500    // number of scenery objects 
#define UNIVERSE_RADIUS            2000   // size of universe

// hacking stuff
vec4 scene_objects[NUM_SCENE_OBJECTS]; // positions of scenery objects

// simple movement model defines
#define CAM_DECEL                  (.25)  // deceleration/friction
#define MAX_SPEED                  15     // maximum speed of camera

Game::Game(HWND handle, HINSTANCE instance)
	: main_window_handle(handle)
	, main_instance(instance)
{
	task = new bspeditor::EditTask;
	gadgets = new bspeditor::Gadgets;
	list = new RenderList;
	bspeditor::Context::Instance()->obj_scene = new RenderObject;
	zbuffer = new ZBuffer;
	background_bmp = NULL;
}

Game::~Game()
{
	delete background_bmp;
	delete zbuffer;
	delete list;
	delete gadgets;
	delete task;
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

	// build the inverse cosine lookup
	BuildInverseCosTable(dp_inverse_cos, 360);

	gadgets = new bspeditor::Gadgets;
	gadgets->background->Create(0,0,800,600,1, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 32);
	BmpFile* bitmap = new BmpFile("../../assets/chap13/bspgui02.bmp");
	gadgets->background->LoadFrame(bitmap,0,0,0,BITMAP_EXTRACT_MODE_ABS);
	delete bitmap;

	gadgets->Load();

// 	// load the sounds
// 	beep0_sound_id = DSound_Load_WAV("../menuitem.wav");

	t3d::BOB* textures = bspeditor::Context::Instance()->textures;
	t3d::BmpImg** texturemaps = bspeditor::Context::Instance()->texturemaps;

	textures->Create(0,0,TEXTURE_SIZE,TEXTURE_SIZE,NUM_TEXTURES, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 32);

	// set visibility
	textures->SetAttrBit(BOB_ATTR_VISIBLE);

	// interate and extract both the off and on bitmaps for each animated button
	for (int index = 0; index < NUM_TEXTURES; index++)
	{
		// load the bitmap to extract imagery
		sprintf(buffer, "../../assets/chap13/bspdemotext128_%d.bmp", index);
		bitmap = new BmpFile(buffer);

		// load the frame for the bob
		textures->LoadFrame(bitmap,index,0,0,BITMAP_EXTRACT_MODE_ABS);

		// load in the frame for the bitmap
		texturemaps[index] = new BmpImg(0,0, TEXTURE_SIZE, TEXTURE_SIZE, 32);
		texturemaps[index]->LoadImage32(*bitmap,0,0,BITMAP_EXTRACT_MODE_ABS);

		// unload the bitmap
		delete bitmap;
	} // end for index

	// initialize the camera with 90 FOV, normalized coordinates
	bspeditor::Context::Instance()->cam = new Camera(
		CAM_MODEL_EULER, // the euler model
		vec4(0,0,0,1),        // initial camera position
		vec4(0,0,0,1),        // initial camera angles
		vec4(0,0,0,1),     // no target
		5.0,            // near and far clipping planes
		12000.0,
		120.0,           // field of view in degrees
		WINDOW_WIDTH,    // size of final screen viewport
		WINDOW_HEIGHT
		);

	// set a scaling vector
	vec4 vscale(20.00,20.00,20.00,1);
	vec4 vpos(0,0,150,1);
	vec4 vrot(0,0,0,1);

	// load the object in
	bspeditor::Context::Instance()->obj_scene->LoadCOB("rec_flat_textured_01.cob",
		&vscale, &vpos, &vrot, VERTEX_FLAGS_SWAP_YZ  | VERTEX_FLAGS_INVERT_Z |
		VERTEX_FLAGS_TRANSFORM_LOCAL 
		/* VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD*/,0 );

	// position the scenery objects randomly
	for (int index = 0; index < NUM_SCENE_OBJECTS; index++)
	{
		// randomly position object
		scene_objects[index].x = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
		scene_objects[index].y = RAND_RANGE(-200, 200);
		scene_objects[index].z = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
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
		black, gray, black,   // color for diffuse term only
		&plight_pos, NULL,     // need pos only
		0,.001,0,              // linear attenuation only
		0,0,1);                // spotlight info N

	// point light
	lights.Init(POINT_LIGHT2_INDEX,
		LIGHT_STATE_ON,    // turn the light on
		LIGHT_ATTR_POINT,  // pointlight type
		black, white, black,   // color for diffuse term only
		&plight_pos, NULL,   // need pos only
		0,.002,0,            // linear attenuation only
		0,0,1);              // spotlight info NA

	// create the z buffer
	zbuffer->Create(WINDOW_WIDTH, WINDOW_HEIGHT, ZBUFFER_ATTR_32BIT);

	// reset the editor
	bspeditor::Context::Instance()->Reset();

	// load background image that scrolls 
	bitmap = new BmpFile("../../assets/chap13/red01a.BMP");
	background_bmp = new BmpImg(0,0,800,600,32);
	background_bmp->LoadImage32(*bitmap,0,0,BITMAP_EXTRACT_MODE_ABS);
	delete bitmap;
}

void Game::Shutdown()
{
	Modules::GetSound().StopAllSounds();
	Modules::GetSound().DeleteAllSounds();
	Modules::GetSound().Shutdown();

// 	Modules::GetMusic().DeleteAllMIDI();
// 	Modules::GetMusic().Shutdown();

	Modules::GetInput().ReleaseKeyboard();
	Modules::GetInput().Shutdown();

	Modules::GetGraphics().Shutdown();

	// release the z buffer
	zbuffer->Delete();

	Modules::GetLog().Shutdown();
}

void Game::Step()
{
	Modules::GetTimer().Step();

	// this is the workhorse of your game it will be called
	// continuously in real-time this is like main() in C
	// all the calls for you game go here!
	static int id = -1;

	static mat4 mrot;   // general rotation matrix

	static float plight_ang = 0, 
		         slight_ang = 0; // angles for light motion

	// use these to rotate objects
	static float x_ang = 0, y_ang = 0, z_ang = 0;

	// state variables for different rendering modes and help
	static int wireframe_mode = 1;
	static int backface_mode  = -1;
	static int lighting_mode  = 1;
	static int help_mode      = 1;
	static int zsort_mode     = -1;
	static int x_clip_mode    = 1;
	static int y_clip_mode    = 1;
	static int z_clip_mode    = 1;
	static int z_buffer_mode  = 1;
	static int display_mode   = 1;
	static int bsp_backface_mode   = 1;

	char work_string[256]; // temp string

	int index; // looping var

	bspeditor::Context* context = bspeditor::Context::Instance();

	Graphics& graphics = Modules::GetGraphics();

	DIMOUSESTATE& mouse_state = t3d::Modules::GetInput().MouseState();

	// is user in editor mode or viewing mode?
	switch(context->editor_state)
	{
	case EDITOR_STATE_INIT: /////////////////////////////////////////////////
		{

			// now transition to edit mode
			context->editor_state = EDITOR_STATE_EDIT;
		} break;

	case EDITOR_STATE_EDIT: /////////////////////////////////////////////////
		{
			// start the timing clock
			Modules::GetTimer().Start_Clock();

			// clear the drawing surface 
			//DDraw_Fill_Surface(graphics.GetBackSurface(), 0);
			graphics.GetBackSurface()->Blt(NULL, gadgets->background->GetImage(), NULL, DDBLT_WAIT, NULL);

			// read keyboard and other devices here
			Modules::GetInput().ReadKeyboard();
			//DInput_Read_Mouse();

			// game logic here...

			// find nearest line 
			context->nearest_line = task->DeleteLine(mouse_state.lX, mouse_state.lY,0);

			// is user trying to press left button?
			if (mouse_state.rgbButtons[0] || mouse_state.rgbButtons[2])// && mouse_debounce[0] == 0)
			{
				// determine area click is occuring in
				if (mouse_state.lX > INTERFACE_MIN_X && mouse_state.lX < INTERFACE_MAX_X && 
					mouse_state.lY > INTERFACE_MIN_Y && mouse_state.lY < INTERFACE_MAX_Y)
				{ 
					// check for gadget
					if ((id = gadgets->Process(mouse_state.lX, mouse_state.lY, mouse_state.rgbButtons)) >= 0)
					{
						// play a sound first
						//DSound_Play(beep0_sound_id);

						// process clicks
						switch(id)
						{
						case GADGET_ID_SEGMENT_MODE_ID:
							{
								task->action = ACTION_STARTING_LINE;

							} break;

						case GADGET_ID_POLYLINE_MODE_ID:
							{
								task->action = ACTION_STARTING_POLYLINE; 
							} break;

						case GADGET_ID_DELETE_MODE_ID: 
							{
								task->action = ACTION_DELETE_LINE;
							} break;

						case GADGET_ID_CLEAR_ALL_ID:
							{
								// reset everything
								context->Reset();

							} break;

						case GADGET_ID_FLOOR_MODE_ID:
							{
								task->action = ACTION_FLOOR_MODE;
							} break;

						case GADGET_ID_WALL_TEXTURE_DEC:
							{
								if (--context->texture_index_wall < -1)
									context->texture_index_wall = -1;

							} break;

						case GADGET_ID_WALL_TEXTURE_INC:
							{
								if (++context->texture_index_wall >= NUM_TEXTURES)
									context->texture_index_wall = NUM_TEXTURES-1;

							} break;

						case GADGET_ID_WALL_HEIGHT_DEC:
							{
								if ((context->wall_height-=8) < 0)
									context->wall_height = 0;

							} break;

						case GADGET_ID_WALL_HEIGHT_INC:
							{
								context->wall_height+=8;

							} break;

						case GADGET_ID_FLOOR_TEXTURE_DEC:
							{
								if (--context->texture_index_floor < -1)
									context->texture_index_floor = -1;

							} break;

						case GADGET_ID_FLOOR_TEXTURE_INC:
							{

								if (++context->texture_index_floor >= NUM_TEXTURES)
									context->texture_index_floor = NUM_TEXTURES-1;

							} break;

						default: break;

						} // end switch

					} // end if
					else
					{

					} // end else

				} // end if interface

				// test for bsp area
				if (mouse_state.lX > BSP_MIN_X && mouse_state.lX < BSP_MAX_X && 
					mouse_state.lY > BSP_MIN_Y && mouse_state.lY < BSP_MAX_Y)
				{
					// process bsp editor area click

					// test if this is starting point or endpoint
					if (task->action == ACTION_STARTING_LINE)
					{
						// set starting field, note the offsets to center the
						// starting point in middle of cross hairs

						if (!context->snap_to_grid)
						{
							context->lines[context->total_lines].p0.x = mouse_state.lX;
							context->lines[context->total_lines].p0.y = mouse_state.lY;
						} // end if
						else
						{
							context->lines[context->total_lines].p0.x = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lX / (float)BSP_GRID_SIZE);
							context->lines[context->total_lines].p0.y = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lY / (float)BSP_GRID_SIZE);
						} // end if


						// remember starting line to close contour
						context->starting_line = context->total_lines;

						// set point type to ending point
						task->action = ACTION_ENDING_LINE;

						// wait for mouse button release
						// Wait_For_Mouse_Up();

					} // end if start of wall
					else
						if (task->action == ACTION_ENDING_LINE)
						{
							// check for normal left mouse click
							if (mouse_state.rgbButtons[0])
							{
								// must be the end of a wall or ending point
								if (!context->snap_to_grid)
								{
									context->lines[context->total_lines].p1.x = mouse_state.lX;
									context->lines[context->total_lines].p1.y = mouse_state.lY;
								} // end if
								else
								{
									context->lines[context->total_lines].p1.x = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lX / (float)BSP_GRID_SIZE);
									context->lines[context->total_lines].p1.y = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lY / (float)BSP_GRID_SIZE);
								} // end if

								// set texture id
								context->lines[context->total_lines].texture_id = context->texture_index_wall;

								// set height
								context->lines[context->total_lines].height = context->wall_height;

								// set attributes
								context->lines[context->total_lines].attr   = context->poly_attr;

								// set color
								context->lines[context->total_lines].color  = context->poly_color;

								// set task->action back to starting point
								task->action = ACTION_STARTING_LINE;

								// advance number of context->lines
								if (++context->total_lines >= MAX_LINES)
									context->total_lines = MAX_LINES - 1;

								// wait for mouse button release
								// Wait_For_Mouse_Up();
							} // end if
							else
								// check for right mouse click trying to close contour
								if (mouse_state.rgbButtons[2])
								{
									// reset, user wants to restart segment
									task->action = ACTION_STARTING_LINE;
								} // end if

						} // end else if
						else
							// test if this is starting point or endpoint
							if (task->action == ACTION_STARTING_POLYLINE)
							{
								// set starting field, note the offsets to center the
								// starting point in middle of cross hairs

								if (!context->snap_to_grid)
								{
									context->lines[context->total_lines].p0.x = mouse_state.lX;
									context->lines[context->total_lines].p0.y = mouse_state.lY;
								} // end if
								else
								{
									context->lines[context->total_lines].p0.x = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lX / (float)BSP_GRID_SIZE);
									context->lines[context->total_lines].p0.y = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lY / (float)BSP_GRID_SIZE);
								} // end if

								// remember starting line to close contour
								context->starting_line = context->total_lines;

								// set point type to ending point
								task->action = ACTION_ADDING_POLYLINE;

								// wait for mouse button release
								// Wait_For_Mouse_Up();

							} // end if start of wall
							else
								if (task->action == ACTION_ADDING_POLYLINE)
								{
									// check for normal left mouse click
									if (mouse_state.rgbButtons[0])
									{
										// must be the end of a wall or ending point
										if (!context->snap_to_grid)
										{
											context->lines[context->total_lines].p1.x = mouse_state.lX;
											context->lines[context->total_lines].p1.y = mouse_state.lY;
										} // end if
										else
										{
											context->lines[context->total_lines].p1.x = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lX / (float)BSP_GRID_SIZE);
											context->lines[context->total_lines].p1.y = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lY / (float)BSP_GRID_SIZE);
										} // end if

										// set texture id
										context->lines[context->total_lines].texture_id = context->texture_index_wall;

										// set height
										context->lines[context->total_lines].height = context->wall_height;

										// set attributes
										context->lines[context->total_lines].attr   = context->poly_attr;

										// set color
										context->lines[context->total_lines].color  = context->poly_color;

										// set task->action back to starting point
										task->action = ACTION_ADDING_POLYLINE;

										// advance number of context->lines
										if (++context->total_lines >= MAX_LINES)
											context->total_lines = MAX_LINES - 1;

										// set start point as last endpoint
										context->lines[context->total_lines].p0.x = context->lines[context->total_lines-1].p1.x;
										context->lines[context->total_lines].p0.y = context->lines[context->total_lines-1].p1.y;

										// wait for mouse button release
										// Wait_For_Mouse_Up();
									} // end if
									else
										// check for right mouse click trying to close contour
										if (mouse_state.rgbButtons[2])
										{
											// end the polyline
											task->action = ACTION_STARTING_POLYLINE;
											// Wait_For_Mouse_Up();
										} // end if

								} // end else if
								else
									if (task->action==ACTION_DELETE_LINE)
									{
										// try and delete the line nearest selected point
										task->DeleteLine(mouse_state.lX, mouse_state.lY,1);

										// wait for mouse release
										// Wait_For_Mouse_Up();

									} // end if
									else
										if (task->action==ACTION_FLOOR_MODE)
										{
											// compute cell that mouse is on
											int cell_x = (mouse_state.lX - BSP_MIN_X) / BSP_GRID_SIZE;
											int cell_y = (mouse_state.lY - BSP_MIN_Y) / BSP_GRID_SIZE;

											// drawing or erasing
											if (mouse_state.rgbButtons[0])
											{
												// set texture id
												context->floors[cell_y][cell_x] = context->texture_index_floor;
											} // end if
											else
												if (mouse_state.rgbButtons[2])
												{
													// set texture id
													context->floors[cell_y][cell_x] = -1;
												} // end if

										} // end if

				} // end if in bsp area

				// force button debounce
				if (mouse_state.rgbButtons[0])
					mouse_state.rgbButtons[0] = 0;

				if (mouse_state.rgbButtons[2])
					mouse_state.rgbButtons[2] = 0;

				//mouse_debounce[0] = 1;

			} // end if

			// debounce mouse
			//if (!mouse_state.rgbButtons[0] && mouse_debounce[0] == 1)
			//   mouse_debounce[0] = 0;

			// draw the floors
			if (context->view_floors == 1)
				task->DrawFloors(graphics.GetBackSurface());

			// lock the back buffer
			graphics.LockBackSurface();

			// draw the context->lines
			if (context->view_walls == 1)
				task->DrawLines(graphics.GetBackBuffer(), graphics.GetBackLinePitch());

			// draw the grid
			if (context->view_grid==1)
				task->DrawGrid(graphics.GetBackBuffer(), graphics.GetBackLinePitch(), _RGB32BIT(255,255,255,255), BSP_MIN_X, BSP_MIN_Y,
				BSP_GRID_SIZE, BSP_GRID_SIZE,
				BSP_CELLS_X,BSP_CELLS_Y);

			// unlock the back buffer
			graphics.UnlockBackSurface();

			// draw all the buttons
			gadgets->Draw(graphics.GetBackSurface());

			// draw the textures previews
			if (context->texture_index_wall >= 0)
			{
				// selected texture
				context->textures->SetPosition(665, 302-55);
				context->textures->SetCurrFrame(context->texture_index_wall);
				context->textures->DrawScaled(64,64,graphics.GetBackSurface());

				// get the line that mouse is closest to
				context->nearest_line = task->DeleteLine(mouse_state.lX, mouse_state.lY,0);

				// draw texture that is applied to line in secondary preview window
				if (context->nearest_line >= 0)
				{
					context->textures->SetPosition(736, 247);
					context->textures->SetCurrFrame(context->lines[context->nearest_line].texture_id);
					context->textures->DrawScaled(32,32,graphics.GetBackSurface());

					// now draw height of line
					sprintf(buffer,"%d",context->lines[context->nearest_line].height);
					graphics.DrawTextGDI(buffer, 750,188,RGB(0,255,0),graphics.GetBackSurface());      

				} // end if

			} // end if 

			// draw the floor texture preview
			if (context->texture_index_floor >= 0)
			{
				context->textures->SetPosition(665, 453);
				context->textures->SetCurrFrame(context->texture_index_floor);
				context->textures->DrawScaled(64,64,graphics.GetBackSurface());
			} // end if

			// draw the wall index
			if (context->texture_index_wall >= 0)
			{
				sprintf(buffer,"%d",context->texture_index_wall);
				graphics.DrawTextGDI(buffer, 738,336-56,RGB(0,255,0),graphics.GetBackSurface());
			} // end if
			else
			{
				graphics.DrawTextGDI("None", 738,336,RGB(0,255,0),graphics.GetBackSurface());
			} // end 

			// wall height
			sprintf(buffer,"%d",context->wall_height);
			graphics.DrawTextGDI(buffer, 688, 188,RGB(0,255,0),graphics.GetBackSurface());

			// texture index for floors
			if (context->texture_index_floor >= 0)
			{
				sprintf(buffer,"%d",context->texture_index_floor);
				graphics.DrawTextGDI(buffer, 738,488,RGB(0,255,0),graphics.GetBackSurface());
			} // end if
			else
			{
				graphics.DrawTextGDI("None", 738,488,RGB(0,255,0),graphics.GetBackSurface());
			} // end 

			// display stats
			sprintf(buffer,"WorldPos=(%d,%d). Num Walls=%d. Num Floor Tiles=%d. Total Polys=%d.",
				mouse_state.lX, mouse_state.lY, context->total_lines+1, context->num_floor_tiles, 2*(context->total_lines+1+context->num_floor_tiles));

			graphics.DrawTextGDI(buffer, 8,WINDOW_HEIGHT - 18,RGB(0,255,0),graphics.GetBackSurface());

			// flip the surfaces
			graphics.Flip(main_window_handle);

			// sync to 30ish fps
			Sleep(10);

#if 0
			// check of user is trying to exit
			if (KEY_DOWN(VK_ESCAPE) || Modules::GetInput().KeyboardState()[DIK_ESCAPE])
			{
				PostMessage(main_window_handle, WM_DESTROY,0,0);
			} // end if
#endif

		} break;

	case EDITOR_STATE_VIEW: ///////////////////////////////////////////////
		{
			// start the timing clock
			Modules::GetTimer().Start_Clock();

			// clear the drawing surface 
			graphics.FillSurface(graphics.GetBackSurface(), 0);

			// read keyboard and other devices here
			Modules::GetInput().ReadKeyboard();

			// game logic here...

			// reset the render list
			list->Reset();

			// modes and lights

			// wireframe mode
			if (Modules::GetInput().KeyboardState()[DIK_W]) 
			{
				// toggle wireframe mode
				if (++wireframe_mode > 1)
					wireframe_mode=0;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// backface removal
			if (Modules::GetInput().KeyboardState()[DIK_B])
			{
				// toggle backface removal
				backface_mode = -backface_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// BSP backface removal
			if (Modules::GetInput().KeyboardState()[DIK_C])
			{
				// toggle backface removal
				bsp_backface_mode = -bsp_backface_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// lighting
			if (Modules::GetInput().KeyboardState()[DIK_L])
			{
				// toggle lighting engine completely
				lighting_mode = -lighting_mode;
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

			// help menu
			if (Modules::GetInput().KeyboardState()[DIK_H])
			{
				// toggle help menu 
				help_mode = -help_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// z-sorting
			if (Modules::GetInput().KeyboardState()[DIK_S])
			{
				// toggle z sorting
				zsort_mode = -zsort_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// z buffer
			if (Modules::GetInput().KeyboardState()[DIK_Z])
			{
				// toggle z buffer
				z_buffer_mode = -z_buffer_mode;
				Modules::GetTimer().Wait_Clock(100); // wait, so keyboard doesn't bounce
			} // end if

			// forward/backward
			if (Modules::GetInput().KeyboardState()[DIK_UP])
			{
				// move forward
				if ( (context->cam_speed+=2) > MAX_SPEED) context->cam_speed = MAX_SPEED;
			} // end if
			else if (Modules::GetInput().KeyboardState()[DIK_DOWN])
			{
				// move backward
				if ((context->cam_speed-=2) < -MAX_SPEED) context->cam_speed = -MAX_SPEED;
			} // end if

			// rotate around y axis or yaw
			if (Modules::GetInput().KeyboardState()[DIK_RIGHT])
			{
				context->cam->DirRef().y+=10;

				// scroll the background
				background_bmp->Scroll(-10);
			} // end if

			if (Modules::GetInput().KeyboardState()[DIK_LEFT])
			{
				context->cam->DirRef().y-=10;

				// scroll the background
				background_bmp->Scroll(10);
			} // end if

			// ambient scrolling 
			static int scroll_counter = 0;
			if (++scroll_counter > 5)
			{
				// scroll the background
				background_bmp->Scroll(1);

				// reset scroll counter
				scroll_counter = 0;
			} // end if

			// move point light source in ellipse around game world
			lights[POINT_LIGHT_INDEX].pos.x = 1000*cos(DEG_TO_RAD(plight_ang));
			lights[POINT_LIGHT_INDEX].pos.y = 100;
			lights[POINT_LIGHT_INDEX].pos.z = 1000*sin(DEG_TO_RAD(plight_ang));

			// move point light source in ellipse around game world
			lights[POINT_LIGHT2_INDEX].pos.x = 500*cos(DEG_TO_RAD(-2*plight_ang));
			lights[POINT_LIGHT2_INDEX].pos.y = 200;
			lights[POINT_LIGHT2_INDEX].pos.z = 1000*sin(DEG_TO_RAD(-2*plight_ang));

			if ((plight_ang+=3) > 360)
				plight_ang = 0;

			// decelerate camera
			if (context->cam_speed > (CAM_DECEL) ) context->cam_speed-=CAM_DECEL;
			else if (context->cam_speed < (-CAM_DECEL) ) context->cam_speed+=CAM_DECEL;
			else context->cam_speed = 0;

			// move camera
			context->cam->PosRef().x += context->cam_speed*sin(DEG_TO_RAD(context->cam->Dir().y));
			context->cam->PosRef().z += context->cam_speed*cos(DEG_TO_RAD(context->cam->Dir().y));

			// generate camera matrix
			context->cam->BuildMatrixEuler(CAM_ROT_SEQ_ZYX);

			// build the camera vector rotation matrix ////////////////////////////////
			mrot = mat4::RotateX(context->cam->Dir().x) * 
				   mat4::RotateX(context->cam->Dir().y) *
				   mat4::RotateX(context->cam->Dir().z);

			// transform the (0,0,1) vector and store it in the cam.n vector, this is now the 
			// view direction vector
			context->cam->_n = mrot * vec4(0,0,1,1);

			// normalize the viewing vector
			context->cam->_n.Normalize();

			// render the floors into polylist first

			// reset the object (this only matters for backface and object removal)
			RenderObject* obj_scene = bspeditor::Context::Instance()->obj_scene;

			obj_scene->Reset();

			// set position of floor
			obj_scene->SetWorldPos(0, 0, 0);

			// attempt to cull object   
			if (!obj_scene->Cull(*context->cam, CULL_OBJECT_XYZ_PLANES))
			{
				// create an identity to work with
				mrot = mat4::Identity();

				// rotate the local coords of the object
				obj_scene->Transform(mrot, TRANSFORM_LOCAL_TO_TRANS,1);

				// perform world transform
				obj_scene->ModelToWorld(TRANSFORM_TRANS_ONLY);

				// insert the object into render list
				list->Insert(*obj_scene,0);
			} // end if

			// reset the object (this only matters for backface and object removal)
			obj_scene->Reset();

			// set position of ceiling
			obj_scene->SetWorldPos(0, context->ceiling_height, 0);

			// attempt to cull object   
			if (!obj_scene->Cull(*context->cam, CULL_OBJECT_XYZ_PLANES))
			{
				// create an identity to work with
				mrot = mat4::Identity();

				// rotate the local coords of the object
				obj_scene->Transform(mrot, TRANSFORM_LOCAL_TO_TRANS,1);

				// perform world transform
				obj_scene->ModelToWorld(TRANSFORM_TRANS_ONLY);

				// insert the object into render list
				list->Insert(*obj_scene,0);
			} // end if

#if 0
			// generate rotation matrix around y axis
			Build_XYZ_Rotation_MATRIX4X4(0, 1, 0, &mrot);

			//MAT_IDENTITY_4X4(&mrot);

			// transform bsp tree
			Bsp_Transform(bsp_root, &mrot, TRANSFORM_LOCAL_ONLY); 
#endif

			int polycount = list->GetNumPolys();

			if (bsp_backface_mode == 1)
			{
				// insert the bsp into rendering list
				context->bspfile->bsp_root->InsertionTraversalFrustrumCull(*list, *context->cam, 1);
			}
			else
			{
				// insert the bsp into rendering list without culling
				context->bspfile->bsp_root->InsertionTraversal(*list, *context->cam, 1);
			} 

			// compute total number of polys inserted into list
			polycount = list->GetNumPolys() - polycount;

			// reset number of polys rendered
			debug_polys_rendered_per_frame = 0;
			debug_polys_lit_per_frame = 0;

			// remove backfaces
			if (backface_mode==1)
				list->RemoveBackfaces(*context->cam);

			// apply world to camera transform
			list->WorldToCamera(*context->cam);

			// clip the polygons themselves now
			list->ClipPolys(*context->cam, ((x_clip_mode == 1) ? CLIP_POLY_X_PLANE : 0) | 
										   ((y_clip_mode == 1) ? CLIP_POLY_Y_PLANE : 0) | 
										   ((z_clip_mode == 1) ? CLIP_POLY_Z_PLANE : 0) );

			// light scene all at once 
			if (lighting_mode==1)
			{
				lights.Transform(context->cam->CameraMat(), TRANSFORM_LOCAL_TO_TRANS);
				list->LightWorld32(*context->cam);
			} // end if

			// sort the polygon list (hurry up!)
			if (zsort_mode == 1)
				list->Sort(SORT_POLYLIST_AVGZ);

			// apply camera to perspective transformation
			list->CameraToPerspective(*context->cam);

			// apply screen transform
			list->PerspectiveToScreen(*context->cam);

			// lock the back buffer
			graphics.LockBackSurface();

			// draw background
			background_bmp->Draw32(graphics.GetBackBuffer(), graphics.GetBackLinePitch(), 0);

			// reset number of polys rendered
			debug_polys_rendered_per_frame = 0;

			// render the renderinglist
			if (wireframe_mode  == 0)
				list->DrawWire32(graphics.GetBackBuffer(), graphics.GetBackLinePitch());
			else if (wireframe_mode  == 1)
			{
				RenderContext rc;

				// is z buffering enabled
				if (z_buffer_mode == 1)
				{
					// set up rendering context for 1/z buffer
					rc.attr         = RENDER_ATTR_INVZBUFFER  
									//| RENDER_ATTR_ALPHA  
									//| RENDER_ATTR_MIPMAP  
									//| RENDER_ATTR_BILERP
									| RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE;


				} // end if
				else
				{    
					// set up rendering context for no zbuffer
					rc.attr         = RENDER_ATTR_NOBUFFER  
									//| RENDER_ATTR_ALPHA  
									//| RENDER_ATTR_MIPMAP  
									// | RENDER_ATTR_BILERP
									| RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE;
				} // end else

				// if z abuffer or 1/z buffer is being used then clear z buffer    
				if (rc.attr & RENDER_ATTR_ZBUFFER)
				{
					// clear the z buffer
					zbuffer->Clear(32000 << FIXP16_SHIFT);
				} // end if
				else if (rc.attr & RENDER_ATTR_INVZBUFFER)
				{
					// clear the 1/z buffer
					zbuffer->Clear(0 << FIXP16_SHIFT);
				} // end if

				rc.video_buffer   = graphics.GetBackBuffer();
				rc.lpitch         = graphics.GetBackLinePitch();
				rc.mip_dist       = 3500;
				rc.zbuffer        = (unsigned char*)zbuffer->Buffer();
				rc.zpitch         = WINDOW_WIDTH*4;
				rc.texture_dist   = 0;
				rc.alpha_override = -1;

				// render scene
				list->DrawContext(rc);
			} // end if

			// unlock the back buffer
			graphics.UnlockBackSurface();

			sprintf(work_string,"Lighting [%s]: Ambient=%d, Infinite=%d, BckFceRM [%s], BSP Frustrum Cull [%s], Zsort [%s], Zbuffer [%s]", 
				((lighting_mode == 1) ? "ON" : "OFF"),
				lights[AMBIENT_LIGHT_INDEX].state,
				lights[INFINITE_LIGHT_INDEX].state, 
				((backface_mode == 1) ? "ON" : "OFF"),
				((bsp_backface_mode == 1) ? "ON" : "OFF"),
				((zsort_mode == 1) ? "ON" : "OFF"),
				((z_buffer_mode == 1) ? "ON" : "OFF"));

			graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16, RGB(0,255,0), graphics.GetBackSurface());

			// draw instructions
			graphics.DrawTextGDI("Press ESC to return to editor mode. Press <H> for Help.", 0, 0, RGB(0,255,0), graphics.GetBackSurface());

			// should we display help
			int text_y = 16;
			if (help_mode==1)
			{
				// draw help menu
				graphics.DrawTextGDI("<A>..............Toggle ambient light source.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<I>..............Toggle infinite light source.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<P>..............Toggle point lights.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<B>..............Toggle backface removal.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<S>..............Toggle Z sorting.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<C>..............Toggle BSP Backface Culling.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<Z>..............Toggle Z buffering.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<Z>..............Toggle Wireframe mode.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<H>..............Toggle Help.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());
				graphics.DrawTextGDI("<ESC>............Exit demo.", 0, text_y+=12, RGB(255,255,255), graphics.GetBackSurface());

			} // end help

			sprintf(work_string,"Polys Rendered: %d, Polys lit: %d, Polys Inserted from BSP: %d", debug_polys_rendered_per_frame, debug_polys_lit_per_frame, polycount);
			graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16-16, RGB(0,255,0), graphics.GetBackSurface());

			sprintf(work_string,"CAM [%5.2f, %5.2f, %5.2f]",  context->cam->Pos().x, context->cam->Pos().y, context->cam->Pos().z);
			graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16-16-16, RGB(0,255,0), graphics.GetBackSurface());

			sprintf(work_string,"FPS: %d", Modules::GetTimer().FPS());
			graphics.DrawTextGDI(work_string, 0, WINDOW_HEIGHT-34-16-16-16-16, RGB(0,255,0), graphics.GetBackSurface());


			// flip the surfaces
			graphics.Flip(main_window_handle);

			// sync to 30ish fps
//			Modules::GetTimer().Wait_Clock(30);

			// check of user is trying to exit
			if (KEY_DOWN(VK_ESCAPE) || Modules::GetInput().KeyboardState()[DIK_ESCAPE])
			{
				// switch game state back to editor more
				context->editor_state = EDITOR_STATE_EDIT;
			} // end if

		} break;     

	default: break;

	} // end switch

}

}