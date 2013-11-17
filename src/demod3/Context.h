#pragma once

#include "BSPLine.h"

namespace t3d { class BmpImg; class BOB; class Camera; class RenderObject; }

namespace bspeditor {

#define MAX_LINES                  256  // maximum number of lines in the demo
										// these lines will later become 3-D walls

// control areas of GUI
#define BSP_MIN_X                  0      // the bounding rectangle of the
#define BSP_MAX_X                  648    // BSP editor area
#define BSP_MIN_Y                  0
#define BSP_MAX_Y                  576

#define BSP_GRID_SIZE              24     // the width/height of each cell
#define BSP_CELLS_X                28     // number of horizontal tick marks
#define BSP_CELLS_Y                25     // number of vertical tick marks

// texture defines
#define NUM_TEXTURES                   23  // number of available textures 
#define TEXTURE_SIZE                   128 // size of texture m x m

// bounds of gui components
#define INTERFACE_MIN_X            656    // the bounding rectangle of the control
#define INTERFACE_MAX_X            800    // button area to the right of editor
#define INTERFACE_MIN_Y            0
#define INTERFACE_MAX_Y            600

// editor states
#define EDITOR_STATE_INIT          0      // the editor is initializing
#define EDITOR_STATE_EDIT          1      // the editor is in edit mode
#define EDITOR_STATE_VIEW          2      // the editor is in viewing mode

class BSPFile;

class Context
{
public:
	int main_window_handle;

	int total_lines;		// number of lines that have been defined
	int starting_line;		// used to close contours
	int snap_to_grid;		// flag to snap to grid        
	int view_grid;			// flag to show grid
	int view_walls;			// flag to show walls
	int view_floors;		// flag to show floors

	int nearest_line;		// index of nearest line to mouse pointer
	int texture_index_wall;	// index of wall texture
	int texture_index_floor;// index of floor texture
	int wall_height;		// initial wall height
	int num_floor_tiles;	// current number of floor tiles
	int poly_attr;			// general polygon attribute
	int poly_color;			// general polygon color
	int ceiling_height;		// height of ceiling 

	BSPLine lines[MAX_LINES];  // this holds the lines in the system, we could
                               // use a linked list, but let's not make this harder
                               // than it already is!

	int floors[BSP_CELLS_Y-1][BSP_CELLS_X-1]; // hold the texture id's for the floor
                                              // which we will turn into a mesh later

	t3d::BOB* textures;          // holds the textures for display in editor
	t3d::BmpImg* texturemaps[NUM_TEXTURES]; // holds the textures for the 3D display copies basically...

	t3d::Camera* cam;           // the single camera
	t3d::RenderObject* obj_scene;     // floor/ceiling object

	bspeditor::BSPFile* bspfile;

	// physical model defines
	float cam_speed;       // speed of the camera/jeep

	int editor_state; // starting state of editor

public:
	void Reset();

public:
	static Context* Instance();

private:
	Context();
	~Context();

private:
	static Context* m_instance;

}; // Context

}