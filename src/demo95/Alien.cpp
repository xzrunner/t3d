#include "Alien.h"

#include "Camera.h"
#include "RenderObject.h"
#include "RenderList.h"
#include "Modules.h"
#include "Graphics.h"
#include "Sound.h"
#include "context.h"
#include "PrimitiveDraw.h"

namespace raider3d {

AlienList::AlienList()
	: obj_alien(NULL)
{
	for (int eindex = 0; eindex < NUM_EXPLOSIONS; eindex++)
		explobj[eindex] = new t3d::RenderObject;

	obj_alien = new t3d::RenderObject;
}

AlienList::~AlienList()
{
	delete obj_alien;
}

void AlienList::Init()
{
	vec4 vscale(18.00,18.00,18.00,1),		// scale of object
		vpos(0,0,0,1),        // position of object
		vrot(0,0,0,1);        // initial orientation of object

	// load the master tank object
	obj_alien->LoadCOB("../../assets/chap09/tie04.cob",&vscale, &vpos, &vrot, 
		VERTEX_FLAGS_SWAP_YZ | VERTEX_FLAGS_SWAP_XZ |
		VERTEX_FLAGS_INVERT_WINDING_ORDER |
		VERTEX_FLAGS_TRANSFORM_LOCAL | 
		VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD);

	// initializes all the ALIEN fighters to a known state
	for (int index = 0; index < NUM_ALIENS; index++)
	{
		// zero ALIEN fighter out
		memset((void *)&aliens[index], 0, sizeof (Alien) );

		// set any other specific info now...
		aliens[index].state = ALIEN_STATE_DEAD;

	} // end for

	explosion_sound_id = t3d::Modules::GetSound().LoadWAV("../../assets/chap09/exp1.wav");
}

int AlienList::Start()
{
	// this function hunts in the alien list, finds an available alien and 
	// starts it up

	for (int index = 0; index < NUM_ALIENS; index++)
	{
		// is this alien available?
		if (aliens[index].state == ALIEN_STATE_DEAD)
		{
			// clear any residual data
			memset((void *)&aliens[index], 0, sizeof (Alien) );

			// start alien up
			aliens[index].state = ALIEN_STATE_ALIVE;

			// select random position in bounding volume
			aliens[index].pos.x = RAND_RANGE(-1000,1000);
			aliens[index].pos.y = RAND_RANGE(-1000,1000);
			aliens[index].pos.z = 20000;

			// select velocity based on difficulty level
			aliens[index].vel.x = RAND_RANGE(-10,10);
			aliens[index].vel.y = RAND_RANGE(-10,10);
			aliens[index].vel.z = -(10*diff_level+rand()%200);

			// set rotation rate for z axis only
			aliens[index].rot.z = RAND_RANGE(-5,5);

			// return the index
			return(index);
		} // end if   

	} // end for index

	// failure
	return(-1);	
}

void AlienList::DrawAliens(const t3d::Camera& cam, t3d::RenderList& rlist)
{
	// this function draws all the active ALIEN fighters

	mat4 mrot; // used to transform objects

	for (int index = 0; index < NUM_ALIENS; index++)
	{
		// which state is alien in
		switch(aliens[index].state)
		{
			// is the alien dead? if so move on
		case ALIEN_STATE_DEAD: 
			break;

			// is the alien alive?
		case ALIEN_STATE_ALIVE:
		case ALIEN_STATE_DYING:
			{
				// reset the object (this only matters for backface and object removal)
				obj_alien->Reset();

				// generate rotation matrix around y axis
				mrot = mat4::RotateX(aliens[index].ang.x) 
					* mat4::RotateY(aliens[index].ang.y) 
					* mat4::RotateZ(aliens[index].ang.z);

				// rotate the local coords of the object
				obj_alien->Transform(mrot, TRANSFORM_LOCAL_TO_TRANS, true);

				// set position of constant shaded cube
				obj_alien->SetWorldPos(aliens[index].pos.x, aliens[index].pos.y, aliens[index].pos.z);

				// attempt to cull object
				if (!obj_alien->Cull(cam, CULL_OBJECT_XYZ_PLANES))
				{
					// perform world transform
					obj_alien->ModelToWorld(TRANSFORM_TRANS_ONLY);

					// insert the object into render list
					rlist.Insert(*obj_alien, false);
				} // end if 


				// ok, now test for collision with energy bursts, strategy is as follows
				// we will project the bounding box of the object into screen space to coincide with
				// the energy burst on the screen, if there is an overlap then the target is hit
				// simply need to do a few transforms, this kinda sucks that we are going to do this work
				// 2x, but the problem is that the graphics pipeline knows nothing about collision etc.,
				// so we can't "get to" the information me want since we are ripping the objects apart
				// and passing them down the pipeline for a later world to camera transform, thus you
				// may want to think about this problem when making your own game, how tight to couple
				// collision and the engine, HOWEVER, the only reason we are having a problem at all is that
				// we want to use the screen coords of the energy burst this is fine, but a more effective method
				// would be to compute the 3D world coords of where the energy burst is firing and then project
				// that parallelpiped in 3D space to see where the player is trying to fire, this is a classic
				// problem with screen picking, hence, in the engine, when an object is rendered sometimes its 
				// a good idea to track somekind of collision boundary in screen coords that can be used later
				// for "object picking" and collision, anyway, let's do it the easy way, but the long way....

				// first is the player firing weapon?
				if (weapon_active)
				{
					// we need 4 transforms total, first we need all our points in world coords,
					// then we need camera coords, then perspective coords, then screen coords

					// we need to transform 2 points: the center and a point lying on the surface of the  
					// bounding sphere, as long as the 

					vec4 pbox[4], // bounding box coordinates, center points, surrounding object
								  // denoted by X's, we need to project these to screen coords
								  // ....X.... 
								  // . |   | .
								  // X |-O-| X
								  // . |   | . 
								  // ....X....
								  // we will use the average radius as the distance to each X from the center

								  presult; // used to hold temp results

					// world to camera transform

					// transform center point only
					presult = cam.CameraMat() * obj_alien->GetWorldPos();

					// result holds center of object in camera coords now
					// now we are in camera coords, aligned to z-axis, compute radial point axis aligned
					// bounding box points

					// x+r, y, z
					pbox[0].x = presult.x + obj_alien->GetAvgRadius();
					pbox[0].y = presult.y;
					pbox[0].z = presult.z;
					pbox[0].w = 1;        

					// x-r, y, z
					pbox[1].x = presult.x - obj_alien->GetAvgRadius();
					pbox[1].y = presult.y;
					pbox[1].z = presult.z;
					pbox[1].w = 1;     

					// x, y+r, z
					pbox[2].x = presult.x;
					pbox[2].y = presult.y + obj_alien->GetAvgRadius();
					pbox[2].z = presult.z;
					pbox[2].w = 1;     

					// x, y-r, z
					pbox[3].x = presult.x;
					pbox[3].y = presult.y - obj_alien->GetAvgRadius();
					pbox[3].z = presult.z;
					pbox[3].w = 1;     


					// now we are ready to project the points to screen space
					float alpha = (0.5*cam.ViewportWidth()-0.5);
					float beta  = (0.5*cam.ViewportHeight()-0.5);

					// loop and process each point
					for (int bindex=0; bindex < 4; bindex++)
					{
						float z = pbox[bindex].z;

						// perspective transform first
						pbox[bindex].x = cam.ViewDist()*pbox[bindex].x/z;
						pbox[bindex].y = cam.ViewDist()*pbox[bindex].y*cam.AspectRatio()/z;
						// z = z, so no change

						// screen projection
						pbox[bindex].x =  alpha*pbox[bindex].x + alpha; 
						pbox[bindex].y = -beta*pbox[bindex].y + beta;

					} // end for bindex

#ifdef DEBUG_ON
					// now we have the 4 points is screen coords and we can test them!!! ya!!!!
					t3d::Graphics& graphics = t3d::Modules::GetGraphics();
					t3d::DrawClipLine32(pbox[0].x, pbox[2].y, pbox[1].x, pbox[2].y, rgb_red, graphics.GetBackBuffer(), graphics.GetBackLinePitch());
					t3d::DrawClipLine32(pbox[0].x, pbox[3].y, pbox[1].x, pbox[3].y, rgb_red, graphics.GetBackBuffer(), graphics.GetBackLinePitch());

					t3d::DrawClipLine32(pbox[0].x, pbox[2].y, pbox[0].x, pbox[3].y, rgb_red, graphics.GetBackBuffer(), graphics.GetBackLinePitch());
					t3d::DrawClipLine32(pbox[1].x, pbox[2].y, pbox[1].x, pbox[3].y, rgb_red, graphics.GetBackBuffer(), graphics.GetBackLinePitch());
#endif

					// test for collision
					if ((cross_x_screen > pbox[1].x) && (cross_x_screen < pbox[0].x) &&
						(cross_y_screen > pbox[2].y) && (cross_y_screen < pbox[3].y) )
					{
						StartMeshExplosion(mrot,       // initial orientation
										   3,          // the detail level,1 highest detail
										   .5,         // velocity of explosion shrapnel
										   100);       // total lifetime of explosion         

						// remove from simulation
						aliens[index].state = ALIEN_STATE_DEAD;

						// make some sound
//						t3d::Modules::GetSound().Play(explosion_sound_id);

						// increment hits
						hits++;

						// take into consideration the z, the speed, the level, blah blah
						score += (int)((diff_level*10 + obj_alien->GetWorldPos().z/10));

					} // end if

				} // end if weapon active

			} break;

		default: break;

		} // end switch

	} // end for index

	// debug code

#ifdef DEBUG_ON
	// reset the object (this only matters for backface and object removal)
	obj_alien->Reset();
	static int ang_y = 0;
	// generate rotation matrix around y axis
	mrot = mat4::RotateY(ang_y++);

	// rotate the local coords of the object
	obj_alien->Transform(mrot, TRANSFORM_LOCAL_TO_TRANS,1);

	// set position of constant shaded cube
	obj_alien->SetWorldPos(px, py, pz);

	// perform world transform
	obj_alien->ModelToWorld(TRANSFORM_TRANS_ONLY);

	// insert the object into render list
	rlist.Insert(obj_alien);
#endif
}

void AlienList::DrawExplosions(t3d::RenderList& rlist)
{
	// this function draws the mesh explosions
	mat4 mrot; // used to transform objects

	// draw the explosions, note we do NOT cull them
	for (int eindex = 0; eindex < NUM_EXPLOSIONS; eindex++)
	{
		// is the mesh explosions active
		if ((explobj[eindex]->_state & OBJECT_STATE_ACTIVE))
		{
			// reset the object
			explobj[eindex]->Reset();

			// generate rotation matrix 
			//Build_XYZ_Rotation_MATRIX4X4(0, 0, 0 ,&mrot);

			// rotate the local coords of the object
			explobj[eindex]->Transform(mrot, TRANSFORM_LOCAL_TO_TRANS,1);

			// perform world transform
			explobj[eindex]->ModelToWorld(TRANSFORM_TRANS_ONLY);

			// insert the object into render list
			rlist.Insert(*explobj[eindex]);

			// now animate the mesh
			vec4* trajectory = (vec4*)explobj[eindex]->_ivar1;

			for (int pindex = 0; pindex < explobj[eindex]->_num_polys; pindex++)
			{
				// vertex 0
				explobj[eindex]->_vlist_local[pindex*3 + 0].v = 
					explobj[eindex]->_vlist_local[pindex*3 + 0].v + trajectory[pindex*2+0];

				// vertex 1
				explobj[eindex]->_vlist_local[pindex*3 + 1].v = 
					explobj[eindex]->_vlist_local[pindex*3 + 1].v + trajectory[pindex*2 + 0];

				// vertex 2
				explobj[eindex]->_vlist_local[pindex*3 + 2].v = 
					explobj[eindex]->_vlist_local[pindex*3 + 2].v + trajectory[pindex*2 + 0];

				// modulate color
				//explobj[eindex]->_plist[pindex].color = RGB16Bit(rand()%255,0,0);

			} // end for pindex

			// update counter, test for terminate
			if (--explobj[eindex]->_ivar2 < 0)
			{
				// reset this object
				explobj[eindex]->_state  = OBJECT_STATE_NULL;           

				// release memory of object, but only data that isn't linked to master object
				// local vertex list
				if (explobj[eindex]->_head_vlist_local)
					free(explobj[eindex]->_head_vlist_local);

				// transformed vertex list
				if (explobj[eindex]->_head_vlist_trans)
					free(explobj[eindex]->_head_vlist_trans);

				// polygon list
				if (explobj[eindex]->_plist)
					free(explobj[eindex]->_plist);

				// trajectory list
				if ((vec4*)(explobj[eindex]->_ivar1))
					free((vec4*)explobj[eindex]->_ivar1);

				// now clear out object completely
				memset((void *)explobj[eindex], 0, sizeof(t3d::RenderObject));

			} // end if 

		} // end if  

	} // end for eindex
}

void AlienList::Update(const t3d::Camera& cam)
{
	// this function moves the aliens and performs AI

	for (int index = 0; index < NUM_ALIENS; index++)
	{
		// which state is alien in
		switch(aliens[index].state)
		{
			// is the alien dead? if so move on
		case ALIEN_STATE_DEAD: break;

			// is the alien alive?
		case ALIEN_STATE_ALIVE:
		case ALIEN_STATE_DYING:
			{
				// move the alien
				aliens[index].pos.x+=aliens[index].vel.y;
				aliens[index].pos.y+=aliens[index].vel.y;
				aliens[index].pos.z+=aliens[index].vel.z;

				// rotate the alien
				if ((aliens[index].ang.x+=aliens[index].rot.x) > 360)
					aliens[index].ang.x = 0;

				if ((aliens[index].ang.y+=aliens[index].rot.y) > 360)
					aliens[index].ang.y = 0;

				if ((aliens[index].ang.z+=aliens[index].rot.z) > 360)
					aliens[index].ang.z = 0;

				// test if alien has past players near clipping plane
				// if so remove from simulation
				if (aliens[index].pos.z < cam.NearClipZ())
				{
					// remove from simulation
					aliens[index].state = ALIEN_STATE_DEAD;

					// another one gets away!
					escaped++;
				} // end if 

			} break;


		default: break;

		} // end switch

	} // end for index
}

int AlienList::StartMeshExplosion(const mat4& mrot, int detail, float rate, int lifetime)
{
	// this function "blows up" an object, it takes a pointer to the object to
	// be destroyed, the detail level of the polyogonal explosion, 1 is high detail, 
	// numbers greater than 1 are lower detail, the detail selects the polygon extraction
	// stepping, hence a detail of 5 would mean every 5th polygon in the original mesh
	// should be part of the explosion, on average no more than 10-50% of the polygons in 
	// the original mesh should be used for the explosion; some would be vaporized and
	// in a more practical sense, when an object is far, there's no need to have detail
	// the next parameter is the rate which is used as a general scale for the explosion
	// velocity, and finally lifetime which is the number of cycles to display the explosion

	// the function works by taking the original object then copying the core information
	// except for the polygon and vertex lists, the function operates by selecting polygon 
	// by polygon to be destroyed and then makes a copy of the polygon AND all of it's vertices,
	// thus the vertex coherence is completely lost, this is a necessity since each polygon must
	// be animated separately by the engine, thus they can't share vertices, additionally at the
	// end if the vertex list will be the velocity and rotation information for each polygon
	// this is hidden to the rest of the engine, now for the explosion, the center of the object
	// is used as the point of origin then a ray is drawn thru each polygon, then each polygon
	// is thrown at some velocity with a small rotation rate
	// finally the system can only have so many explosions at one time

	// step: 1 find an available explosion
	for (int eindex = 0; eindex < NUM_EXPLOSIONS; eindex++)
	{
		// is this explosion available?
		if (explobj[eindex]->_state == OBJECT_STATE_NULL)
		{
			// copy the object, including the pointers which we will unlink shortly...
			*explobj[eindex]                =  *obj_alien;

			explobj[eindex]->_state = OBJECT_STATE_ACTIVE | OBJECT_STATE_VISIBLE;

			// recompute a few things
			explobj[eindex]->_num_polys      = obj_alien->_num_polys/detail;
			explobj[eindex]->_num_vertices   = 3*obj_alien->_num_polys;
			explobj[eindex]->_total_vertices = 3*obj_alien->_num_polys; // one frame only

			// unlink/re-allocate all the pointers except the texture coordinates, we can use those, they don't
			// depend on vertex coherence
			// allocate memory for vertex lists
			if (!(explobj[eindex]->_vlist_local = (t3d::Vertex*)malloc(sizeof(t3d::Vertex)*explobj[eindex]->_num_vertices)))
				return(0);

			// clear data
			memset((void *)explobj[eindex]->_vlist_local,0,sizeof(t3d::Vertex)*explobj[eindex]->_num_vertices);

			if (!(explobj[eindex]->_vlist_trans = (t3d::Vertex*)malloc(sizeof(t3d::Vertex)*explobj[eindex]->_num_vertices)))
				return(0);

			// clear data
			memset((void *)explobj[eindex]->_vlist_trans,0,sizeof(t3d::Vertex)*explobj[eindex]->_num_vertices);

			// allocate memory for polygon list
			if (!(explobj[eindex]->_plist = (t3d::Polygon*)malloc(sizeof(t3d::Polygon)*explobj[eindex]->_num_polys)))
				return(0);

			// clear data
			memset((void *)explobj[eindex]->_plist,0,sizeof(t3d::Polygon)*explobj[eindex]->_num_polys);

			// now, we need somewhere to store the vector trajectories of the polygons and their rotational rates
			// so let's allocate an array of vec4 elements to hold this information and then store it
			// in ivar1, therby using ivar as a pointer, this is perfectly fine :)
			// each record will consist of a velocity and a rotational rate in x,y,z, so V0,R0, V1,R1,...Vn-1, Rn-1
			// allocate memory for polygon list
			if (!(explobj[eindex]->_ivar1 = (int)malloc(sizeof(vec4)*2*explobj[eindex]->_num_polys)))
				return(0);

			// clear data
			memset((void *)explobj[eindex]->_ivar1,0,sizeof(vec4)*2*explobj[eindex]->_num_polys);

			// alias working pointer
			vec4* trajectory = (vec4*)explobj[eindex]->_ivar1;

			// these are needed to track the "head" of the vertex list for multi-frame objects
			explobj[eindex]->_head_vlist_local = explobj[eindex]->_vlist_local;
			explobj[eindex]->_head_vlist_trans = explobj[eindex]->_vlist_trans;

			// set the lifetime in ivar2
			explobj[eindex]->_ivar2 = lifetime;

			// now comes the tricky part, loop and copy each polygon, but as we copy the polygons, we have to copy
			// the vertices, and insert them, and fix up the vertice indices etc.
			for (int pindex = 0; pindex < explobj[eindex]->_num_polys; pindex++)
			{
				// alright, we want to copy the (pindex*detail)th poly from the master to the (pindex)th
				// polygon in explosion
				explobj[eindex]->_plist[pindex] = obj_alien->_plist[pindex*detail];

				// we need to modify, the vertex list pointer and the vertex indices themselves
				explobj[eindex]->_plist[pindex].vlist = explobj[eindex]->_vlist_local;

				// now comes the hard part, we need to first copy the vertices from the original mesh
				// into the new vertex storage, but at the same time keep the same vertex ordering

				// vertex 0
				explobj[eindex]->_plist[pindex].vert[0] = pindex*3 + 0;
				explobj[eindex]->_vlist_local[pindex*3 + 0] = obj_alien->_vlist_local[ obj_alien->_plist[pindex*detail].vert[0] ];

				// vertex 1
				explobj[eindex]->_plist[pindex].vert[1] = pindex*3 + 1;
				explobj[eindex]->_vlist_local[pindex*3 + 1] = obj_alien->_vlist_local[ obj_alien->_plist[pindex*detail].vert[1] ];

				// vertex 2
				explobj[eindex]->_plist[pindex].vert[2] = pindex*3 + 2;
				explobj[eindex]->_vlist_local[pindex*3 + 2] = obj_alien->_vlist_local[ obj_alien->_plist[pindex*detail].vert[2] ];

				// reset shading flag to constant for some of the shrapnel since they are giving off light 
				if ((rand()%5) == 1)
				{
					SET_BIT(explobj[eindex]->_plist[pindex].attr, POLY_ATTR_SHADE_MODE_PURE);
					RESET_BIT(explobj[eindex]->_plist[pindex].attr, POLY_ATTR_SHADE_MODE_GOURAUD);
					RESET_BIT(explobj[eindex]->_plist[pindex].attr, POLY_ATTR_SHADE_MODE_FLAT);

					// set color
					explobj[eindex]->_plist[pindex].color = t3d::Modules::GetGraphics().GetColor(128+rand()%128,0,0);

				} // end if

				// add some random noise to trajectory
				static vec4 mv;

				// first compute trajectory as vector from center to vertex piercing polygon
				trajectory[pindex*2+0] = obj_alien->_vlist_local[ obj_alien->_plist[pindex*detail].vert[0] ].v;
				trajectory[pindex*2+0] = trajectory[pindex*2+0] * rate;     

				mv.x = RAND_RANGE(-10,10);
				mv.y = RAND_RANGE(-10,10);
				mv.z = RAND_RANGE(-10,10);
				mv.w = 1;

				trajectory[pindex*2+0] = trajectory[pindex*2+0] + mv;

				// now rotation rate, this is difficult to do since we don't have the center of the polygon
				// maybe later...
				// trajectory[pindex*2+1] = 

			} // end for pindex


			// now transform final mesh into position destructively
			explobj[eindex]->Transform(mrot, TRANSFORM_LOCAL_ONLY,1);

			return(1);

		} // end if found a free explosion

	} // end for eindex

	// return failure
	return(0);
}

}