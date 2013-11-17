#include "Camera.h"

#include "tmath.h"
#include "defines.h"

namespace t3d
{
Camera::Camera(int cam_attr,          // attributes
			   const vec4& cam_pos,   // initial camera position
			   const vec4& cam_dir,  // initial camera angles
			   const vec4& cam_target, // UVN target
			   float near_clip_z,     // near and far clipping planes
			   float far_clip_z,
			   float fov,             // field of view in degrees
			   float viewport_width,  // size of final screen viewport
			   float viewport_height)
{
	// this function initializes the camera object cam, the function
	// doesn't do a lot of error checking or sanity checking since 
	// I want to allow you to create projections as you wish, also 
	// I tried to minimize the number of parameters the functions needs

	// first set up parms that are no brainers
	_attr = cam_attr;              // camera attributes

	_pos = cam_pos; // positions
	_dir = cam_dir; // direction vector or angles for
	// euler camera
	// for UVN camera
	_u.Assign(1, 0, 0);	 // set to +x
	_v.Assign(0, 1, 0);	 // set to +y
	_n.Assign(0, 0, 1);	 // set to +z   

	_target = cam_target; // UVN target

	_near_clip_z = near_clip_z;     // near z=constant clipping plane
	_far_clip_z  = far_clip_z;      // far z=constant clipping plane

	_viewport_width  = viewport_width;   // dimensions of viewport
	_viewport_height = viewport_height;

	_viewport_center_x = (viewport_width-1)/2.0f; // center of viewport
	_viewport_center_y = (viewport_height-1)/2.0f;

	_aspect_ratio = (float)viewport_width/(float)viewport_height;

	// set independent vars
	_fov              = fov;

	// set the viewplane dimensions up, they will be 2 x (2/ar)
	_viewplane_width  = 2.0f;
	_viewplane_height = 2.0f/_aspect_ratio;

	// now we know fov and we know the viewplane dimensions plug into formula and
	// solve for view distance parameters
	float tan_fov_div2 = tan(DEG_TO_RAD(fov/2.0f));

	_view_dist = (0.5f)*(_viewplane_width)*tan_fov_div2;

	// test for 90 fov first since it's easy :)
	if (fov == 90.0f)
	{
		// set up the clipping planes -- easy for 90 degrees!
		vec3 pt_origin; // point on the plane

		vec3 vn; // normal to plane

		// right clipping plane 
		vn.Assign(1, 0, -1); // x=z plane
		_rt_clip_plane.Assign(pt_origin, vn);

		// left clipping plane
		vn.Assign(-1,0,-1);	// -x=z plane
		_lt_clip_plane.Assign(pt_origin, vn);

		// top clipping plane
		vn.Assign(0,1,-1);	// y=z plane
		_tp_clip_plane.Assign(pt_origin, vn);

		// bottom clipping plane
		vn.Assign(0,-1,-1);	// y=z plane
		_bt_clip_plane.Assign(pt_origin, vn);

	} // end if d=1
	else 
	{
		// now compute clipping planes yuck!
		vec3 pt_origin; // point on the plane

		vec3 vn; // normal to plane

		// since we don't have a 90 fov, computing the normals
		// are a bit tricky, there are a number of geometric constructions
		// that solve the problem, but I'm going to solve for the
		// vectors that represent the 2D projections of the frustrum planes
		// on the x-z and y-z planes and then find perpendiculars to them

		// right clipping plane, check the math on graph paper 
		vn.Assign(_view_dist,0,-_viewplane_width/2.0f);
		_rt_clip_plane.Assign(pt_origin, vn);

		// left clipping plane, we can simply reflect the right normal about
		// the z axis since the planes are symetric about the z axis
		// thus invert x only
		vn.Assign(-_view_dist,0,-_viewplane_width/2.0f);
		_lt_clip_plane.Assign(pt_origin, vn);

		// top clipping plane, same construction
		vn.Assign(0, _view_dist,-_viewplane_width/2.0f);
		_tp_clip_plane.Assign(pt_origin, vn);

		// bottom clipping plane, same inversion
		vn.Assign(0,-_view_dist,-_viewplane_width/2.0f);
		_bt_clip_plane.Assign(pt_origin, vn);

	} // end else
}

void Camera::BuildMatrixEuler(int cam_rot_seq)
{
	// this creates a camera matrix based on Euler angles 
	// and stores it in the sent camera object
	// if you recall from chapter 6 to create the camera matrix
	// we need to create a transformation matrix that looks like:

	// Mcam = mt(-1) * my(-1) * mx(-1) * mz(-1)
	// that is the inverse of the camera translation matrix mutilplied
	// by the inverses of yxz, in that order, however, the order of
	// the rotation matrices is really up to you, so we aren't going
	// to force any order, thus its programmable based on the value
	// of cam_rot_seq which can be any value CAM_ROT_SEQ_XYZ where 
	// XYZ can be in any order, YXZ, ZXY, etc.

	mat4 mt_inv,  // inverse camera translation matrix
		 mx_inv,  // inverse camera x axis rotation matrix
		 my_inv,  // inverse camera y axis rotation matrix
		 mz_inv,  // inverse camera z axis rotation matrix
		 mrot;    // concatenated inverse rotation matrices

	// step 1: create the inverse translation matrix for the camera position
	mt_inv = mat4::Translate(-_pos.x, -_pos.y, -_pos.z);

	// step 2: create the inverse rotation sequence for the camera
	// rember either the transpose of the normal rotation matrix or
	// plugging negative values into each of the rotations will result
	// in an inverse matrix

	// first compute all 3 rotation matrices

	// extract out euler angles
	float theta_x = _dir.x;
	float theta_y = _dir.y;
	float theta_z = _dir.z;

	// set the matrix up 
	mx_inv = mat4::RotateX(-theta_x);

	// set the matrix up 
	my_inv = mat4::RotateY(-theta_y);

	// set the matrix up 
	mz_inv = mat4::RotateZ(-theta_z);

	// now compute inverse camera rotation sequence
	switch(cam_rot_seq)
	{
	case CAM_ROT_SEQ_XYZ:
		mrot = mx_inv * my_inv * mz_inv;
		break;
	case CAM_ROT_SEQ_YXZ:
		mrot = my_inv * mx_inv * mz_inv;
		break;
	case CAM_ROT_SEQ_XZY:
		mrot = mx_inv * mz_inv * my_inv;
		break;
	case CAM_ROT_SEQ_YZX:
		mrot = my_inv * mz_inv * mx_inv;
		break;
	case CAM_ROT_SEQ_ZYX:
		mrot = mz_inv * my_inv * mx_inv;
		break;
	case CAM_ROT_SEQ_ZXY:
		mrot = mz_inv * mx_inv * my_inv;
		break;
	default: 
		break;
	} // end switch

	// now mrot holds the concatenated product of inverse rotation matrices
	// multiply the inverse translation matrix against it and store in the 
	// camera objects' camera transform matrix we are done!
	_mcam = mt_inv * mrot;
}

void Camera::BuildMatrixUVN(int mode)
{
	// this creates a camera matrix based on a look at vector n,
	// look up vector v, and a look right (or left) u
	// and stores it in the sent camera object, all values are
	// extracted out of the camera object itself
	// mode selects how uvn is computed
	// UVN_MODE_SIMPLE - low level simple model, use the target and view reference point
	// UVN_MODE_SPHERICAL - spherical mode, the x,y components will be used as the
	//     elevation and heading of the view vector respectively
	//     along with the view reference point as the position
	//     as usual

	mat4 mt_inv,  // inverse camera translation matrix
		 mt_uvn,  // the final uvn matrix
		 mtmp;    // temporary working matrix

	// step 1: create the inverse translation matrix for the camera
	// position
	mt_inv = mat4::Translate(-_pos.x, -_pos.y, -_pos.z);

	// step 2: determine how the target point will be computed
	if (mode == UVN_MODE_SPHERICAL)
	{
		// use spherical construction
		// target needs to be recomputed

		// extract elevation and heading 
		float phi   = _dir.x; // elevation
		float theta = _dir.y; // heading

		// compute trig functions once
		float sin_phi = sin(phi);
		float cos_phi = cos(phi);

		float sin_theta = sin(theta);
		float cos_theta = cos(theta);

		// now compute the target point on a unit sphere x,y,z
		_target.x = -1*sin_phi*sin_theta;
		_target.y =  1*cos_phi;
		_target.z =  1*sin_phi*cos_theta;
	}

	// at this point, we have the view reference point, the target and that's
	// all we need to recompute u,v,n
	vec3 u, v, n;
	// Step 1: n = <target position - view reference point>
	vec4 tmp = _target - _pos;
	n.Assign(tmp.x, tmp.y, tmp.z);

	// Step 2: Let v = <0,1,0>
	v.Assign(0,1,0);

	// Step 3: u = (v x n)
	u = v.Cross(n);

	// Step 4: v = (n x u)
	v = n.Cross(u);

	// Step 5: normalize all vectors
	_u.Assign(u.Normalized());
	_v.Assign(v.Normalized());
	_n.Assign(n.Normalized());

	// build the UVN matrix by placing u,v,n as the columns of the matrix
	float mt[16] = {
		_u.x,    _v.x,     _n.x,     0,
		_u.y,    _v.y,     _n.y,     0,
		_u.z,    _v.z,     _n.z,     0,
		0,          0,        0,     1
	};
	mt_uvn.Assign(mt);

	// now multiply the translation matrix and the uvn matrix and store in the 
	// final camera matrix mcam
	_mcam = mt_inv * mt_uvn;
}

}