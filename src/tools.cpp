#include "tools.h"

#include <string.h>

namespace t3d {

char* ExtractFilenameFromPath(char *filepath, char *filename)
{
	// this function extracts the filename from a complete path and file
	// "../folder/.../filname.ext"
	// the function operates by scanning backward and looking for the first 
	// occurance of "\" or "/" then copies the filename from there to the end
	// test of filepath is valid
	if (!filepath || strlen(filepath)==0) 
		return(NULL);

	int index_end = strlen(filepath)-1;

	// find filename
	while( (filepath[index_end]!='\\') && 
		(filepath[index_end]!='/') && 
		(filepath[index_end] > 0) ) 
		index_end--; 

	// copy file name out into filename var
	memcpy(filename, &filepath[index_end+1], strlen(filepath) - index_end);

	// return result
	return(filename);

} // end Extract_Filename_From_Path

}