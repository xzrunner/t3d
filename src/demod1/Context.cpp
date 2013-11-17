#include "context.h"

#include <locale.h>

#include "BOB.h"
#include "Polygon.h"
#include "Graphics.h"

#include "BSPFile.h"

namespace bspeditor {

Context* Context::m_instance = NULL;

Context::Context() 
{
	textures = new t3d::BOB;

	cam = NULL;
	obj_scene = NULL;

	bspfile = new bspeditor::BSPFile;

	total_lines     = 0;     // number of lines that have been defined
	starting_line   = 0;     // used to close contours
	snap_to_grid    = 1;     // flag to snap to grid        
	view_grid       = 1;     // flag to show grid
	view_walls      = 1;     // flag to show walls
	view_floors     = 1;     // flag to show floors

	nearest_line        = -1;      // index of nearest line to mouse pointer
	texture_index_wall  = 11;      // index of wall texture
	texture_index_floor = 13;      // index of floor texture
	wall_height         = 64;      // initial wall height
	num_floor_tiles     = 0;       // current number of floor tiles
	poly_attr           = 0;       // general polygon attribute
	poly_color          = 0;       // general polygon color
	ceiling_height      = 0;       // height of ceiling 

	cam_speed = 0;

	editor_state        = EDITOR_STATE_INIT; // starting state of editor
}

Context::~Context()
{
	delete bspfile;
	delete obj_scene;
	delete cam;
	delete textures;
}

void Context::Reset()
{
	// this function resets all the runtime variables of the editor and zeros things
	// out

	// reset all variables
	total_lines         = 0;     // number of lines that have been defined
	starting_line       = 0;     // used to close contours
	snap_to_grid        = 1;     // flag to snap to grid        
	view_grid           = 1;     // flag to show grid
	view_walls          = 1;
	view_floors         = 1;

	nearest_line        = -1;
	texture_index_wall  = 11;
	texture_index_floor = 13;
	wall_height         = 64;
	ceiling_height      = 0;   
	num_floor_tiles     = 0; 
	poly_attr           = (POLY_ATTR_DISABLE_MATERIAL | 
						   POLY_ATTR_2SIDED | 
						   POLY_ATTR_RGB24 | 
						   POLY_ATTR_SHADE_MODE_FLAT );
	poly_color          = _RGB32BIT(255,255,255,255);

	// initialize the floors
	for (int cell_y = 0; cell_y < BSP_CELLS_Y-1; cell_y++)
		for (int cell_x = 0; cell_x < BSP_CELLS_X-1; cell_x++)
			floors[cell_y][cell_x] = -1;

	// initialize the wall lines
	memset(lines, 0, sizeof(BSPLine)*MAX_LINES);
}

Context* Context::Instance()
{
	if (!m_instance)
	{
		m_instance = new Context();
	}
	return m_instance;
}

}