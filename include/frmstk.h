/**
 * frmstk.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See frmstack.c for up to date description.
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

#ifndef FRMSTK__H__
#define FRMSTK__H__

#include <stdlib.h>
#include "gsbool.h"
#include "vmdefs.h"

typedef struct FrameHeader {
  size_t returnAddr;
  int numVarArgs;
} FrameHeader;

typedef struct FrmStk {
  void * buffer;
  size_t usedStack;
  size_t stackSize;
  int stackDepth;
} FrmStk;

FrmStk * frmstk_new(size_t stackSize);

bool frmstk_push(FrmStk * fs, size_t returnAddr, int numVarArgs);

bool frmstk_pop(FrmStk * fs);

void * frmstk_var_addr(FrmStk * fs, int stackDepth, int varArgsIndex);

bool frmstk_var_write(FrmStk * fs, int stackDepth, int varArgsIndex,
		      void * value, size_t valueSize, VarType type);

bool frmstk_var_read(FrmStk * fs, int stackDepth, int varArgsIndex,
		     void * outValue, size_t outValueSize, VarType * outType);

size_t frmstk_ret_addr(FrmStk * fs);

int frmstk_size(FrmStk * fs);

void frmstk_free(FrmStk * fs);

#endif /* FRMSTK__H__ */
