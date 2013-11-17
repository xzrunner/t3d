#include "RenderObject.h"

#include <float.h>

#include "Modules.h"
#include "Log.h"
#include "Graphics.h"
#include "Camera.h"
#include "PrimitiveDraw.h"
#include "Light.h"
#include "BmpFile.h"
#include "BmpImg.h"

namespace t3d {

RenderObject::RenderObject()
{
	memset(this, 0, sizeof(RenderObject));
	_state = OBJECT_STATE_NULL;
}

int RenderObject::Init(int num_vertices, int num_polys, int num_frames, bool destroy)
{
	// this function does nothing more than allocate the memory for an OBJECT4DV2
	// based on the sent data, later we may want to create more robust initializers
	// but the problem is that we don't want to tie the initializer to anthing yet
	// in 99% of cases this all will be done by the call to load object
	// we just might need this function if we manually want to build an object???


	// first destroy the object if it exists
	if (destroy)
		Destroy();

	// allocate memory for vertex lists
	if (!(_vlist_local = (Vertex*)malloc(sizeof(Vertex)*num_vertices*num_frames)))
		return(0);

	// clear data
	memset((void *)_vlist_local, 0, sizeof(Vertex)*num_vertices*num_frames);

	if (!(_vlist_trans = (Vertex*)malloc(sizeof(Vertex)*num_vertices*num_frames)))
		return(0);

	// clear data
	memset((void *)_vlist_trans,0,sizeof(Vertex)*num_vertices*num_frames);

	// number of texture coordinates always 3*number of polys
	if (!(_tlist = (vec2*)malloc(sizeof(vec2)*num_polys*3)))
		return(0);

	// clear data
	memset((void *)_tlist,0,sizeof(vec2)*num_polys*3);


	// allocate memory for radii arrays
	if (!(_avg_radius = (float *)malloc(sizeof(float)*num_frames)))
		return(0);

	// clear data
	memset((void *)_avg_radius,0,sizeof(float)*num_frames);


	if (!(_max_radius = (float *)malloc(sizeof(float)*num_frames)))
		return(0);

	// clear data
	memset((void *)_max_radius,0,sizeof(float)*num_frames);

	// allocate memory for polygon list
	if (!(_plist = (Polygon*)malloc(sizeof(Polygon)*num_polys)))
		return(0);

	// clear data
	memset((void *)_plist,0,sizeof(Polygon)*num_polys);

	// alias head pointers
	_head_vlist_local = _vlist_local;
	_head_vlist_trans = _vlist_trans;

	// set some internal variables
	_num_frames     = num_frames;
	_num_polys      = num_polys;
	_num_vertices   = num_vertices;
	_total_vertices = num_vertices*num_frames;

	// return success
	return(1);
}

int RenderObject::Destroy()
{
	// this function destroys the sent object, basically frees the memory
	// if any that has been allocated

	// local vertex list
	if (_head_vlist_local)
		free(_head_vlist_local);

	// transformed vertex list
	if (_head_vlist_trans)
		free(_head_vlist_trans);

	// texture coordinate list
	if (_tlist)
		free(_tlist);

	// polygon list
	if (_plist)
		free(_plist);

	// object radii arrays
	if (_avg_radius)
		free(_avg_radius);

	if (_max_radius)
		free(_max_radius);

	// now clear out object completely
	memset((void *)this, 0, sizeof(RenderObject));

	// return success
	return(1);
}

int RenderObject::SetFrame(int frame)
{
	// this functions sets the current frame in a multi frame object
	// if the object is not multiframed then the function has no effect
	// totally external function

	// test if its multiframe?
	if (!(_attr & OBJECT_ATTR_MULTI_FRAME))
		return(0);

	// we have a valid object and its multiframe, vector pointers to frame data

	// check bounds on frame
	if (frame < 0 )
		frame = 0;
	else if (frame >= _num_frames)
		frame = _num_frames - 1;

	// set frame
	_curr_frame = frame;

	// update pointers to point to "block" of vertices that represent this frame
	// the vertices from each frame are 1-1 and onto relative to the polygons that
	// make up the object, they are simply shifted, this we need to re-vector the 
	// pointers based on the head pointers

	_vlist_local = &(_head_vlist_local[frame*_num_vertices]);
	_vlist_trans = &(_head_vlist_trans[frame*_num_vertices]);

	// return success
	return(1);
}

int RenderObject::LoadPLG(const char* filename, const vec4* scale, const vec4* pos, const vec4* rot) 
{
	return load_plg(this, filename, scale, pos, rot);
}

int RenderObject::Load3DSASC(char *filename, const vec4* scale, const vec4* pos, 
							 const vec4* rot, int vertex_flags) 
{
	return load_3dsasc(this, filename, scale, pos, rot, vertex_flags);
}

int RenderObject::LoadCOB(char *filename, const vec4* scale, const vec4* pos, 
						  const vec4* rot, int vertex_flags, bool mipmap)
{
	return load_cob(this, filename, scale, pos, rot, vertex_flags, mipmap);
}

int RenderObject::GenerateTerrain(float twidth, float theight, float vscale, 
								  char *height_map_file, char *texture_map_file, 
								  int rgbcolor, vec4* pos, vec4* rot, int poly_attr,
								  int sea_level, int alpha)
{
	// this function generates a terrain of width x height in the x-z plane
	// the terrain is defined by a height field encoded as color values 
	// of a 256 color texture, 0 being ground level 255 being 1.0, this value
	// is scaled by vscale for the height of each point in the height field
	// the height field generated will be composed of triangles where each vertex
	// in the height field is derived from the height map, thus if the height map
	// is 256 x 256 points then the final mesh will be (256-1) x (256-1) polygons 
	// with a absolute world size of width x height (in the x-z plane)
	// also if there is a texture map file then it will be mapped onto the terrain
	// and texture coordinates will be generated
	// this version allows transparency control for "sea level", if transparency is 
	// desired to create transparent water, send a non negative for alpha
	// alpha must be between 0 and 255, 255.0 being fully opaque

	char buffer[256];  // working buffer

	float col_tstep, row_tstep;
	float col_vstep, row_vstep;
	int columns, rows;

	int rgbwhite;

	// Step 1: clear out the object and initialize it a bit
	memset(this, 0, sizeof(RenderObject));

	// set state of object to active and visible
	_state = OBJECT_STATE_ACTIVE | OBJECT_STATE_VISIBLE;

	// set position of object
	_world_pos.x = pos->x;
	_world_pos.y = pos->y;
	_world_pos.z = pos->z;
	_world_pos.w = pos->w;

	// create proper color word based on selected bit depth of terrain
	// rgbcolor is always in rgb5.6.5 format, so only need to downconvert for
	// 8-bit mesh
	if (poly_attr & POLY_ATTR_8BITCOLOR)
	{ 
// 		rgbcolor = rgblookup[rgbcolor];
// 		rgbwhite = rgblookup[RGB16Bit(255,255,255)];
	} // end if
	else
	{
		rgbwhite = _RGB32BIT(255,255,255,255);
	} // end else

	// set number of frames
	_num_frames = 1;
	_curr_frame = 0;
	_attr = OBJECT_ATTR_SINGLE_FRAME;

	// Step 2: load in the height field
	BmpFile* height_bitmap = new BmpFile(height_map_file); // holds the height bitmap

	// compute basic information
	columns = height_bitmap->Width();
	rows    = height_bitmap->Height();

	col_vstep = twidth / (float)(columns - 1);
	row_vstep = theight / (float)(rows - 1);

	sprintf(_name ,"Terrain:%s%s", height_map_file, texture_map_file);
	_num_vertices = columns * rows;
	_num_polys    = ((columns - 1) * (rows - 1) ) * 2;

	// store some results to help with terrain following
	// use the auxialiary variables in the object -- might as well!
	_ivar1 = columns;
	_ivar2 = rows;
	_fvar1 = col_vstep;
	_fvar2 = row_vstep;

	// allocate the memory for the vertices and number of polys
	// the call parameters are redundant in this case, but who cares
	if (!Init(_num_vertices, _num_polys, _num_frames))
	{
		Modules::GetLog().WriteError("\nTerrain generator error (can't allocate memory).");
	} // end if

	// load texture map if there is one
	if ( (poly_attr & POLY_ATTR_SHADE_MODE_TEXTURE) && texture_map_file)
	{
		// load the texture from disk
		BmpFile* texture_bitmap = new BmpFile(texture_map_file);

		// create a proper size and bitdepth bitmap
		_texture = new BmpImg(0, 0, texture_bitmap->Width(), texture_bitmap->Height(), texture_bitmap->BitCount());

		// load the bitmap image (later make this 8/16 bit)
		if (_texture->BitsPerPixel() == 32)
			_texture->LoadImage32(*texture_bitmap, 0, 0, BITMAP_EXTRACT_MODE_ABS);
		if (_texture->BitsPerPixel() == 16)
			_texture->LoadImage16(*texture_bitmap, 0, 0, BITMAP_EXTRACT_MODE_ABS);
		else
			_texture->LoadImage8(*texture_bitmap, 0, 0, BITMAP_EXTRACT_MODE_ABS);

		// compute stepping factors in texture map for texture coordinate computation
		col_tstep = (float)(texture_bitmap->Width()-1)/(float)(columns - 1);
		row_tstep = (float)(texture_bitmap->Height()-1)/(float)(rows - 1);

		// flag object as having textures
		SET_BIT(_attr, OBJECT_ATTR_TEXTURES);

		// done, so unload the bitmap
		delete texture_bitmap;
	} // end if

	Modules::GetLog().WriteError("\ncolumns = %d, rows = %d", columns, rows);
	Modules::GetLog().WriteError("\ncol_vstep = %f, row_vstep = %f", col_vstep, row_vstep);
	Modules::GetLog().WriteError("\ncol_tstep=%f, row_tstep=%f", col_tstep, row_tstep);
	Modules::GetLog().WriteError("\nnum_vertices = %d, num_polys = %d", _num_vertices, _num_polys);

	// Step 4: generate the vertex list, and texture coordinate list in row major form
	for (int curr_row = 0; curr_row < rows; curr_row++)
	{
		for (int curr_col = 0; curr_col < columns; curr_col++)
		{
			int vertex = (curr_row * columns) + curr_col;
			// compute the vertex
			_vlist_local[vertex].x = curr_col * col_vstep - (twidth/2);
			_vlist_local[vertex].y = vscale*((float)height_bitmap->Buffer()[curr_col + (curr_row * columns) ]) / 255;
			_vlist_local[vertex].z = curr_row * row_vstep - (theight/2);

			_vlist_local[vertex].w = 1;  

			// every vertex has a point at least, set that in the flags attribute
			SET_BIT(_vlist_local[vertex].attr, VERTEX_ATTR_POINT);

			// need texture coord?
			if ( (poly_attr & POLY_ATTR_SHADE_MODE_TEXTURE) && texture_map_file)
			{
				// now texture coordinates
				_tlist[vertex].x = curr_col * col_tstep;
				_tlist[vertex].y = curr_row * row_tstep;

			} // end if

			Modules::GetLog().WriteError("\nVertex %d: V[%f, %f, %f], T[%f, %f]", vertex, _vlist_local[vertex].x,
				_vlist_local[vertex].y,
				_vlist_local[vertex].z,
				_tlist[vertex].x,
				_tlist[vertex].y);


		} // end for curr_col

	} // end curr_row

	// perform rotation transformation?

	// compute average and max radius
	ComputeRadius();

	Modules::GetLog().WriteError("\nObject average radius = %f, max radius = %f", 
		_avg_radius[0], _max_radius[0]);

	// Step 5: generate the polygon list
	for (int poly=0; poly < _num_polys/2; poly++)
	{
		// polygons follow a regular pattern of 2 per square, row
		// major form, finding the correct indices is a pain, but 
		// the bottom line is we have an array of vertices mxn and we need
		// a list of polygons that is (m-1) x (n-1), with 2 triangles per
		// square with a consistent winding order... this is one one to arrive
		// at the indices, another way would be to use two loops, etc., 

		int base_poly_index = (poly % (columns-1)) + (columns * (poly / (columns - 1)) );

		// upper left poly
		_plist[poly*2].vert[0] = base_poly_index;
		_plist[poly*2].vert[1] = base_poly_index+columns;
		_plist[poly*2].vert[2] = base_poly_index+columns+1;

		// lower right poly
		_plist[poly*2+1].vert[0] = base_poly_index;
		_plist[poly*2+1].vert[1] = base_poly_index+columns+1;
		_plist[poly*2+1].vert[2] = base_poly_index+1;

		// point polygon vertex list to object's vertex list
		// note that this is redundant since the polylist is contained
		// within the object in this case and its up to the user to select
		// whether the local or transformed vertex list is used when building up
		// polygon geometry, might be a better idea to set to NULL in the context
		// of polygons that are part of an object
		_plist[poly*2].vlist = _vlist_local; 
		_plist[poly*2+1].vlist = _vlist_local; 

		// set attributes of polygon with sent attributes
		_plist[poly*2].attr = poly_attr;
		_plist[poly*2+1].attr = poly_attr;

		// now perform some test to make sure any secondary data elements are
		// set properly

		// set color of polygon
		_plist[poly*2].color = rgbcolor;
		_plist[poly*2+1].color = rgbcolor;

		// check for gouraud of phong shading, if so need normals
		if ( (_plist[poly*2].attr & POLY_ATTR_SHADE_MODE_GOURAUD) ||  
			(_plist[poly*2].attr & POLY_ATTR_SHADE_MODE_PHONG) )
		{
			// the vertices from this polygon all need normals, set that in the flags attribute
			SET_BIT(_vlist_local[ _plist[poly*2].vert[0] ].attr, VERTEX_ATTR_NORMAL); 
			SET_BIT(_vlist_local[ _plist[poly*2].vert[1] ].attr, VERTEX_ATTR_NORMAL); 
			SET_BIT(_vlist_local[ _plist[poly*2].vert[2] ].attr, VERTEX_ATTR_NORMAL); 

			SET_BIT(_vlist_local[ _plist[poly*2+1].vert[0] ].attr, VERTEX_ATTR_NORMAL); 
			SET_BIT(_vlist_local[ _plist[poly*2+1].vert[1] ].attr, VERTEX_ATTR_NORMAL); 
			SET_BIT(_vlist_local[ _plist[poly*2+1].vert[2] ].attr, VERTEX_ATTR_NORMAL); 

		} // end if

		// if texture in enabled the enable texture coordinates
		if (poly_attr & POLY_ATTR_SHADE_MODE_TEXTURE)
		{
			// apply texture to this polygon
			_plist[poly*2].texture = _texture;
			_plist[poly*2+1].texture = _texture;

			// assign the texture coordinates
			// upper left poly
			_plist[poly*2].text[0] = base_poly_index;
			_plist[poly*2].text[1] = base_poly_index+columns;
			_plist[poly*2].text[2] = base_poly_index+columns+1;

			// lower right poly
			_plist[poly*2+1].text[0] = base_poly_index;
			_plist[poly*2+1].text[1] = base_poly_index+columns+1;
			_plist[poly*2+1].text[2] = base_poly_index+1;

			// override base color to make poly more reflective
			_plist[poly*2].color = rgbwhite;
			_plist[poly*2+1].color = rgbwhite;

			// set texture coordinate attributes
			SET_BIT(_vlist_local[ _plist[poly*2].vert[0] ].attr, VERTEX_ATTR_TEXTURE); 
			SET_BIT(_vlist_local[ _plist[poly*2].vert[1] ].attr, VERTEX_ATTR_TEXTURE); 
			SET_BIT(_vlist_local[ _plist[poly*2].vert[2] ].attr, VERTEX_ATTR_TEXTURE); 

			SET_BIT(_vlist_local[ _plist[poly*2+1].vert[0] ].attr, VERTEX_ATTR_TEXTURE); 
			SET_BIT(_vlist_local[ _plist[poly*2+1].vert[1] ].attr, VERTEX_ATTR_TEXTURE); 
			SET_BIT(_vlist_local[ _plist[poly*2+1].vert[2] ].attr, VERTEX_ATTR_TEXTURE); 

		} // end if

		// check for alpha enable for sea level polygons to help with water effect
		if (alpha >=0 )
		{
			// compute heights of both polygons
			float avg_y_poly1 = (_vlist_local[ _plist[poly*2].vert[0] ].y + 
				_vlist_local[ _plist[poly*2].vert[1] ].y + 
				_vlist_local[ _plist[poly*2].vert[2] ].y )/3;

			float avg_y_poly2 = (_vlist_local[ _plist[poly*2+1].vert[0] ].y + 
				_vlist_local[ _plist[poly*2+1].vert[1] ].y + 
				_vlist_local[ _plist[poly*2+1].vert[2] ].y )/3;


			// test height of poly1 relative to sea level
			if (avg_y_poly1 <= sea_level)
			{
//				int ialpha = (int)( (float)alpha/255 * (float)(NUM_ALPHA_LEVELS-1) + (float)0.5);

				// set the alpha color
//				_plist[poly*2+0].color+= (ialpha << 24);
				_plist[poly*2+0].color &= (alpha << 24);

				// set the alpha flag in polygon  
				SET_BIT(_plist[poly*2+0].attr, POLY_ATTR_TRANSPARENT);
			} // end if


			// test height of poly1 relative to sea level
			if (avg_y_poly2 <= sea_level)
			{
//				int ialpha = (int)( (float)alpha/255 * (float)(NUM_ALPHA_LEVELS-1) + (float)0.5);

				// set the alpha color
//				_plist[poly*2+1].color+= (ialpha << 24);
				_plist[poly*2+1].color &= (alpha << 24);

				// set the alpha flag in polygon  
				SET_BIT(_plist[poly*2+1].attr, POLY_ATTR_TRANSPARENT);
			} // end if

		} // end if 

		// set the material mode to ver. 1.0 emulation
		SET_BIT(_plist[poly*2].attr, POLY_ATTR_DISABLE_MATERIAL);
		SET_BIT(_plist[poly*2+1].attr, POLY_ATTR_DISABLE_MATERIAL);

		// finally set the polygon to active
		_plist[poly*2].state = POLY_STATE_ACTIVE;    
		_plist[poly*2+1].state = POLY_STATE_ACTIVE;  

		// point polygon vertex list to object's vertex list
		// note that this is redundant since the polylist is contained
		// within the object in this case and its up to the user to select
		// whether the local or transformed vertex list is used when building up
		// polygon geometry, might be a better idea to set to NULL in the context
		// of polygons that are part of an object
		_plist[poly*2].vlist = _vlist_local; 
		_plist[poly*2+1].vlist = _vlist_local; 

		// set texture coordinate list, this is needed
		_plist[poly*2].tlist = _tlist;
		_plist[poly*2+1].tlist = _tlist;

	} // end for poly
#if 0
	for (poly=0; poly < _num_polys; poly++)
	{
		Modules::GetLog().WriteError("\nPoly %d: Vi[%d, %d, %d], Ti[%d, %d, %d]",poly,
			_plist[poly].vert[0],
			_plist[poly].vert[1],
			_plist[poly].vert[2],
			_plist[poly].text[0],
			_plist[poly].text[1],
			_plist[poly].text[2]);

	} // end 
#endif

	// compute the polygon normal lengths
	ComputePolyNormals();

	// compute vertex normals for any gouraud shaded polys
	ComputeVertexNormals();

	// return success
	return(1);
}

void RenderObject::Reset()
{
	// this function resets the sent object and redies it for 
	// transformations, basically just resets the culled, clipped and
	// backface flags, but here's where you would add stuff
	// to ready any object for the pipeline
	// the object is valid, let's rip it apart polygon by polygon

	// reset object's culled flag
	RESET_BIT(_state, OBJECT_STATE_CULLED);

	// now the clipped and backface flags for the polygons 
	for (int poly = 0; poly < _num_polys; poly++)
	{
		// acquire polygon
		Polygon* curr_poly = &_plist[poly];

		// first is this polygon even visible?
		if (!(curr_poly->state & POLY_STATE_ACTIVE))
			continue; // move onto next poly

		// reset clipped and backface flags
		RESET_BIT(curr_poly->state, POLY_STATE_CLIPPED);
		RESET_BIT(curr_poly->state, POLY_STATE_BACKFACE);
		RESET_BIT(curr_poly->state, POLY_STATE_LIT);

	} // end for poly
}

void RenderObject::Scale(const vec3& scale, bool all_frames)
{
	// NOTE: Not matrix based
	// this function scales and object without matrices 
	// modifies the object's local vertex list 
	// additionally the radii is updated for the object

	// for each vertex in the mesh scale the local coordinates by
	// vs on a componentwise basis, that is, sx, sy, sz
	// also, since the object may hold multiple frames this update
	// will ONLY scale the currently selected frame in default mode
	// unless all_frames = 1, in essence the function emulates version 1
	// for single frame objects
	// also, normal lengths will change if the object is scaled!
	// so we must scale those too, we simple need to scale the length of
	// each normal n*(sx*sy*sz) 

	if (!all_frames)
	{
		// perform transform on selected frame only
		for (int vertex=0; vertex < _num_vertices; vertex++)
		{
			_vlist_local[vertex].x*=scale.x;
			_vlist_local[vertex].y*=scale.y;
			_vlist_local[vertex].z*=scale.z;
			// leave w unchanged, always equal to 1

		} // end for vertex

		// now since the object is scaled we have to do something with 
		// the radii calculation, but we don't know how the scaling
		// factors relate to the original major axis of the object,
		// therefore for scaling factors all ==1 we will simple multiply
		// which is correct, but for scaling factors not equal to 1, we
		// must take the largest scaling factor and use it to scale the
		// radii with since it's the worst case scenario of the new max and
		// average radii, the ONLY reason we do this is to keep the function
		// fast, you may want to recompute the radii if you need the accuracy

		// find max scaling factor
		float s = max(scale.x, scale.y, scale.z);

		// now scale current frame
		_max_radius[_curr_frame]*=s;
		_avg_radius[_curr_frame]*=s;

	} // end if
	else  // 
	{
		// perform transform
		for (int vertex=0; vertex < _total_vertices; vertex++)
		{
			_head_vlist_local[vertex].x*=scale.x;
			_head_vlist_local[vertex].y*=scale.y;
			_head_vlist_local[vertex].z*=scale.z;
			// leave w unchanged, always equal to 1

		} // end for vertex

		// now since the object is scaled we have to do something with 
		// the radii calculation, but we don't know how the scaling
		// factors relate to the original major axis of the object,
		// therefore for scaling factors all ==1 we will simple multiply
		// which is correct, but for scaling factors not equal to 1, we
		// must take the largest scaling factor and use it to scale the
		// radii with since it's the worst case scenario of the new max and
		// average radii,  the ONLY reason we do this is to keep the function
		// fast, you may want to recompute the radii if you need the accuracy

		for (int curr_frame = 0; curr_frame < _num_frames; curr_frame++)
		{
			// find max scaling factor
			float s = max(scale.x, scale.y, scale.z);

			// now scale
			_max_radius[curr_frame]*=s;
			_avg_radius[curr_frame]*=s;
		} // for

	} // end else

	// now scale the polygon normals
	for (int poly=0; poly < _num_polys; poly++)
	{
		_plist[poly].nlength*=(scale.x * scale.y * scale.z);
	} // end for poly
}

void RenderObject::Transform(const mat4& mt, int coord_select, 
							 int transform_basis, bool all_frames)
{

	// this function simply transforms all of the vertices in the local or trans
	// array by the sent matrix, since the object may have multiple frames, it
	// takes that into consideration
	// also vertex normals are rotated, however, if there is a translation factor
	// in the sent matrix that will corrupt the normals, later we might want to
	// null out the last row of the matrix before transforming the normals?
	// future optimization: set flag in object attributes, and objects without 
	// vertex normals can be rotated without the test in line


	// single frame or all frames?
	if (!all_frames)
	{
		// what coordinates should be transformed?
		switch(coord_select)
		{
		case TRANSFORM_LOCAL_ONLY:
			{
				// transform each local/model vertex of the object mesh in place
				for (int vertex=0; vertex < _num_vertices; vertex++)
				{
					vec4 presult; // hold result of each transformation

					// transform point
					_vlist_local[vertex].v = mt * _vlist_local[vertex].v;

					// transform vertex normal if needed
					if (_vlist_local[vertex].attr & VERTEX_ATTR_NORMAL)
					{
						// transform normal
						_vlist_local[vertex].n = mt * _vlist_local[vertex].n;
					} // end if

				} // end for index
			} break;

		case TRANSFORM_TRANS_ONLY:
			{
				// transform each "transformed" vertex of the object mesh in place
				// remember, the idea of the vlist_trans[] array is to accumulate
				// transformations
				for (int vertex=0; vertex < _num_vertices; vertex++)
				{
					vec4 presult; // hold result of each transformation

					// transform point
					_vlist_trans[vertex].v = mt * _vlist_trans[vertex].v;

					// transform vertex normal if needed
					if (_vlist_trans[vertex].attr & VERTEX_ATTR_NORMAL)
					{
						// transform normal
						_vlist_trans[vertex].n = mt * _vlist_trans[vertex].n;
					} // end if

				} // end for index

			} break;

		case TRANSFORM_LOCAL_TO_TRANS:
			{
				// transform each local/model vertex of the object mesh and store result
				// in "transformed" vertex list
				for (int vertex=0; vertex < _num_vertices; vertex++)
				{
					vec4 presult; // hold result of each transformation

					// transform point
					_vlist_trans[vertex].v = mt * _vlist_local[vertex].v;

					// transform vertex normal if needed
					if (_vlist_local[vertex].attr & VERTEX_ATTR_NORMAL)
					{
						// transform point
						_vlist_trans[vertex].n = mt * _vlist_local[vertex].n;
					} // end if

				} // end for index
			} break;

		default: break;

		} // end switch

	} // end if single frame
	else // transform all frames
	{
		// what coordinates should be transformed?
		switch(coord_select)
		{
		case TRANSFORM_LOCAL_ONLY:
			{
				// transform each local/model vertex of the object mesh in place
				for (int vertex=0; vertex < _total_vertices; vertex++)
				{
					vec4 presult; // hold result of each transformation

					// transform point
					_head_vlist_local[vertex].v = mt * _head_vlist_local[vertex].v;

					// transform vertex normal if needed
					if (_head_vlist_local[vertex].attr & VERTEX_ATTR_NORMAL)
					{
						// transform normal
						_head_vlist_local[vertex].n = mt * _head_vlist_local[vertex].n;
					} // end if


				} // end for index
			} break;

		case TRANSFORM_TRANS_ONLY:
			{
				// transform each "transformed" vertex of the object mesh in place
				// remember, the idea of the vlist_trans[] array is to accumulate
				// transformations
				for (int vertex=0; vertex < _total_vertices; vertex++)
				{
					vec4 presult; // hold result of each transformation

					// transform point
					_head_vlist_trans[vertex].v = mt * _head_vlist_trans[vertex].v;

					// transform vertex normal if needed
					if (_head_vlist_trans[vertex].attr & VERTEX_ATTR_NORMAL)
					{
						// transform normal
						_head_vlist_trans[vertex].n = mt * _head_vlist_trans[vertex].n;
					} // end if

				} // end for index

			} break;

		case TRANSFORM_LOCAL_TO_TRANS:
			{
				// transform each local/model vertex of the object mesh and store result
				// in "transformed" vertex list
				for (int vertex=0; vertex < _total_vertices; vertex++)
				{
					vec4 presult; // hold result of each transformation

					// transform point
					_head_vlist_trans[vertex].v = mt * _head_vlist_local[vertex].v;

					// transform vertex normal if needed
					if (_head_vlist_local[vertex].attr & VERTEX_ATTR_NORMAL)
					{
						// transform point
						_head_vlist_trans[vertex].n = mt * _head_vlist_local[vertex].n;
					} // end if

				} // end for index
			} break;

		default: break;

		} // end switch

	} // end else multiple frames

	// finally, test if transform should be applied to orientation basis
	// hopefully this is a rotation, otherwise the basis will get corrupted
	if (transform_basis)
	{
		// now rotate orientation basis for object
		vec4 vresult; // use to rotate each orientation vector axis

		// rotate ux of basis
		_ux = mt * _ux;

		// rotate uy of basis
		_uy = mt * _uy;

		// rotate uz of basis
		_uz = mt * _uz;
	} // end if
}

void RenderObject::ModelToWorld(int coord_select, bool all_frames)
{

	// NOTE: Not matrix based
	// this function converts the local model coordinates of the
	// sent object into world coordinates, the results are stored
	// in the transformed vertex list (vlist_trans) within the object

	// interate thru vertex list and transform all the model/local 
	// coords to world coords by translating the vertex list by
	// the amount world_pos and storing the results in vlist_trans[]
	// no need to transform vertex normals, they are invariant of position

	if (!all_frames)
	{
		if (coord_select == TRANSFORM_LOCAL_TO_TRANS)
		{
			for (int vertex=0; vertex < _num_vertices; vertex++)
			{
				// translate vertex
				_vlist_trans[vertex].v = _vlist_local[vertex].v + _world_pos;
				// copy normal
				_vlist_trans[vertex].n = _vlist_local[vertex].n;

			} // end for vertex
		} // end if local
		else
		{ // TRANSFORM_TRANS_ONLY
			for (int vertex=0; vertex < _num_vertices; vertex++)
			{
				// translate vertex
				_vlist_trans[vertex].v = _vlist_trans[vertex].v + _world_pos;
			} // end for vertex
		} // end else trans

	} // end if single frame
	else // all frames
	{
		if (coord_select == TRANSFORM_LOCAL_TO_TRANS)
		{
			for (int vertex=0; vertex < _total_vertices; vertex++)
			{
				// translate vertex
				_head_vlist_trans[vertex].v = _head_vlist_local[vertex].v + _world_pos;
				// copy normal
				_head_vlist_trans[vertex].n = _head_vlist_local[vertex].n;
			} // end for vertex
		} // end if local
		else
		{ // TRANSFORM_TRANS_ONLY
			for (int vertex=0; vertex < _total_vertices; vertex++)
			{
				// translate vertex
				_head_vlist_trans[vertex].v = _head_vlist_trans[vertex].v + _world_pos;
			} // end for vertex
		} // end else trans

	} // end if all frames
}

int RenderObject::Cull(const Camera& cam,	// camera to cull relative to
					   int cull_flags)		// clipping planes to consider
{

	// NOTE: is matrix based
	// this function culls an entire object from the viewing
	// frustrum by using the sent camera information and object
	// the cull_flags determine what axes culling should take place
	// x, y, z or all which is controlled by ORing the flags
	// together
	// if the object is culled its state is modified thats all
	// this function assumes that both the camera and the object
	// are valid!
	// also for OBJECT4DV2, only the current frame matters for culling


	// step 1: transform the center of the object's bounding
	// sphere into camera space

	vec4 sphere_pos; // hold result of transforming center of bounding sphere

	// transform point
	sphere_pos = cam.CameraMat() * _world_pos;

	// step 2:  based on culling flags remove the object
	if (cull_flags & CULL_OBJECT_Z_PLANE)
	{
		// cull only based on z clipping planes

		// test far plane
		if ( ((sphere_pos.z - _max_radius[_curr_frame]) > cam.FarClipZ()) ||
			((sphere_pos.z + _max_radius[_curr_frame]) < cam.NearClipZ()) )
		{ 
			SET_BIT(_state, OBJECT_STATE_CULLED);
			return(1);
		} // end if

	} // end if

	if (cull_flags & CULL_OBJECT_X_PLANE)
	{
		// cull only based on x clipping planes
		// we could use plane equations, but simple similar triangles
		// is easier since this is really a 2D problem
		// if the view volume is 90 degrees the the problem is trivial
		// buts lets assume its not

		// test the the right and left clipping planes against the leftmost and rightmost
		// points of the bounding sphere
		float z_test = (0.5)*cam.ViewplaneWidth()*sphere_pos.z/cam.ViewDist();

		if ( ((sphere_pos.x-_max_radius[_curr_frame]) > z_test)  || // right side
			((sphere_pos.x+_max_radius[_curr_frame]) < -z_test) )  // left side, note sign change
		{ 
			SET_BIT(_state, OBJECT_STATE_CULLED);
			return(1);
		} // end if
	} // end if

	if (cull_flags & CULL_OBJECT_Y_PLANE)
	{
		// cull only based on y clipping planes
		// we could use plane equations, but simple similar triangles
		// is easier since this is really a 2D problem
		// if the view volume is 90 degrees the the problem is trivial
		// buts lets assume its not

		// test the the top and bottom clipping planes against the bottommost and topmost
		// points of the bounding sphere
		float z_test = (0.5)*cam.ViewplaneHeight()*sphere_pos.z/cam.ViewDist();

		if ( ((sphere_pos.y-_max_radius[_curr_frame]) > z_test)  || // top side
			((sphere_pos.y+_max_radius[_curr_frame]) < -z_test) )  // bottom side, note sign change
		{ 
			SET_BIT(_state, OBJECT_STATE_CULLED);
			return(1);
		} // end if

	} // end if

	// return failure to cull
	return(0);
}

void RenderObject::RemoveBackfaces(const Camera& cam)
{

	// NOTE: this is not a matrix based function
	// this function removes the backfaces from an object's
	// polygon mesh, the function does this based on the vertex
	// data in vlist_trans along with the camera position (only)
	// note that only the backface state is set in each polygon
	// also since this works on polygons the current frame is the frame
	// that's vertices are used by the backface cull
	// note: only operates on the current frame

	// test if the object is culled
	if (_state & OBJECT_STATE_CULLED)
		return;

	// process each poly in mesh
	for (int poly=0; poly < _num_polys; poly++)
	{
		// acquire polygon
		Polygon* curr_poly = &_plist[poly];

		// is this polygon valid?
		// test this polygon if and only if it's not clipped, not culled,
		// active, and visible and not 2 sided. Note we test for backface in the event that
		// a previous call might have already determined this, so why work
		// harder!
		if (!(curr_poly->state & POLY_STATE_ACTIVE) ||
			(curr_poly->state & POLY_STATE_CLIPPED ) ||
			(curr_poly->attr  & POLY_ATTR_2SIDED)    ||
			(curr_poly->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// extract vertex indices into master list, rember the polygons are 
		// NOT self contained, but based on the vertex list stored in the object
		// itself
		int vindex_0 = curr_poly->vert[0];
		int vindex_1 = curr_poly->vert[1];
		int vindex_2 = curr_poly->vert[2];

		// we will use the transformed polygon vertex list since the backface removal
		// only makes sense at the world coord stage further of the pipeline 

		// we need to compute the normal of this polygon face, and recall
		// that the vertices are in cw order, u = p0->p1, v=p0->p2, n=uxv
		vec4 u, v, n;

		// build u, v
		u = _vlist_trans[ vindex_1 ].v - _vlist_trans[ vindex_0 ].v;
		v = _vlist_trans[ vindex_2 ].v - _vlist_trans[ vindex_0 ].v;

		// compute cross product
		n = u.Cross(v);

		// now create eye vector to viewpoint
		vec4 view = cam.Pos() - _vlist_trans[ vindex_0 ].v;

		// and finally, compute the dot product
		float dp = n.Dot(view);

		// if the sign is > 0 then visible, 0 = scathing, < 0 invisible
		if (dp <= 0.0 )
			SET_BIT(curr_poly->state, POLY_STATE_BACKFACE);

	} // end for poly
}

void RenderObject::WorldToCamera(const Camera& cam)
{
	// NOTE: this is a matrix based function
	// this function transforms the world coordinates of an object
	// into camera coordinates, based on the sent camera matrix
	// but it totally disregards the polygons themselves,
	// it only works on the vertices in the vlist_trans[] list
	// this is one way to do it, you might instead transform
	// the global list of polygons in the render list since you 
	// are guaranteed that those polys represent geometry that 
	// has passed thru backfaces culling (if any)
	// note: only operates on the current frame

	// transform each vertex in the object to camera coordinates
	// assumes the object has already been transformed to world
	// coordinates and the result is in vlist_trans[]
	for (int vertex = 0; vertex < _num_vertices; vertex++)
	{
		// transform the vertex by the mcam matrix within the camera
		// it better be valid!
		vec4 presult; // hold result of each transformation

		// transform point
		_vlist_trans[vertex].v = cam.CameraMat() * _vlist_trans[vertex].v;

	} // end for vertex
}

void RenderObject::CameraToPerspective(const Camera& cam)
{

	// NOTE: this is not a matrix based function
	// this function transforms the camera coordinates of an object
	// into perspective coordinates, based on the 
	// sent camera object, but it totally disregards the polygons themselves,
	// it only works on the vertices in the vlist_trans[] list
	// this is one way to do it, you might instead transform
	// the global list of polygons in the render list since you 
	// are guaranteed that those polys represent geometry that 
	// has passed thru backfaces culling (if any)
	// finally this function is really for experimental reasons only
	// you would probably never let an object stay intact this far down
	// the pipeline, since it's probably that there's only a single polygon
	// that is visible! But this function has to transform the whole mesh!
	// note: only operates on the current frame

	// transform each vertex in the object to perspective coordinates
	// assumes the object has already been transformed to camera
	// coordinates and the result is in vlist_trans[]
	for (int vertex = 0; vertex < _num_vertices; vertex++)
	{
		float z = _vlist_trans[vertex].z;

		// transform the vertex by the view parameters in the camera
		_vlist_trans[vertex].x = cam.ViewDist()*_vlist_trans[vertex].x/z;
		_vlist_trans[vertex].y = cam.ViewDist()*_vlist_trans[vertex].y*cam.AspectRatio()/z;
		// z = z, so no change

		// not that we are NOT dividing by the homogenous w coordinate since
		// we are not using a matrix operation for this version of the function 

	} // end for vertex

} // end CameraToPerspective

void RenderObject::PerspectiveToScreen(const Camera& cam)
{

	// NOTE: this is not a matrix based function
	// this function transforms the perspective coordinates of an object
	// into screen coordinates, based on the sent viewport info
	// but it totally disregards the polygons themselves,
	// it only works on the vertices in the vlist_trans[] list
	// this is one way to do it, you might instead transform
	// the global list of polygons in the render list since you 
	// are guaranteed that those polys represent geometry that 
	// has passed thru backfaces culling (if any)
	// finally this function is really for experimental reasons only
	// you would probably never let an object stay intact this far down
	// the pipeline, since it's probably that there's only a single polygon
	// that is visible! But this function has to transform the whole mesh!
	// this function would be called after a perspective
	// projection was performed on the object

	// transform each vertex in the object to screen coordinates
	// assumes the object has already been transformed to perspective
	// coordinates and the result is in vlist_trans[]
	// note: only operates on the current frame

	float alpha = (0.5*cam.ViewportWidth()-0.5);
	float beta  = (0.5*cam.ViewportHeight()-0.5);

	for (int vertex = 0; vertex < _num_vertices; vertex++)
	{
		// assumes the vertex is in perspective normalized coords from -1 to 1
		// on each axis, simple scale them to viewport and invert y axis and project
		// to screen

		// transform the vertex by the view parameters in the camera
		_vlist_trans[vertex].x = alpha + alpha*_vlist_trans[vertex].x;
		_vlist_trans[vertex].y = beta  - beta *_vlist_trans[vertex].y;

	} // end for vertex
}

// float RenderObject::GetMaxRadius() const
// {
// 	float ret = 0;
// 	for (size_t i = 0; i < _num_frames; ++i)
// 		if (_max_radius[i] > ret)
// 			ret = _max_radius[i];
// 	return ret;
// }

void RenderObject::LightWorld32(const Camera& cam)
{
	// 32-bit version of function
	// function lights an object based on the sent lights and camera. the function supports
	// constant/pure shading (emmisive), flat shading with ambient, infinite, point lights, and spot lights
	// note that this lighting function is rather brute force and simply follows the math, however
	// there are some clever integer operations that are used in scale 256 rather than going to floating
	// point, but why? floating point and ints are the same speed, HOWEVER, the conversion to and from floating
	// point can be cycle intensive, so if you can keep your calcs in ints then you can gain some speed
	// also note, type 1 spot lights are simply point lights with direction, the "cone" is more of a function
	// of the falloff due to attenuation, but they still look like spot lights
	// type 2 spot lights are implemented with the intensity having a dot product relationship with the
	// angle from the surface point to the light direction just like in the optimized model, but the pf term
	// that is used for a concentration control must be 1,2,3,.... integral and non-fractional

	unsigned int tmpa;
	unsigned int r_base, g_base, b_base,  // base color being lit
				 r_sum,  g_sum,  b_sum,   // sum of lighting process over all lights
				 r_sum0,  g_sum0,  b_sum0,
				 r_sum1,  g_sum1,  b_sum1,
				 r_sum2,  g_sum2,  b_sum2,
				 ri, gi, bi,
				 shaded_color;            // final color

	float dp,     // dot product 
		  dist,   // distance from light to surface
		  dists,
		  i,      // general intensities
		  nl,     // length of normal
		  atten;  // attenuation computations

	vec4 u, v, n, l, d, s; // used for cross product and light vector calculations

	// test if the object is culled
	if (!(_state & OBJECT_STATE_ACTIVE) ||
		(_state & OBJECT_STATE_CULLED) ||
		!(_state & OBJECT_STATE_VISIBLE))
		return; 

	// for each valid poly, light it...
	for (int poly=0; poly < _num_polys; poly++)
	{
		// acquire polygon
		Polygon* curr_poly = &_plist[poly];

		// is this polygon valid?
		// test this polygon if and only if it's not clipped, not culled,
		// active, and visible. Note we test for backface in the event that
		// a previous call might have already determined this, so why work
		// harder!
		if (!(curr_poly->state & POLY_STATE_ACTIVE) ||
			(curr_poly->state & POLY_STATE_CLIPPED ) ||
			(curr_poly->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// set state of polygon to lit, so we don't light again in renderlist
		// lighting system if it happens to get called
		SET_BIT(curr_poly->state, POLY_STATE_LIT);

		// extract vertex indices into master list, rember the polygons are 
		// NOT self contained, but based on the vertex list stored in the object
		// itself
		int vindex_0 = curr_poly->vert[0];
		int vindex_1 = curr_poly->vert[1];
		int vindex_2 = curr_poly->vert[2];

		// we will use the transformed polygon vertex list since the backface removal
		// only makes sense at the world coord stage further of the pipeline 

		//Modules::GetLog().WriteError("\npoly %d",poly);

		// we will use the transformed polygon vertex list since the backface removal
		// only makes sense at the world coord stage further of the pipeline 

		LightsMgr& lights = Modules::GetGraphics().GetLights();

		// test the lighting mode of the polygon (use flat for flat, gouraud))
		if (curr_poly->attr & POLY_ATTR_SHADE_MODE_FLAT)
		{
			// step 1: extract the base color out in RGB mode, assume RGB 888
			_RGB8888FROM32BIT(curr_poly->color, &tmpa, &r_base, &g_base, &b_base);

			// initialize color sum
			r_sum  = 0;
			g_sum  = 0;
			b_sum  = 0;

			//Modules::GetLog().WriteError("\nsum color=%d,%d,%d", r_sum, g_sum, b_sum);

			// new optimization:
			// when there are multiple lights in the system we will end up performing numerous
			// redundant calculations to minimize this my strategy is to set key variables to 
			// to MAX values on each loop, then during the lighting calcs to test the vars for
			// the max value, if they are the max value then the first light that needs the math
			// will do it, and then save the information into the variable (causing it to change state
			// from an invalid number) then any other lights that need the math can use the previously
			// computed value

			// set surface normal.z to FLT_MAX to flag it as non-computed
			n.z = FLT_MAX;

			// loop thru lights
			for (int curr_light = 0; curr_light < lights.Size(); curr_light++)
			{
				// is this light active
				if (lights[curr_light].state==LIGHT_STATE_OFF)
					continue;

				//Modules::GetLog().WriteError("\nprocessing light %d",curr_light);

				// what kind of light are we dealing with
				if (lights[curr_light].attr & LIGHT_ATTR_AMBIENT)
				{
					//Modules::GetLog().WriteError("\nEntering ambient light...");

					// simply multiply each channel against the color of the 
					// polygon then divide by 256 to scale back to 0..255
					// use a shift in real life!!! >> 8
					r_sum+= ((lights[curr_light].c_ambient.r * r_base) / 256);
					g_sum+= ((lights[curr_light].c_ambient.g * g_base) / 256);
					b_sum+= ((lights[curr_light].c_ambient.b * b_base) / 256);

					//Modules::GetLog().WriteError("\nambient sum=%d,%d,%d", r_sum, g_sum, b_sum);

					// there better only be one ambient light!

				} // end if
				else if (lights[curr_light].attr & LIGHT_ATTR_INFINITE)
				{
					//Modules::GetLog().WriteError("\nEntering infinite light...");

					// infinite lighting, we need the surface normal, and the direction
					// of the light source

					// test if we already computed poly normal in previous calculation
					if (n.z==FLT_MAX)       
					{
						// we need to compute the normal of this polygon face, and recall
						// that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

						// build u, v
						u = _vlist_trans[vindex_1].v - _vlist_trans[vindex_0].v;
						v = _vlist_trans[vindex_2].v - _vlist_trans[vindex_0].v;

						// compute cross product
						n = u.Cross(v);
					}

					// at this point, we are almost ready, but we have to normalize the normal vector!
					// this is a key optimization we can make later, we can pre-compute the length of all polygon
					// normals, so this step can be optimized
					// compute length of normal
					//nl = n.LengthFast();
					nl = curr_poly->nlength;

					// ok, recalling the lighting model for infinite lights
					// I(d)dir = I0dir * Cldir
					// and for the diffuse model
					// Itotald =   Rsdiffuse*Idiffuse * (n . l)
					// so we basically need to multiple it all together
					// notice the scaling by 128, I want to avoid floating point calculations, not because they 
					// are slower, but the conversion to and from cost cycles

					dp = n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						i = 128*dp/nl; 
						r_sum+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

						//Modules::GetLog().WriteError("\ninfinite sum=%d,%d,%d", r_sum, g_sum, b_sum);
					}
				}
				else if (lights[curr_light].attr & LIGHT_ATTR_POINT)
				{
					//Modules::GetLog().WriteError("\nEntering point light...");

					// perform point light computations
					// light model for point light is once again:
					//              I0point * Clpoint
					//  I(d)point = ___________________
					//              kc +  kl*d + kq*d2              
					//
					//  Where d = |p - s|
					// thus it's almost identical to the infinite light, but attenuates as a function
					// of distance from the point source to the surface point being lit

					// test if we already computed poly normal in previous calculation
					if (n.z==FLT_MAX)       
					{
						// we need to compute the normal of this polygon face, and recall
						// that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

						// build u, v
						u = _vlist_trans[vindex_1].v - _vlist_trans[vindex_0].v;
						v = _vlist_trans[vindex_2].v - _vlist_trans[vindex_0].v;

						// compute cross product
						n = u.Cross(v);
					}

					// at this point, we are almost ready, but we have to normalize the normal vector!
					// this is a key optimization we can make later, we can pre-compute the length of all polygon
					// normals, so this step can be optimized
					// compute length of normal
					//nl = n.LengthFast();
					nl = curr_poly->nlength;  

					// compute vector from surface to light
					l = lights[curr_light].pos - _vlist_trans[vindex_0].v;

					// compute distance and attenuation
					dist = l.LengthFast(); 

					// and for the diffuse model
					// Itotald =   Rsdiffuse*Idiffuse * (n . l)
					// so we basically need to multiple it all together
					// notice the scaling by 128, I want to avoid floating point calculations, not because they 
					// are slower, but the conversion to and from cost cycles
					dp = n.Dot(l);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

						i = 128*dp / (nl * dist * atten ); 

						r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} 

					//Modules::GetLog().WriteError("\npoint sum=%d,%d,%d",r_sum,g_sum,b_sum);
				}
				else if (lights[curr_light].attr & LIGHT_ATTR_SPOTLIGHT1)
				{
					//Modules::GetLog().WriteError("\nentering spot light1...");

					// perform spotlight/point computations simplified model that uses
					// point light WITH a direction to simulate a spotlight
					// light model for point light is once again:
					//              I0point * Clpoint
					//  I(d)point = ___________________
					//              kc +  kl*d + kq*d2              
					//
					//  Where d = |p - s|
					// thus it's almost identical to the infinite light, but attenuates as a function
					// of distance from the point source to the surface point being lit

					// test if we already computed poly normal in previous calculation
					if (n.z==FLT_MAX)       
					{
						// we need to compute the normal of this polygon face, and recall
						// that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

						// build u, v
						u = _vlist_trans[vindex_1].v - _vlist_trans[vindex_0].v;
						v = _vlist_trans[vindex_2].v - _vlist_trans[vindex_0].v;

						// compute cross product
						n = u.Cross(v);
					}

					// at this point, we are almost ready, but we have to normalize the normal vector!
					// this is a key optimization we can make later, we can pre-compute the length of all polygon
					// normals, so this step can be optimized
					// compute length of normal
					//nl = n.LengthFast();
					nl = curr_poly->nlength;

					// compute vector from surface to light
					l = lights[curr_light].pos - _vlist_trans[vindex_0].v;

					// compute distance and attenuation
					dist = l.LengthFast();

					// and for the diffuse model
					// Itotald =   Rsdiffuse*Idiffuse * (n . l)
					// so we basically need to multiple it all together
					// notice the scaling by 128, I want to avoid floating point calculations, not because they 
					// are slower, but the conversion to and from cost cycles

					// note that I use the direction of the light here rather than a the vector to the light
					// thus we are taking orientation into account which is similar to the spotlight model
					dp = n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

						i = 128*dp / (nl * atten ); 

						r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					}

					//Modules::GetLog().WriteError("\nspotlight sum=%d,%d,%d",r_sum, g_sum, b_sum);
				}
				else if (lights[curr_light].attr & LIGHT_ATTR_SPOTLIGHT2) // simple version
				{
					//Modules::GetLog().WriteError("\nEntering spotlight2 ...");

					// perform spot light computations
					// light model for spot light simple version is once again:
					//         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
					// I(d)spotlight = __________________________________________      
					//               		 kc + kl*d + kq*d2        
					// Where d = |p - s|, and pf = power factor

					// thus it's almost identical to the point, but has the extra term in the numerator
					// relating the angle between the light source and the point on the surface

					// test if we already computed poly normal in previous calculation
					if (n.z==FLT_MAX)       
					{
						// we need to compute the normal of this polygon face, and recall
						// that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

						// build u, v
						u = _vlist_trans[vindex_1].v - _vlist_trans[vindex_0].v;
						v = _vlist_trans[vindex_2].v - _vlist_trans[vindex_0].v;

						// compute cross product
						n = u.Cross(v);
					}

					// at this point, we are almost ready, but we have to normalize the normal vector!
					// this is a key optimization we can make later, we can pre-compute the length of all polygon
					// normals, so this step can be optimized
					// compute length of normal
					//nl = n.LengthFast();
					 nl = curr_poly->nlength;

					// and for the diffuse model
					// Itotald =   Rsdiffuse*Idiffuse * (n . l)
					// so we basically need to multiple it all together
					// notice the scaling by 128, I want to avoid floating point calculations, not because they 
					// are slower, but the conversion to and from cost cycles
					dp = n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						// compute vector from light to surface (different from l which IS the light dir)
						s = _vlist_trans[vindex_0].v - lights[curr_light].pos;

						// compute length of s (distance to light source) to normalize s for lighting calc
						dists = s.LengthFast();

						// compute spot light term (s . l)
						float dpsl = s.Dot(lights[curr_light].dir) / dists;

						// proceed only if term is positive
						if (dpsl > 0) 
						{
							// compute attenuation
							atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

							// for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
							// must be integral
							float dpsl_exp = dpsl;

							// exponentiate for positive integral powers
							for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
								dpsl_exp*=dpsl;

							// now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

							i = 128*dp * dpsl_exp / (nl * atten ); 

							r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
							g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
							b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

							//Modules::GetLog().WriteError("\nSpotlight sum=%d,%d,%d",r_sum, g_sum, b_sum);
						} // end if
					} // end if
				} // end if spot light
			} // end for light

			// make sure colors aren't out of range
			if (r_sum  > 255) r_sum = 255;
			if (g_sum  > 255) g_sum = 255;
			if (b_sum  > 255) b_sum = 255;

			//Modules::GetLog().WriteError("\nWriting final values to polygon %d = %d,%d,%d", poly, r_sum, g_sum, b_sum);

			// write the color over current color
			curr_poly->lit_color[0] = Modules::GetGraphics().GetColor(r_sum, g_sum, b_sum);
		} // end if
		else if (curr_poly->attr & POLY_ATTR_SHADE_MODE_GOURAUD)
		{
			// gouraud shade, unfortunetly at this point in the pipeline, we have lost the original
			// mesh, and only have triangles, thus, many triangles will share the same vertices and
			// they will get lit 2x since we don't have any way to tell this, alas, performing lighting
			// at the object level is a better idea when gouraud shading is performed since the 
			// commonality of vertices is still intact, in any case, lighting here is similar to polygon
			// flat shaded, but we do it 3 times, once for each vertex, additionally there are lots
			// of opportunities for optimization, but I am going to lay off them for now, so the code
			// is intelligible, later we will optimize

			//Modules::GetLog().WriteError("\nEntering gouraud shader...");

			// step 1: extract the base color out in RGB mode
			// assume 888 format
			_RGB8888FROM32BIT(curr_poly->color, &tmpa, &r_base, &g_base, &b_base);

			//Modules::GetLog().WriteError("\nBase color=%d, %d, %d", r_base, g_base, b_base);

			// initialize color sum(s) for vertices
			r_sum0  = 0;
			g_sum0  = 0;
			b_sum0  = 0;

			r_sum1  = 0;
			g_sum1  = 0;
			b_sum1  = 0;

			r_sum2  = 0;
			g_sum2  = 0;
			b_sum2  = 0;

			//Modules::GetLog().WriteError("\nColor sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,   r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

			// new optimization:
			// when there are multiple lights in the system we will end up performing numerous
			// redundant calculations to minimize this my strategy is to set key variables to 
			// to MAX values on each loop, then during the lighting calcs to test the vars for
			// the max value, if they are the max value then the first light that needs the math
			// will do it, and then save the information into the variable (causing it to change state
			// from an invalid number) then any other lights that need the math can use the previously
			// computed value

			LightsMgr& lights = Modules::GetGraphics().GetLights();

			// loop thru lights
			for (int curr_light = 0; curr_light < lights.Size(); curr_light++)
			{
				// is this light active
				if (lights[curr_light].state==LIGHT_STATE_OFF)
					continue;

				//Modules::GetLog().WriteError("\nprocessing light %d", curr_light);

				// what kind of light are we dealing with
				if (lights[curr_light].attr & LIGHT_ATTR_AMBIENT) ///////////////////////////////
				{
					//Modules::GetLog().WriteError("\nEntering ambient light....");

					// simply multiply each channel against the color of the 
					// polygon then divide by 256 to scale back to 0..255
					// use a shift in real life!!! >> 8
					ri = ((lights[curr_light].c_ambient.r * r_base) / 256);
					gi = ((lights[curr_light].c_ambient.g * g_base) / 256);
					bi = ((lights[curr_light].c_ambient.b * b_base) / 256);

					// ambient light has the same affect on each vertex
					r_sum0+=ri;
					g_sum0+=gi;
					b_sum0+=bi;

					r_sum1+=ri;
					g_sum1+=gi;
					b_sum1+=bi;

					r_sum2+=ri;
					g_sum2+=gi;
					b_sum2+=bi;

					// there better only be one ambient light!
					//Modules::GetLog().WriteError("\nexiting ambient ,sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

				} // end if
				else if (lights[curr_light].attr & LIGHT_ATTR_INFINITE) /////////////////////////////////
				{
					//Modules::GetLog().WriteError("\nentering infinite light...");

					// infinite lighting, we need the surface normal, and the direction
					// of the light source

					// no longer need to compute normal or length, we already have the vertex normal
					// and it's length is 1.0  
					// ....

					// ok, recalling the lighting model for infinite lights
					// I(d)dir = I0dir * Cldir
					// and for the diffuse model
					// Itotald =   Rsdiffuse*Idiffuse * (n . l)
					// so we basically need to multiple it all together
					// notice the scaling by 128, I want to avoid floating point calculations, not because they 
					// are slower, but the conversion to and from cost cycles

					// need to perform lighting for each vertex (lots of redundant math, optimize later!)

					//Modules::GetLog().WriteError("\nv0=[%f, %f, %f]=%f, v1=[%f, %f, %f]=%f, v2=[%f, %f, %f]=%f",
					// curr_poly->tvlist[0].n.x, curr_poly->tvlist[0].n.y,curr_poly->tvlist[0].n.z, VECTOR4D_Length(&curr_poly->tvlist[0].n),
					// curr_poly->tvlist[1].n.x, curr_poly->tvlist[1].n.y,curr_poly->tvlist[1].n.z, VECTOR4D_Length(&curr_poly->tvlist[1].n),
					// curr_poly->tvlist[2].n.x, curr_poly->tvlist[2].n.y,curr_poly->tvlist[2].n.z, VECTOR4D_Length(&curr_poly->tvlist[2].n) );

					// vertex 0
					dp = _vlist_trans[ vindex_0].n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						i = 128*dp; 
						r_sum0+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum0+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum0+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} // end if

					// vertex 1
					dp = _vlist_trans[ vindex_1].n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						i = 128*dp; 
						r_sum1+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum1+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum1+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} // end if

					// vertex 2
					dp = _vlist_trans[ vindex_2].n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						i = 128*dp; 
						r_sum2+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum2+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum2+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} // end if

					//Modules::GetLog().WriteError("\nexiting infinite, color sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

				} // end if infinite light
				else if (lights[curr_light].attr & LIGHT_ATTR_POINT) //////////////////////////////////////
				{
					// perform point light computations
					// light model for point light is once again:
					//              I0point * Clpoint
					//  I(d)point = ___________________
					//              kc +  kl*d + kq*d2              
					//
					//  Where d = |p - s|
					// thus it's almost identical to the infinite light, but attenuates as a function
					// of distance from the point source to the surface point being lit

					// .. normal already in vertex

					//Modules::GetLog().WriteError("\nEntering point light....");

					// compute vector from surface to light
					l = lights[curr_light].pos - _vlist_trans[ vindex_0].v;

					// compute distance and attenuation
					dist = l.LengthFast();

					// and for the diffuse model
					// Itotald =   Rsdiffuse*Idiffuse * (n . l)
					// so we basically need to multiple it all together
					// notice the scaling by 128, I want to avoid floating point calculations, not because they 
					// are slower, but the conversion to and from cost cycles

					// perform the calculation for all 3 vertices

					// vertex 0
					dp = _vlist_trans[ vindex_0].n.Dot(l);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

						i = 128*dp / (dist * atten ); 

						r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} // end if


					// vertex 1
					dp = _vlist_trans[ vindex_1].n.Dot(l);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

						i = 128*dp / (dist * atten ); 

						r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} // end if

					// vertex 2
					dp = _vlist_trans[ vindex_2].n.Dot(l);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

						i = 128*dp / (dist * atten ); 

						r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} // end if

					//Modules::GetLog().WriteError("\nexiting point light, rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

				} // end if point
				else if (lights[curr_light].attr & LIGHT_ATTR_SPOTLIGHT1) ///////////////////////////////////////
				{
					// perform spotlight/point computations simplified model that uses
					// point light WITH a direction to simulate a spotlight
					// light model for point light is once again:
					//              I0point * Clpoint
					//  I(d)point = ___________________
					//              kc +  kl*d + kq*d2              
					//
					//  Where d = |p - s|
					// thus it's almost identical to the infinite light, but attenuates as a function
					// of distance from the point source to the surface point being lit

					//Modules::GetLog().WriteError("\nentering spotlight1....");

					// .. normal is already computed

					// compute vector from surface to light
					l = lights[curr_light].pos - _vlist_trans[ vindex_0].v;

					// compute distance and attenuation
					dist = l.LengthFast();

					// and for the diffuse model
					// Itotald =   Rsdiffuse*Idiffuse * (n . l)
					// so we basically need to multiple it all together
					// notice the scaling by 128, I want to avoid floating point calculations, not because they 
					// are slower, but the conversion to and from cost cycles

					// note that I use the direction of the light here rather than a the vector to the light
					// thus we are taking orientation into account which is similar to the spotlight model

					// vertex 0
					dp = _vlist_trans[ vindex_0].n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

						i = 128*dp / ( atten ); 

						r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} // end if

					// vertex 1
					dp = _vlist_trans[ vindex_1].n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

						i = 128*dp / ( atten ); 

						r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} // end i

					// vertex 2
					dp = _vlist_trans[ vindex_2].n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

						i = 128*dp / ( atten ); 

						r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} // end i

					//Modules::GetLog().WriteError("\nexiting spotlight1, sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

				} // end if spotlight1
				else if (lights[curr_light].attr & LIGHT_ATTR_SPOTLIGHT2) // simple version //////////////////////////
				{
					// perform spot light computations
					// light model for spot light simple version is once again:
					//         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
					// I(d)spotlight = __________________________________________      
					//               		 kc + kl*d + kq*d2        
					// Where d = |p - s|, and pf = power factor

					// thus it's almost identical to the point, but has the extra term in the numerator
					// relating the angle between the light source and the point on the surface

					// .. already have normals and length are 1.0

					// and for the diffuse model
					// Itotald =   Rsdiffuse*Idiffuse * (n . l)
					// so we basically need to multiple it all together
					// notice the scaling by 128, I want to avoid floating point calculations, not because they 
					// are slower, but the conversion to and from cost cycles

					//Modules::GetLog().WriteError("\nEntering spotlight2...");

					// tons of redundant math here! lots to optimize later!

					// vertex 0
					dp = _vlist_trans[ vindex_0].n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						// compute vector from light to surface (different from l which IS the light dir)
						s = _vlist_trans[ vindex_0].v - lights[curr_light].pos;

						// compute length of s (distance to light source) to normalize s for lighting calc
						dists = s.LengthFast();

						// compute spot light term (s . l)
						float dpsl = s.Dot(lights[curr_light].dir) / dists;
							
						// proceed only if term is positive
						if (dpsl > 0) 
						{
							// compute attenuation
							atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

							// for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
							// must be integral
							float dpsl_exp = dpsl;

							// exponentiate for positive integral powers
							for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
								dpsl_exp*=dpsl;

							// now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

							i = 128*dp * dpsl_exp / ( atten ); 

							r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
							g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
							b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

						} // end if

					} // end if

					// vertex 1
					dp = _vlist_trans[ vindex_1].n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						// compute vector from light to surface (different from l which IS the light dir)
						s = _vlist_trans[ vindex_1].v - lights[curr_light].pos;

						// compute length of s (distance to light source) to normalize s for lighting calc
						dists = s.LengthFast();

						// compute spot light term (s . l)
						float dpsl = s.Dot(lights[curr_light].dir) / dists;

						// proceed only if term is positive
						if (dpsl > 0) 
						{
							// compute attenuation
							atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

							// for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
							// must be integral
							float dpsl_exp = dpsl;

							// exponentiate for positive integral powers
							for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
								dpsl_exp*=dpsl;

							// now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

							i = 128*dp * dpsl_exp / ( atten ); 

							r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
							g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
							b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

						} // end if

					} // end if

					// vertex 2
					dp = _vlist_trans[ vindex_2].n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						// compute vector from light to surface (different from l which IS the light dir)
						s = _vlist_trans[ vindex_2].v - lights[curr_light].pos;

						// compute length of s (distance to light source) to normalize s for lighting calc
						dists = s.LengthFast();

						// compute spot light term (s . l)
						float dpsl = s.Dot(lights[curr_light].dir) / dists;

						// proceed only if term is positive
						if (dpsl > 0) 
						{
							// compute attenuation
							atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

							// for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
							// must be integral
							float dpsl_exp = dpsl;

							// exponentiate for positive integral powers
							for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
								dpsl_exp*=dpsl;

							// now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

							i = 128*dp * dpsl_exp / ( atten ); 

							r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
							g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
							b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
						} // end if

					} // end if

					//Modules::GetLog().WriteError("\nexiting spotlight2, sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,   r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

				} // end if spot light

			} // end for light

			// make sure colors aren't out of range
			if (r_sum0  > 255) r_sum0 = 255;
			if (g_sum0  > 255) g_sum0 = 255;
			if (b_sum0  > 255) b_sum0 = 255;

			if (r_sum1  > 255) r_sum1 = 255;
			if (g_sum1  > 255) g_sum1 = 255;
			if (b_sum1  > 255) b_sum1 = 255;

			if (r_sum2  > 255) r_sum2 = 255;
			if (g_sum2  > 255) g_sum2 = 255;
			if (b_sum2  > 255) b_sum2 = 255;

			//Modules::GetLog().WriteError("\nwriting color for poly %d", poly);

			//Modules::GetLog().WriteError("\n******** final sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

			// write the colors
			curr_poly->lit_color[0] = Modules::GetGraphics().GetColor(r_sum0, g_sum0, b_sum0);
			curr_poly->lit_color[1] = Modules::GetGraphics().GetColor(r_sum1, g_sum1, b_sum1);
			curr_poly->lit_color[2] = Modules::GetGraphics().GetColor(r_sum2, g_sum2, b_sum2);
		}
		else // assume POLY_ATTR_SHADE_MODE_CONSTANT
		{
			// emmisive shading only, do nothing
			// ...
			curr_poly->lit_color[0] = curr_poly->color;

			//Modules::GetLog().WriteError("\nentering constant shader, and exiting...");
		} // end if

	} // end for poly
}

void RenderObject::DrawWire32(unsigned char* video_buffer, int lpitch)
{
	// this function renders an object to the screen in wireframe, 
	// 16 bit mode, it has no regard at all about hidden surface removal, 
	// etc. the function only exists as an easy way to render an object 
	// without converting it into polygons, the function assumes all 
	// coordinates are screen coordinates, but will perform 2D clipping

	// iterate thru the poly list of the object and simply draw
	// each polygon
	for (int poly=0; poly < _num_polys; poly++)
	{
		// render this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concecpt of "backface" is 
		// irrelevant in a wire frame engine though
		if (!(_plist[poly].state & POLY_STATE_ACTIVE) ||
			(_plist[poly].state & POLY_STATE_CLIPPED ) ||
			(_plist[poly].state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// extract vertex indices into master list, rember the polygons are 
		// NOT self contained, but based on the vertex list stored in the object
		// itself
		int vindex_0 = _plist[poly].vert[0];
		int vindex_1 = _plist[poly].vert[1];
		int vindex_2 = _plist[poly].vert[2];

		// draw the lines now
		DrawClipLine32(_vlist_trans[ vindex_0 ].x, _vlist_trans[ vindex_0 ].y, 
			_vlist_trans[ vindex_1 ].x, _vlist_trans[ vindex_1 ].y, 
			_plist[poly].color,
			video_buffer, lpitch);

		DrawClipLine32(_vlist_trans[ vindex_1 ].x, _vlist_trans[ vindex_1 ].y, 
			_vlist_trans[ vindex_2 ].x, _vlist_trans[ vindex_2 ].y, 
			_plist[poly].color,
			video_buffer, lpitch);

		DrawClipLine32(_vlist_trans[ vindex_2 ].x, _vlist_trans[ vindex_2 ].y, 
			_vlist_trans[ vindex_0 ].x, _vlist_trans[ vindex_0 ].y, 
			_plist[poly].color,
			video_buffer, lpitch);

		// track rendering stats
#ifdef DEBUG_ON
		debug_polys_rendered_per_frame++;
#endif


	} // end for poly
}

void RenderObject::DrawSolid32(unsigned char* video_buffer, int lpitch)
{

	// this function renders an object to the screen  in
	// 16 bit mode, it has no regard at all about hidden surface removal, 
	// etc. the function only exists as an easy way to render an object 
	// without converting it into polygons, the function assumes all 
	// coordinates are screen coordinates, but will perform 2D clipping

	PolygonF face; // temp face used to render polygon

	// at this point, all we have is a list of polygons and it's time
	// to draw them
	for (int poly=0; poly < _num_polys; poly++)
	{
		// render this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concecpt of "backface" is 
		// irrelevant in a wire frame engine though
		if (!(_plist[poly].state & POLY_STATE_ACTIVE) ||
			(_plist[poly].state & POLY_STATE_CLIPPED ) ||
			(_plist[poly].state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// extract vertex indices into master list, rember the polygons are 
		// NOT self contained, but based on the vertex list stored in the object
		// itself
		int vindex_0 = _plist[poly].vert[0];
		int vindex_1 = _plist[poly].vert[1];
		int vindex_2 = _plist[poly].vert[2];


		// need to test for textured first, since a textured poly can either
		// be emissive, or flat shaded, hence we need to call different
		// rasterizers    
		if (_plist[poly].attr  & POLY_ATTR_SHADE_MODE_TEXTURE)
		{

			// set the vertices
			face.tvlist[0].x = (int)_vlist_trans[ vindex_0].x;
			face.tvlist[0].y = (int)_vlist_trans[ vindex_0].y;
			face.tvlist[0].u0 = (int)_vlist_trans[ vindex_0].u0;
			face.tvlist[0].v0 = (int)_vlist_trans[ vindex_0].v0;

			face.tvlist[1].x = (int)_vlist_trans[ vindex_1].x;
			face.tvlist[1].y = (int)_vlist_trans[ vindex_1].y;
			face.tvlist[1].u0 = (int)_vlist_trans[ vindex_1].u0;
			face.tvlist[1].v0 = (int)_vlist_trans[ vindex_1].v0;

			face.tvlist[2].x = (int)_vlist_trans[ vindex_2].x;
			face.tvlist[2].y = (int)_vlist_trans[ vindex_2].y;
			face.tvlist[2].u0 = (int)_vlist_trans[ vindex_2].u0;
			face.tvlist[2].v0 = (int)_vlist_trans[ vindex_2].v0;


			// assign the texture
			face.texture = _plist[poly].texture;

			// is this a plain emissive texture?
			if (_plist[poly].attr & POLY_ATTR_SHADE_MODE_CONSTANT)
			{
				// draw the textured triangle as emissive
				DrawTexturedTriangle32(&face, video_buffer, lpitch);
			} // end if
			else
			{
				// draw as flat shaded
				face.lit_color[0] = _plist[poly].lit_color[0];
				DrawTexturedTriangleFS32(&face, video_buffer, lpitch);
			} // end else

		} // end if      
		else if ((_plist[poly].attr  & POLY_ATTR_SHADE_MODE_FLAT) || 
				(_plist[poly].attr  & POLY_ATTR_SHADE_MODE_CONSTANT) )
		{
			// draw the triangle with basic flat rasterizer
			DrawTriangle32(_vlist_trans[ vindex_0].x, _vlist_trans[ vindex_0].y,
						   _vlist_trans[ vindex_1].x, _vlist_trans[ vindex_1].y,
						   _vlist_trans[ vindex_2].x, _vlist_trans[ vindex_2].y,
						   _plist[poly].lit_color[0], video_buffer, lpitch);

		} // end if
		else if (_plist[poly].attr & POLY_ATTR_SHADE_MODE_GOURAUD)
		{
			// {andre take advantage of the data structures later..}
			// set the vertices
			face.tvlist[0].x  = (int)_vlist_trans[ vindex_0].x;
			face.tvlist[0].y  = (int)_vlist_trans[ vindex_0].y;
			face.lit_color[0] = _plist[poly].lit_color[0];

			face.tvlist[1].x  = (int)_vlist_trans[ vindex_1].x;
			face.tvlist[1].y  = (int)_vlist_trans[ vindex_1].y;
			face.lit_color[1] = _plist[poly].lit_color[1];

			face.tvlist[2].x  = (int)_vlist_trans[ vindex_2].x;
			face.tvlist[2].y  = (int)_vlist_trans[ vindex_2].y;
			face.lit_color[2] = _plist[poly].lit_color[2];

			// draw the gouraud shaded triangle
			DrawGouraudTriangle32(&face, video_buffer, lpitch);
		} // end if gouraud

	} // end for poly
}

///////////////////////////////////////////////////////////////

float RenderObject::ComputeRadius()
{

	// this function computes the average and maximum radius for 
	// sent object and opdates the object data for the "current frame"
	// it's up to the caller to make sure Set_Frame() for this object
	// has been called to set the object up properly

	// reset incase there's any residue
	_avg_radius[_curr_frame] = 0;
	_max_radius[_curr_frame] = 0;

	// loop thru and compute radius
	for (int vertex = 0; vertex < _num_vertices; vertex++)
	{
		// update the average and maximum radius
		float dist_to_vertex = 
			sqrt(_vlist_local[vertex].x*_vlist_local[vertex].x +
				 _vlist_local[vertex].y*_vlist_local[vertex].y +
				 _vlist_local[vertex].z*_vlist_local[vertex].z);

		// accumulate total radius
		_avg_radius[_curr_frame]+=dist_to_vertex;

		// update maximum radius   
		if (dist_to_vertex > _max_radius[_curr_frame])
			_max_radius[_curr_frame] = dist_to_vertex; 

	} // end for vertex

	// finallize average radius computation
	_avg_radius[_curr_frame]/=_num_vertices;

	// return max radius of frame 0
	return(_max_radius[0]);

} // end ComputeRadius

int RenderObject::ComputePolyNormals()
{
	// the normal of a polygon is commonly needed in a number 
	// of functions, however, to store a normal turns out to
	// be counterproductive in most cases since the transformation
	// to rotate the normal ends up taking as long as computing the
	// normal -- HOWEVER, if the normal must have unit length, then
	// pre-computing the length of the normal, and then in real-time
	// dividing by this save a length computation, so we get the 
	// best of both worlds... thus, this function computes the length
	// of a polygon's normal, but care must be taken, so that we compute
	// the length based on the EXACT same two vectors that all other 
	// functions will use when computing the normal
	// in most cases the functions of interest are the lighting functions
	// if we can pre-compute the normal length
	// for all these functions then that will save at least:
	// num_polys_per_frame * (time to compute length of vector)

	// the way we have written the engine, in all cases the normals 
	// during lighting are computed as u = v0->v1, and v = v1->v2
	// so as long as we follow that convention we are fine.
	// also, since the new OBJECT4DV2 format supports multiple frames
	// we must perform these calculations for EACH frame of the animation
	// since although the poly indices don't change, the vertice positions
	// do and thus, so do the normals!!!

	// iterate thru the poly list of the object and compute normals
	// each polygon
	for (int poly=0; poly < _num_polys; poly++)
	{

		// extract vertex indices into master list, rember the polygons are 
		// NOT self contained, but based on the vertex list stored in the object
		// itself
		int vindex_0 = _plist[poly].vert[0];
		int vindex_1 = _plist[poly].vert[1];
		int vindex_2 = _plist[poly].vert[2];

		// we need to compute the normal of this polygon face, and recall
		// that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
		vec4 u, v, n;

		// build u, v
		u = _vlist_local[ vindex_1 ].v - _vlist_local[ vindex_0 ].v;
		v = _vlist_local[ vindex_2 ].v - _vlist_local[ vindex_0 ].v;

		// compute cross product
		n = u.Cross(v);

		// compute length of normal accurately and store in poly nlength
		// +- epsilon later to fix over/underflows
		_plist[poly].nlength = n.Length();
	} // end for poly

	// return success
	return(1);
}

int RenderObject::ComputeVertexNormals()
{
	// the vertex normals of each polygon are commonly needed in a number 
	// functions, most importantly lighting calculations for gouraud shading
	// however, we only need to compute the vertex normals for polygons that are
	// gouraud shader, so for every vertex we must determine the polygons that
	// share the vertex then compute the average normal, to determine if a polygon
	// contributes we look at the shading flags for the polygon

	// algorithm: we are going to scan the polygon list and for every polygon
	// that needs normals we are going to "accumulate" the surface normal into all
	// vertices that the polygon touches, and increment a counter to track how many
	// polys contribute to vertex, then when the scan is done the counts will be used
	// to average the accumulated values, so instead of an O(n^2) algorithm, we get a O(c*n)

	// this tracks the polygon indices that touch a particular vertex
	// the array is used to count the number of contributors to the vertex
	// so at the end of the process we can divide each "accumulated" normal
	// and average
	int polys_touch_vertex[MAX_VERTICES];
	memset((void *)polys_touch_vertex, 0, sizeof(int)*MAX_VERTICES);

	// iterate thru the poly list of the object, compute its normal, then add
	// each vertice that composes it to the "touching" vertex array
	// while accumulating the normal in the vertex normal array

	for (int poly=0; poly < _num_polys; poly++)
	{
		Modules::GetLog().WriteError("\nprocessing poly %d", poly);

		// test if this polygon needs vertex normals
		if (_plist[poly].attr & POLY_ATTR_SHADE_MODE_GOURAUD)
		{
			// extract vertex indices into master list, rember the polygons are 
			// NOT self contained, but based on the vertex list stored in the object
			// itself
			int vindex_0 = _plist[poly].vert[0];
			int vindex_1 = _plist[poly].vert[1];
			int vindex_2 = _plist[poly].vert[2];

			Modules::GetLog().WriteError("\nTouches vertices: %d, %d, %d", vindex_0, vindex_1, vindex_2);

			// we need to compute the normal of this polygon face, and recall
			// that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
			vec4 u, v, n;

			// build u, v
			u = _vlist_local[ vindex_1 ].v - _vlist_local[ vindex_0 ].v;
			v = _vlist_local[ vindex_2 ].v - _vlist_local[ vindex_0 ].v;

			// compute cross product
			n = u.Cross(v);

			// update vertex array to flag this polygon as a contributor
			polys_touch_vertex[vindex_0]++;
			polys_touch_vertex[vindex_1]++;
			polys_touch_vertex[vindex_2]++;

			Modules::GetLog().WriteError("\nPoly touch array v[%d] = %d,  v[%d] = %d,  v[%d] = %d", 
				vindex_0, polys_touch_vertex[vindex_0],
				vindex_1, polys_touch_vertex[vindex_1],
				vindex_2, polys_touch_vertex[vindex_2]);

			// now accumulate the normal into the vertex normal itself
			// note, we do NOT normalize at this point since we want the length of the normal
			// to weight on the average, and since the length is in fact the area of the parallelogram
			// constructed by uxv, so we are taking the "influence" of the area into consideration
			_vlist_local[vindex_0].n = _vlist_local[vindex_0].n + n;
			_vlist_local[vindex_1].n = _vlist_local[vindex_1].n + n;
			_vlist_local[vindex_2].n = _vlist_local[vindex_2].n + n;
		} // end for poly

	} // end if needs vertex normals

	// now we are almost done, we have accumulated all the vertex normals, but need to average them
	for (int vertex = 0; vertex < _num_vertices; vertex++)
	{
		// if this vertex has any contributors then it must need averaging, OR we could check
		// the shading hints flags, they should be one to one
		Modules::GetLog().WriteError("\nProcessing vertex: %d, attr: %d, contributors: %d", vertex, 
			_vlist_local[vertex].attr, 
			polys_touch_vertex[vertex]);

		// test if this vertex has a normal and needs averaging
		if (polys_touch_vertex[vertex] >= 1)
		{
			_vlist_local[vertex].nx/=polys_touch_vertex[vertex];
			_vlist_local[vertex].ny/=polys_touch_vertex[vertex];
			_vlist_local[vertex].nz/=polys_touch_vertex[vertex];

			// now normalize the normal
			_vlist_local[vertex].n.Normalize();

			Modules::GetLog().WriteError("\nAvg Vertex normal: [%f, %f, %f]", 
				_vlist_local[vertex].nx,
				_vlist_local[vertex].ny,
				_vlist_local[vertex].nz);

		} // end if

	} // end for

	// return success
	return(1);
}

}