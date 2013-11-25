/**
 * vm.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See vm.c for full description.
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

#ifndef VM__H__
#define VM__H__

#include "frmstk.h"
#include "stk.h"

typedef enum {
  VMERR_SUCCESS,
  VMERR_INVALID_OPCODE
} VMErr;

typedef struct VM {
  FrmStk * frmStk;
  Stk * opStk;
  VMErr err;
}VM;



VM * vm_new(size_t stackSize);

bool vm_exec(VM * vm, unsigned char * byteCode,
	     size_t byteCodeLen, size_t startIndex);

void vm_set_err(VM * vm, VMErr err);

VMErr vm_get_err(VM * vm);

VM * vm_free(VM * vm);


#endif // VM__H__
