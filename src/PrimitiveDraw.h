#pragma once

#include <ddraw.h>

namespace t3d {

class PolygonF;

extern int DrawClipLine(int x0,int y0, int x1, int y1, int color,unsigned char* dest_buffer, int lpitch);
extern int DrawClipLine16(int x0,int y0, int x1, int y1, int color,unsigned char* dest_buffer, int lpitch);
extern int DrawClipLine32(int x0,int y0, int x1, int y1, int color,unsigned char* dest_buffer, int lpitch);
extern int ClipLine(int &x1,int &y1,int &x2, int &y2);
extern int DrawLine(int xo, int yo, int x1,int y1, int color,unsigned char* vb_start,int lpitch);
extern int DrawLine16(int xo, int yo, int x1,int y1, int color,unsigned char* vb_start,int lpitch);
extern int DrawLine32(int xo, int yo, int x1,int y1, int color,unsigned char* vb_start,int lpitch);
extern int DrawPixel(int x, int y,int color,unsigned char* video_buffer, int lpitch);
extern int DrawPixel16(int x, int y,int color,unsigned char* video_buffer, int lpitch);
extern int DrawPixel32(int x, int y,int color,unsigned char* video_buffer, int lpitch);
extern int DrawRectangle(int x1, int y1, int x2, int y2, int color,LPDIRECTDRAWSURFACE7 lpdds);

extern void DrawTriangle32(float x1,float y1,float x2,float y2,float x3,float y3, int color,unsigned char *dest_buffer, int mempitch);
extern void DrawTriangle32(PolygonF* face,unsigned char *dest_buffer, int mempitch);
extern void DrawTriangleAlpha32(PolygonF* face,unsigned char* _dest_buffer, int mem_pitch, int alpha);
extern void DrawTriangleZB32(PolygonF* face,unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch);
extern void DrawTriangleWTZB32(PolygonF* face,unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch);
extern void DrawTriangleZBAlpha32(PolygonF* face,unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch, int alpha);
extern void DrawTriangleINVZB32(PolygonF* face,unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch);
extern void DrawTriangleINVZBAlpha32(PolygonF* face,unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch, int alpha);
extern void DrawGouraudTriangle32(PolygonF* face, unsigned char *dest_buffer, int mempitch);
extern void DrawGouraudTriangleAlpha32(PolygonF* face, unsigned char *dest_buffer, int mempitch, int alpha);
extern void DrawGouraudTriangleZB32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch);
extern void DrawGouraudTriangleWTZB32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch);
extern void DrawGouraudTriangleZBAlpha32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, unsigned char* _zbuffer, int zpitch, int alpha);
extern void DrawGouraudTriangleINVZB32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch);
extern void DrawGouraudTriangleINVZBAlpha32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch, int alpha);
extern void DrawTexturedTriangle32(PolygonF* face, unsigned char *dest_buffer, int mempitch); 
extern void DrawTexturedTriangleAlpha32(PolygonF* face, unsigned char *dest_buffer, int mempitch, int alpha); 
extern void DrawTexturedTriangleZB32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch);
extern void DrawTexturedTriangleWTZB32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch);
extern void DrawTexturedTriangleZBAlpha32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch, int alpha); 
extern void DrawTexturedTriangleINVZB32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch); 
extern void DrawTexturedTriangleINVZBAlpha32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch, int alpha); 
extern void DrawTexturedTriangleFS32(PolygonF* face, unsigned char *dest_buffer, int mempitch); 
extern void DrawTexturedTriangleFSAlpha32(PolygonF* face, unsigned char *dest_buffer, int mempitch, int alpha); 
extern void DrawTexturedTriangleFSZB32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch); 
extern void DrawTexturedTriangleFSWTZB32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch); 
extern void DrawTexturedTriangleFSZBAlpha32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch, int alpha); 
extern void DrawTexturedTriangleFSINVZB32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, unsigned char* _zbuffer, int zpitch);
extern void DrawTexturedTriangleFSINVZBAlpha32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, unsigned char* _zbuffer, int zpitch, int alpha);
extern void DrawTexturedTriangleGS32(PolygonF* face, unsigned char *dest_buffer, int mempitch); 
extern void DrawTexturedTriangleGSAlpha32(PolygonF* face, unsigned char *dest_buffer, int mempitch, int alpha); 
extern void DrawTexturedTriangleGSZB32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch); 
extern void DrawTexturedTriangleGSWTZB32(PolygonF* face, unsigned char *dest_buffer, int mempitch, unsigned char* zbuffer, int zpitch); 
extern void DrawTexturedTriangleGSZBAlpha32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, unsigned char* _zbuffer, int zpitch, int alpha);
extern void DrawTexturedTriangleGSINVZB32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, unsigned char* _zbuffer, int zpitch);
extern void DrawTexturedTriangleGSINVZBAlpha32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, unsigned char* _zbuffer, int zpitch, int alpha);
extern void DrawTexturedBilerpTriangle32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch);
extern void DrawTexturedBilerpTriangleZB32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, unsigned char* _zbuffer, int zpitch);
extern void DrawTexturedBilerpTriangleINVZB32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, unsigned char* _zbuffer, int zpitch);
extern void DrawTexturedPerspectiveTriangleINVZB32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, unsigned char* _zbuffer, int zpitch);
extern void DrawTexturedPerspectiveLPTriangleINVZB32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, unsigned char* _zbuffer, int zpitch);
extern void DrawTexturedPerspectiveTriangleFSINVZB32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, unsigned char* _zbuffer, int zpitch);
extern void DrawTexturedPerspectiveLPTriangleFSINVZB32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, unsigned char* _zbuffer, int zpitch);
}