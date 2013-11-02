/**
 * frmstk.h
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

#ifndef FRMSTK__H__
#define FRMSTK__H__

#include <stdlib.h>
#include "gsbool.h"

typedef struct FrameHeader {
  size_t returnAddr;
  int varBytes;
} FrameHeader;

typedef struct FrmStk {
  void * buffer;
  size_t usedStack;
  size_t stackSize;
} FrmStk;

FrmStk * frmstk_new(size_t stackSize);

void frmstk_free(FrmStk * fs);


bool frmstk_pop(FrmStk * fs);

bool frmstk_push(FrmStk * fs, size_t returnAddr, size_t varBytes);

void * frmstk_var_addr(FrmStk * fs, int stackDepth, size_t argAddr);

size_t frmstk_ret_addr(FrmStk * fs);

#endif /* FRMSTK__H__ */
