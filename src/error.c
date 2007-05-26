/* vim: set noet:
 *
 * OpenTyrian Classic: A modern cross-platform port of Tyrian
 * Copyright (C) 2007  The OpenTyrian Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "opentyr.h"
#include "vga256d.h"

#define NO_EXTERNS
#include "error.h"
#undef NO_EXTERNS

#include <stdio.h>
#include <errno.h>

JE_word randomcount;
JE_char dir[12];

JE_boolean errorActive = TRUE;
JE_boolean errorOccurred = FALSE;

JE_boolean dont_die = FALSE;

char err_msg[128] = "No error?!?!";

static const char *tyrian_searchpaths[] = { "data", "tyrian", "tyrian2k" };

long get_stream_size( FILE *f )
{
	long size = 0;
	long pos;

	pos = ftell(f);

	fseek(f, 0, SEEK_END);
	size = ftell(f);

	fseek(f, pos, SEEK_SET);

	return size;
}

FILE *fopen_check( const char *file, const char *mode )
{
	char buf[64];
	FILE *f;

	f = fopen(file, mode);
	if (!f)
	{
		switch (errno)
		{
			case EACCES:
				strcpy(buf, "Access denied");
				break;
			default:
				strcpy(buf, "Unknown error");
				break;
		}
		snprintf(err_msg, sizeof err_msg, "ERROR opening %s: %s\n", file, buf);
		printf("%s", err_msg);
		return NULL;
	}

	return f;
}


JE_longint JE_getFileSize( const char *filename )
{
	FILE *f;
	JE_longint size = 0;

	errorActive = FALSE;
	f = fopen(JE_locateFile(filename), "rb");
	errorActive = TRUE;
	if (errorOccurred)
	{
		errorOccurred = FALSE;
		return 0;
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);

	fclose(f);

	return size;
}

void JE_errorHand( const char *s )
{
	if (errorActive)
	{
		JE_closeVGA256();
		printf("WARNING: Unable to find Tyrian data files.\n"
		       "Stopped on file %s.\n"
		       "OpenTyrian needs the Tyrian data files to run. Please read the README file.\n\n", s);
		exit(1);
	} else {
		errorOccurred = 1;
	}
}

JE_boolean JE_find( const char *s )
{
	FILE *f;

	if ((f = fopen(s, "r")))
	{
		fclose(f);
		return TRUE;
	} else {
		return FALSE;
	}
}

void JE_findTyrian( const char *filename )
{
	char *strbuf;

	if (JE_find(filename))
	{
		dir[0] = '\0';
	} else {
		unsigned int i;
		/* If you ever add a longer dir, increase the magic number. */
		size_t tmpsize = (strlen(filename)+10) * sizeof (*strbuf);

		/* Let's find it! */
		printf("Searching for Tyrian files...\n\n");

		strbuf = malloc(tmpsize);
		for (i = 0; i < COUNTOF(tyrian_searchpaths); i++)
		{
			sprintf(strbuf, "%s/%s", tyrian_searchpaths[i], filename);
			if (JE_find(strbuf))
			{
				free(strbuf);

				sprintf(dir, "%s/", tyrian_searchpaths[i]);
				printf("Tyrian data files found at %s\n\n", dir);
				return;
			}
		}
	}
}

char *JE_locateFile( const char *filename ) /* !!! WARNING: Non-reentrant !!! */
{
	static JE_char buf[1024];

	if (JE_find(filename))
	{
		strcpy(buf, filename);
	} else {
		if (strcmp(dir, "") == 0 && errorActive)
		{
			JE_findTyrian(filename);
		}

		snprintf(buf, sizeof buf, "%s%s", dir, filename);
		if (!JE_find(buf))
		{
			if (dont_die)
			{
				return NULL;
			}
			errorActive = TRUE;
			JE_errorHand(filename);
		}

	}
	
	return buf;
}

void JE_resetFile( FILE **f, const char *filename )
{
	char *tmp;

	tmp = JE_locateFile(filename);
	*f = tmp ? fopen_check(tmp, "rb") : NULL;
}

void JE_resetText( FILE **f, const char *filename )
{
	char *tmp;

	tmp = JE_locateFile(filename);
	*f = tmp ? fopen_check(tmp, "r") : NULL;
}

JE_boolean JE_isCFGThere( void ) /* Warning: It actually returns false when the config file exists */
{
	FILE *f;

	dont_die = TRUE; /* Disabled barf'n'die */
	JE_resetFile(&f, "TYRIAN.CFG");
	dont_die = FALSE;

	if (f && get_stream_size(f) == 28)
	{
		fclose(f);
		return FALSE;
	} else {
		printf("\nInvalid or missing TYRIAN.CFG! Continuing using defaults.\n");
		return TRUE;
	}
}

void JE_DetectCFG( void )
{
	STUB(JE_DetectCFG);
}

void JE_outputString( JE_char* s )
{
	STUB(JE_outputString);
}
