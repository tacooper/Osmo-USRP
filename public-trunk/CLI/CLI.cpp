/*
* Copyright 2009, 2010 Free Software Foundation, Inc.
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

#include <errno.h>
#include <string.h>

#include <config.h>
#include "CLI.h"
#include <Logger.h>
#include <Globals.h>

#ifdef HAVE_LIBREADLINE // [
//#  include <stdio.h>
#  include <readline/readline.h>
#  include <readline/history.h>
#endif // HAVE_LIBREADLINE ]

using namespace std;
using namespace CommandLine;

void CommandLine::runCLI(ParserBase *processor)
{
#ifdef HAVE_LIBREADLINE // [
	// start console
	using_history();

	static const char * const history_file_name = "/.openbts_history";
	char *history_name = 0;
	char *home_dir = getenv("HOME");

	if(home_dir) {
		size_t home_dir_len = strlen(home_dir);
		size_t history_file_len = strlen(history_file_name);
		size_t history_len = home_dir_len + history_file_len + 1;
		if(history_len > home_dir_len) {
			if(!(history_name = (char *)malloc(history_len))) {
				LOG(ERROR) << "malloc failed: " << strerror(errno);
				exit(-1);
			}
			memcpy(history_name, home_dir, home_dir_len);
			memcpy(history_name + home_dir_len, history_file_name,
			   history_file_len + 1);
			read_history(history_name);
		}
	}
#endif // HAVE_LIBREADLINE ]

	COUT("\n\nWelcome to OpenBTS.  Type \"help\" to see available commands.");
   // FIXME: We want to catch control-d (emacs keybinding for exit())

	// The logging parts were removed from this loop.
	// If we want them back, they will need to go into their own thread.
	while (1) {
#ifdef HAVE_LIBREADLINE // [
		char *inbuf = readline(gConfig.getStr("CLI.Prompt"));
		if (!inbuf) break;
		if (*inbuf) {
			add_history(inbuf);
			// The parser returns -1 on exit.
			if (processor->process(inbuf, cout)<0) {
				free(inbuf);
				break;
			}
		}
		free(inbuf);
#else // HAVE_LIBREADLINE ][
		cout << endl << gConfig.getStr("CLI.Prompt");
		cout.flush();
		std::string inbuf;
		getline(cin, inbuf, '\n');
		if (!cin.good()) break;
		// The parser returns -1 on exit.
		if (processor->process(inbuf,cout)<0) break;
#endif // !HAVE_LIBREADLINE ]
	}

#ifdef HAVE_LIBREADLINE // [
	if(history_name) {
		int e = write_history(history_name);
		if(e) {
			fprintf(stderr, "error: history: %s\n", strerror(e));
		}
		free(history_name);
		history_name = 0;
	}
#endif // HAVE_LIBREADLINE ]
}

// vim: ts=4 sw=4
