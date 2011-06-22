/*
* Copyright 2010 Free Software Foundation, Inc.
*
* This software is distributed under the terms of the GNU Affero Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <string.h>
#include "Tokenizer.h"


#include <stdio.h> // For debug only

using namespace CommandLine;

int CommandLine::tokenize(char *line, char **argv, int maxArgs)
{
	char *lineData = line;
	int argc;
	const char *tokenEnd = " ";

	// Find the beginning of the first token
	lineData += strspn(lineData, " ");
	// Check if this is a start of quoting
	if (*lineData == '"') {
		tokenEnd = "\"";
		lineData++;
	} else {
		tokenEnd = " ";
	}
	
	for (argc = 0; argc < maxArgs && *(argv[argc] = lineData) != '\0'; argc++) {
		// Find the end of the token
		lineData = strpbrk(lineData, tokenEnd);
		if (lineData == NULL) {
			argc++;
			break;
		}
		*lineData = '\0';

		// Find the beginning of the next token
		lineData++;
		lineData += strspn(lineData, " ");
		if (*lineData == '\0') {
			argc++;
			break;
		}

		// Check if this is a start of quoting
		if (*lineData == '"') {
			tokenEnd = "\"";
			lineData++;
		} else {
			tokenEnd = " ";
		}
	}

	return argc;
}

// vim: ts=4 sw=4
