#include "RenderObject.h"
#include "Modules.h"
#include "Log.h"
#include "Graphics.h"
#include "Parser.h"
#include "Material.h"
#include "tools.h"
#include "BmpFile.h"
#include "Mipmaps.h"

namespace t3d {

int load_cob(RenderObject* obj, char *filename, const vec4* scale, const vec4* pos, 
			 const vec4* rot, int vertex_flags, bool mipmap)
{
	// this function loads a Caligari TrueSpace .COB file object in off disk, additionally
	// it allows the caller to scale, position, and rotate the object
	// to save extra calls later for non-dynamic objects, note that this function 
	// works with a OBJECT4DV2 which has support for textures, but not materials, etc, 
	// however we will still parse out the material stuff and get them ready for the 
	// next incarnation objects, so we can re-use this code to support those features
	// also, since this version IS going to read in the texture map and texture coordinates
	// we have a couple issues to think about, first COB format like absolute texture paths
	// we can't have that, so we will simple extract out ONLY the texture map bitmap name
	// and use the global texture path variable to build a real file path, also texture
	// coordinates are in 0..1 0..1 form, I still haven't decided if I want to use absolute
	// coordinates or 0..1 0..1, but right now the affine texture mapper uses 

	// new version 2.0 supports alpha channel and mip mapping, if mipmap = 1 then the
	// loader will automatically mipmap the texture and then fix the object pointer and 
	// all subsequent polygon texture pointers to point to the mipmap chain d=0 texture
	// pointer rather than directly to the texture, thus mipmapped objects
	// must be flagged and processed differently

	// create a parser object
	Parser parser; 

	char seps[16];          // seperators for token scanning
	char token_buffer[256]; // used as working buffer for token
	char *token;            // pointer to next token

	int r,g,b;              // working colors

	// cache for texture vertices
	vec2 texture_vertices[RenderObject::MAX_VERTICES];

	int num_texture_vertices = 0;

	mat4 mat_local,  // storage for local transform if user requests it in cob format
		 mat_world;  // "   " for local to world " "

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
		Modules::GetLog().WriteError("Couldn't open .COB file %s.", filename);
		return(0);
	} // end if

	// Step 3: 

	// lets find the name of the object first 
	while(1)
	{
		// get the next line, we are looking for "Name"
		if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
		{
			Modules::GetLog().WriteError("Image 'name' not found in .COB file %s.", filename);
			return(0);
		} // end if

		// check for pattern?  
		if ( parser.Pattern_Match(parser.buffer, "['Name'] [s>0]") )
		{
			// name should be in second string variable, index 1
			strcpy(obj->_name, parser.pstrings[1]);          
			Modules::GetLog().WriteError("\nCOB Reader Object Name: %s", obj->_name);

			break;    
		} // end if

	} // end while


	// step 4: get local and world transforms and store them

	// center 0 0 0
	// x axis 1 0 0
	// y axis 0 1 0
	// z axis 0 0 1

	while(1)
	{
		// get the next line, we are looking for "center"
		if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
		{
			Modules::GetLog().WriteError("Center not found in .COB file %s.", filename);
			return(0);
		} // end if

		// check for pattern?  
		if ( parser.Pattern_Match(parser.buffer, "['center'] [f] [f] [f]") )
		{
			// the "center" holds the translation factors, so place in
			// last row of homogeneous matrix, note that these are row vectors
			// that we need to drop in each column of matrix
			mat_local.c[3][0] = -parser.pfloats[0]; // center x
			mat_local.c[3][1] = -parser.pfloats[1]; // center y
			mat_local.c[3][2] = -parser.pfloats[2]; // center z

			// ok now, the next 3 lines should be the x,y,z transform vectors
			// so build up   

			// "x axis" 
			parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS);
			parser.Pattern_Match(parser.buffer, "['x'] ['axis'] [f] [f] [f]");

			// place row in x column of transform matrix
			mat_local.c[0][0] = parser.pfloats[0]; // rxx
			mat_local.c[1][0] = parser.pfloats[1]; // rxy
			mat_local.c[2][0] = parser.pfloats[2]; // rxz

			// "y axis" 
			parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS);
			parser.Pattern_Match(parser.buffer, "['y'] ['axis'] [f] [f] [f]");

			// place row in y column of transform matrix
			mat_local.c[0][1] = parser.pfloats[0]; // ryx
			mat_local.c[1][1] = parser.pfloats[1]; // ryy
			mat_local.c[2][1] = parser.pfloats[2]; // ryz

			// "z axis" 
			parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS);
			parser.Pattern_Match(parser.buffer, "['z'] ['axis'] [f] [f] [f]");

			// place row in z column of transform matrix
			mat_local.c[0][2] = parser.pfloats[0]; // rzx
			mat_local.c[1][2] = parser.pfloats[1]; // rzy
			mat_local.c[2][2] = parser.pfloats[2]; // rzz

			Modules::GetLog().WriteError(mat_local, "Local COB Matrix:");

			break;    
		} // end if

	} // end while

	// now "Transform"
	while(1)
	{
		// get the next line, we are looking for "Transform"
		if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
		{
			Modules::GetLog().WriteError("Transform not found in .COB file %s.", filename);
			return(0);
		} // end if

		// check for pattern?  
		if ( parser.Pattern_Match(parser.buffer, "['Transform']") )
		{

			// "x axis" 
			parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS);
			parser.Pattern_Match(parser.buffer, "[f] [f] [f]");

			// place row in x column of transform matrix
			mat_world.c[0][0] = parser.pfloats[0]; // rxx
			mat_world.c[1][0] = parser.pfloats[1]; // rxy
			mat_world.c[2][0] = parser.pfloats[2]; // rxz

			// "y axis" 
			parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS);
			parser.Pattern_Match(parser.buffer, "[f] [f] [f]");

			// place row in y column of transform matrix
			mat_world.c[0][1] = parser.pfloats[0]; // ryx
			mat_world.c[1][1] = parser.pfloats[1]; // ryy
			mat_world.c[2][1] = parser.pfloats[2]; // ryz

			// "z axis" 
			parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS);
			parser.Pattern_Match(parser.buffer, "[f] [f] [f]");

			// place row in z column of transform matrix
			mat_world.c[0][2] = parser.pfloats[0]; // rzx
			mat_world.c[1][2] = parser.pfloats[1]; // rzy
			mat_world.c[2][2] = parser.pfloats[2]; // rzz

			Modules::GetLog().WriteError(mat_world, "World COB Matrix:");

			// no need to read in last row, since it's always 0,0,0,1 and we don't use it anyway
			break;    

		} // end if

	} // end while

	// step 6: get number of vertices and polys in object
	while(1)
	{
		// get the next line, we are looking for "World Vertices" 
		if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
		{
			Modules::GetLog().WriteError("'World Vertices' line not found in .COB file %s.", filename);
			return(0);
		} // end if

		// check for pattern?  
		if (parser.Pattern_Match(parser.buffer, "['World'] ['Vertices'] [i]") )
		{
			// simply extract the number of vertices from the pattern matching 
			// output arrays
			obj->_num_vertices = parser.pints[0];

			Modules::GetLog().WriteError("\nCOB Reader Num Vertices: %d", obj->_num_vertices);
			break;    

		} // end if

	} // end while

 	// allocate the memory for the vertices and number of polys (unknown, so use 3*num_vertices)
 	// the call parameters are redundant in this case, but who cares
	if (!obj->Init(obj->_num_vertices, obj->_num_vertices*3, obj->_num_frames))
 	{
 		Modules::GetLog().WriteError("\nASC file error with file %s (can't allocate memory).",filename);
 	} // end if

	// Step 7: load the vertex list
	// now read in vertex list, format:
	// "d.d d.d d.d"
	for (int vertex = 0; vertex < obj->_num_vertices; vertex++)
	{
		// hunt for vertex
		while(1)
		{
			// get the next vertex
			if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
			{
				Modules::GetLog().WriteError("\nVertex list ended abruptly! in .COB file %s.", filename);
				return(0);
			} // end if

			// check for pattern?  
			if (parser.Pattern_Match(parser.buffer, "[f] [f] [f]"))
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
				//#define VERTEX_FLAGS_TRANSFORM_LOCAL         512   // if file format has local transform then do it!
				//#define VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD  1024  // if file format has local to world then do it!

				vec4 temp_vector; // temp for calculations

				// now apply local and world transformations encoded in COB format
				if (vertex_flags & VERTEX_FLAGS_TRANSFORM_LOCAL )
					obj->_vlist_local[vertex].v = mat_local * obj->_vlist_local[vertex].v;

				if (vertex_flags & VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD )
					obj->_vlist_local[vertex].v = mat_world * obj->_vlist_local[vertex].v;

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

				// scale vertices
				if (scale)
				{
					obj->_vlist_local[vertex].x*=scale->x;
					obj->_vlist_local[vertex].y*=scale->y;
					obj->_vlist_local[vertex].z*=scale->z;
				} // end if

				Modules::GetLog().WriteError("\nVertex %d = %f, %f, %f, %f", vertex,
					obj->_vlist_local[vertex].x, 
					obj->_vlist_local[vertex].y, 
					obj->_vlist_local[vertex].z,
					obj->_vlist_local[vertex].w);

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


	// step 8: get number of texture vertices
	while(1)
	{
		// get the next line, we are looking for "Texture Vertices ddd" 
		if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
		{
			Modules::GetLog().WriteError("'Texture Vertices' line not found in .COB file %s.", filename);
			return(0);
		} // end if

		// check for pattern?  
		if (parser.Pattern_Match(parser.buffer, "['Texture'] ['Vertices'] [i]") )
		{
			// simply extract the number of texture vertices from the pattern matching 
			// output arrays
			num_texture_vertices = parser.pints[0];

			Modules::GetLog().WriteError("\nCOB Reader Texture Vertices: %d", num_texture_vertices);
			break;    

		} // end if

	} // end while

	// Step 9: load the texture vertex list in format "U V"
	// "d.d d.d"
	for (int tvertex = 0; tvertex < num_texture_vertices; tvertex++)
	{
		// hunt for texture
		while(1)
		{
			// get the next vertex
			if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
			{
				Modules::GetLog().WriteError("\nTexture Vertex list ended abruptly! in .COB file %s.", filename);
				return(0);
			} // end if

			// check for pattern?  
			if (parser.Pattern_Match(parser.buffer, "[f] [f]"))
			{
				// at this point we have the U V in the the pfloats array locations 0,1 for this 
				// texture vertex, store in texture coordinate list
				// note texture coords are in 0..1 format, and must be scaled to texture size
				// after we load the texture
				obj->_tlist[tvertex].x = parser.pfloats[0]; 
				obj->_tlist[tvertex].y = parser.pfloats[1];

				Modules::GetLog().WriteError("\nTexture Vertex %d: U=%f, V=%f", 
					tvertex,
					obj->_tlist[tvertex].x, 
					obj->_tlist[tvertex].y );

				// found vertex, break out of while for next pass
				break;

			} // end if

		} // end while

	} // end for

	// when we load in the polygons then we will copy the texture vertices into the polygon
	// vertices assuming that each vertex has a SINGLE texture coordinate, this means that
	// you must NOT use multiple textures on an object! in other words think "skin" this is
	// inline with Quake II md2 format, in 99% of the cases a single object can be textured
	// with a single skin and the texture coordinates can be unique for each vertex and 1:1

	int poly_material[RenderObject::MAX_POLYS]; // this holds the material index for each polygon
	// we need these indices since when reading the file
	// we read the polygons BEFORE the materials, so we need
	// this data, so we can go back later and extract the material
	// that each poly WAS assigned and get the colors out, since
	// objects and polygons do not currenlty support materials


	int material_index_referenced[MaterialsMgr::MAX_MATERIALS];   // used to track if an index has been used yet as a material 
	// reference. since we don't know how many materials, we need
	// a way to count them up, but if we have seen a material reference
	// more than once then we don't increment the total number of materials
	// this array is for this

	// clear out reference array
	memset(material_index_referenced,0, sizeof(material_index_referenced));


	// step 10: load in the polygons
	// poly list starts off with:
	// "Faces ddd:"
	while(1)
	{
		// get next line
		if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
		{
			Modules::GetLog().WriteError("\n'Faces' line not found in .COB file %s.", filename);
			return(0);
		} // end if

		// check for pattern?  
		if (parser.Pattern_Match(parser.buffer, "['Faces'] [i]"))
		{
			Modules::GetLog().WriteError("\nCOB Reader found face list in .COB file %s.", filename);

			// finally set number of polys
			obj->_num_polys = parser.pints[0];

			break;
		} // end if
	} // end while

	// now read each face in format:
	// Face verts nn flags ff mat mm
	// the nn is the number of vertices, always 3
	// the ff is the flags, unused for now, has to do with holes
	// the mm is the material index number 


	int poly_surface_desc    = 0; // ASC surface descriptor/material in this case
	int poly_num_verts       = 0; // number of vertices for current poly (always 3)
	int num_materials_object = 0; // number of materials for this object

	for (int poly=0; poly < obj->_num_polys; poly++)
	{
		Modules::GetLog().WriteError("\nPolygon %d:", poly);
		// hunt until next face is found
		while(1)
		{
			// get the next polygon face
			if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
			{
				Modules::GetLog().WriteError("\nface list ended abruptly! in .COB file %s.", filename);
				return(0);
			} // end if

			// check for pattern?  
			if (parser.Pattern_Match(parser.buffer, "['Face'] ['verts'] [i] ['flags'] [i] ['mat'] [i]"))
			{
				// at this point we have the number of vertices for the polygon, the flags, and it's material index
				// in the integer output array locations 0,1,2

				// store the material index for this polygon for retrieval later, but make sure adjust the 
				// the index to take into consideration that the data in parser.pints[2] is 0 based, and we need
				// an index relative to the entire library, so we simply need to add num_materials to offset the 
				// index properly, but we will leave this reference zero based for now... and fix up later
				poly_material[poly] = parser.pints[2];

				// update the reference array
				if (material_index_referenced[ poly_material[poly] ] == 0)
				{
					// mark as referenced
					material_index_referenced[ poly_material[poly] ] = 1;

					// increment total number of materials for this object
					num_materials_object++;
				} // end if        


				// test if number of vertices is 3
				if (parser.pints[0]!=3)
				{
					Modules::GetLog().WriteError("\nface not a triangle! in .COB file %s.", filename);
					return(0);
				} // end if

				// now read out the vertex indices and texture indices format:
				// <vindex0, tindex0>  <vindex1, tindex1> <vindex1, tindex1> 
				if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
				{
					Modules::GetLog().WriteError("\nface list ended abruptly! in .COB file %s.", filename);
					return(0);
				} // end if

				// lets replace ",<>" with ' ' to make extraction easy
				Parser::ReplaceChars(parser.buffer, parser.buffer, ",<>",' ');      
				parser.Pattern_Match(parser.buffer, "[i] [i] [i] [i] [i] [i]");

				// 0,2,4 holds vertex indices
				// 1,3,5 holds texture indices

				// insert polygon, check for winding order invert
				if (vertex_flags & VERTEX_FLAGS_INVERT_WINDING_ORDER)
				{     
					poly_num_verts           = 3;
					obj->_plist[poly].vert[0] = parser.pints[4];
					obj->_plist[poly].vert[1] = parser.pints[2];
					obj->_plist[poly].vert[2] = parser.pints[0];

					// now copy the texture coordinates into the vertices, this
					// may not be needed if the polygon doesn't have texture mapping
					// enabled, etc., 

					// so here's the deal the texture coordinates that 
					// map to vertex 0,1,2 have indices stored in the odd
					// numbered pints[] locations, so we simply need to copy
					// the right texture coordinate into the right vertex
					obj->_plist[poly].text[0] = parser.pints[5];
					obj->_plist[poly].text[1] = parser.pints[3];
					obj->_plist[poly].text[2] = parser.pints[1];

					Modules::GetLog().WriteError("\nAssigning texture vertex index %d [%f, %f] to mesh vertex %d",
						parser.pints[5],
						obj->_tlist[ parser.pints[5] ].x, 
						obj->_tlist[ parser.pints[5] ].y,
						obj->_plist[poly].vert[0] );

					Modules::GetLog().WriteError("\nAssigning texture vertex index %d [%f, %f] to mesh vertex %d",
						parser.pints[3],
						obj->_tlist[ parser.pints[3] ].x, 
						obj->_tlist[ parser.pints[3] ].y,
						obj->_plist[poly].vert[1] );

					Modules::GetLog().WriteError("\nAssigning texture vertex index %d [%f, %f] to mesh vertex %d",
						parser.pints[1],
						obj->_tlist[ parser.pints[1] ].x, 
						obj->_tlist[ parser.pints[1] ].y,
						obj->_plist[poly].vert[2] );

				} // end if
				else
				{ // leave winding order alone
					poly_num_verts           = 3;
					obj->_plist[poly].vert[0] = parser.pints[0];
					obj->_plist[poly].vert[1] = parser.pints[2];
					obj->_plist[poly].vert[2] = parser.pints[4];

					// now copy the texture coordinates into the vertices, this
					// may not be needed if the polygon doesn't have texture mapping
					// enabled, etc., 


					// so here's the deal the texture coordinates that 
					// map to vertex 0,1,2 have indices stored in the odd
					// numbered pints[] locations, so we simply need to copy
					// the right texture coordinate into the right vertex
					obj->_plist[poly].text[0] = parser.pints[1];
					obj->_plist[poly].text[1] = parser.pints[3];
					obj->_plist[poly].text[2] = parser.pints[5];

					Modules::GetLog().WriteError("\nAssigning texture vertex index %d [%f, %f] to mesh vertex %d",
						parser.pints[1],
						obj->_tlist[ parser.pints[1] ].x, 
						obj->_tlist[ parser.pints[1] ].y,
						obj->_plist[poly].vert[0] );

					Modules::GetLog().WriteError("\nAssigning texture vertex index %d [%f, %f] to mesh vertex %d",
						parser.pints[3],
						obj->_tlist[ parser.pints[3] ].x, 
						obj->_tlist[ parser.pints[3] ].y,
						obj->_plist[poly].vert[1] );

					Modules::GetLog().WriteError("\nAssigning texture vertex index %d [%f, %f] to mesh vertex %d",
						parser.pints[5],
						obj->_tlist[ parser.pints[5] ].x, 
						obj->_tlist[ parser.pints[5] ].y,
						obj->_plist[poly].vert[2] );

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

				// set polygon to active
				obj->_plist[poly].state = POLY_STATE_ACTIVE;    

				// found the face, break out of while for another pass
				break;

			} // end if

		} // end while      

		Modules::GetLog().WriteError("\nLocal material Index=%d, total materials for object = %d, vert_indices [%d, %d, %d]", 
			poly_material[poly],
			num_materials_object,
			obj->_plist[poly].vert[0],
			obj->_plist[poly].vert[1],
			obj->_plist[poly].vert[2]);       
	} // end for poly

	// now find materials!!! and we are out of here!
	MaterialsMgr& materials = Modules::GetGraphics().GetMaterials();
	int curr_material = 0;
	for (; curr_material < num_materials_object; curr_material++)
	{
		// hunt for the material header "mat# ddd"
		while(1)
		{
			// get the next polygon material 
			if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
			{
				Modules::GetLog().WriteError("\nmaterial list ended abruptly! in .COB file %s.", filename);
				return(0);
			} // end if

			// check for pattern?  
			if (parser.Pattern_Match(parser.buffer, "['mat#'] [i]") )
			{
				// extract the material that is being defined 
				int material_index = parser.pints[0];

				// get color of polygon, although it might be irrelevant for a textured surface
				while(1)
				{
					// get the next line
					if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
					{
						Modules::GetLog().WriteError("\nRGB color ended abruptly! in .COB file %s.", filename);
						return(0);
					} // end if

					// replace the , comma's if there are any with spaces
					Parser::ReplaceChars(parser.buffer, parser.buffer, ",", ' ', 1);

					// look for "rgb float,float,float"
					if (parser.Pattern_Match(parser.buffer, "['rgb'] [f] [f] [f]") )
					{
						// extract data and store color in material libary
						// pfloats[] 0,1,2,3, has data
						materials[material_index + materials.Size()].color.r = (int)(parser.pfloats[0]*255 + 0.5);
						materials[material_index + materials.Size()].color.g = (int)(parser.pfloats[1]*255 + 0.5);
						materials[material_index + materials.Size()].color.b = (int)(parser.pfloats[2]*255 + 0.5);

						break; // while looking for rgb
					} // end if

				} // end while    

				// extract out lighting constants for the heck of it, they are on a line like this:
				// "alpha float ka float ks float exp float ior float"
				// alpha is transparency           0 - 1
				// ka is ambient coefficient       0 - 1
				// ks is specular coefficient      0 - 1
				// exp is highlight power exponent 0 - 1
				// ior is index of refraction (unused)

				// although our engine will have minimal support for these, we might as well get them
				while(1)
				{
					// get the next line
					if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
					{
						Modules::GetLog().WriteError("\nmaterial properties ended abruptly! in .COB file %s.", filename);
						return(0);
					} // end if

					// look for "alpha float ka float ks float exp float ior float"
					if (parser.Pattern_Match(parser.buffer, "['alpha'] [f] ['ka'] [f] ['ks'] [f] ['exp'] [f]") )
					{
						// extract data and store in material libary
						// pfloats[] 0,1,2,3, has data
						materials[material_index + materials.Size()].color.a  = (UCHAR)(parser.pfloats[0]*255 + 0.5);
						materials[material_index + materials.Size()].ka       = parser.pfloats[1];
						materials[material_index + materials.Size()].kd       = 1; // hard code for now
						materials[material_index + materials.Size()].ks       = parser.pfloats[2];
						materials[material_index + materials.Size()].power    = parser.pfloats[3];

						// compute material reflectivities in pre-multiplied format to help engine
						for (int rgb_index=0; rgb_index < 3; rgb_index++)
						{
							// ambient reflectivity
							materials[material_index + materials.Size()].ra.rgba_M[rgb_index] = 
								( (UCHAR)(materials[material_index + materials.Size()].ka * 
								(float)materials[material_index + materials.Size()].color.rgba_M[rgb_index] + 0.5) );


							// diffuse reflectivity
							materials[material_index + materials.Size()].rd.rgba_M[rgb_index] = 
								( (UCHAR)(materials[material_index + materials.Size()].kd * 
								(float)materials[material_index + materials.Size()].color.rgba_M[rgb_index] + 0.5) );


							// specular reflectivity
							materials[material_index + materials.Size()].rs.rgba_M[rgb_index] = 
								( (UCHAR)(materials[material_index + materials.Size()].ks * 
								(float)materials[material_index + materials.Size()].color.rgba_M[rgb_index] + 0.5) );

						} // end for rgb_index

						break;
					} // end if

				} // end while    

				// now we need to know the shading model, it's a bit tricky, we need to look for the lines
				// "Shader class: color" first, then after this line is:
				// "Shader name: "xxxxxx" (xxxxxx) "
				// where the xxxxx part will be "plain color" and "plain" for colored polys 
				// or "texture map" and "caligari texture"  for textures
				// THEN based on that we hunt for "Shader class: reflectance" which is where the type
				// of shading is encoded, we look for the "Shader name: "xxxxxx" (xxxxxx) " again, 
				// and based on it's value we map it to our shading system as follows:
				// "constant" -> MAT_ATTR_SHADE_MODE_CONSTANT 
				// "matte"    -> MAT_ATTR_SHADE_MODE_FLAT
				// "plastic"  -> MAT_ATTR_SHADE_MODE_GOURAUD
				// "phong"    -> MAT_ATTR_SHADE_MODE_FASTPHONG 
				// and in the case that in the "color" class, we found a "texture map" then the "shading mode" is
				// "texture map" -> MAT_ATTR_SHADE_MODE_TEXTURE 
				// which must be logically or'ed with the other previous modes

				//  look for the "shader class: color"
				while(1)
				{
					// get the next line
					if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
					{
						Modules::GetLog().WriteError("\nshader class ended abruptly! in .COB file %s.", filename);
						return(0);
					} // end if

					if (parser.Pattern_Match(parser.buffer, "['Shader'] ['class:'] ['color']") )
					{
						break;
					} // end if

				} // end while

				// now look for the shader name for this class
				// Shader name: "plain color" or Shader name: "texture map"
				while(1)
				{
					// get the next line
					if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
					{
						Modules::GetLog().WriteError("\nshader name ended abruptly! in .COB file %s.", filename);
						return(0);
					} // end if

					// replace the " with spaces
					Parser::ReplaceChars(parser.buffer, parser.buffer, "\"", ' ', 1);

					// is this a "plain color" poly?
					if (parser.Pattern_Match(parser.buffer, "['Shader'] ['name:'] ['plain'] ['color']") )
					{
						// not much to do this is default, we need to wait for the reflectance type
						// to tell us the shading mode

						break;
					} // end if

					// is this a "texture map" poly?
					if (parser.Pattern_Match(parser.buffer, "['Shader'] ['name:'] ['texture'] ['map']") )
					{
						// set the texture mapping flag in material
						SET_BIT(materials[material_index + materials.Size()].attr, MAT_ATTR_SHADE_MODE_TEXTURE);

						// almost done, we need the file name of the darn texture map, its in this format:
						// file name: string "D:\Source\models\textures\wall01.bmp"

						// of course the filename in the quotes will change
						// so lets hunt until we find it...
						while(1)
						{
							// get the next line
							if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
							{
								Modules::GetLog().WriteError("\ncouldnt find texture name! in .COB file %s.", filename);
								return(0);
							} // end if

							// replace the " with spaces
							Parser::ReplaceChars(parser.buffer, parser.buffer, "\"", ' ', 1);

							// is this the file name?
							if (parser.Pattern_Match(parser.buffer, "['file'] ['name:'] ['string'] [s>0]") )
							{
								MaterialsMgr& materials = Modules::GetGraphics().GetMaterials();

								// and save the FULL filename (useless though since its the path from the 
								// machine that created it, but later we might want some of the info).
								// filename and path starts at char position 19, 0 indexed
								memcpy(materials[material_index + materials.Size()].texture_file, &parser.buffer[18], strlen(parser.buffer) - 18 + 2 );

								// the OBJECT4DV2 is only allowed a single texture, although we are loading in all
								// the materials, if this is the first texture map, load it, and set a flag disallowing
								// any more texture loads for the object
								if (!obj->_texture)
								{
									// step 1: allocate memory for bitmap
									obj->_texture = (BmpImg*)malloc(sizeof(BmpImg));

									// 
									std::string filepath(filename);
									std::string dirpath = filepath.substr(0, filepath.find_last_of('/')+1);

									// load the texture, just use the final file name and the absolute global 
									// texture path
									char filename[80];
									char path_filename[80];
									// get the filename                     
									ExtractFilenameFromPath(materials[material_index + materials.Size()].texture_file, filename);

									// build the filename with root path
// 									char texture_path[80] = "../../assets/chap09/"; // root path to ALL textures, make current directory for now
 									strcpy(path_filename, dirpath.c_str());
 									strcat(path_filename, filename);

									// buffer now holds final texture path and file name
									// load the bitmap(8/16 bit)
									BmpFile* bmpfile = new BmpFile(path_filename);

									// create a proper size and bitdepth bitmap
									obj->_texture = new BmpImg(0, 0, bmpfile->Width(), bmpfile->Height(), bmpfile->BitCount());
									obj->_texture->LoadImage32(*bmpfile,0,0,BITMAP_EXTRACT_MODE_CELL);

									// done, so unload the bitmap
									delete bmpfile, bmpfile = NULL;

									// flag object as having textures
									SET_BIT(obj->_attr, OBJECT_ATTR_TEXTURES);

								} // end if
								break;
							} // end if

						} // end while

						break;
					} // end if

				} // end while 

				////////////////////////////////////////////////////////////////////////////////////////////
				// ADDED CODE FOR TRANSPARENCY AND ALPHA BLENDING //////////////////////////////////////////
				////////////////////////////////////////////////////////////////////////////////////////////
				// Now we need to know if there is any transparency for the material
				// we have decided to encoded this in the shader class: transparency :)
				// also, you must use the "filter" shader, and then set the RGB color to
				// the level of transparency you want, 0,0,0 means totally transparent
				// 255,255,255 means totally opaque, so we are looking for something like
				// this:
				// Shader class: transparency
				// Shader name: "filter" (plain)
				// Number of parameters: 1
				// colour: color (146, 146, 146)
				// 
				// and if there isn't transparency then we will see this:
				//
				// Shader class: transparency
				// Shader name: "none" (none)
				// Number of parameters: 0
				// 
				// now, since we aren't doing anykind of RGB transparency, we are only concerned
				// with the overall value, so the way, I am going to do this is to look at the 
				// 3 values of R, G, B, and use the highest one as the final alpha/transparency 
				// value, so a value of 255, 255, 255 with be 100% alpha or totally opaque

				//  look for the "Shader class: transparency"
				while(1)
				{
					// get the next line
					if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
					{
						Modules::GetLog().WriteError("\nshader transparency class not found in .COB file %s.", filename);
						return(0);
					} // end if

					// look for "Shader class: transparency"
					if (parser.Pattern_Match(parser.buffer, "['Shader'] ['class:'] ['transparency']") )
					{
						// now we know the next "shader name" is what we are looking for so, break

						break;
					} // end if

				} // end while    

				while(1)
				{
					// get the next line
					if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
					{
						Modules::GetLog().WriteError("\nshader name ended abruptly! in .COB file %s.", filename);
						return(0);
					} // end if

					// get rid of those quotes
					Parser::ReplaceChars(parser.buffer, parser.buffer, "\"",' ',1);

					// did we find the name?
					if (parser.Pattern_Match(parser.buffer, "['Shader'] ['name:'] [s>0]" ) )
					{
						// figure out if transparency is enabled
						if (strcmp(parser.pstrings[2], "none") == 0)
						{
							// disable the alpha bit and write 0 alpha
							RESET_BIT(materials[material_index + materials.Size()].attr, MAT_ATTR_TRANSPARENT);

							// set alpha to 0, unused
							materials[material_index + materials.Size()].color.a = 0;

						} // end if
						else
							if (strcmp(parser.pstrings[2], "filter") == 0)
							{
								// enable the alpha bit and write the alpha level
								SET_BIT(materials[material_index + materials.Size()].attr, MAT_ATTR_TRANSPARENT);

								// now search for color line to extract alpha level
								//  look for the "Shader class: transparency"
								while(1)
								{
									// get the next line
									if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
									{
										Modules::GetLog().WriteError("\ntransparency color not found in .COB file %s.", filename);
										return(0);
									} // end if

									// get rid of extraneous characters
									Parser::ReplaceChars(parser.buffer, parser.buffer, ":(,)",' ',1);

									// look for colour: color (146, 146, 146)
									if (parser.Pattern_Match(parser.buffer, "['colour'] ['color'] [i] [i] [i]") )
									{
										// set the alpha level to the highest value
										int max_alpha = max(parser.pints[0], parser.pints[1]);
										max_alpha = max(max_alpha, parser.pints[2]);

// 										// set alpha value
// 										materials[material_index + materials.Size()].color.a = 
// 											(int)( (float)max_alpha/255 * (float)(NUM_ALPHA_LEVELS-1) + (float)0.5);
// 
// 										// clamp
// 										if (materials[material_index + materials.Size()].color.a >= NUM_ALPHA_LEVELS)
// 											materials[material_index + materials.Size()].color.a = NUM_ALPHA_LEVELS-1;

										materials[material_index + materials.Size()].color.a = max_alpha;

										break;
									} // end if

								} // end while    

							} // end if

							break;
					} // end if

				} // end while
				//////////////////////////////////////////////////////////////////////////////////////////////

				// alright, finally! Now we need to know what the actual shader type, now in the COB format
				// I have decided that in the "reflectance" class that's where we will look at what kind
				// of shader is supposed to be used on the polygon

				//  look for the "Shader class: reflectance"
				while(1)
				{
					// get the next line
					if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
					{
						Modules::GetLog().WriteError("\nshader reflectance class not found in .COB file %s.", filename);
						return(0);
					} // end if

					// look for "Shader class: reflectance"
					if (parser.Pattern_Match(parser.buffer, "['Shader'] ['class:'] ['reflectance']") )
					{
						// now we know the next "shader name" is what we are looking for so, break

						break;
					} // end if

				} // end while    

				// looking for "Shader name: "xxxxxx" (xxxxxx) " again, 
				// and based on it's value we map it to our shading system as follows:
				// "constant" -> MAT_ATTR_SHADE_MODE_CONSTANT 
				// "matte"    -> MAT_ATTR_SHADE_MODE_FLAT
				// "plastic"  -> MAT_ATTR_SHADE_MODE_GOURAUD
				// "phong"    -> MAT_ATTR_SHADE_MODE_FASTPHONG 
				// and in the case that in the "color" class, we found a "texture map" then the "shading mode" is
				// "texture map" -> MAT_ATTR_SHADE_MODE_TEXTURE 
				// which must be logically or'ed with the other previous modes
				while(1)
				{
					// get the next line
					if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
					{
						Modules::GetLog().WriteError("\nshader name ended abruptly! in .COB file %s.", filename);
						return(0);
					} // end if

					// get rid of those quotes
					Parser::ReplaceChars(parser.buffer, parser.buffer, "\"",' ',1);

					// did we find the name?
					if (parser.Pattern_Match(parser.buffer, "['Shader'] ['name:'] [s>0]" ) )
					{
						// figure out which shader to use
						if (strcmp(parser.pstrings[2], "constant") == 0)
						{
							// set the shading mode flag in material
							SET_BIT(materials[material_index + materials.Size()].attr, MAT_ATTR_SHADE_MODE_CONSTANT);
						} 
						else if (strcmp(parser.pstrings[2], "matte") == 0)
						{
							// set the shading mode flag in material
							SET_BIT(materials[material_index + materials.Size()].attr, MAT_ATTR_SHADE_MODE_FLAT);
						} // end if
						else if (strcmp(parser.pstrings[2], "plastic") == 0)
						{
							// set the shading mode flag in material
							SET_BIT(materials[curr_material + materials.Size()].attr, MAT_ATTR_SHADE_MODE_GOURAUD);
						} // end if
						else if (strcmp(parser.pstrings[2], "phong") == 0)
						{
							// set the shading mode flag in material
							SET_BIT(materials[material_index + materials.Size()].attr, MAT_ATTR_SHADE_MODE_FASTPHONG);
						} // end if
						else
						{
							// set the shading mode flag in material
							SET_BIT(materials[material_index + materials.Size()].attr, MAT_ATTR_SHADE_MODE_FLAT);
						} // end else

						break;
					} // end if

				} // end while

				// found the material, break out of while for another pass
				break;

			} // end if found material

		} // end while looking for mat#1

	} // end for curr_material

	// before performing texture application we need to determine if the texture on this
	// object is mipmapped, if so we need to create a mip map chain, and set the proper 
	// attributes in both the object and the polygons themselves
	if (mipmap)
	{
		// set the mipmap bit in the object
		SET_BIT(obj->_attr, OBJECT_ATTR_MIPMAP);

		// now with the base texture as level d=0 call the mipmap chain generator
		GenerateMipmaps(*obj->_texture, &obj->_texture); // (BITMAP_IMAGE_PTR *)&obj->_texture);
	} // end if 

	// at this point poly_material[] holds all the indices for the polygon materials (zero based, so they need fix up)
	// and we must access the materials array to fill in each polygon with the polygon color, etc.
	// now that we finally have the material libary loaded
	for (int curr_poly = 0; curr_poly < obj->_num_polys; curr_poly++)
	{
		Modules::GetLog().WriteError("\nfixing poly material %d from index %d to index %d", curr_poly, 
			poly_material[curr_poly],
			poly_material[curr_poly] + materials.Size()  );
		// fix up offset
		poly_material[curr_poly]  = poly_material[curr_poly] + materials.Size();

		// we need to know what color depth we are dealing with, so check
		// the bits per pixel, this assumes that the system has already
		// made the call to DDraw_Init() or set the bit depth
		if (Modules::GetGraphics().GetDDPixelFormat() == 16)
		{
			// cool, 16 bit mode
			SET_BIT(obj->_plist[curr_poly].attr,POLY_ATTR_RGB16);

			// test if this is a textured poly, if so override the color to WHITE,
			// so we get maximum reflection in lighting stage
			if (materials[ poly_material[curr_poly] ].attr & MAT_ATTR_SHADE_MODE_TEXTURE)
				obj->_plist[curr_poly].color = Modules::GetGraphics().GetColor(255,255,255);
			else
				obj->_plist[curr_poly].color = Modules::GetGraphics().GetColor(
					materials[ poly_material[curr_poly] ].color.r, 
					materials[ poly_material[curr_poly] ].color.g, 
					materials[ poly_material[curr_poly] ].color.b);
			Modules::GetLog().WriteError("\nPolygon 16-bit");
		} // end
		else if (Modules::GetGraphics().GetDDPixelFormat() == 32)
		{
			// cool, 32 bit mode
			SET_BIT(obj->_plist[curr_poly].attr,POLY_ATTR_RGB24);

			// test if this is a textured poly, if so override the color to WHITE,
			// so we get maximum reflection in lighting stage
			if (materials[ poly_material[curr_poly] ].attr & MAT_ATTR_SHADE_MODE_TEXTURE)
				obj->_plist[curr_poly].color = _RGB32BIT(255, 255, 255, 255);
			else
				obj->_plist[curr_poly].color = _RGB32BIT(materials[poly_material[curr_poly]].color.a,
														 materials[poly_material[curr_poly]].color.r,
														 materials[poly_material[curr_poly]].color.g,
														 materials[poly_material[curr_poly]].color.b);
			Modules::GetLog().WriteError("\nPolygon 32-bit");
		} // end
		else
		{
			// todo: zz
// 			// 8 bit mode
// 			SET_BIT(obj->_plist[curr_poly].attr,POLY_ATTR_8BITCOLOR);
// 			// test if this is a textured poly, if so override the color to WHITE,
// 			// so we get maximum reflection in lighting stage
// 			if (materials[ poly_material[curr_poly] ].attr & MAT_ATTR_SHADE_MODE_TEXTURE)
// 				obj->_plist[curr_poly].color = RGBto8BitIndex(255, 255, 255, palette, 0);
// 			else
// 			obj->_plist[curr_poly].color = RGBto8BitIndex(materials[ poly_material[curr_poly] ].color.r,
// 				materials[ poly_material[curr_poly] ].color.g,
// 				materials[ poly_material[curr_poly] ].color.b,
// 				palette, 0);
// 
// 			Modules::GetLog().WriteError("\nPolygon 8-bit, index=%d", obj->_plist[curr_poly].color);
		} // end else

		// now set all the shading flags
		// figure out which shader to use
		if (materials[ poly_material[curr_poly] ].attr & MAT_ATTR_SHADE_MODE_CONSTANT)
		{
			// set shading mode
			SET_BIT(obj->_plist[curr_poly].attr, POLY_ATTR_SHADE_MODE_CONSTANT);
		} // end if
		else if (materials[ poly_material[curr_poly] ].attr & MAT_ATTR_SHADE_MODE_FLAT)
		{
			// set shading mode
			SET_BIT(obj->_plist[curr_poly].attr, POLY_ATTR_SHADE_MODE_FLAT);
		} // end if
		else if (materials[ poly_material[curr_poly] ].attr & MAT_ATTR_SHADE_MODE_GOURAUD)
		{
			// set shading mode
			SET_BIT(obj->_plist[curr_poly].attr, POLY_ATTR_SHADE_MODE_GOURAUD);

			// going to need vertex normals!
			SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly].vert[0] ].attr, VERTEX_ATTR_NORMAL); 
			SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly].vert[1] ].attr, VERTEX_ATTR_NORMAL); 
			SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly].vert[2] ].attr, VERTEX_ATTR_NORMAL); 

		} // end if
		else if (materials[ poly_material[curr_poly] ].attr & MAT_ATTR_SHADE_MODE_FASTPHONG)
		{
			// set shading mode
			SET_BIT(obj->_plist[curr_poly].attr, POLY_ATTR_SHADE_MODE_FASTPHONG);

			// going to need vertex normals!
			SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly].vert[0] ].attr, VERTEX_ATTR_NORMAL); 
			SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly].vert[1] ].attr, VERTEX_ATTR_NORMAL); 
			SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly].vert[2] ].attr, VERTEX_ATTR_NORMAL); 

		} // end if
		else
		{
			// set shading mode to default flat
			SET_BIT(obj->_plist[curr_poly].attr, POLY_ATTR_SHADE_MODE_FLAT);
		} // end if

		if (materials[ poly_material[curr_poly] ].attr & MAT_ATTR_SHADE_MODE_TEXTURE)
		{
			// set shading mode
			SET_BIT(obj->_plist[curr_poly].attr, POLY_ATTR_SHADE_MODE_TEXTURE);

			// apply texture to this polygon
			obj->_plist[curr_poly].texture = obj->_texture;

			// if the object was mipmapped this above assignment will just point the texture to
			// the mipmap chain array, however we still need to set the polygon attribute, so 
			// we know the poly is mip mapped, since once its in the rendering list we will have
			// no idea
			if (mipmap)
				SET_BIT(obj->_plist[curr_poly].attr,POLY_ATTR_MIPMAP);

			// set texture coordinate attributes
			SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly].vert[0] ].attr, VERTEX_ATTR_TEXTURE); 
			SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly].vert[1] ].attr, VERTEX_ATTR_TEXTURE); 
			SET_BIT(obj->_vlist_local[ obj->_plist[curr_poly].vert[2] ].attr, VERTEX_ATTR_TEXTURE); 
		} // end if

		// now test for alpha channel
		if (materials[ poly_material[curr_poly] ].attr & MAT_ATTR_TRANSPARENT)
		{
			// set the value of the alpha channel, upper 8-bits of color will be used to hold it
			// lets hope this doesn't break the lighting engine!!!!
			obj->_plist[curr_poly].color+= (materials[ poly_material[curr_poly] ].color.a << 24);

			// set the alpha flag in polygon  
			SET_BIT(obj->_plist[curr_poly].attr, POLY_ATTR_TRANSPARENT);

		} // end if

		// set the material mode to ver. 1.0 emulation (for now only!!!)
		SET_BIT(obj->_plist[curr_poly].attr, POLY_ATTR_DISABLE_MATERIAL);
	} // end for curr_poly

	// local object materials have been added to database, update total materials in system
	materials.SetSize(materials.Size() + num_materials_object);

	// now fix up all texture coordinates
	if (obj->_texture)
	{
		for (int tvertex = 0; tvertex < num_texture_vertices; tvertex++)
		{
			// step 1: scale the texture coordinates by the texture size in lod = 0
			// make sure to access mipmap chain if needed!!!!!!!
			int texture_size; 
			if (!mipmap)
				texture_size = obj->_texture->Width(); 
			else // must be a mipmap chain, cast pointer to ptr to array of pointers and access elem 0
				texture_size = ((BmpImg**)(obj->_texture))[0]->Width();

			// scale 0..1 to 0..texture_size-1
			obj->_tlist[tvertex].x *= (texture_size-1); 
			obj->_tlist[tvertex].y *= (texture_size-1);

			// now test for vertex transformation flags
			if (vertex_flags & VERTEX_FLAGS_INVERT_TEXTURE_U)  
			{
				obj->_tlist[tvertex].x = (texture_size-1) - obj->_tlist[tvertex].x;
			} // end if

			if (vertex_flags & VERTEX_FLAGS_INVERT_TEXTURE_V)  
			{
				obj->_tlist[tvertex].y = (texture_size-1) - obj->_tlist[tvertex].y;
			} // end if

			if (vertex_flags & VERTEX_FLAGS_INVERT_SWAP_UV)  
			{
				float temp;
				std::swap(obj->_tlist[tvertex].x, obj->_tlist[tvertex].y);
			} // end if

		} // end for

	} // end if there was a texture loaded for this object

#ifdef DEBUG_ON
	for (curr_material = 0; curr_material < materials.Size(); curr_material++)
	{
		Modules::GetLog().WriteError("\nMaterial %d", curr_material);

		Modules::GetLog().WriteError("\nint  state    = %d", materials[curr_material].state);
		Modules::GetLog().WriteError("\nint  id       = %d", materials[curr_material].id);
		Modules::GetLog().WriteError("\nchar name[64] = %s", materials[curr_material].name);
		Modules::GetLog().WriteError("\nint  attr     = %d", materials[curr_material].attr); 
		Modules::GetLog().WriteError("\nint r         = %d", materials[curr_material].color.r); 
		Modules::GetLog().WriteError("\nint g         = %d", materials[curr_material].color.g); 
		Modules::GetLog().WriteError("\nint b         = %d", materials[curr_material].color.b); 
		Modules::GetLog().WriteError("\nint alpha     = %d", materials[curr_material].color.a);
		Modules::GetLog().WriteError("\nint color     = %d", materials[curr_material].attr); 
		Modules::GetLog().WriteError("\nfloat ka      = %f", materials[curr_material].ka); 
		Modules::GetLog().WriteError("\nkd            = %f", materials[curr_material].kd); 
		Modules::GetLog().WriteError("\nks            = %f", materials[curr_material].ks); 
		Modules::GetLog().WriteError("\npower         = %f", materials[curr_material].power);
		Modules::GetLog().WriteError("\nchar texture_file = %s\n", materials[curr_material].texture_file);
	} // end for curr_material
#endif

	// now that we know the correct number of polygons, we must allocate memory for them
	// and fix up the object, this is a hack, but the file formats are so stupid by not
	// all starting with NUM_VERTICES, NUM_POLYGONS -- that would make everyone's life
	// easier!

#if 0

	// step 1: allocate memory for the polygons
	POLY4DV1_PTR plist_temp = NULL;

	// allocate memory for polygon list, the correct number of polys was overwritten
	// into the object during parsing, so we can use the num_polys field
	if (!(plist_temp = (POLY4DV1_PTR)malloc(sizeof(POLY4DV1)*obj->_num_polys)))
		return(0);

	// step 2:  now copy the polygons into the correct list
	memcpy((void *)plist_temp, (void *)obj->_plist, sizeof(POLY4DV1));

	// step 3: now free the old memory and fix the pointer
	free(obj->_plist);

	// now fix the pointer
	obj->_plist = plist_temp;

#endif

	// compute the polygon normal lengths
	obj->ComputePolyNormals();

	// compute vertex normals for any gouraud shaded polys
	obj->ComputeVertexNormals();

	// return success
	return(1);
}

}