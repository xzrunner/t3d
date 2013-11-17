#include "BSPFile.h"

#include <stdio.h>
#include <Windows.h>

#include "BSP.h"
#include "BmpImg.h"
#include "Modules.h"
#include "Log.h"
#include "RenderObject.h"
#include "BOB.h"

#include "Context.h"
#include "BSPLine.h"

namespace bspeditor {

BSPFile::BSPFile()
{
	wall_list = bsp_root = NULL;
}

BSPFile::~BSPFile()
{
	delete wall_list;
	delete bsp_root;
}

void BSPFile::Load(char *filename)
{
	// this function loads a bsp .LEV file, it uses very simple fscanf functions
	// rather than our parser 
	float version;     // holds the version number

	int num_sections,  // number of sections
		num_walls,     // number of walls
		bsp_cells_x,   // number of cells on x,y axis
		bsp_cells_y;

	FILE *fp; // file pointer

	char token1[80], token2[80], token3[80]; // general string tokens

	// try and open file for writing
	char filepath[255] = "../../assets/chap13/";
	if ((fp = fopen(strcat(filepath, filename), "r")) == NULL)
	   {
	   sprintf(buffer, "Error - File %s not found.",filename);
	   MessageBox((HWND)Context::Instance()->main_window_handle, buffer, "BSP Level Generator",MB_OK);   
	   return;
	   } // end if

	BSPLine* lines = Context::Instance()->lines;

	// read version
	fscanf(fp, "%s %f", token1, &version);

	if (version!=1.0f)
	   {
	   sprintf(buffer, "Error - Wrong Version.",filename);
	   MessageBox((HWND)Context::Instance()->main_window_handle, buffer, "BSP Level Generator",MB_OK);   
	   return;
	   } // end if

	// read number of sections
	fscanf(fp, "%s %d", token1, &num_sections);

	if (num_sections!=2)
	   {
	   sprintf(buffer, "Error - Wrong Number of Sections.",filename);
	   MessageBox((HWND)Context::Instance()->main_window_handle, buffer, "BSP Level Generator",MB_OK);   
	   return;
	   } // end if

	// read wall section
	fscanf(fp,"%s %s", token1, token2);

	// read number of walls
	fscanf(fp,"%s %d", token1, &num_walls);

	// read each wall
	for (int w_index = 0; w_index < num_walls; w_index++)
		{
		// x0.f y0.f x1.f y1.f elev.f height.f text_id.d color.id attr.d
		fscanf(fp, "%f %f %f %f %d %d %d %d %d", &lines[w_index].p0.x, 
												 &lines[w_index].p0.y,
												 &lines[w_index].p1.x, 
												 &lines[w_index].p1.y,
												 &lines[w_index].elev, 
												 &lines[w_index].height,
												 &lines[w_index].texture_id,
												 &lines[w_index].color,
												 &lines[w_index].attr );

		} // end for w_index

	// read the end section 
	fscanf(fp,"%s", token1);

	// read floor section
	fscanf(fp,"%s %s", token1, token2);

	// read number of x floors
	fscanf(fp,"%s %d", token1, &bsp_cells_x);

	// read number of y floors
	fscanf(fp,"%s %d", token1, &bsp_cells_y);

	// read in actual texture indices of floor tiles
	for (int y_index = 0; y_index < BSP_CELLS_Y-1; y_index++)
		{
		// read in next row of indices
		for (int x_index = 0; x_index < BSP_CELLS_X-1; x_index++)
			{
			fscanf(fp, "%d", &Context::Instance()->floors[y_index][x_index]);
			} // end for x_index       

		} // end for

	// read the end section 
	fscanf(fp,"%s", token1);

	// set globals
	Context::Instance()->total_lines = num_walls;
	// bsp_cells_x
	// bsp_cells_y

	// close the file
	fclose(fp);

}

void BSPFile::Save(char *filename, int flags)
{
	// this function loads a bsp .LEV file using fprint for simplicity
	FILE *fp; // file pointer

	// try and open file for writing
	char filepath[255] = "../../assets/chap13/";
	if ((fp = fopen(strcat(filepath, filename), "w")) == NULL)
	   return;

	BSPLine* lines = Context::Instance()->lines;

	// write version
	fprintf(fp, "\nVersion: 1.0");

	// print out a newline
	fprintf(fp, "\n");

	// write number of sections
	fprintf(fp, "\nNumSections: 2");

	// print out a newline
	fprintf(fp, "\n");

	// write wall section
	fprintf(fp,"\nSection: walls");

	// print out a newline
	fprintf(fp, "\n");

	// write number of walls
	fprintf(fp,"\nNumWalls: %d", Context::Instance()->total_lines);

	// print out a newline
	fprintf(fp, "\n");

	// write each wall
	for (int w_index = 0; w_index < Context::Instance()->total_lines; w_index++)
		{
		// x0.f y0.f x1.f y1.f elev.f height.f text_id.d
		fprintf(fp, "\n%d %d %d %d %d %d %d %d %d ", (int)lines[w_index].p0.x, 
													 (int)lines[w_index].p0.y,
													 (int)lines[w_index].p1.x, 
													 (int)lines[w_index].p1.y,
													 lines[w_index].elev, 
													 lines[w_index].height,
													 lines[w_index].texture_id,
													 lines[w_index].color,
													 lines[w_index].attr);
		} // end for w_index

	// print out a newline
	fprintf(fp, "\n");

	// write the end section 
	fprintf(fp,"\nEndSection");

	// print out a newline
	fprintf(fp, "\n");


	// write floor section
	fprintf(fp,"\nSection: floors");

	// print out a newline
	fprintf(fp, "\n");

	// write number of x floors
	fprintf(fp,"\nNumFloorsX: %d", BSP_CELLS_X-1);

	// write number of y floors
	fprintf(fp,"\nNumFloorsY: %d", BSP_CELLS_Y-1);

	// print out a newline
	fprintf(fp, "\n");

	// write out actual texture indices of floor tiles
	for (int y_index = 0; y_index < BSP_CELLS_Y-1; y_index++)
		{
		// print out a newline
		fprintf(fp, "\n");

		// print out next row of indices
		for (int x_index = 0; x_index < BSP_CELLS_X-1; x_index++)
			{
			fprintf(fp, "%d ", Context::Instance()->floors[y_index][x_index]);
			} // end for x_index       

		} // end for

	// print out a newline
	fprintf(fp, "\n");

	// write the end section 
	fprintf(fp,"\nEndSection");

	// close the file
	fclose(fp);
}

void BSPFile::ConvertLinesToWalls()
{
	// this function converts the list of 2-D lines into a linked list of 3-D
	// walls, computes the normal of each wall and sets the pointers up
	// also the function labels the lines on the screen so the user can see them

	t3d::BSPNode *last_wall,    // used to track the last wall processed
				 *temp_wall;    // used as a temporary to build a wall up

	vec4 u,v;            // working vectors

	BSPLine* lines = Context::Instance()->lines;

	// process each 2-d line and convert it into a 3-d wall
	for (int index=0; index < Context::Instance()->total_lines; index++)
	{
		// allocate the memory for the wall
		temp_wall = (t3d::BSPNode*)malloc(sizeof(t3d::BSPNode));
		memset(temp_wall, 0, sizeof(t3d::BSPNode));

		// set up links
		temp_wall->link  = NULL;
		temp_wall->front = NULL;
		temp_wall->back  = NULL;

		// assign points, note how y and z are transposed and the y's of the
		// walls are fixed, this is because we ae looking down on the universe
		// from an aerial view and the wall height is arbitrary; however, with
		// the constants we have selected the walls are WALL_CEILING units tall centered
		// about the x-z plane

		// vertex 0
		temp_wall->wall.vlist[0].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + lines[index].p0.x);
		temp_wall->wall.vlist[0].y = lines[index].elev + lines[index].height;
		temp_wall->wall.vlist[0].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + lines[index].p0.y);
		temp_wall->wall.vlist[0].w = 1;

		// vertex 1
		temp_wall->wall.vlist[1].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + lines[index].p1.x);
		temp_wall->wall.vlist[1].y = lines[index].elev + lines[index].height;
		temp_wall->wall.vlist[1].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + lines[index].p1.y);
		temp_wall->wall.vlist[1].w = 1;

		// vertex 2
		temp_wall->wall.vlist[2].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + lines[index].p1.x);
		temp_wall->wall.vlist[2].y = lines[index].elev;
		temp_wall->wall.vlist[2].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + lines[index].p1.y);
		temp_wall->wall.vlist[2].w = 1;

		// vertex 3
		temp_wall->wall.vlist[3].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + lines[index].p0.x);
		temp_wall->wall.vlist[3].y = lines[index].elev;
		temp_wall->wall.vlist[3].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + lines[index].p0.y);
		temp_wall->wall.vlist[3].w = 1;

		// compute normal to wall

		// find two vectors co-planer in the wall
		u = temp_wall->wall.vlist[1].v - temp_wall->wall.vlist[0].v;
		v = temp_wall->wall.vlist[3].v - temp_wall->wall.vlist[0].v;

		// use cross product to compute normal
		temp_wall->wall.normal = u.Cross(v);

		// normalize the normal vector
		float length = temp_wall->wall.normal.Length();

		temp_wall->wall.normal.x /= length;
		temp_wall->wall.normal.y /= length;
		temp_wall->wall.normal.z /= length; 
		temp_wall->wall.normal.w  = 1.0;
		temp_wall->wall.nlength   = length;

		// set id number for debugging
		temp_wall->id = index;

		// set attributes of polygon
		temp_wall->wall.attr = lines[index].attr;

		// set color of polygon, if texture then white
		// else whatever the color is
		temp_wall->wall.color = lines[index].color;

		// now based on attributes set vertex flags

		// every vertex has a point at least, set that in the flags attribute
		SET_BIT(temp_wall->wall.vlist[0].attr, VERTEX_ATTR_POINT); 
		SET_BIT(temp_wall->wall.vlist[1].attr, VERTEX_ATTR_POINT);
		SET_BIT(temp_wall->wall.vlist[2].attr, VERTEX_ATTR_POINT);
		SET_BIT(temp_wall->wall.vlist[3].attr, VERTEX_ATTR_POINT);

		// check for gouraud of phong shading, if so need normals
		if ( (temp_wall->wall.attr & POLY_ATTR_SHADE_MODE_GOURAUD) ||  
			(temp_wall->wall.attr & POLY_ATTR_SHADE_MODE_PHONG) )
		{
			// the vertices from this polygon all need normals, set that in the flags attribute
			SET_BIT(temp_wall->wall.vlist[0].attr, VERTEX_ATTR_NORMAL); 
			SET_BIT(temp_wall->wall.vlist[1].attr, VERTEX_ATTR_NORMAL);
			SET_BIT(temp_wall->wall.vlist[2].attr, VERTEX_ATTR_NORMAL);
			SET_BIT(temp_wall->wall.vlist[3].attr, VERTEX_ATTR_NORMAL);
			// ??? make sure to create vertex normal....
		} // end if

		// if texture in enabled the enable texture coordinates
		if (lines[index].texture_id >= 0)
		{
			// apply texture to this polygon
			SET_BIT(temp_wall->wall.attr, POLY_ATTR_SHADE_MODE_TEXTURE);
			temp_wall->wall.texture = Context::Instance()->texturemaps[lines[index].texture_id];

			// assign the texture coordinates
			temp_wall->wall.vlist[0].u0 = 0;
			temp_wall->wall.vlist[0].v0 = 0;

			temp_wall->wall.vlist[1].u0 = TEXTURE_SIZE-1;
			temp_wall->wall.vlist[1].v0 = 0;

			temp_wall->wall.vlist[2].u0 = TEXTURE_SIZE-1;
			temp_wall->wall.vlist[2].v0 = TEXTURE_SIZE-1;

			temp_wall->wall.vlist[3].u0 = 0;
			temp_wall->wall.vlist[3].v0 = TEXTURE_SIZE-1;

			// set texture coordinate attributes
			SET_BIT(temp_wall->wall.vlist[0].attr, VERTEX_ATTR_TEXTURE); 
			SET_BIT(temp_wall->wall.vlist[1].attr, VERTEX_ATTR_TEXTURE); 
			SET_BIT(temp_wall->wall.vlist[2].attr, VERTEX_ATTR_TEXTURE); 
			SET_BIT(temp_wall->wall.vlist[3].attr, VERTEX_ATTR_TEXTURE); 
		} // end if

#if 0
		// check for alpha enable 
		if (temp_wall->wall.attr & POLY_ATTR_TRANSPARENT)
		{
			int ialpha = (int)( (float)alpha/255 * (float)(NUM_ALPHA_LEVELS-1) + (float)0.5);

			// set the alpha color in upper 8 bits of color
			temp_wall->wall.color += (ialpha << 24);

		} // end if 
#endif

		// finally set the polygon to active
		temp_wall->wall.state = POLY_STATE_ACTIVE;    

		// update ceiling height to max wall height
		if ((lines[index].elev + lines[index].height) > Context::Instance()->ceiling_height)
			Context::Instance()->ceiling_height = lines[index].elev + lines[index].height;

		t3d::Modules::GetLog().WriteError("\n\nConverting wall %d, v0=[%f,%f,%f]\n v1=[%f,%f,%f]\n v2=[%f,%f,%f]\n v3=[%f,%f,%f] \nnl=%f", index,
			temp_wall->wall.vlist[0].x, temp_wall->wall.vlist[0].y, temp_wall->wall.vlist[0].z, 
			temp_wall->wall.vlist[1].x, temp_wall->wall.vlist[1].y, temp_wall->wall.vlist[1].z, 
			temp_wall->wall.vlist[2].x, temp_wall->wall.vlist[2].y, temp_wall->wall.vlist[2].z, 
			temp_wall->wall.vlist[3].x, temp_wall->wall.vlist[3].y, temp_wall->wall.vlist[3].z, 
			temp_wall->wall.nlength );

		// test if this is first wall
		if (index==0)
		{
			// set head of wall list pointer and last wall pointer
			wall_list = temp_wall;
			last_wall = temp_wall;
		} // end if first wall
		else
		{
			// the first wall has been taken care of

			// link the last wall to the next wall
			last_wall->link = temp_wall;

			// move the last wall to the next wall
			last_wall = temp_wall;
		} // end else

	} // end for index
}

void BSPFile::GenerateFloorsObj(t3d::RenderObject* obj, int rgbcolor, 
							    vec4* pos, vec4* rot, int poly_attr)
{
	// this function uses the global floor and texture arrays to generate the
	// floor mesh under the player, basically for each floor tile that is not -1
	// there should be a floor tile at that position, hence the function uses
	// this fact to create an object that is composed of triangles that makes
	// up the floor mesh, also the texture pointers are assigned to the global
	// texture bitmaps, the function only creates a single floor mesh, the 
	// same mesh is used for the ceiling, but only raised during rendering...

// 	t3d::BmpFile* height_bitmap; // holds the height bitmap

	// Step 1: clear out the object and initialize it a bit
	memset(obj, 0, sizeof(t3d::RenderObject));

	// set state of object to active and visible
	obj->_state = OBJECT_STATE_ACTIVE | OBJECT_STATE_VISIBLE;

	// set position of object
	obj->_world_pos.x = pos->x;
	obj->_world_pos.y = pos->y;
	obj->_world_pos.z = pos->z;
	obj->_world_pos.w = pos->w;

	// set number of frames
	obj->_num_frames = 1;
	obj->_curr_frame = 0;
	obj->_attr       = OBJECT_ATTR_SINGLE_FRAME;

	// store any outputs here..
	obj->_ivar1 = 0;
	obj->_ivar2 = 0;
	obj->_fvar1 = 0;
	obj->_fvar2 = 0;

	int floor_tiles = 0;

	// set number of vertices and polygons to zero
	obj->_num_vertices = 0;
	obj->_num_polys    = 0;

	// count up floor tiles
	for (int y_index = 0; y_index < BSP_CELLS_Y-1; y_index++)
	{
		// print out next row of indices
		for (int x_index = 0; x_index < BSP_CELLS_X-1; x_index++)
		{
			// is there an active tile here?
			if (Context::Instance()->floors[y_index][x_index] >=0 )
			{
				// 2 triangles per tile
				obj->_num_polys+=2;     

				// 4 vertices per tile
				obj->_num_vertices+=4;        
			} // end if 

		} // end for x_index       

	} // end for

	// allocate the memory for the vertices and number of polys
	// the call parameters are redundant in this case, but who cares
	if (!obj->Init(obj->_num_vertices, obj->_num_polys, obj->_num_frames))
	{
		t3d::Modules::GetLog().WriteError("\nTerrain generator error (can't allocate memory).");
	} // end if

	// flag object as having textures
	SET_BIT(obj->_attr, OBJECT_ATTR_TEXTURES);

	// now generate the vertex list and polygon list at the same time
	// 2 triangles per floor tile, 4 vertices per floor tile
	// generate the data row by row

	// initialize indices, we generate on the fly when we find an active tile
	int curr_vertex = 0;
	int curr_poly   = 0;

	for (int y_index = 0; y_index < BSP_CELLS_Y-1; y_index++)
	{
		for (int x_index = 0; x_index < BSP_CELLS_X-1; x_index++)
		{
			int texture_id = -1;

			// does this tile have a texture    
			if ((texture_id = Context::Instance()->floors[y_index][x_index]) >=0 )
			{ 
				// compute the vertices

				// vertex 0
				obj->_vlist_local[curr_vertex].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + (x_index+0)*BSP_GRID_SIZE);
				obj->_vlist_local[curr_vertex].y = 0;
				obj->_vlist_local[curr_vertex].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + (y_index+1)*BSP_GRID_SIZE);
				obj->_vlist_local[curr_vertex].w = 1;  

				// we need texture coordinates
				obj->_tlist[curr_vertex].x = 0;
				obj->_tlist[curr_vertex].y = 0;

				// every vertex has a point at least, set that in the flags attribute
				SET_BIT(obj->_vlist_local[curr_vertex].attr, VERTEX_ATTR_POINT);

				// increment vertex index
				curr_vertex++;

				// vertex 1
				obj->_vlist_local[curr_vertex].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + (x_index+0)*BSP_GRID_SIZE);
				obj->_vlist_local[curr_vertex].y = 0;
				obj->_vlist_local[curr_vertex].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + (y_index+0)*BSP_GRID_SIZE);
				obj->_vlist_local[curr_vertex].w = 1;  

				// we need texture coordinates
				obj->_tlist[curr_vertex].x = 0;
				obj->_tlist[curr_vertex].y = TEXTURE_SIZE-1;

				// every vertex has a point at least, set that in the flags attribute
				SET_BIT(obj->_vlist_local[curr_vertex].attr, VERTEX_ATTR_POINT);

				// increment vertex index
				curr_vertex++;

				// vertex 2
				obj->_vlist_local[curr_vertex].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + (x_index+1)*BSP_GRID_SIZE);
				obj->_vlist_local[curr_vertex].y = 0;
				obj->_vlist_local[curr_vertex].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + (y_index+0)*BSP_GRID_SIZE);
				obj->_vlist_local[curr_vertex].w = 1;  

				// we need texture coordinates
				obj->_tlist[curr_vertex].x = TEXTURE_SIZE-1;
				obj->_tlist[curr_vertex].y = TEXTURE_SIZE-1;

				// every vertex has a point at least, set that in the flags attribute
				SET_BIT(obj->_vlist_local[curr_vertex].attr, VERTEX_ATTR_POINT);

				// increment vertex index
				curr_vertex++;

				// vertex 3
				obj->_vlist_local[curr_vertex].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + (x_index+1)*BSP_GRID_SIZE);
				obj->_vlist_local[curr_vertex].y = 0;
				obj->_vlist_local[curr_vertex].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + (y_index+1)*BSP_GRID_SIZE);
				obj->_vlist_local[curr_vertex].w = 1;  

				// we need texture coordinates
				obj->_tlist[curr_vertex].x = TEXTURE_SIZE-1;
				obj->_tlist[curr_vertex].y = 0;

				// every vertex has a point at least, set that in the flags attribute
				SET_BIT(obj->_vlist_local[curr_vertex].attr, VERTEX_ATTR_POINT);

				// increment vertex index
				curr_vertex++;

				// upper left poly vertex index
				obj->_plist[curr_poly].vert[0] = curr_vertex-4; // 0
				obj->_plist[curr_poly].vert[1] = curr_vertex-3; // 1
				obj->_plist[curr_poly].vert[2] = curr_vertex-2; // 2

				// lower right poly vertex index
				obj->_plist[curr_poly+1].vert[0] = curr_vertex-4; // 0
				obj->_plist[curr_poly+1].vert[1] = curr_vertex-2; // 2
				obj->_plist[curr_poly+1].vert[2] = curr_vertex-1; // 3

				// point polygon vertex list to object's vertex list
				// note that this is redundant since the polylist is contained
				// within the object in this case and its up to the user to select
				// whether the local or transformed vertex list is used when building up
				// polygon geometry, might be a better idea to set to NULL in the context
				// of polygons that are part of an object
				obj->_plist[curr_poly].vlist   = obj->_vlist_local; 
				obj->_plist[curr_poly+1].vlist = obj->_vlist_local; 

				// set attributes of polygon with sent attributes
				obj->_plist[curr_poly].attr    = poly_attr;
				obj->_plist[curr_poly+1].attr  = poly_attr;

				// set color of polygon 
				obj->_plist[curr_poly].color   = rgbcolor;
				obj->_plist[curr_poly+1].color = rgbcolor;

				// apply texture to this polygon
				obj->_plist[curr_poly].texture   = Context::Instance()->texturemaps[ Context::Instance()->floors[y_index][x_index] ];
				obj->_plist[curr_poly+1].texture = Context::Instance()->texturemaps[ Context::Instance()->floors[y_index][x_index] ];

				// assign the texture coordinate index
				// upper left poly
				obj->_plist[curr_poly].text[0] = curr_vertex-4; // 0
				obj->_plist[curr_poly].text[1] = curr_vertex-3; // 1
				obj->_plist[curr_poly].text[2] = curr_vertex-2; // 2

				// lower right poly
				obj->_plist[curr_poly+1].text[0] = curr_vertex-4; // 0
				obj->_plist[curr_poly+1].text[1] = curr_vertex-2; // 2
				obj->_plist[curr_poly+1].text[2] = curr_vertex-1; // 3

				// set texture coordinate attributes
				SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly].vert[0] ].attr, VERTEX_ATTR_TEXTURE); 
				SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly].vert[1] ].attr, VERTEX_ATTR_TEXTURE); 
				SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly].vert[2] ].attr, VERTEX_ATTR_TEXTURE); 

				SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly+1].vert[0] ].attr, VERTEX_ATTR_TEXTURE); 
				SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly+1].vert[1] ].attr, VERTEX_ATTR_TEXTURE); 
				SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly+1].vert[2] ].attr, VERTEX_ATTR_TEXTURE); 

				// set the material mode to ver. 1.0 emulation
				SET_BIT(obj->_plist[curr_poly].attr, POLY_ATTR_DISABLE_MATERIAL);
				SET_BIT(obj->_plist[curr_poly+1].attr, POLY_ATTR_DISABLE_MATERIAL);

				// finally set the polygon to active
				obj->_plist[curr_poly].state   = POLY_STATE_ACTIVE;    
				obj->_plist[curr_poly+1].state = POLY_STATE_ACTIVE;  

				// point polygon vertex list to object's vertex list
				// note that this is redundant since the polylist is contained
				// within the object in this case and its up to the user to select
				// whether the local or transformed vertex list is used when building up
				// polygon geometry, might be a better idea to set to NULL in the context
				// of polygons that are part of an object
				obj->_plist[curr_poly].vlist   = obj->_vlist_local; 
				obj->_plist[curr_poly+1].vlist = obj->_vlist_local; 

				// set texture coordinate list, this is needed
				obj->_plist[curr_poly].tlist   = obj->_tlist;
				obj->_plist[curr_poly+1].tlist = obj->_tlist;

				// increase polygon count
				curr_poly+=2;

			} // end if there is a textured poly here

		} // end for x_index

	} // end for y_index

	// compute average and max radius
	obj->ComputeRadius();

	// compute the polygon normal lengths
	obj->ComputePolyNormals();

	// compute vertex normals for any gouraud shaded polys
	obj->ComputeVertexNormals();
}

}