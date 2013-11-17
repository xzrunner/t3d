#pragma once

namespace t3d {

class BmpImg;

int GenerateMipmaps(const BmpImg& source,	// source bitmap for mipmap
					BmpImg** mipmaps,		// pointer to array to store mipmap chain
					float gamma = 1.01);	// gamma correction factor

int DeleteMipmaps(BmpImg** mipmaps, bool leave_level_0);

}