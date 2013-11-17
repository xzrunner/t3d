#include "Log.h"

#include <stdarg.h>
#include <ctime>
#include <sys/timeb.h>

namespace t3d {

int Log::Init()
{
	return OpenErrorFile("ERROR.TXT");
}

void Log::Shutdown()
{
	CloseErrorFile();
}

int Log::WriteError(char *string, ...)
{
	// this function prints out the error string to the error file

	char buffer[1024]; // working buffer

	va_list arglist; // variable argument list

	// make sure both the error file and string are valid
	if (!string || !fp_error)
		return(0);

	// print out the string using the variable number of arguments on stack
	va_start(arglist,string);
	vsprintf(buffer,string,arglist);
	va_end(arglist);

	// write string to file
	fprintf(fp_error,buffer);

	// flush buffer incase the system bails
	fflush(fp_error);

	return(1);
}

int Log::WriteError(const mat4& mt, const char* name)
{
	// prints out a 4x4 matrix
	WriteError("\n%s=\n",name);
	for (int r=0; r < 4; r++, WriteError("\n"))
		for (int c=0; c < 4; c++)
			WriteError("%f ",mt.c[r][c]);  

	return(1);
}

int Log::OpenErrorFile(char *filename, FILE *fp_override)
{
	// this function creates the output error file

	// is user requesting special file handle? stdout, stderr, etc.?
	if (fp_override)
	{
		fp_error = fp_override;
	}
	else
	{
		// test if this file is valid
		if ((fp_error = fopen(filename,"w"))==NULL)
			return(0);
	}

	// get the current time
	struct _timeb timebuffer;
	char *timeline;
	char timestring[280];

	_ftime(&timebuffer);
	timeline = ctime(&(timebuffer.time));

	sprintf(timestring, "%.19s.%hu, %s", timeline, timebuffer.millitm, &timeline[20]);

	// write out error header with time
	WriteError("\nOpening Error Output File (%s) on %s\n",filename,timestring);

	// now the file is created, re-open with append mode

	if (!fp_override)
	{
		fclose(fp_error);
		if ((fp_error = fopen(filename,"a+"))==NULL)
			return(0);
	}

	return(1);
}

int Log::CloseErrorFile()
{
	if (fp_error)
	{
		// write close file string
		WriteError("\nClosing Error Output File.");

		if (fp_error!=stdout || fp_error!=stderr)
		{
			// close the file handle
			fclose(fp_error);
		} 

		fp_error = NULL;

		return(1);
	}
	else
		return(0);
}

}