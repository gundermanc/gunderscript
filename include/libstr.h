/**
 * libstr.h
 * (C) 2014 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See description in libstr.c.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBSTR__H__
#define LIBSTR__H__

#include "gunderscript.h"

#define LIBSTR_STRING_TYPE     "LIBSTR.STR"
#define LIBSTR_STRING_TYPE_LEN    10
#define LIBSTR_STRING_BLOCKSIZE   10

VMLibData * libstr_string_new(int bufferLen);

char * libstr_string(VMLibData * data);

int libstr_string_length(VMLibData * data);

bool libstr_install(Gunderscript * gunderscript);

bool libstr_string_append(VMLibData * data, char * string, int stringLen);

#endif /*LIBSTR__H__*/
