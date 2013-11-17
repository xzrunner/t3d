#define DEBUG_ON

#define INITGUID

#define WIN32_LEAN_AND_MEAN  

#include <conio.h>
#include <stdlib.h>

#include "Parser.h"

void main()
{
	t3d::Parser parser;     // create a parser
	char filename[80];    // holds fileaname
	char pattern[80];     // holds pattern
	int sel;              // user input

	// enter main loop
	while(1)
	{
		// display menu
		printf("\nParser Menu:\n");
		printf("\n1. Enter in a filename to load.");
		printf("\n2. Display file untouched.");     
		printf("\n3. Display file with all comments removed (comments begin with #).");
		printf("\n4. Display file with all blank lines and comments removed.");
		printf("\n5. Search file for pattern.");
		printf("\n6. Exit program.\n");
		printf("\nPlease make a selection?");

		// get user selection
		scanf("%d", &sel);

		// what to do?
		switch(sel)
		{
		case 1: // enter filename
			{
				printf("\nEnter filename?");
				scanf("%s", filename);
			} break;

		case 2: // display file untouched
			{
				// open the file
				if (!parser.Open(filename))
				{ printf("\nFile not found!"); break; }

				int line = 0;

				// loop until no more lines
				while(parser.Getline(0))
					printf("line %d: %s", line++, parser.buffer);                                     

				parser.Close();

			} break;

		case 3: // display file with comments removed
			{
				// open the file
				if (!parser.Open(filename))
				{ printf("\nFile not found!"); break; }

				int line = 0;

				// loop until no more lines
				while(parser.Getline(PARSER_STRIP_COMMENTS))
					printf("line %d: %s", line++, parser.buffer);                                     

				parser.Close();
			} break;

		case 4: // Display file with all blank lines and comments removed
			{
				// open the file
				if (!parser.Open(filename))
				{ printf("\nFile not found!"); break; }

				int line = 0;

				// loop until no more lines
				while(parser.Getline(PARSER_STRIP_COMMENTS | PARSER_STRIP_EMPTY_LINES))
					printf("line %d: %s", line++, parser.buffer);                                     

				parser.Close();
			} break;

		case 5: // Search file for pattern
			{
				printf("\nEnter pattern to match?");
				pattern[0] = 80;
				_cgets(pattern);

				// open the file
				if (!parser.Open(filename))
				{ printf("\nFile not found!"); break; }

				// loop until no more lines
				while(parser.Getline(PARSER_STRIP_COMMENTS | PARSER_STRIP_EMPTY_LINES))
				{
					// does this line have the pattern?
					if (parser.Pattern_Match(parser.buffer, &pattern[2]))
					{
						printf("\nPattern: \"%s\" matched, stats:", &pattern[2]); 

						printf("\n%d ints matched", parser.num_pints);
						for (int i=0; i < parser.num_pints; i++)
							printf("\nInt[%d]: %i", i, parser.pints[i]);

						printf("\n%d floats matched", parser.num_pfloats);
						for (int f=0; f < parser.num_pfloats; f++)
							printf("\nFloat[%d]: %f", f, parser.pfloats[f]);

						printf("\n%d strings matched", parser.num_pstrings);
						for (int s=0; s < parser.num_pstrings; s++)
							printf("\nString[%d]: %s", s, parser.pstrings[s]);

					} // end if

				} // end while

				parser.Close();
			} break;


		case 6: // exit program
			{
				exit(0);
			} break;

		} // end switch

	} // end while

} // end main