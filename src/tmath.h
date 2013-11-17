#pragma once

#include <math.h>
#include <vector>

namespace t3d {

// defines for small numbers
#define EPSILON_E3 (float)(1E-3) 
#define EPSILON_E4 (float)(1E-4) 
#define EPSILON_E5 (float)(1E-5)
#define EPSILON_E6 (float)(1E-6)

// floating point comparison
#define FCMP(a,b) ( (fabs(a-b) < EPSILON_E3) ? 1 : 0)

// pi defines
#define PI         ((float)3.141592654f)
#define PI2        ((float)6.283185307f)
#define PI_DIV_2   ((float)1.570796327f)
#define PI_DIV_4   ((float)0.785398163f) 
#define PI_INV     ((float)0.318309886f) 

// some math macros
#define DEG_TO_RAD(ang) ((ang)*PI/180.0f)
#define RAD_TO_DEG(rads) ((rads)*180.0f/PI)

#define RAND_RANGE(x,y) ( (x) + (rand()%((y)-(x)+1)))

// dot product look up table
extern float dp_inverse_cos[360+2]; // 0 to 180 in .5 degree steps
									// the +2 for padding

// this macro assumes the lookup has 360 elements, where
// each element is in .5 degree increments roughly
// x must be in the range [-1,1]
#define FAST_INV_COS(x) (dp_inverse_cos[(int)(((float)x+1)*(float)180)])

inline float FastDistance(float fx, float fy, float fz)
{
	// this function computes the distance from the origin to x,y,z

	int x,y,z; // used for algorithm

	// make sure values are all positive
	x = (int)(fabs(fx) * 1024);
	y = (int)(fabs(fy) * 1024);
	z = (int)(fabs(fz) * 1024);

	// sort values
	if (y < x) std::swap(x,y);
	if (z < y) std::swap(y,z);
	if (y < x) std::swap(x,y);

	int dist = (z + 11 * (y >> 5) + (x >> 2) );

	// compute distance with 8% error
	return (float)(dist >> 10);

} // end Fast_Distance_3D

inline void IntersectLines(float x0,float y0,float x1,float y1,
						   float x2,float y2,float x3,float y3,
						   float *xi,float *yi)
{
	// this function computes the intersection of the sent lines
	// and returns the intersection point, note that the function assumes
	// the lines intersect. the function can handle vertical as well
	// as horizontal lines. note the function isn't very clever, it simply applies
	// the math, but we don't need speed since this is a pre-processing step
	// we could have used parametric lines if we wanted, but this is more straight
	// forward for this use, I want the intersection of 2 infinite lines
	// rather than line segments as the parametric system defines

	float a1,b1,c1, // constants of linear equations
		  a2,b2,c2,
		  det_inv,  // the inverse of the determinant of the coefficient matrix
		  m1,m2;    // the slopes of each line

	// compute slopes, note the cludge for infinity, however, this will
	// be close enough
	if ((x1-x0)!=0)
	   m1 = (y1-y0) / (x1-x0);
	else
	   m1 = (float)1.0E+20;   // close enough to infinity

	if ((x3-x2)!=0)
	   m2 = (y3-y2) / (x3-x2);
	else
	   m2 = (float)1.0E+20;   // close enough to infinity

	// compute constants 
	a1 = m1;
	a2 = m2;

	b1 = -1;
	b2 = -1;

	c1 = (y0-m1*x0);
	c2 = (y2-m2*x2);

	// compute the inverse of the determinate
	det_inv = 1 / (a1*b2 - a2*b1);

	// use cramers rule to compute xi and yi
	*xi=((b1*c2 - b2*c1)*det_inv);
	*yi=((a2*c1 - a1*c2)*det_inv);
}

// this array is used to quickly compute the nearest power of 2 for a number from 0-256
// and always rounds down
// basically it's the log (b2) of n, but returns an integer
// logbase2ofx[x] = (int)log2 x, [x:0-512]
static unsigned char logbase2ofx[513] = 
{
	0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4, 
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5, 
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6, 
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6, 
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,

	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,

}; 

inline void BuildInverseCosTable(float *invcos,     // storage for table
								 int   range_scale) // range for table to span
{
	// this function builds an inverse cosine table used to help with
	// dot product calculations where the angle is needed rather than 
	// the value -1 to 1, the function maps the values [-1, 1] to
	// [0, range] and then breaks that interval up into n intervals
	// where the interval size is 180/range then the function computes
	// the arccos for each value -1 to 1 scaled and mapped as 
	// referred to above and places the values in the table
	// for example if the range is 360 then the interval will be
	// [0, 360] therefore, since the inverse cosine must
	// always be from 0 - 180, we are going to map 0 - 180 to 0 - 360
	// or in other words, the array elements will each represent .5
	// degree increments
	// the table must be large enough to hold the results which will
	// be = (range_scale+1) * sizeof(float)

	// to use the table, the user must scale the value of [-1, 1] to the 
	// exact table size, for example say you made a table with 0..180 
	// the to access it with values from x = [-1, 1] would be:
	// invcos[ (x+1)*180 ], the result would be the angle in degrees
	// to a 1.0 degree accuracy.


	float val = -1; // starting value

	// create table
	int index = 0;
	for (; index <= range_scale; index++)
	{
		// insert next element in table in degrees
		val = (val > 1) ? 1 : val;
		invcos[index] = RAD_TO_DEG(acos(val));
		//Write_Error("\ninvcos[%d] = %f, val = %f", index, invcos[index], val);

		// increment val by interval
		val += ((float)1/(float)(range_scale/2));

	} // end for index

	// insert one more element for padding, so that durring access if there is
	// an overflow of 1, we won't go out of bounds and we can save the clamp in the
	// floating point logic
	invcos[index] = invcos[index-1];
}

}