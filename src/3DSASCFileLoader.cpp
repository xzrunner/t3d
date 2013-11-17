#include "RenderObject.h"
#include "Modules.h"
#include "Log.h"
#include "Graphics.h"
#include "Parser.h"

namespace t3d {

int load_3dsasc(RenderObject* obj, char *filename, const vec4* scale, 
			    const vec4* pos, const vec4* rot, int vertex_flags)
{
	// this function loads a 3D Studio .ASC file object in off disk, additionally
	// it allows the caller to scale, position, and rotate the object
	// to save extra calls later for non-dynamic objects, also new functionality
	// is supported in the vertex_flags, they allow the specification of shading 
	// overrides since .ASC doesn't support anything other than polygon colors,
	// so with these overrides you can force the entire object to be emmisive, flat,
	// gouraud, etc., and the function will set up the vertices, and normals, for 
	// you based on these overrides.

	Parser parser; 

	char seps[16];          // seperators for token scanning
	char token_buffer[256]; // used as working buffer for token
	char *token;            // pointer to next token

	int r,g,b;              // working colors


	// Step 1: clear out the object and initialize it a bit
	memset(obj, 0, sizeof(RenderObject));

	// set state of object to active and visible
	obj->_state = OBJECT_STATE_ACTIVE | OBJECT_STATE_VISIBLE;

	// set number of frames
	obj->_num_frames = 1;
	obj->_curr_frame = 0;
	obj->_attr = OBJECT_ATTR_SINGLE_FRAME;

	// set position of object is caller requested position
	if (pos)
	{
		// set position of object
		obj->_world_pos.x = pos->x;
		obj->_world_pos.y = pos->y;
		obj->_world_pos.z = pos->z;
		obj->_world_pos.w = pos->w;
	} // end 
	else
	{
		// set it to (0,0,0,1)
		obj->_world_pos.x = 0;
		obj->_world_pos.y = 0;
		obj->_world_pos.z = 0;
		obj->_world_pos.w = 1;
	} // end else

	// Step 2: open the file for reading using the parser
	if (!parser.Open(filename))
	{
		Modules::GetLog().WriteError("Couldn't open .ASC file %s.", filename);
		return(0);
	} // end if

	// Step 3: 

	// lets find the name of the object first 
	while(1)
	{
		// get the next line, we are looking for "Named object:"
		if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
		{
			Modules::GetLog().WriteError("Image 'name' not found in .ASC file %s.", filename);
			return(0);
		} // end if

		// check for pattern?  
		if (parser.Pattern_Match(parser.buffer, "['Named'] ['object:']"))
		{
			// at this point we have the string with the name in it, parse it out by finding 
			// name between quotes "name...."
			strcpy(token_buffer, parser.buffer);
			strcpy(seps, "\"");        
			strtok( token_buffer, seps );

			// this will be the token between the quotes
			token = strtok(NULL, seps);

			// copy name into structure
			strcpy(obj->_name, token);          
			Modules::GetLog().WriteError("\nASC Reader Object Name: %s", obj->_name);

			break;    
		} // end if

	} // end while

	// step 4: get number of vertices and polys in object
	while(1)
	{
		// get the next line, we are looking for "Tri-mesh, Vertices:" 
		if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
		{
			Modules::GetLog().WriteError("'Tri-mesh' line not found in .ASC file %s.", filename);
			return(0);
		} // end if

		// check for pattern?  
		if (parser.Pattern_Match(parser.buffer, "['Tri-mesh,'] ['Vertices:'] [i] ['Faces:'] [i]"))
		{
			// simply extract the number of vertices and polygons from the pattern matching 
			// output arrays
			obj->_num_vertices = parser.pints[0];
			obj->_num_polys    = parser.pints[1];

			Modules::GetLog().WriteError("\nASC Reader Num Vertices: %d, Num Polys: %d", 
				obj->_num_vertices, obj->_num_polys);
			break;    

		} // end if

	} // end while

	// allocate the memory for the vertices and number of polys
	// the call parameters are redundant in this case, but who cares
	if (!obj->Init(obj->_num_vertices, obj->_num_polys, obj->_num_frames))
	{
		Modules::GetLog().WriteError("\nASC file error with file %s (can't allocate memory).",filename);
	} // end if


	// Step 5: load the vertex list

	// advance parser to vertex list denoted by:
	// "Vertex list:"
	while(1)
	{
		// get next line
		if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
		{
			Modules::GetLog().WriteError("\n'Vertex list:' line not found in .ASC file %s.", filename);
			return(0);
		} // end if

		// check for pattern?  
		if (parser.Pattern_Match(parser.buffer, "['Vertex'] ['list:']"))
		{
			Modules::GetLog().WriteError("\nASC Reader found vertex list in .ASC file %s.", filename);
			break;
		} // end if
	} // end while

	// now read in vertex list, format:
	// "Vertex: d  X:d.d Y:d.d  Z:d.d"
	for (int vertex = 0; vertex < obj->_num_vertices; vertex++)
	{
		// hunt for vertex
		while(1)
		{
			// get the next vertex
			if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
			{
				Modules::GetLog().WriteError("\nVertex list ended abruptly! in .ASC file %s.", filename);
				return(0);
			} // end if

			// strip all ":XYZ", make this easier, note use of input and output as same var, this is legal
			// since the output is guaranteed to be the same length or shorter as the input :)
			Parser::StripChars(parser.buffer, parser.buffer, ":XYZ");

			// check for pattern?  
			if (parser.Pattern_Match(parser.buffer, "['Vertex'] [i] [f] [f] [f]"))
			{
				// at this point we have the x,y,z in the the pfloats array locations 0,1,2
				obj->_vlist_local[vertex].x = parser.pfloats[0];
				obj->_vlist_local[vertex].y = parser.pfloats[1];
				obj->_vlist_local[vertex].z = parser.pfloats[2];
				obj->_vlist_local[vertex].w = 1;

				// do vertex swapping right here, allow muliple swaps, why not!
				// defines for vertex re-ordering flags

				//#define VERTEX_FLAGS_INVERT_X   1    // inverts the Z-coordinates
				//#define VERTEX_FLAGS_INVERT_Y   2    // inverts the Z-coordinates
				//#define VERTEX_FLAGS_INVERT_Z   4    // inverts the Z-coordinates
				//#define VERTEX_FLAGS_SWAP_YZ    8    // transforms a RHS model to a LHS model
				//#define VERTEX_FLAGS_SWAP_XZ    16   // ???
				//#define VERTEX_FLAGS_SWAP_XY    32
				//#define VERTEX_FLAGS_INVERT_WINDING_ORDER 64  // invert winding order from cw to ccw or ccw to cc

				float temp_f; // used for swapping

				// invert signs?
				if (vertex_flags & VERTEX_FLAGS_INVERT_X)
					obj->_vlist_local[vertex].x=-obj->_vlist_local[vertex].x;

				if (vertex_flags & VERTEX_FLAGS_INVERT_Y)
					obj->_vlist_local[vertex].y=-obj->_vlist_local[vertex].y;

				if (vertex_flags & VERTEX_FLAGS_INVERT_Z)
					obj->_vlist_local[vertex].z=-obj->_vlist_local[vertex].z;

				// swap any axes?
				if (vertex_flags & VERTEX_FLAGS_SWAP_YZ)
					std::swap(obj->_vlist_local[vertex].y, obj->_vlist_local[vertex].z);

				if (vertex_flags & VERTEX_FLAGS_SWAP_XZ)
					std::swap(obj->_vlist_local[vertex].x, obj->_vlist_local[vertex].z);

				if (vertex_flags & VERTEX_FLAGS_SWAP_XY)
					std::swap(obj->_vlist_local[vertex].x, obj->_vlist_local[vertex].y);

				Modules::GetLog().WriteError("\nVertex %d = %f, %f, %f, %f", vertex,
					obj->_vlist_local[vertex].x, 
					obj->_vlist_local[vertex].y, 
					obj->_vlist_local[vertex].z,
					obj->_vlist_local[vertex].w);

				// scale vertices
				if (scale)
				{
					obj->_vlist_local[vertex].x*=scale->x;
					obj->_vlist_local[vertex].y*=scale->y;
					obj->_vlist_local[vertex].z*=scale->z;
				} // end if

				// set point field in this vertex, we need that at least 
				SET_BIT(obj->_vlist_local[vertex].attr, VERTEX_ATTR_POINT);

				// found vertex, break out of while for next pass
				break;

			} // end if

		} // end while

	} // end for vertex

	// compute average and max radius
	obj->ComputeRadius();

	Modules::GetLog().WriteError("\nObject average radius = %f, max radius = %f", 
		obj->_avg_radius, obj->_max_radius);

	// step 6: load in the polygons
	// poly list starts off with:
	// "Face list:"
	while(1)
	{
		// get next line
		if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
		{
			Modules::GetLog().WriteError("\n'Face list:' line not found in .ASC file %s.", filename);
			return(0);
		} // end if

		// check for pattern?  
		if (parser.Pattern_Match(parser.buffer, "['Face'] ['list:']"))
		{
			Modules::GetLog().WriteError("\nASC Reader found face list in .ASC file %s.", filename);
			break;
		} // end if
	} // end while

	// now read each face in format:
	// Face ddd:    A:ddd B:ddd C:ddd AB:1|0 BC:1|0 CA:1|
	// Material:"rdddgdddbddda0"
	// the A, B, C part is vertex 0,1,2 but the AB, BC, CA part
	// has to do with the edges and the vertex ordering
	// the material indicates the color, and has an 'a0' tacked on the end???

	int  poly_surface_desc = 0; // ASC surface descriptor/material in this case
	int  poly_num_verts    = 0; // number of vertices for current poly (always 3)
	char tmp_string[8];         // temp string to hold surface descriptor in and
	// test if it need to be converted from hex

	for (int poly=0; poly < obj->_num_polys; poly++)
	{
		// hunt until next face is found
		while(1)
		{
			// get the next polygon face
			if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
			{
				Modules::GetLog().WriteError("\nface list ended abruptly! in .ASC file %s.", filename);
				return(0);
			} // end if

			// strip all ":ABC", make this easier, note use of input and output as same var, this is legal
			// since the output is guaranteed to be the same length or shorter as the input :)
			Parser::StripChars(parser.buffer, parser.buffer, ":ABC");

			// check for pattern?  
			if (parser.Pattern_Match(parser.buffer, "['Face'] [i] [i] [i] [i]"))
			{
				// at this point we have the vertex indices in the the pints array locations 1,2,3, 
				// 0 contains the face number

				// insert polygon, check for winding order invert
				if (vertex_flags & VERTEX_FLAGS_INVERT_WINDING_ORDER)
				{     
					poly_num_verts           = 3;
					obj->_plist[poly].vert[0] = parser.pints[3];
					obj->_plist[poly].vert[1] = parser.pints[2];
					obj->_plist[poly].vert[2] = parser.pints[1];
				} // end if
				else
				{ // leave winding order alone
					poly_num_verts           = 3;
					obj->_plist[poly].vert[0] = parser.pints[1];
					obj->_plist[poly].vert[1] = parser.pints[2];
					obj->_plist[poly].vert[2] = parser.pints[3];
				} // end else

				// point polygon vertex list to object's vertex list
				// note that this is redundant since the polylist is contained
				// within the object in this case and its up to the user to select
				// whether the local or transformed vertex list is used when building up
				// polygon geometry, might be a better idea to set to NULL in the context
				// of polygons that are part of an object
				obj->_plist[poly].vlist = obj->_vlist_local; 

				// set texture coordinate list, this is needed
				obj->_plist[poly].tlist = obj->_tlist;

				// found the face, break out of while for another pass
				break;

			} // end if

		} // end while      

		// hunt until next material for face is found
		while(1)
		{
			// get the next polygon material (the "page xxx" breaks mess everything up!!!)
			if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
			{
				Modules::GetLog().WriteError("\nmaterial list ended abruptly! in .ASC file %s.", filename);
				return(0);
			} // end if

			// Material:"rdddgdddbddda0"
			// replace all ':"rgba', make this easier, note use of input and output as same var, this is legal
			// since the output is guaranteed to be the same length or shorter as the input :)
			// the result will look like:
			// "M t ri l   ddd ddd ddd 0" 
			// which we can parse!
			Parser::ReplaceChars(parser.buffer, parser.buffer, ":\"rgba", ' ');

			// check for pattern?  
			if (parser.Pattern_Match(parser.buffer, "[i] [i] [i]"))
			{
				// at this point we have the red, green, and blue components in the the pints array locations 0,1,2, 
				r = parser.pints[0];
				g = parser.pints[1];
				b = parser.pints[2];

				// set all the attributes of polygon as best we can with this format
				// SET_BIT(obj->_plist[poly].attr, POLY_ATTR_2SIDED);

				// we need to know what color depth we are dealing with, so check
				// the bits per pixel, this assumes that the system has already
				// made the call to DDraw_Init() or set the bit depth
				if (Modules::GetGraphics().GetDDPixelFormat()==16)
				{
					// cool, 16 bit mode
					SET_BIT(obj->_plist[poly].attr,POLY_ATTR_RGB16);
					obj->_plist[poly].color = Modules::GetGraphics().GetColor(r, g, b);
					Modules::GetLog().WriteError("\nPolygon 16-bit");
				} // end if 
				else if (Modules::GetGraphics().GetDDPixelFormat()==32)
				{
					// very cool, 32 bit mode
					SET_BIT(obj->_plist[poly].attr,POLY_ATTR_RGB24);
					obj->_plist[poly].color = Modules::GetGraphics().GetColor(r, g, b);
					Modules::GetLog().WriteError("\nPolygon 32-bit");
				}
				else
				{
// 					// 8 bit mode
// 					SET_BIT(obj->_plist[poly].attr,POLY_ATTR_8BITCOLOR);
// 					obj->_plist[poly].color = RGBto8BitIndex(r,g,b, palette, 0);
// 					Modules::GetLog().WriteError("\nPolygon 8-bit, index=%d", obj->_plist[poly].color);
				} // end else

				// we have added the ability to manually override the shading mode
				// of the object, not a face by face basis, but at least on an
				// object wide basis, normally all ASC files are loaded with flat shading as
				// the default, by adding flags to the vertex flags this can be overridden

				#define VERTEX_FLAGS_OVERRIDE_MASK          0xf000 // this masks these bits to extract them
				#define VERTEX_FLAGS_OVERRIDE_CONSTANT      0x1000
				#define VERTEX_FLAGS_OVERRIDE_EMISSIVE      0x1000 (alias)
				#define VERTEX_FLAGS_OVERRIDE_PURE          0x1000
				#define VERTEX_FLAGS_OVERRIDE_FLAT          0x2000
				#define VERTEX_FLAGS_OVERRIDE_GOURAUD       0x4000
				#define VERTEX_FLAGS_OVERRIDE_TEXTURE       0x8000

				// first test to see if there is an override at all 
				int vertex_overrides = (vertex_flags & VERTEX_FLAGS_OVERRIDE_MASK);        

				if (vertex_overrides)
				{
					// which override?
					if (vertex_overrides & VERTEX_FLAGS_OVERRIDE_PURE)
						SET_BIT(obj->_plist[poly].attr, POLY_ATTR_SHADE_MODE_PURE);

					if (vertex_overrides & VERTEX_FLAGS_OVERRIDE_FLAT)
						SET_BIT(obj->_plist[poly].attr, POLY_ATTR_SHADE_MODE_FLAT);

					if (vertex_overrides & VERTEX_FLAGS_OVERRIDE_GOURAUD)
					{
						SET_BIT(obj->_plist[poly].attr, POLY_ATTR_SHADE_MODE_GOURAUD);

						// going to need vertex normals!
						SET_BIT(obj->_vlist_local[ obj->_plist[poly].vert[0] ].attr, VERTEX_ATTR_NORMAL); 
						SET_BIT(obj->_vlist_local[ obj->_plist[poly].vert[1] ].attr, VERTEX_ATTR_NORMAL); 
						SET_BIT(obj->_vlist_local[ obj->_plist[poly].vert[2] ].attr, VERTEX_ATTR_NORMAL); 
					} // end if

					if (vertex_overrides & VERTEX_FLAGS_OVERRIDE_TEXTURE)
						SET_BIT(obj->_plist[poly].attr, POLY_ATTR_SHADE_MODE_TEXTURE);

				} // end if
				else
				{
					// for now manually set shading mode to flat
					//SET_BIT(obj->_plist[poly].attr, POLY_ATTR_SHADE_MODE_PURE);
					//SET_BIT(obj->_plist[poly].attr, POLY_ATTR_SHADE_MODE_GOURAUD);
					//SET_BIT(obj->_plist[poly].attr, POLY_ATTR_SHADE_MODE_PHONG);
					SET_BIT(obj->_plist[poly].attr, POLY_ATTR_SHADE_MODE_FLAT);
				} // end else

				// set the material mode to ver. 1.0 emulation
				SET_BIT(obj->_plist[poly].attr, POLY_ATTR_DISABLE_MATERIAL);

				// set polygon to active
				obj->_plist[poly].state = POLY_STATE_ACTIVE;    

				// found the material, break out of while for another pass
				break;

			} // end if

		} // end while      

		Modules::GetLog().WriteError("\nPolygon %d:", poly);
		Modules::GetLog().WriteError("\nSurface Desc = [RGB]=[%d, %d, %d], vert_indices [%d, %d, %d]", 
			r,g,b,
			obj->_plist[poly].vert[0],
			obj->_plist[poly].vert[1],
			obj->_plist[poly].vert[2]);

	} // end for poly

	// compute the polygon normal lengths
	obj->ComputePolyNormals();

	// compute vertex normals for any gouraud shaded polys
	obj->ComputeVertexNormals();
}

}