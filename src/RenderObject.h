#pragma once

#include <stdio.h>

#include "Vector.h"
#include "Polygon.h"
#include "Matrix.h"
#include "defines.h"

namespace raider3d { class AlienList; }
namespace bspeditor { class BSPFile; }

namespace t3d {

#define VERTEX_FLAGS_INVERT_X               0x0001   // inverts the Z-coordinates
#define VERTEX_FLAGS_INVERT_Y               0x0002   // inverts the Z-coordinates
#define VERTEX_FLAGS_INVERT_Z               0x0004   // inverts the Z-coordinates
#define VERTEX_FLAGS_SWAP_YZ                0x0008   // transforms a RHS model to a LHS model
#define VERTEX_FLAGS_SWAP_XZ                0x0010   
#define VERTEX_FLAGS_SWAP_XY                0x0020
#define VERTEX_FLAGS_INVERT_WINDING_ORDER   0x0040   // invert winding order from cw to ccw or ccw to cc

#define VERTEX_FLAGS_TRANSFORM_LOCAL        0x0200   // if file format has local transform then do it!
#define VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD  0x0400  // if file format has local to world then do it!

// states for objects
#define OBJECT_STATE_NULL             0x0000
#define OBJECT_STATE_ACTIVE           0x0001
#define OBJECT_STATE_VISIBLE          0x0002 
#define OBJECT_STATE_CULLED           0x0004

#define OBJECT_ATTR_SINGLE_FRAME      0x0001 // single frame object (emulates ver 1.0)
#define OBJECT_ATTR_MULTI_FRAME       0x0002 // multi frame object for .md2 support etc.
#define OBJECT_ATTR_TEXTURES          0x0004 // flags if object contains textured polys?
#define OBJECT_ATTR_MIPMAP			  0x0008 // flags if object has a mipmap

// general culling flags
#define CULL_OBJECT_X_PLANE           0x0001 // cull on the x clipping planes
#define CULL_OBJECT_Y_PLANE           0x0002 // cull on the y clipping planes
#define CULL_OBJECT_Z_PLANE           0x0004 // cull on the z clipping planes
#define CULL_OBJECT_XYZ_PLANES        (CULL_OBJECT_X_PLANE | CULL_OBJECT_Y_PLANE | CULL_OBJECT_Z_PLANE)

class Camera;
class RenderObject;

int load_plg(RenderObject* obj, const char* filename, 
			const vec4* scale, const vec4* pos, const vec4* rot);
int load_3dsasc(RenderObject* obj, char *filename, const vec4* scale, const vec4* pos, 
			   const vec4* rot, int vertex_flags);
int load_cob(RenderObject* obj, char *filename, const vec4* scale, const vec4* pos, 
			 const vec4* rot, int vertex_flags, bool mipmap);

class RenderObject
{
public:
	RenderObject();

	int Init(int num_vertices, int num_polys, int num_frames, bool destroy = false);
	int Destroy();

	int SetFrame(int frame);

	int LoadPLG(const char* filename, const vec4* scale, const vec4* pos, const vec4* rot);
	int Load3DSASC(char *filename, const vec4* scale, const vec4* pos, 
		const vec4* rot, int vertex_flags=0);
	int LoadCOB(char *filename,			// filename of Caligari COB file
				const vec4* scale,		// initial scaling factors	
				const vec4* pos,		// initial position
				const vec4* rot,		// initial rotations
				int vertex_flags,		// flags to re-order vertices and perform transforms
				bool mipmap = false);	// mipmap enable flag

	int GenerateTerrain(float twidth,            // width in world coords on x-axis
						float theight,           // height (length) in world coords on z-axis
						float vscale,            // vertical scale of terrain
						char *height_map_file,   // filename of height bitmap encoded in 256 colors
						char *texture_map_file,  // filename of texture map
						int rgbcolor,            // color of terrain if no texture        
						vec4* pos,				 // initial position
						vec4* rot,				 // initial rotations
						int poly_attr,			 // the shading attributes we would like
						int sea_level = -1,		 // height of sea level
						int alpha = -1);		 // alpha level for sea polygons 

	void Reset();

	const vec4& Dir() const { return _dir; }
	vec4& DirRef() { return _dir; }

	void Scale(const vec3& scale, bool all_frames = true);

	void Transform(const mat4& mt, int coord_select, int transform_basis,
		bool all_frames = true);

	void ModelToWorld(int coord_select=TRANSFORM_LOCAL_TO_TRANS,
		bool all_frames = true);

	int Cull(const Camera& cam, int cull_flags);

	void RemoveBackfaces(const Camera& cam);

	void WorldToCamera(const Camera& cam);

	void CameraToPerspective(const Camera& cam);

	void PerspectiveToScreen(const Camera& cam);

	void LightWorld32(const Camera& cam);

	void DrawWire32(unsigned char* video_buffer, int lpitch);
	void DrawSolid32(unsigned char* video_buffer, int lpitch);

	void SetWorldPos(float x, float y, float z) { _world_pos.Assign(x, y, z); }
	const vec4& GetWorldPos() const { return _world_pos; }

	float GetMaxRadius() const { return _max_radius[_curr_frame]; }
	float GetAvgRadius() const { return _avg_radius[_curr_frame]; }

	float GetFVar1() const { return _fvar1; }
	float GetFVar2() const { return _fvar2; }
	float GetIVar1() const { return _ivar1; }
	float GetIVar2() const { return _ivar2; }

	void SetIVar1(int v) { _ivar1 = v; } 

	const Vertex& GetLocalVertex(size_t index) const { return _vlist_local[index]; }

	const Vertex& GetTransVertex(size_t index) const { return _vlist_trans[index]; }
	Vertex& GetTransVertexRef(size_t index) { return _vlist_trans[index]; }

	const Polygon& GetPolygon(size_t index) const { return _plist[index]; }
	Polygon& GetPolygonRef(size_t index) { return _plist[index]; }

	int VerticesNum() const { return _num_vertices; }
	int PolyNum() const { return _num_polys; }

	int State() const { return _state; }
	int Attr() const { return _attr; }

	const BmpImg* GetTexture() const { return _texture; }

private:
	float ComputeRadius();
	int ComputePolyNormals();
	int ComputeVertexNormals();

public:
	static const int MAX_VERTICES = 4096;
	static const int MAX_POLYS = 8192;

private:
	int  _id;				// numeric id of this object
	char _name[256];		// ASCII name of object just for kicks
	int  _state;			// state of object
	int  _attr;				// attributes of object
	int  _mati;				// material index overide (-1) - no material
	float* _avg_radius;		// [MAX_FRAMES] average radius of object used for collision detection
	float* _max_radius;		// [MAX_FRAMES] maximum radius of object

	vec4 _world_pos;		// position of object in world

	vec4 _dir;				// rotation angles of object in local
							// cords or unit direction vector user defined???

	vec4 _ux,_uy,_uz;		// local axes to track full orientation
							// this is updated automatically during
							// rotation calls

	int _num_vertices;		// number of vertices of this object
	int _num_frames;		// number of frames
	int _total_vertices;	// total vertices, redudant, but it saves a multiply in a lot of places
	int _curr_frame;		// current animation frame (0) if single frame

	Vertex* _vlist_local;	// [MAX_VERTICES] // array of local vertices
	Vertex* _vlist_trans;	// [MAX_VERTICES] // array of transformed vertices

	// these are needed to track the "head" of the vertex list for mult-frame objects
	Vertex* _head_vlist_local;
	Vertex* _head_vlist_trans;

	vec2* _tlist;			// texture coordinates list, 3*num polys at max

	BmpImg* _texture;		// pointer to the texture information for simple texture mapping (new)

	int _num_polys;			// number of polygons in object mesh
	Polygon* _plist;		// array of polygons

	int   _ivar1, _ivar2;   // auxiliary vars
	float _fvar1, _fvar2;   // auxiliary vars

	friend class RenderList;
	friend class MD2Container;

	// todo
	friend class raider3d::AlienList;
	friend class bspeditor::BSPFile;

	friend int load_plg(RenderObject* obj, const char* filename, 
		const vec4* scale, const vec4* pos, const vec4* rot);
	friend int load_3dsasc(RenderObject* obj, char *filename, const vec4* scale, 
		const vec4* pos, const vec4* rot, int vertex_flags);
	friend int load_cob(RenderObject* obj, char *filename, const vec4* scale, const vec4* pos, 
		const vec4* rot, int vertex_flags, bool mipmap);
}; // RenderObject

}