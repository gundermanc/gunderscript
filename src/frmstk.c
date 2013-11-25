/**
 * frmstk.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * The frame stack is a data structure that stores the state of the current
 * logical block. Each time a function call is made or a logical block is
 * entered (if, while, else, for, etc.) a new frame is pushed to the frame
 * stack. Each frame contains a frame header that is a set size and stores
 * the block return address and number of variables/arguments in this frame.
 * Below the header is a buffer of bytes that is (64 bits) * (num. of varargs)
 * in size. Obviously, this implementation means that all variables require
 * 64 bits to be stored. This is because x86_64 pointers and doubles are both
 * 64 bits in length. Unfortunately, this means that Booleans also take up
 * 64 bits. We trade memory for constant time lookup.
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
#include <assert.h>
#include "frmstk.h"

/* number of bytes for each object */
static const size_t argSize = 8;

/**
 * Creates new instance of a frmstk with a preallocated buffer.
 * stackSize: Number of bytes in preallocated buffer.
 * returns: new FrmStk* object, or NULL if allocation fails.
 */
FrmStk * frmstk_new(size_t stackSize) {
  FrmStk * fs = calloc(1, sizeof(FrmStk));

  assert(stackSize > 0);

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

/**
 * Gets the number of free bytes in the frmstk's preallocated buffer
 * returns: free bytes.
 */
static size_t free_space(FrmStk * fs) {
  return fs->stackSize - fs->usedStack;
}

bool frmstk_push(FrmStk * fs, size_t returnAddr, int numVarArgs) {
  size_t varArgsSize = (argSize * numVarArgs);
  size_t newFrameSize = sizeof(FrameHeader) + varArgsSize;

  assert(fs != NULL);
  assert(returnAddr > 0);
  assert(numVarArgs >= 0);

  /* if there is enough free space, create the frame */
  if(free_space(fs) >= newFrameSize
     && numVarArgs >= 0) {
    FrameHeader * header = fs->buffer + fs->usedStack + varArgsSize;
    memset(fs->buffer + fs->usedStack, 0, varArgsSize);

    header->returnAddr = returnAddr;
    header->numVarArgs = numVarArgs;

    fs->usedStack += newFrameSize;
    fs->stackDepth++;
    
    return true;
  }

  return false;
}

bool frmstk_pop(FrmStk * fs) {
  assert(fs != NULL);

  if(fs->usedStack > 0) {
    FrameHeader * header = fs->buffer + fs->usedStack - sizeof(FrameHeader);
    size_t frameSize = sizeof(FrameHeader) + (header->numVarArgs * argSize);

    fs->usedStack -= frameSize;
    fs->stackDepth--;
    return true;
  }

  return false;
}

void * frmstk_var_addr(FrmStk * fs, int stackDepth, int varArgsIndex) {

  assert(fs != NULL);
  assert(stackDepth >= 0);
  assert(varArgsIndex >= 0);

  /* check stack goes deep enough */
  if(fs->stackDepth >= 1 && stackDepth < fs->stackDepth) {
    /*
     * Allow me to RANT a moment:
     * C standards don't allow pointer arithmetic upon void* pointers because
     * they have no "object" property, and therefore, no size. Because of this
     * we have to typecast to a buffer of unsigned chars (bytes) and do our op
     * in terms of bytes.
     */
    /* TODO: Do this another way, if possible! */
    int i;
    unsigned char * buffer = fs->buffer + fs->usedStack - sizeof(FrameHeader);

    /* iterate to the requested frame in the stack */
    for(i = 0; i < stackDepth; i++) {
      buffer -= (sizeof(FrameHeader) + (((FrameHeader*)buffer)->numVarArgs
					* argSize));
    }
 
    /* return the pointer to the requested argument in the frame */
    if(varArgsIndex < ((FrameHeader*)buffer)->numVarArgs) {
      return (void*)(buffer - (argSize * varArgsIndex));
    }
  }

  return NULL;
}

bool frmstk_var_write(FrmStk * fs, int stackDepth, int varArgsIndex,
		      void * value, size_t valueSize) {

  assert(fs != NULL);
  assert(stackDepth >= 0);
  assert(varArgsIndex >= 0);
  assert(value != NULL);
  assert(valueSize > 0);

  if(valueSize > 0 && valueSize <= argSize) {
    void * outPtr = frmstk_var_addr(fs, stackDepth, varArgsIndex);

    if(outPtr != NULL) {
      memcpy(outPtr, value, valueSize);
      return true;
    }
  }
  return false;
}

bool frmstk_var_read(FrmStk * fs, int stackDepth, int varArgsIndex,
		     void * outValue, size_t outValueSize) {

  assert(fs != NULL);
  assert(stackDepth >= 0);
  assert(varArgsIndex >= 0);
  assert(outValue != NULL);
  assert(outValueSize > 0);

  if(outValueSize > 0 && outValueSize <= argSize) {
    void * inPtr = frmstk_var_addr(fs, stackDepth, varArgsIndex);

    if(inPtr != NULL) {
      memcpy(outValue, inPtr, outValueSize);
      return true;
    }
  }
  return false;
}

size_t frmstk_ret_addr(FrmStk * fs) {

  assert(fs != NULL);

  if(fs->usedStack > 0) {
    FrameHeader * header = fs->buffer + fs->usedStack - sizeof(FrameHeader);
    return header->returnAddr;
  }

  return 0;
}

int frmstk_depth(FrmStk * fs) {
  assert(fs != NULL);
  return fs->stackDepth;
}

void frmstk_free(FrmStk * fs) {
  assert(fs != NULL);
  assert(fs->buffer != NULL);

  free(fs->buffer);
  free(fs);
}
