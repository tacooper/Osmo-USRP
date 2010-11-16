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


#ifndef OPENBTSCLIPARSER_H
#define OPENBTSCLIPARSER_H

#include <string>
#include <map>
#include <iostream>
#include <CLIParserBase.h>
#include <Threads.h>


namespace CommandLine {

typedef int (*CLICommandFunc)(int,char**,std::ostream&);

/** A table for matching strings to actions. */
typedef std::map<std::string,CLICommandFunc> ParseTable;

/** The help table. */
typedef std::map<std::string,std::string> HelpTable;

class Parser : public ParserBase {

public:

	Parser();

	/**
		Process a command line.
		@return 0 on success, -1 on exit request, error codes otherwise
	*/
	virtual int process(const std::string &line, std::ostream& os);

	/** Add a command to the parsing table. */
	void addCommand(const char* name, CLICommandFunc func, const char* helpString)
		{ mMutex.lock(); mParseTable[name] = func; mHelpTable[name]=helpString; mMutex.unlock(); }

	/** Print command list to ostream */
	void printCommands(std::ostream &os, int numCols=1) const;

	/** Return a help string. */
	const char *help(const std::string& cmd) const;

protected:

	mutable Mutex mMutex;    ///< Mutex to protect mParseTable and mHelpTable.
	ParseTable mParseTable;
	HelpTable mHelpTable;
	static const int mMaxArgs = 10;

	/** Parse and execute a command string. */
	int execute(std::string &line, std::ostream& os) const;

};

extern CommandLine::Parser gParser;

} 	// CLI


#endif
// vim: ts=4 sw=4
