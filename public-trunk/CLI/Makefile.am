#
# Copyright 2009 Free Software Foundation, Inc.
#
# This software is distributed under the terms of the GNU Affero Public License.
# See the COPYING file in the main directory for details.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

include $(top_srcdir)/Makefile.common

EXTRA_DIST = \
	README.CLI 

AM_CXXFLAGS = -Wall
AM_CPPFLAGS = $(STD_DEFINES_AND_INCLUDES)

noinst_LTLIBRARIES = libcli.la libcli_commands.la

noinst_PROGRAMS = \
	TokenizerTest

libcli_la_SOURCES = \
	CLI.cpp \
	CLIServer.cpp \
	CLIClient.cpp \
	CLIConnection.cpp \
	Tokenizer.cpp

libcli_commands_la_SOURCES = \
	CLIParser.cpp

TokenizerTest_SOURCES = \
	TokenizerTest.cpp
TokenizerTest_LDADD = \
	$(CLI_LA)

noinst_HEADERS = \
	CLI.h \
	CLIServer.h \
	CLIClient.h \
	CLIConnection.h \
	CLIParser.h \
	CLIParserBase.h \
	Tokenizer.h
