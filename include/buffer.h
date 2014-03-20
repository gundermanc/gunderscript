/**
 * buffer.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See buffer.c for description.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BUFFER__H__
#define BUFFER__H__
#include <stdlib.h>
#include <string.h>
#include "gsbool.h"

typedef struct {
  char * buffer;
  int index;
  size_t blockSize;
  size_t currentSize;
} Buffer;
  
Buffer * buffer_new(size_t initialSize, size_t blockSize);

bool buffer_append_char(Buffer * buffer, char c);

bool buffer_append_string(Buffer * buffer, char * input, size_t inputLen);

bool buffer_set_char(Buffer * buffer, char c, int index);

char * buffer_get_buffer(Buffer * buffer);

bool buffer_set_string(Buffer * buffer, char * input,
		       size_t inputLen, int index);

void buffer_free(Buffer * buffer);


#endif /* BUFFER__H__ */
