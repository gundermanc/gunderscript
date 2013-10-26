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
CFLAGS  = -g -Wall -I include
LIBCFLAGS = $(CFLAGS) -o $(OBJDIR)/$@
SRCDIR = src
DOCSDIR = docs
DATASTRUCTSDIR = c-datastructs


all: c-datastructs-build buildfs

# builds everything!!
all: testapp

# builds the testing application
testapp: library
	$(CC) $(CFLAGS) -o testapp test_app.c gunderscript.a

# build just the static library
library: lexer.o
	$(AR) $(ARFLAGS) gunderscript.a $(OBJDIR)/lexer.o

# build lexer object
lexer.o: c-datastructs-build  $(SRCDIR)/lexer.c
	$(CC) $(LIBCFLAGS) -c $(SRCDIR)/lexer.c

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
	$(RM) lib.a testapp $(SRCDIR)/*~ $(INCDIR)/*~ $(DOCSDIR)/*~ *~
	$(RM) -rf objs
