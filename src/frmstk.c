/**
 * frmstk.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * The frame stack is a data structure that stores the state of the current
 * logical block. Each time a function call is made, or a logical block is
 * entered (if, while, else, for, etc.) a new frame is pushed to the frame
 * stack. Each frame contains a frame header that is a set size and stores
 * the block return address, number of bytes in this frame for variables,
 * and below the header in the stack, a variable number of bytes for storing
 * localized variables. It is the responsibility of the calling methods to
 * manage the size of the data that they copy in and out of the framestack.
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

#include <string.h>
#include "frmstk.h"

FrmStk * frmstk_new(size_t stackSize) {
  FrmStk * fs = calloc(1, sizeof(FrmStk));

  if(fs != NULL) {

    fs->buffer = calloc(1, stackSize);

    if(fs->buffer != NULL) {
      fs->stackSize = stackSize;
      return fs;
    } else {
      free(fs);
    }
  }
  return NULL;
}

bool frmstk_push(FrmStk * fs, size_t returnAddr, size_t varBytes) {

  /* if there is enough memory left to place the new frame, do it */
  size_t frameSize = varBytes + sizeof(FrameHeader);
  if(fs->stackSize >= (fs->usedStack + frameSize)) {
    FrameHeader * header = fs->buffer + fs->usedStack + varBytes;
    header->returnAddr = returnAddr;
    header->varBytes = varBytes;

    printf("FrameSize: %i", frameSize);

    /* clear mem before header to be used for local variables */
    memset(fs->buffer + fs->usedStack, 0, varBytes);

    fs->usedStack += frameSize;
    return true;
  }

  return false;
}

bool frmstk_pop(FrmStk * fs) {
  if(fs->usedStack > 0) {
    FrameHeader * header = fs->buffer + fs->usedStack - sizeof(FrameHeader);
    fs->usedStack -= (header->varBytes + sizeof(FrameHeader));
    return true;
  }

  return false;
}

void * frmstk_var_addr(FrmStk * fs, int stackDepth, size_t argAddr) {
  size_t headerPos = fs->usedStack - sizeof(FrameHeader);
  int depth = 0;

  /* find proper frame */
  while(depth < stackDepth) {
    FrameHeader * header = fs->buffer + headerPos;
    if(headerPos < 0) {
      return NULL;
    }
    headerPos -= header->varBytes;
  }

  /* get frame's variable */
  headerPos -= argAddr;
  if(headerPos >= 0) {
    return fs->buffer + headerPos;
  }

  return NULL;
}

size_t frmstk_ret_addr(FrmStk * fs) {
  if(fs->usedStack > 0) {
    FrameHeader * header = fs->buffer + fs->usedStack - sizeof(FrameHeader);
    return header->returnAddr;
  }

  return 0;
}

void frmstk_free(FrmStk * fs) {
  free(fs->buffer);
  free(fs);
}
