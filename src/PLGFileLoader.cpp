#include <ctype.h>

#include "RenderObject.h"
#include "Modules.h"
#include "Log.h"
#include "Graphics.h"

namespace t3d {

// defines for enhanced PLG file format -> PLX
// the surface descriptor is still 16-bit now in the following format
// d15                      d0
//   CSSD | RRRR| GGGG | BBBB

// C is the RGB/indexed color flag
// SS are two bits that define the shading mode
// D is the double sided flag
// and RRRR, GGGG, BBBB are the red, green, blue bits for RGB mode
// or GGGGBBBB is the 8-bit color index for 8-bit mode

// bit masks to simplify testing????
#define PLX_RGB_MASK          0x8000   // mask to extract RGB or indexed color
#define PLX_SHADE_MODE_MASK   0x6000   // mask to extract shading mode
#define PLX_2SIDED_MASK       0x1000   // mask for double sided
#define PLX_COLOR_MASK        0x0fff   // xxxxrrrrggggbbbb, 4-bits per channel RGB
                                       // xxxxxxxxiiiiiiii, indexed mode 8-bit index

// these are the comparision flags after masking
// color mode of polygon
#define PLX_COLOR_MODE_RGB_FLAG     0x8000   // this poly uses RGB color
#define PLX_COLOR_MODE_INDEXED_FLAG 0x0000   // this poly uses an indexed 8-bit color 

// double sided flag
#define PLX_2SIDED_FLAG              0x1000   // this poly is double sided
#define PLX_1SIDED_FLAG              0x0000   // this poly is single sided

// shading mode of polygon
#define PLX_SHADE_MODE_PURE_FLAG      0x0000  // this poly is a constant color
#define PLX_SHADE_MODE_CONSTANT_FLAG  0x0000  // alias
#define PLX_SHADE_MODE_FLAT_FLAG      0x2000  // this poly uses flat shading
#define PLX_SHADE_MODE_GOURAUD_FLAG   0x4000  // this poly used gouraud shading
#define PLX_SHADE_MODE_PHONG_FLAG     0x6000  // this poly uses phong shading
#define PLX_SHADE_MODE_FASTPHONG_FLAG 0x6000  // this poly uses phong shading (alias)

char* GetLinePLG(char *buffer, int maxlength, FILE *fp)
{
	// this little helper function simply read past comments 
	// and blank lines in a PLG file and always returns full 
	// lines with something on them on NULL if the file is empty

	int index = 0;  // general index
	int length = 0; // general length

	// enter into parsing loop
	while(1)
	{
		// read the next line
		if (!fgets(buffer, maxlength, fp))
			return(NULL);

		// kill the whitespace
		for (length = strlen(buffer), index = 0; isspace(buffer[index]); index++);

		// test if this was a blank line or a comment
		if (index >= length || buffer[index]=='#') 
			continue;

		// at this point we have a good line
		return(&buffer[index]);
	} // end while
}

int load_plg(RenderObject* obj, const char* filename, 
			 const vec4* scale, const vec4* pos, const vec4* rot)
{
	// this function loads a plg object in off disk, additionally
	// it allows the caller to scale, position, and rotate the object
	// to save extra calls later for non-dynamic objects
	// there is only one frame, so load the object and set the fields
	// appropriately for a single frame RenderObject

	FILE *fp;          // file pointer
	char buffer[256];  // working buffer

	char *token_string;  // pointer to actual token text, ready for parsing

	// file format review, note types at end of each description
	// # this is a comment

	// # object descriptor
	// object_name_string num_verts_int num_polys_int

	// # vertex list
	// x0_float y0_float z0_float
	// x1_float y1_float z1_float
	// x2_float y2_float z2_float
	// .
	// .
	// xn_float yn_float zn_float
	//
	// # polygon list
	// surface_description_ushort num_verts_int v0_index_int v1_index_int ..  vn_index_int
	// .
	// .
	// surface_description_ushort num_verts_int v0_index_int v1_index_int ..  vn_index_int

	// lets keep it simple and assume one element per line
	// hence we have to find the object descriptor, read it in, then the
	// vertex list and read it in, and finally the polygon list -- simple :)

	// Step 1: clear out the object and initialize it a bit
	memset(obj, 0, sizeof(RenderObject));

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
	obj->_attr = OBJECT_ATTR_SINGLE_FRAME;

	// Step 2: open the file for reading
	if (!(fp = fopen(filename, "r")))
	{
		Modules::GetLog().WriteError("Couldn't open PLG file %s.", filename);
		return(0);
	} // end if

	// Step 3: get the first token string which should be the object descriptor
	if (!(token_string = GetLinePLG(buffer, 255, fp)))
	{
		Modules::GetLog().WriteError("PLG file error with file %s (object descriptor invalid).",filename);
		return(0);
	} // end if

	Modules::GetLog().WriteError("Object Descriptor: %s", token_string);

	// parse out the info object
	sscanf(token_string, "%s %d %d", obj->_name, &obj->_num_vertices, &obj->_num_polys);

	// allocate the memory for the vertices and number of polys
	// the call parameters are redundant in this case, but who cares
	if (obj->Init(obj->_num_vertices, obj->_num_polys, obj->_num_frames))
	{
		Modules::GetLog().WriteError("\nPLG file error with file %s (can't allocate memory).",filename);
	} // end if

	// Step 4: load the vertex list
	for (int vertex = 0; vertex < obj->_num_vertices; vertex++)
	{
		// get the next vertex
		if (!(token_string = GetLinePLG(buffer, 255, fp)))
		{
			Modules::GetLog().WriteError("PLG file error with file %s (vertex list invalid).",filename);
			return(0);
		} // end if

		// parse out vertex
		sscanf(token_string, "%f %f %f", &obj->_vlist_local[vertex].x, 
			&obj->_vlist_local[vertex].y, 
			&obj->_vlist_local[vertex].z);
		obj->_vlist_local[vertex].w = 1;    

		// scale vertices
		obj->_vlist_local[vertex].x*=scale->x;
		obj->_vlist_local[vertex].y*=scale->y;
		obj->_vlist_local[vertex].z*=scale->z;

		Modules::GetLog().WriteError("\nVertex %d = %f, %f, %f, %f", vertex,
			obj->_vlist_local[vertex].x, 
			obj->_vlist_local[vertex].y, 
			obj->_vlist_local[vertex].z,
			obj->_vlist_local[vertex].w);

		// every vertex has a point at least, set that in the flags attribute
		SET_BIT(obj->_vlist_local[vertex].attr, VERTEX_ATTR_POINT);

	} // end for vertex

	// compute average and max radius
	obj->ComputeRadius();

	Modules::GetLog().WriteError("\nObject average radius = %f, max radius = %f", 
		obj->_avg_radius, obj->_max_radius);

	int poly_surface_desc = 0; // PLG/PLX surface descriptor
	int poly_num_verts    = 0; // number of vertices for current poly (always 3)
	char tmp_string[8];        // temp string to hold surface descriptor in and
	// test if it need to be converted from hex

	// Step 5: load the polygon list
	for (int poly=0; poly < obj->_num_polys; poly++)
	{
		// get the next polygon descriptor
		if (!(token_string = GetLinePLG(buffer, 255, fp)))
		{
			Modules::GetLog().WriteError("PLG file error with file %s (polygon descriptor invalid).",filename);
			return(0);
		} // end if

		Modules::GetLog().WriteError("\nPolygon %d:", poly);

		// each vertex list MUST have 3 vertices since we made this a rule that all models
		// must be constructed of triangles
		// read in surface descriptor, number of vertices, and vertex list
		sscanf(token_string, "%s %d %d %d %d", tmp_string,
			&poly_num_verts, // should always be 3 
			&obj->_plist[poly].vert[0],
			&obj->_plist[poly].vert[1],
			&obj->_plist[poly].vert[2]);


		// since we are allowing the surface descriptor to be in hex format
		// with a leading "0x" we need to test for it
		if (tmp_string[0] == '0' && toupper(tmp_string[1]) == 'X')
			sscanf(tmp_string,"%x", &poly_surface_desc);
		else
			poly_surface_desc = atoi(tmp_string);

		// point polygon vertex list to object's vertex list
		// note that this is redundant since the polylist is contained
		// within the object in this case and its up to the user to select
		// whether the local or transformed vertex list is used when building up
		// polygon geometry, might be a better idea to set to NULL in the context
		// of polygons that are part of an object
		obj->_plist[poly].vlist = obj->_vlist_local; 

		Modules::GetLog().WriteError("\nSurface Desc = 0x%.4x, num_verts = %d, vert_indices [%d, %d, %d]", 
			poly_surface_desc, 
			poly_num_verts, 
			obj->_plist[poly].vert[0],
			obj->_plist[poly].vert[1],
			obj->_plist[poly].vert[2]);

		// now we that we have the vertex list and we have entered the polygon
		// vertex index data into the polygon itself, now let's analyze the surface
		// descriptor and set the fields for the polygon based on the description

		// extract out each field of data from the surface descriptor
		// first let's get the single/double sided stuff out of the way
		if ((poly_surface_desc & PLX_2SIDED_FLAG))
		{
			SET_BIT(obj->_plist[poly].attr, POLY_ATTR_2SIDED);
			Modules::GetLog().WriteError("\n2 sided.");
		} // end if
		else
		{
			// one sided
			Modules::GetLog().WriteError("\n1 sided.");
		} // end else

		// now let's set the color type and color
		if ((poly_surface_desc & PLX_COLOR_MODE_RGB_FLAG)) 
		{
			// this is an RGB 4.4.4 surface
			SET_BIT(obj->_plist[poly].attr,POLY_ATTR_RGB16);

			// now extract color and copy into polygon color
			// field in proper 16-bit format 
			// 0x0RGB is the format, 4 bits per pixel 
			int red   = ((poly_surface_desc & 0x0f00) >> 8);
			int green = ((poly_surface_desc & 0x00f0) >> 4);
			int blue  = (poly_surface_desc & 0x000f);

			// although the data is always in 4.4.4 format, the graphics card
			// is either 5.5.5 or 5.6.5, but our virtual color system translates
			// 8.8.8 into 5.5.5 or 5.6.5 for us, but we have to first scale all
			// these 4.4.4 values into 8.8.8
			obj->_plist[poly].color = Modules::GetGraphics().GetColor(red*16, green*16, blue*16);
			Modules::GetLog().WriteError("\nRGB color = [%d, %d, %d]", red, green, blue);
		} // end if
		else
		{
			// this is an 8-bit color indexed surface
			SET_BIT(obj->_plist[poly].attr,POLY_ATTR_8BITCOLOR);

			// and simple extract the last 8 bits and that's the color index
			obj->_plist[poly].color = (poly_surface_desc & 0x00ff);

			Modules::GetLog().WriteError("\n8-bit color index = %d", obj->_plist[poly].color);

		} // end else

		// handle shading mode
		int shade_mode = (poly_surface_desc & PLX_SHADE_MODE_MASK);

		// set polygon shading mode
		switch(shade_mode)
		{
		case PLX_SHADE_MODE_PURE_FLAG: {
			SET_BIT(obj->_plist[poly].attr, POLY_ATTR_SHADE_MODE_PURE);
			Modules::GetLog().WriteError("\nShade mode = pure");
									   } break;

		case PLX_SHADE_MODE_FLAT_FLAG: {
			SET_BIT(obj->_plist[poly].attr, POLY_ATTR_SHADE_MODE_FLAT);
			Modules::GetLog().WriteError("\nShade mode = flat");

									   } break;

		case PLX_SHADE_MODE_GOURAUD_FLAG: {
			SET_BIT(obj->_plist[poly].attr, POLY_ATTR_SHADE_MODE_GOURAUD);

			// the vertices from this polygon all need normals, set that in the flags attribute
			SET_BIT(obj->_vlist_local[ obj->_plist[poly].vert[0] ].attr, VERTEX_ATTR_NORMAL); 
			SET_BIT(obj->_vlist_local[ obj->_plist[poly].vert[1] ].attr, VERTEX_ATTR_NORMAL); 
			SET_BIT(obj->_vlist_local[ obj->_plist[poly].vert[2] ].attr, VERTEX_ATTR_NORMAL); 

			Modules::GetLog().WriteError("\nShade mode = gouraud");
										  } break;

		case PLX_SHADE_MODE_PHONG_FLAG: {
			SET_BIT(obj->_plist[poly].attr, POLY_ATTR_SHADE_MODE_PHONG);

			// the vertices from this polygon all need normals, set that in the flags attribute
			SET_BIT(obj->_vlist_local[ obj->_plist[poly].vert[0] ].attr, VERTEX_ATTR_NORMAL); 
			SET_BIT(obj->_vlist_local[ obj->_plist[poly].vert[1] ].attr, VERTEX_ATTR_NORMAL); 
			SET_BIT(obj->_vlist_local[ obj->_plist[poly].vert[2] ].attr, VERTEX_ATTR_NORMAL); 

			Modules::GetLog().WriteError("\nShade mode = phong");
										} break;

		default: break;
		} // end switch

		// set the material mode to ver. 1.0 emulation
		SET_BIT(obj->_plist[poly].attr, POLY_ATTR_DISABLE_MATERIAL);

		// finally set the polygon to active
		obj->_plist[poly].state = POLY_STATE_ACTIVE;    

		// point polygon vertex list to object's vertex list
		// note that this is redundant since the polylist is contained
		// within the object in this case and its up to the user to select
		// whether the local or transformed vertex list is used when building up
		// polygon geometry, might be a better idea to set to NULL in the context
		// of polygons that are part of an object
		obj->_plist[poly].vlist = obj->_vlist_local; 

		// set texture coordinate list, this is needed
		obj->_plist[poly].tlist = obj->_tlist;

	} // end for poly

	// compute the polygon normal lengths
	obj->ComputePolyNormals();

	// compute vertex normals for any gouraud shaded polys
	obj->ComputeVertexNormals();

	fclose(fp);

	return(1);
}

}