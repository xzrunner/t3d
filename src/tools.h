#pragma once

namespace t3d {

char* ExtractFilenameFromPath(char *filepath, char *filename);

inline void Mem_Set_QUAD(void *dest, unsigned int data, int count)
{
	// this function fills or sets unsigned 32-bit aligned memory
	// count is number of quads

	_asm 
	{ 
		mov edi, dest   ; edi points to destination memory
		mov ecx, count  ; number of 32-bit words to move
		mov eax, data   ; 32-bit data
		rep stosd       ; move data
	} // end asm

} // end Mem_Set_QUAD

}