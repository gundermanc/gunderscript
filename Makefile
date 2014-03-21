#
# Gunderscript Makefile
# (C) 2013 Christian Gunderman
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program.  If not, see
# <http://www.gnu.org/licenses/>.
#
# Contact Email: gundermanc@gmail.com
#
CC = gcc
AR = ar
ARFLAGS = rcs
INCDIR = include
OBJDIR = objs
DATASTRUCTSDIR = c-datastructs
CFLAGS  = -g -std=c89 -Wall -I $(INCDIR) -I $(DATASTRUCTSDIR)/include
LIBCFLAGS = $(CFLAGS) -o $(OBJDIR)/$@
SRCDIR = src
DOCSDIR = docs


all: c-datastructs-build buildfs

# builds everything!!
all: testapp

# builds the testing application
linuxapp: linuxlibrary
	$(CC) $(CFLAGS) -o gunderscript main.c gunderscript.a $(DATASTRUCTSDIR)/lib.a

# build just the static library
linuxlibrary: gunderscript.o lexer.o frmstk.o vm.o compiler.o
	$(AR) $(ARFLAGS) gunderscript.a $(OBJDIR)/lexer.o $(OBJDIR)/ophandlers.o $(OBJDIR)/frmstk.o $(OBJDIR)/vm.o $(OBJDIR)/typestk.o $(OBJDIR)/parsers.o $(OBJDIR)/compiler.o $(OBJDIR)/compcommon.o $(OBJDIR)/gunderscript.o $(OBJDIR)/buffer.o

# build lexer object
lexer.o: buildfs $(SRCDIR)/lexer.c
	$(CC) $(LIBCFLAGS) -c $(SRCDIR)/lexer.c

# build typestk object
typestk.o: buildfs $(SRCDIR)/typestk.c
	$(CC) $(LIBCFLAGS) -c $(SRCDIR)/typestk.c

# build Gunderscript object
gunderscript.o: buildfs vm.o compiler.o $(SRCDIR)/gunderscript.c
	$(CC) $(LIBCFLAGS) -c $(SRCDIR)/gunderscript.c

# build vm object
vm.o: buildfs c-datastructs-build frmstk.o typestk.o ophandlers.o $(SRCDIR)/vm.c
	$(CC) $(LIBCFLAGS) -c $(SRCDIR)/vm.c

# build ophandlers object
ophandlers.o: buildfs c-datastructs-build $(SRCDIR)/ophandlers.c
	$(CC) $(LIBCFLAGS) -c $(SRCDIR)/ophandlers.c

# build compcommon object
compcommon.o: buildfs $(SRCDIR)/compcommon.c
	$(CC) $(LIBCFLAGS) -c $(SRCDIR)/compcommon.c

# build parsers object
parsers.o: buildfs compcommon.o $(SRCDIR)/parsers.c
	$(CC) $(LIBCFLAGS) -c $(SRCDIR)/parsers.c

# build compiler object
compiler.o: buildfs c-datastructs-build buffer.o compcommon.o lexer.o parsers.o $(SRCDIR)/compiler.c
	$(CC) $(LIBCFLAGS) -c $(SRCDIR)/compiler.c

# build buffer object
buffer.o: buildfs $(SRCDIR)/buffer.c
	$(CC) $(LIBCFLAGS) -c $(SRCDIR)/buffer.c

# build framestack object
frmstk.o: buildfs c-datastructs-build $(SRCDIR)/frmstk.c
	$(CC) $(LIBCFLAGS) -c $(SRCDIR)/frmstk.c

# build the file system
buildfs:
	mkdir -p $(OBJDIR)

# build c-datastructs module
c-datastructs-build:
	$(MAKE) -C $(DATASTRUCTSDIR)

c-datastructs-clean:
	$(MAKE) -C $(DATASTRUCTSDIR) clean

# remove all binaries and annoying Emacs Backups
clean: c-datastructs-clean
	$(RM) gunderscript.a testapp $(SRCDIR)/*~ $(INCDIR)/*~ $(DOCSDIR)/*~ *~
	$(RM) -rf objs
