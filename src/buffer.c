/**
 * buffer.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * A dynamically expanding byte buffer that is useful for storing data of an
 * unknown size.
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
#include "buffer.h"
#include <assert.h>

/**
 * Creates a new buffer object.
 * initialSize: the initial size of the new buffer in chars.
 * blockSize: the number of bytes to add to the buffer each time it fills up and
 * needs to be expanded.
 * returns: a new buffer, or NULL if the malloc fails.
 */
Buffer * buffer_new(size_t initialSize, size_t blockSize) {

  assert(initialSize > 0);
  assert(blockSize > 0);

  /* allocate buffer struct */
  Buffer * buffer = (Buffer*)calloc(1, sizeof(Buffer));
  if(buffer == NULL) {
    return NULL;
  }

  /* allocate data for buffer */
  buffer->buffer = (void*) calloc(initialSize, sizeof(char));
  if(buffer->buffer == NULL) {
    free(buffer);
    return NULL;
  }

  buffer->blockSize = blockSize;
  buffer->currentSize = initialSize;

  return buffer;
}

/**
 * Resizes the buffer and retains whatever data will fit.
 * buffer: an instance of buffer.
 * newSize: the new size for the buffer in chars.
 * returns: true if the resize succeeds and false if it fails.
 */
static bool resize_buffer(Buffer * buffer, size_t newSize) {

  /* alloc memory and check for failure...buffer is one bigger so last char
   * can act as null terminator for string
   */
  void * newBuffer = calloc(newSize + 1, sizeof(char));
  if(newBuffer == NULL) {
    return false;
  }

  /* copy data from old buffer to new one */
  memcpy(newBuffer, buffer->buffer, buffer->index);

  /* free old buffer and replace with new one*/
  free(buffer->buffer);
  buffer->buffer = newBuffer;
  buffer->currentSize = newSize;

  return true;
}

/**
 * Appends a char to the end of the end-most character in the buffer.
 * buffer: an instance of buffer.
 * c: a char to append.
 */
bool buffer_append_char(Buffer * buffer, char c) {
  assert(buffer != NULL);
  buffer_set_char(buffer, c, buffer->index);
  return true;
}

/**
 * Returns the buffer.
 * returns: a null terminated character buffer. Get length with buffer_size().
 */
char * buffer_get_buffer(Buffer * buffer) {
  assert(buffer != NULL);
  return buffer->buffer;
}

/**
 * Appends a string to the buffer after the end-most character.
 * buffer: an instance of buffer.
 * input: the string to append.
 * inputLen: the length of the input string.
 * returns: true upon success, and false on malloc error.
 */
bool buffer_append_string(Buffer * buffer, char * input, size_t inputLen) {
  buffer_set_string(buffer, input, inputLen, buffer->index);
  return true;
}

/**
 * Sets a character at a particular index in the buffer.
 * c: the char to store.
 * index: the index to store the char at.
 * returns: true if success, false upon malloc error.
 */
bool buffer_set_char(Buffer * buffer, char c, int index) {
  
  /* if insertion is out of range of the current buffer, realloc */
  if(index >= buffer->currentSize) {
    if(!resize_buffer(buffer, buffer->currentSize + buffer->blockSize)) {
      return false;
    }
  }

  /* change end index, if neccessary */
  if(index >= buffer->index) {
    buffer->index = index + 1;
  }
  buffer->buffer[index] = c;
  return true;
}

/**
 * Sets the string, starting at the specified index.
 * buffer: an instance of buffer.
 * input: the string to store at index.
 * inputLen: the length of the string to store.
 * index: the index to start storing the string.
 * returns: true if success, false if malloc failure.
 */
bool buffer_set_string(Buffer * buffer, char * input,
		       size_t inputLen, int index) {
  int i = index;
  
  for(i = 0; i < inputLen; i++) {
    if(!buffer_set_char(buffer, input[i], index + i)) {
      return false;
    }
  }

  return true;
}

/**
 * Gets index of end-most character in buffer. Note: characters between
 * will be NULL characters if not otherwise assigned.
 * buffer: an instance of buffer.
 * returns: the index of the end-most character.
 */
int buffer_size(Buffer * buffer) {
  assert(buffer != NULL);
  return buffer->index;
}

/**
 * Frees an instance of buffer.
 */
void buffer_free(Buffer * buffer) {
  assert(buffer != NULL);

  free(buffer->buffer);
  free(buffer);
}
