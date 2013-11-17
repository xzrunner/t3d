#include "MD2.h"

#include "Modules.h"
#include "Log.h"
#include "RenderObject.h"
#include "BmpFile.h"
#include "BmpImg.h"

namespace t3d {

// the animations, the frames, and timing look good on most
// models, but tweak them to suit your needs OR create a table
// for every character in your game to fine tune your animation
MD2Animation MD2_ANIMATIONS[NUM_MD2_ANIMATIONS]  = 
{
	// format: start frame (0..197), end frame (0..197), 
	// interpolation rate (0..1, 1 for no interpolation), 
	// speed (0..10, 0 fastest, 1 fast, 2 medium, 3 slow...)

	{0,39,0.5,1},    // MD2_ANIM_STATE_STANDING_IDLE       0
	{40,45,0.5,2},   // MD2_ANIM_STATE_RUN                 1
	{46,53,0.5,1},   // MD2_ANIM_STATE_ATTACK              2 
	{54,57,0.5,1},   // MD2_ANIM_STATE_PAIN_1              3
	{58,61,0.5,1},   // MD2_ANIM_STATE_PAIN_2              4
	{62,65,0.5,1},   // MD2_ANIM_STATE_PAIN_3              5
	{66,71,0.5,1},   // MD2_ANIM_STATE_JUMP                6
	{72,83,0.5,1},   // MD2_ANIM_STATE_FLIP                7
	{84,94,0.5,1},   // MD2_ANIM_STATE_SALUTE              8
	{95,111,0.5,1},  // MD2_ANIM_STATE_TAUNT               9
	{112,122,0.5,1}, // MD2_ANIM_STATE_WAVE                10
	{123,134,0.5,1}, // MD2_ANIM_STATE_POINT               11 
	{135,153,0.5,1}, // MD2_ANIM_STATE_CROUCH_STAND        12 
	{154,159,0.5,1}, // MD2_ANIM_STATE_CROUCH_WALK         13
	{160,168,0.5,1}, // MD2_ANIM_STATE_CROUCH_ATTACK       14
	{169,172,0.5,1}, // MD2_ANIM_STATE_CROUCH_PAIN         15
	{173,177,0.25,0}, // MD2_ANIM_STATE_CROUCH_DEATH        16  
	{178,183,0.25,0}, // MD2_ANIM_STATE_DEATH_BACK          17
	{184,189,0.25,0}, // MD2_ANIM_STATE_DEATH_FORWARD       18
	{190,197,0.25,0}, // MD2_ANIM_STATE_DEATH_SLOW          19
};

// ASCII names for debugging
char* MD2_ANIM_STRINGS[] = 
{
	"MD2_ANIM_STATE_STANDING_IDLE ",
	"MD2_ANIM_STATE_RUN",
	"MD2_ANIM_STATE_ATTACK", 
	"MD2_ANIM_STATE_PAIN_1",
	"MD2_ANIM_STATE_PAIN_2",
	"MD2_ANIM_STATE_PAIN_3",
	"MD2_ANIM_STATE_JUMP",
	"MD2_ANIM_STATE_FLIP",
	"MD2_ANIM_STATE_SALUTE",
	"MD2_ANIM_STATE_TAUNT",
	"MD2_ANIM_STATE_WAVE",
	"MD2_ANIM_STATE_POINT",       
	"MD2_ANIM_STATE_CROUCH_STAND", 
	"MD2_ANIM_STATE_CROUCH_WALK ",
	"MD2_ANIM_STATE_CROUCH_ATTACK",
	"MD2_ANIM_STATE_CROUCH_PAIN",
	"MD2_ANIM_STATE_CROUCH_DEATH",  
	"MD2_ANIM_STATE_DEATH_BACK",
	"MD2_ANIM_STATE_DEATH_FORWARD",
	"MD2_ANIM_STATE_DEATH_SLOW",
};

void MD2Container::Load(char *modelfile,		// the filename of the .MD2 model
					    vec4* scale,			// initial scaling factors
					    vec4* pos,			// initial position
					    vec4* rot,			// initial rotations
					    char *texturefile,	// the texture filename for the model
					    int attr,				// the lighting/model attributes for the model
					    int color,			// base color if no texturing
					    int vertex_flags)	// control ordering etc.
{
	// this function loads in an md2 file, extracts all the data and stores it in the 
	// container class which will be used later to load frames into a the standard object
	// type for rendering on the fly

	FILE *fp      = NULL;  // file pointer to model
	int   flength = -1;    // general file length
	unsigned char *buffer = NULL;  // used to buffer md2 file data

	MD2Header* md2_header;   // pointer to the md2 header

	// begin by loading in the .md2 model file
	if ((fp = fopen(modelfile, "rb"))==NULL)
	{
		Modules::GetLog().WriteError("\nLoad_Object_MD2 - couldn't find file %s", modelfile);
		return;
	} // end if

	// find the length of the model file
	// seek to end of file
	fseek(fp, 0, SEEK_END);

	// where is the file pointer?
	flength = ftell(fp);

	// now read the md2 file into a buffer to analyze it

	// re-position file pointer to beginning of file
	fseek(fp, 0, SEEK_SET);

	// allocate memory to hold file
	buffer = (unsigned char *)malloc(flength+1);

	// load data into buffer
	int bytes_read = fread(buffer, sizeof(unsigned char), flength, fp);

	// the header is the first item in the buffer, so alias a pointer
	// to it, so we can start analyzing it and creating the model
	md2_header = (MD2Header*)buffer;

	Modules::GetLog().WriteError("\nint identifier        = %d", md2_header->identifier);
	Modules::GetLog().WriteError("\nint version           = %d", md2_header->version);
	Modules::GetLog().WriteError("\nint skin_width        = %d", md2_header->skin_width);
	Modules::GetLog().WriteError("\nint skin_height       = %d", md2_header->skin_height);
	Modules::GetLog().WriteError("\nint framesize         = %d", md2_header->framesize);
	Modules::GetLog().WriteError("\nint num_skins         = %d", md2_header->num_skins);
	Modules::GetLog().WriteError("\nint num_verts         = %d", md2_header->num_verts);
	Modules::GetLog().WriteError("\nint num_textcoords    = %d", md2_header->num_textcoords);
	Modules::GetLog().WriteError("\nint num_polys         = %d", md2_header->num_polys);
	Modules::GetLog().WriteError("\nint num_openGLcmds    = %d", md2_header->num_openGLcmds);
	Modules::GetLog().WriteError("\nint num_frames        = %d", md2_header->num_frames);
	Modules::GetLog().WriteError("\nint offset_skins      = %d", md2_header->offset_skins);
	Modules::GetLog().WriteError("\nint offset_textcoords = %d", md2_header->offset_textcoords);
	Modules::GetLog().WriteError("\nint offset_polys      = %d", md2_header->offset_polys);
	Modules::GetLog().WriteError("\nint offset_frames     = %d", md2_header->offset_frames);
	Modules::GetLog().WriteError("\nint offset_openGLcmds = %d", md2_header->offset_openGLcmds);
	Modules::GetLog().WriteError("\nint offset_end        = %d", md2_header->offset_end);

	// test for valid file
	if (md2_header->identifier != MD2_MAGIC_NUM || md2_header->version != MD2_VERSION)
	{
		fclose(fp);
		return;
	} // end if

	// assign fields to container class
	_state          = 0;                          // state of the model
	_attr           = attr;                       // attributes of the model
	_color          = color;                      // 
	_num_frames     = md2_header->num_frames;     // number of frames in the model
	_num_polys      = md2_header->num_polys;      // number of polygons
	_num_verts      = md2_header->num_verts;      // number of vertices 
	_num_textcoords = md2_header->num_textcoords; // number of texture coordinates
	_curr_frame     = 0;                          // current frame in animation
	_skin           = NULL;                       // pointer to texture skin for model
	_world_pos      = *pos;                       // position object in world

	// allocate memory for mesh data
	_polys = (MD2Poly*)malloc(md2_header->num_polys   * sizeof(MD2Poly)); // pointer to polygon list
	_vlist = (vec3*)malloc(md2_header->num_frames     * md2_header->num_verts * sizeof(vec3)); // pointer to vertex coordinate list
	_tlist = (vec2*)malloc(md2_header->num_textcoords * sizeof(vec2)); // pointer to texture coordinate list

#if (MD2_DEBUG==1)
	Modules::GetLog().WriteError("\nTexture Coordinates:");
#endif

	for (int tindex = 0; tindex < md2_header->num_textcoords; tindex++)
	{
#if (MD2_DEBUG==1)
		Modules::GetLog().WriteError("\ntextcoord[%d] = (%d, %d)", tindex,
			((MD2Textcoord*)(buffer+md2_header->offset_textcoords))[tindex].u, 
			((MD2Textcoord*)(buffer+md2_header->offset_textcoords))[tindex].v);
#endif    
		// insert texture coordinate into storage container
		_tlist[tindex].x = ((MD2Textcoord*)(buffer+md2_header->offset_textcoords))[tindex].u;
		_tlist[tindex].y = ((MD2Textcoord*)(buffer+md2_header->offset_textcoords))[tindex].v;
	} // end for vindex

#if (MD2_DEBUG==1)
	Modules::GetLog().WriteError("\nVertex List:");
#endif

	for (int findex = 0; findex < md2_header->num_frames; findex++)
	{
#if (MD2_DEBUG==1)
		Modules::GetLog().WriteError("\n\n******************************************************************************");
		Modules::GetLog().WriteError("\n\nF R A M E # %d", findex);
		Modules::GetLog().WriteError("\n\n******************************************************************************\n");
#endif

		MD2Frame* frame_ptr = (MD2Frame*)(buffer + md2_header->offset_frames + md2_header->framesize * findex);

		// extract md2 scale and translate, additionally use sent scale and translate
		float sx = frame_ptr->scale[0],
			sy = frame_ptr->scale[1],
			sz = frame_ptr->scale[2],
			tx = frame_ptr->translate[0],  
			ty = frame_ptr->translate[1],  
			tz = frame_ptr->translate[2];

#if (MD2_DEBUG==1)    
		Modules::GetLog().WriteError("\nScale: (%f, %f, %f)\nTranslate: (%f, %f, %f)", sx, sy, sz, tx, ty, tz);  
#endif

		for (int vindex = 0; vindex < md2_header->num_verts; vindex++)
		{
			vec3 v;

			// scale and translate compressed vertex
			v.x = (float)frame_ptr->vlist[vindex].v[0] * sx + tx;
			v.y = (float)frame_ptr->vlist[vindex].v[1] * sy + ty;
			v.z = (float)frame_ptr->vlist[vindex].v[2] * sz + tz;

			// scale final point based on sent data
			v.x = scale->x * v.x;
			v.y = scale->y * v.y;
			v.z = scale->z * v.z;

			// test for vertex modifications to winding order etc.
			if (vertex_flags & VERTEX_FLAGS_INVERT_X)
				v.x = -v.x;

			if (vertex_flags & VERTEX_FLAGS_INVERT_Y)
				v.y = -v.y;

			if (vertex_flags & VERTEX_FLAGS_INVERT_Z)
				v.z = -v.z;

			if (vertex_flags & VERTEX_FLAGS_SWAP_YZ)
				std::swap(v.y, v.z);

			if (vertex_flags & VERTEX_FLAGS_SWAP_XZ)
				std::swap(v.x, v.z);

			if (vertex_flags & VERTEX_FLAGS_SWAP_XY)
				std::swap(v.x, v.y);

#if (MD2_DEBUG==1)
			Modules::GetLog().WriteError("\nVertex #%d = (%f, %f, %f)", vindex, v.x, v.y, v.z);
#endif
			// insert vertex into vertex list which is laid out frame 0, frame 1,..., frame n 
			// frame i: vertex 0, vertex 1,....vertex j
			_vlist[vindex + (findex * _num_verts)] = v;
		} // end vindex

	} // end findex

#if (MD2_DEBUG==1)
	Modules::GetLog().WriteError("\nPolygon List:");
#endif

	MD2Poly* poly_ptr = (MD2Poly*)(buffer + md2_header->offset_polys);

	for (int pindex = 0; pindex < md2_header->num_polys; pindex++)
	{
		// insert polygon into polygon list in container
		if (vertex_flags & VERTEX_FLAGS_INVERT_WINDING_ORDER)
		{
			// inverted winding order

			// vertices
			_polys[pindex].vindex[0] = poly_ptr[pindex].vindex[2];
			_polys[pindex].vindex[1] = poly_ptr[pindex].vindex[1];
			_polys[pindex].vindex[2] = poly_ptr[pindex].vindex[0];

			// texture coordinates
			_polys[pindex].tindex[0] = poly_ptr[pindex].tindex[2];
			_polys[pindex].tindex[1] = poly_ptr[pindex].tindex[1];
			_polys[pindex].tindex[2] = poly_ptr[pindex].tindex[0];
		} // end if
		else
		{
			// normal winding order

			// vertices
			_polys[pindex].vindex[0] = poly_ptr[pindex].vindex[0];
			_polys[pindex].vindex[1] = poly_ptr[pindex].vindex[1];
			_polys[pindex].vindex[2] = poly_ptr[pindex].vindex[2];

			// texture coordinates
			_polys[pindex].tindex[0] = poly_ptr[pindex].tindex[0];
			_polys[pindex].tindex[1] = poly_ptr[pindex].tindex[1];
			_polys[pindex].tindex[2] = poly_ptr[pindex].tindex[2];

		} // end if

#if (MD2_DEBUG==1)
		Modules::GetLog().WriteError("\npoly %d: v(%d, %d, %d), t(%d, %d, %d)", pindex,
			_polys[pindex].vindex[0], _polys[pindex].vindex[1], _polys[pindex].vindex[2],
			_polys[pindex].tindex[0], _polys[pindex].tindex[1], _polys[pindex].tindex[2]);
#endif
	} // end for vindex

	// close the file
	fclose(fp);

	//////////////////////////////////////////////////////////////////////////////
	// load the texture from disk
	BmpFile* bitmap = new BmpFile(texturefile);

	// create a proper size and bitdepth bitmap
	_skin = (BmpImg*)malloc(sizeof(BmpImg));

	// initialize bitmap
	_skin = new BmpImg(0,0,bitmap->Width(),bitmap->Height(),bitmap->BitCount());

	// load the bitmap image
	_skin->LoadImage32(*bitmap,0,0,BITMAP_EXTRACT_MODE_ABS);

	// done, so unload the bitmap
	delete bitmap, bitmap = 0;


	// finally release the memory for the temporary buffer
	if (buffer)
		free(buffer);
}

void MD2Container::PrepareObject(RenderObject* obj)
{
	// this function prepares the RenderObject to be used as a vessel to hold
	// frames from the md2 container, it allocated the memory needed, set fields
	// and pre-computes as much as possible since each new frame will change only
	// the vertex list

	Modules::GetLog().WriteError("\nPreparing MD2_CONTAINER_PTR %x for OBJECT_PTR %x", this, obj);

	///////////////////////////////////////////////////////////////////////////////
	// clear out the object and initialize it a bit
	memset(obj, 0, sizeof(RenderObject));

	// set state of object to active and visible
	obj->_state = OBJECT_STATE_ACTIVE | OBJECT_STATE_VISIBLE;

	// set some information in object
	obj->_num_frames   = 1;   // always set to 1
	obj->_curr_frame   = 0;
	obj->_attr         = OBJECT_ATTR_SINGLE_FRAME | OBJECT_ATTR_TEXTURES; 

	obj->_num_vertices = _num_verts;
	obj->_num_polys    = _num_polys;
	obj->_texture      = _skin;

	// set position of object
	obj->_world_pos = _world_pos;

	// allocate the memory for the vertices and number of polys
	// the call parameters are redundant in this case, but who cares
	if (!obj->Init(obj->_num_vertices, obj->_num_polys, obj->_num_frames))
	{
		Modules::GetLog().WriteError("\n(can't allocate memory).");
	} // end if

	////////////////////////////////////////////////////////////////////////////////
	// compute average and max radius using the vertices from frame 0, this isn't
	// totally accurate, but the volume of the object hopefully does vary wildly 
	// during animation

	// reset incase there's any residue
	obj->_avg_radius[0] = 0;
	obj->_max_radius[0] = 0;

	// loop thru and compute radius
	for (int vindex = 0; vindex < _num_verts; vindex++)
	{
		// update the average and maximum radius (use frame 0)
		float dist_to_vertex = 
			sqrt(_vlist[vindex].x * _vlist[vindex].x +
			_vlist[vindex].y * _vlist[vindex].y +
			_vlist[vindex].z * _vlist[vindex].z );

		// accumulate total radius
		obj->_avg_radius[0]+=dist_to_vertex;

		// update maximum radius   
		if (dist_to_vertex > obj->_max_radius[0])
			obj->_max_radius[0] = dist_to_vertex; 

	} // end for vertex

	// finallize average radius computation
	obj->_avg_radius[0]/=obj->_num_vertices;

	Modules::GetLog().WriteError("\nMax radius=%f, Avg. Radius=%f",obj->_max_radius[0], obj->_avg_radius[0]);
	Modules::GetLog().WriteError("\nWriting texture coordinates...");

	///////////////////////////////////////////////////////////////////////////////
	// copy texture coordinate list always the same
	for (int tindex = 0; tindex < _num_textcoords; tindex++)
	{
		// now texture coordinates
		obj->_tlist[tindex].x = _tlist[tindex].x;
		obj->_tlist[tindex].y = _tlist[tindex].y;
	} // end for tindex

	Modules::GetLog().WriteError("\nWriting polygons...");

	// generate the polygon index list, always the same
	for (int pindex=0; pindex < _num_polys; pindex++)
	{
		// set polygon indices
		obj->_plist[pindex].vert[0] = _polys[pindex].vindex[0];
		obj->_plist[pindex].vert[1] = _polys[pindex].vindex[1];
		obj->_plist[pindex].vert[2] = _polys[pindex].vindex[2];

		// point polygon vertex list to object's vertex list
		// note that this is redundant since the polylist is contained
		// within the object in this case and its up to the user to select
		// whether the local or transformed vertex list is used when building up
		// polygon geometry, might be a better idea to set to NULL in the context
		// of polygons that are part of an object
		obj->_plist[pindex].vlist   = obj->_vlist_local; 

		// set attributes of polygon with sent attributes
		obj->_plist[pindex].attr    = _attr;

		// set color of polygon
		obj->_plist[pindex].color   = _color;

		// apply texture to this polygon
		obj->_plist[pindex].texture = _skin;

		// assign the texture coordinates
		obj->_plist[pindex].text[0] = _polys[pindex].tindex[0];
		obj->_plist[pindex].text[1] = _polys[pindex].tindex[1];
		obj->_plist[pindex].text[2] = _polys[pindex].tindex[2];

		// set texture coordinate attributes
		SET_BIT(obj->_vlist_local[ obj->_plist[pindex].vert[0] ].attr, VERTEX_ATTR_TEXTURE); 
		SET_BIT(obj->_vlist_local[ obj->_plist[pindex].vert[1] ].attr, VERTEX_ATTR_TEXTURE); 
		SET_BIT(obj->_vlist_local[ obj->_plist[pindex].vert[2] ].attr, VERTEX_ATTR_TEXTURE); 

		// set the material mode to ver. 1.0 emulation
		SET_BIT(obj->_plist[pindex].attr, POLY_ATTR_DISABLE_MATERIAL);

		// finally set the polygon to active
		obj->_plist[pindex].state = POLY_STATE_ACTIVE;    

		// point polygon vertex list to object's vertex list
		// note that this is redundant since the polylist is contained
		// within the object in this case and its up to the user to select
		// whether the local or transformed vertex list is used when building up
		// polygon geometry, might be a better idea to set to NULL in the context
		// of polygons that are part of an object
		obj->_plist[pindex].vlist = obj->_vlist_local; 

		// set texture coordinate list, this is needed
		obj->_plist[pindex].tlist = obj->_tlist;

		// extract vertex indices
		int vindex_0 = _polys[pindex].vindex[0];
		int vindex_1 = _polys[pindex].vindex[1];
		int vindex_2 = _polys[pindex].vindex[2];

		// we need to compute the normal of this polygon face, and recall
		// that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
		vec4 u, v, n;

		// build u, v
		//VECTOR4D_Build(&obj->_vlist_local[ vindex_0 ].v, &obj->_vlist_local[ vindex_1 ].v, &u);
		//VECTOR4D_Build(&obj->_vlist_local[ vindex_0 ].v, &obj->_vlist_local[ vindex_2 ].v, &v);

		u.x = _vlist[vindex_1].x - _vlist[vindex_0].x;
		u.y = _vlist[vindex_1].y - _vlist[vindex_0].y;
		u.z = _vlist[vindex_1].z - _vlist[vindex_0].z;
		u.w = 1;

		v.x = _vlist[vindex_2].x - _vlist[vindex_0].x;
		v.y = _vlist[vindex_2].y - _vlist[vindex_0].y;
		v.z = _vlist[vindex_2].z - _vlist[vindex_0].z;
		v.w = 1;

		// compute cross product
		n = u.Cross(v);

		// compute length of normal accurately and store in poly nlength
		// +- epsilon later to fix over/underflows
		obj->_plist[pindex].nlength = n.Length();

	} // end for poly
}

void MD2Container::ExtractFrame(RenderObject* obj)
{
	// this function extracts a single frame of animation from the md2 container and stores
	// in into the object4dv2 container, this is so we can leverage the library to transform,
	// light, etc. the mesh rather than writing new functions, granted the process of extraction
	// is an unneeded overhead, but since we will only have a few MD2 object running around 
	// the rendering swamps the time to extract by many orders of magnitude, so the extraction
	// is negligable
	// the function also interpolates frames; if the curr_frame value is non-intergral then
	// the function will blend frames together based on the decimal fraction 

	// test if frame number is greater than max frames allowable, some models are malformed
	int frame_0,   // starting frame to interpolate
		frame_1;   // ending frame to interpolate


	// step 1: decide if this is an interpolated frame?

	float ivalue = _curr_frame - (int)_curr_frame;

	// test for integer?
	if (ivalue == 0.0)
	{
		// single frame, no interpolation
		frame_0  = _curr_frame;

		// check for overflow?
		if (frame_0 >= _num_frames)
			frame_0 = _num_frames-1;

		// copy vertex list for selected frame, vertex list begins at
		// base index (_num_verts * _curr_frame)
		int base_index = _num_verts * frame_0;

		// copy vertices now from base
		for (int vindex = 0; vindex < _num_verts; vindex++)
		{
			// copy the vertex
			obj->_vlist_local[vindex].x = _vlist[vindex + base_index].x;
			obj->_vlist_local[vindex].y = _vlist[vindex + base_index].y;
			obj->_vlist_local[vindex].z = _vlist[vindex + base_index].z;
			obj->_vlist_local[vindex].w = 1;  

			// every vertex has a point and texture attached, set that in the flags attribute
			SET_BIT(obj->_vlist_local[vindex].attr, VERTEX_ATTR_POINT);
			SET_BIT(obj->_vlist_local[vindex].attr, VERTEX_ATTR_TEXTURE);

		} // end for vindex

	} // end if
	else
	{
		// interpolate between curr_frame and curr_frame+1 based
		// on ivalue
		frame_0  = _curr_frame;
		frame_1  = _curr_frame+1;

		// check for overflow?
		if (frame_0 >= _num_frames)
			frame_0 = _num_frames-1;

		// check for overflow?
		if (frame_1 >= _num_frames)
			frame_1 = _num_frames-1;

		// interpolate vertex lists for selected frame(s), vertex list(s) begin at
		// base index (_num_verts * _curr_frame)
		int base_index_0 = _num_verts * frame_0;
		int base_index_1 = _num_verts * frame_1;

		// interpolate vertices now from base frame 0,1
		for (int vindex = 0; vindex < _num_verts; vindex++)
		{
			// interpolate the vertices
			obj->_vlist_local[vindex].x = ((1-ivalue)*_vlist[vindex + base_index_0].x + ivalue*_vlist[vindex + base_index_1].x);
			obj->_vlist_local[vindex].y = ((1-ivalue)*_vlist[vindex + base_index_0].y + ivalue*_vlist[vindex + base_index_1].y);
			obj->_vlist_local[vindex].z = ((1-ivalue)*_vlist[vindex + base_index_0].z + ivalue*_vlist[vindex + base_index_1].z);
			obj->_vlist_local[vindex].w = 1;  

			// every vertex has a point and texture attached, set that in the flags attribute
			SET_BIT(obj->_vlist_local[vindex].attr, VERTEX_ATTR_POINT);
			SET_BIT(obj->_vlist_local[vindex].attr, VERTEX_ATTR_TEXTURE);
		} // end for vindex

	} // end if

}

void MD2Container::SetAnimation(int anim_state, int anim_mode)
{
	// this function initializes an animation for play back
	_anim_state   = anim_state; 
	_anim_counter = 0;    
	_anim_speed   = MD2_ANIMATIONS[anim_state].anim_speed;
	_anim_mode    = anim_mode; 

	// set initial frame 
	_curr_frame   = MD2_ANIMATIONS[anim_state].start_frame;

	// set animation complete flag
	_anim_complete = 0;
}

void MD2Container::Animate()
{
	// animate the mesh to next frame based on state and interpolation values...

	// update animation counter
	if (++_anim_counter >= _anim_speed)
	{
		// reset counter
		_anim_counter = 0;

		// animate mesh with interpolation, algorithm is straightforward interpolate
		// from current frame in animation to next and blend vertex positions based  
		// on interpolant ivalue in the md2_container class, couple tricky parts
		// are to watch out for the endpost values of the animation, etc.
		// if the interpolation rate irate=1.0 then there is no interpolation
		// for all intent purposes...

		// add interpolation rate to interpolation value, test for next frame
		_curr_frame+=MD2_ANIMATIONS[_anim_state].irate;

		// test if sequence is complete?
		if (_curr_frame > MD2_ANIMATIONS[_anim_state].end_frame)
		{
			// test for one shot, if so then reset to last frame, if loop, the loop
			if (_anim_mode == MD2_ANIM_LOOP)
			{
				// loop animation back to starting frame
				_curr_frame = MD2_ANIMATIONS[_anim_state].start_frame;
			} // end if
			else
			{
				// MD2_ANIM_SINGLE_SHOT
				_curr_frame = MD2_ANIMATIONS[_anim_state].end_frame; 

				// set complete flag incase outside wants to take action
				_anim_complete = 1;
			} // end else

		} // end if sequence complete

	} // end if time to animate

}

}