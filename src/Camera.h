#pragma once

#include "Plane.h"
#include "Matrix.h"

namespace t3d {

class Camera
{
public:
	Camera(int cam_attr, const vec4& cam_pos, const vec4& cam_dir, const vec4& cam_target,
		float near_clip_z, float far_clip_z, float fov, float viewport_width, float viewport_height);

	void BuildMatrixEuler(int cam_rot_seq);
	void BuildMatrixUVN(int mode);

	const mat4& CameraMat() const { return _mcam; }

	float ViewDist() const { return _view_dist; }

	float AspectRatio() const { return _aspect_ratio; }

	float ViewportWidth() const { return _viewport_width; }
	float ViewportHeight() const { return _viewport_height; }

	const vec4& Pos() const { return _pos; }
	vec4& PosRef() { return _pos; }
	const vec4& Dir() const { return _dir; }
	vec4& DirRef() { return _dir; }

	const vec4& Normal() const { return _n; }

	float FOV() const { return _fov; }

	float NearClipZ() const { return _near_clip_z; }
	float FarClipZ() const { return _far_clip_z; }

	float ViewplaneWidth() const { return _viewplane_width; }
	float ViewplaneHeight() const { return _viewplane_height; }

public:
	int _state;      // state of camera
	int _attr;       // camera attributes

	vec4 _pos;    // world position of camera used by both camera models

	vec4 _dir;	// angles or look at direction of camera for simple 
				// euler camera models, elevation and heading for
				// uvn model

	vec4 _u;     // extra vectors to track the camera orientation
	vec4 _v;     // for more complex UVN camera model
	vec4 _n;        

	vec4 _target; // look at target

	float _view_dist;  // focal length 

	float _fov;          // field of view for both horizontal and vertical axes

	// 3d clipping planes
	// if view volume is NOT 90 degree then general 3d clipping
	// must be employed
	float _near_clip_z;     // near z=constant clipping plane
	float _far_clip_z;      // far z=constant clipping plane

	Plane _rt_clip_plane;  // the right clipping plane
	Plane _lt_clip_plane;  // the left clipping plane
	Plane _tp_clip_plane;  // the top clipping plane
	Plane _bt_clip_plane;  // the bottom clipping plane                        

	float _viewplane_width;     // width and height of view plane to project onto
	float _viewplane_height;    // usually 2x2 for normalized projection or 
	// the exact same size as the viewport or screen window

	// remember screen and viewport are synonomous 
	float _viewport_width;     // size of screen/viewport
	float _viewport_height;
	float _viewport_center_x;  // center of view port (final image destination)
	float _viewport_center_y;

	// aspect ratio
	float _aspect_ratio;

	// these matrices are not necessarily needed based on the method of
	// transformation, for example, a manual perspective or screen transform
	// and or a concatenated perspective/screen, however, having these 
	// matrices give us more flexibility         

	mat4 _mcam;   // storage for the world to camera transform matrix
	mat4 _mper;   // storage for the camera to perspective transform matrix
	mat4 _mscr;   // storage for the perspective to screen transform matrix

}; // Camera

}