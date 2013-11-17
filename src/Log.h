#pragma once

#include <stdio.h>

#include "Matrix.h"

namespace t3d {

class Log
{
public:
	Log() : fp_error(NULL) {}

	int Init();
	void Shutdown();

	int WriteError(char *string, ...);
	int WriteError(const mat4& mt, const char* name = "M");

private:
	int OpenErrorFile(char *filename, FILE *fp_override=NULL);
	int CloseErrorFile();

private:
	FILE* fp_error; // general error file

}; // Log

}