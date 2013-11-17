#pragma once

// these are some defines for conditional compilation of the new rasterizers
// I don't want 80 million different functions, so I have decided to 
// use some conditionals to change some of the logic in each
// these names aren't necessarily the most accurate, but 3 should be enough
#define RASTERIZER_ACCURATE    0 // sub-pixel accurate with fill convention
#define RASTERIZER_FAST        1 // 
#define RASTERIZER_FASTEST     2

// set this to the mode you want the engine to use
#define RASTERIZER_MODE        RASTERIZER_ACCURATE

// fixed point mathematics constants
#define FIXP16_SHIFT     16
//  #define FIXP16_MAG       65536
//  #define FIXP16_DP_MASK   0x0000ffff
//  #define FIXP16_WP_MASK   0xffff0000
#define FIXP16_ROUND_UP  0x00008000

// perspective correct/ 1/z buffer defines
#define FIXP28_SHIFT                  28  // used for 1/z buffering
#define FIXP22_SHIFT                  22  // used for u/z, v/z perspective texture mapping

// defines for texture mapper triangular analysis
#define TRI_TYPE_NONE           0
#define TRI_TYPE_FLAT_TOP       1 
#define TRI_TYPE_FLAT_BOTTOM	2
#define TRI_TYPE_FLAT_MASK      3
#define TRI_TYPE_GENERAL        4
#define INTERP_LHS              0
#define INTERP_RHS              1
#define MAX_VERTICES_PER_POLY   6

// transformation control flags
#define TRANSFORM_LOCAL_ONLY       0  // perform the transformation in place on the
                                      // local/world vertex list 
#define TRANSFORM_TRANS_ONLY       1  // perfrom the transformation in place on the 
                                      // "transformed" vertex list

#define TRANSFORM_LOCAL_TO_TRANS   2  // perform the transformation to the local
                                      // vertex list, but store the results in the
                                      // transformed vertex list
#define TRANSFORM_COPY_LOCAL_TO_TRANS   3 // copy data as is, no transform

// defines for camera rotation sequences
#define CAM_ROT_SEQ_XYZ  0
#define CAM_ROT_SEQ_YXZ  1
#define CAM_ROT_SEQ_XZY  2
#define CAM_ROT_SEQ_YZX  3
#define CAM_ROT_SEQ_ZYX  4
#define CAM_ROT_SEQ_ZXY  5

// defines for special types of camera projections
#define CAM_PROJ_NORMALIZED        0x0001
#define CAM_PROJ_SCREEN            0x0002
#define CAM_PROJ_FOV90             0x0004

#define CAM_MODEL_EULER            0x0008
#define CAM_MODEL_UVN              0x0010

#define UVN_MODE_SIMPLE            0 
#define UVN_MODE_SPHERICAL         1

// bit manipulation macros
#define SET_BIT(word,bit_flag)   ((word)=((word) | (bit_flag)))
#define RESET_BIT(word,bit_flag) ((word)=((word) & (~bit_flag)))

// flags for sorting algorithm
#define SORT_POLYLIST_AVGZ  0  // sorts on average of all vertices
#define SORT_POLYLIST_NEARZ 1  // sorts on closest z vertex of each poly
#define SORT_POLYLIST_FARZ  2  // sorts on farthest z vertex of each poly

// alpha blending defines
#define NUM_ALPHA_LEVELS              8   // total number of alpha levels