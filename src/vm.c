/**
 * vm.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * The basic intention for the VM is to simplify the overall implementation of
 * the programming language by having a unified byte code that can be compiled to.
 * In this interest, the byte code will have muliple variations.
 * - System Independent :: This byte code refers to functions in native function
 *       calls by their String names. This strategy is less efficient, because the
 *	 function address then has to be looked up in a hash table, however, it is 
 *	 system independent.
 * - Function Call Optimized :: This optimization is a run time optimization that
 *	 runs before code execution and replaces all function calls using CALL_STR_N
 *	 with CALL_PTR_N, replacing the String names to functions with their function
 *	 pointers from the hashtable in the Gunderscript instance.
 * - Condensation Optimization :: This MAY be implemented in future versions. At the
 *	 moment, GunderScript depends highly on unneccessary stack operations to
 *	 simplify the instruction set. This optimization makes use of an extended
 *	 instruction set that combines multiple operations into one instruction.
 * 
 * TODO: This code file is particularly dangerous at the moment because there
 * is no type checking on operations. While this doesn't usually cause a
 * problem, it can lead to segfaults, or memory leaks on improperly constructed
 * byte codes, and it relies WAY too much on the compiler to check types and
 * free memory. This is no problem when using my compiler, which does/will
 * handle all problems, however, there is no defense against corrupted byte code.
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

#include "vm.h"
#include "vmdefs.h"
#include "gsbool.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* TODO: make self expanding */
const size_t opStackSize = 1000000;

VM * vm_new(size_t stackSize) {

  assert(stackSize > 0);

  VM * vm = calloc(1, sizeof(VM));

  if(vm != NULL) {
    vm->frmStk = frmstk_new(stackSize);
    vm->opStk = stk_new(opStackSize);

    if(vm->frmStk != NULL && vm->opStk != NULL) {
      return vm;
    }
    vm_free(vm);
  }

  return NULL;
}

bool op_frame_push(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {

  /* check there is at least one byte left for the number of varargs */
  if((byteCodeLen - *index) > 0) {

    /* advance index to varargs number byte */
    uint8_t numVarArgs = 0;

    *index += 1;
    numVarArgs = byteCode[*index];
    *index += 1;

    /* push new frame with current index as return val */
    if(frmstk_push(vm->frmStk, *index, numVarArgs)) {
      printf("Pushed!");
      return true;
    }
    vm_set_err(vm, VMERR_STACK_OVERFLOW);
  } else {
    vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  }
  return false;
}

bool op_frame_pop(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {
  /* push new frame with current index as return val */
  if(frmstk_pop(vm->frmStk)) {
    printf("Popped!");
    *index += 1;
    return true;
  }

  vm_set_err(vm, VMERR_STACK_EMPTY);
  return false;
}

/* concats a string */
bool op_concat(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {
  if(stk_size(vm->opStk) >= 2) {
    DSValue dsVal;
    char * string1;
    char * string2;
    char * newString;

    *index += 1;

    /* pop topmost two strings */
    stk_pop(vm->opStk, &dsVal);
    string1 = dsVal.pointerVal;
    stk_pop(vm->opStk, &dsVal);
    string2 = dsVal.pointerVal;

    /* allocate and push new string, and free old ones */
    newString = calloc(strlen(string1) + strlen(string2) + 1, sizeof(char));
    if(newString == NULL) {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }

    strcpy(newString, string2);
    strcat(newString, string1);
    stk_push_pointer(vm->opStk, newString);

    free(string1);
    free(string2);
    printf("Concat.\n");
    return true;
  }

  vm_set_err(vm, VMERR_STACK_EMPTY);
  return false;
}

bool op_pop(VM * vm,  char * byteCode, 
	    size_t byteCodeLen, int * index, bool freePtr) {

  if(stk_size(vm->opStk) > 0) {
    DSValue val;
    stk_pop(vm->opStk, &val);
    *index += 1;
    if(freePtr) {
      free(val.pointerVal);
    }
    return true;
  }

  vm_set_err(vm, VMERR_STACK_EMPTY);
  return false;
}

/* TODO: This whole function seems to have semi-redundant checks.
 * clean up and optimize before file is moved to release candidate.
 */
bool op_str_push(VM * vm, char * byteCode, 
		   size_t byteCodeLen, int * index) {
  /* check there is at least one byte left for the string length */
  if((byteCodeLen - *index) > 0) {

    uint8_t strLen;
    char * string;

    *index += 1;
    strLen = byteCode[*index];
    *index += 1;

    printf("String len: %i\n", strLen);

    if((byteCodeLen - *index) < strLen) {
      vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
      return false;
    }

    string = calloc(strLen + 1, sizeof(char));
    if(string == NULL) {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }

    /* push new string */
    strncpy(string, byteCode + *index, strLen);
    printf("Push Succeed: %i\n", stk_push_pointer(vm->opStk, string));
    printf("String B4 Push: %s\n", string);
    *index += strLen;
    return true;
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

bool vm_exec(VM * vm, char * byteCode, 
	     size_t byteCodeLen, size_t startIndex) {
  int i = startIndex;

  assert(vm != NULL);
  assert(startIndex >= 0);
  assert(startIndex < byteCodeLen);

  while(i < byteCodeLen) {

    vm_set_err(vm, VMERR_SUCCESS);

    switch(byteCode[i]) {
    case OP_VAR_PUSH:
      printf("Not yet implemented!");
      break;
    case OP_VAR_STOR:
      printf("Not yet implemented!");
      break;
    case OP_FRM_PUSH:
      if(!op_frame_push(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_FRM_POP:
      if(!op_frame_pop(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_ADD:
      printf("Not yet implemented!");
      break;
    case OP_CONCAT:
      if(!op_concat(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_SUB:
      printf("Not yet implemented!");
      break;
    case OP_MUL:
      printf("Not yet implemented!");
      break;
    case OP_DIV:
      printf("Not yet implemented!");
      break;
    case OP_MOD:
      printf("Not yet implemented!");
      break;
    case OP_LT:
      printf("Not yet implemented!");
      break;
    case OP_GT:
      printf("Not yet implemented!");
      break;
    case OP_LTE:
      printf("Not yet implemented!");
      break;
    case OP_GTE:
      printf("Not yet implemented!");
      break;
    case OP_GOTO:
      printf("Not yet implemented!");
      break;
    case OP_BOOL_PUSH:
      printf("Not yet implemented!");
      break;
    case OP_NUM_PUSH:
      printf("Not yet implemented!");
      break;
    case OP_EQUALS:
      printf("Not yet implemented!");
      break;
    case OP_EXIT:
      printf("Not yet implemented!");
      break;
    case OP_STR_PUSH:
      if(!op_str_push(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_CALL_STR_N:
      printf("Not yet implemented!");
      break;
    case OP_CALL_PTR_N:
      printf("Not yet implemented!");
      break;
    case OP_CALL_B:
      printf("Not yet implemented!");
      break;
    case OP_NOT:
      printf("Not yet implemented!");
      break;
    case OP_COND_GOTO:
      printf("Not yet implemented!");
      break;
    case OP_NOT_EQUALS:
      printf("Not yet implemented!");
      break;
    case OP_POP_PTR:
      if(!op_pop(vm, byteCode, byteCodeLen, &i, true)) {
	return false;
      }
      break;
    case OP_POP_LITERAL:
      if(!op_pop(vm, byteCode, byteCodeLen, &i, false)) {
	return false;
      }
      break;

    default:
      printf("Invalid OpCode at Index: %i\n", i);
      vm_set_err(vm, VMERR_INVALID_OPCODE);
      return false;
    }
  }
  return true;
}

void vm_set_err(VM * vm, VMErr err) {
  assert(vm != NULL);

  vm->err = err;
}

VMErr vm_get_err(VM * vm) {
  assert(vm != NULL);

  return vm->err;
}

void vm_free(VM * vm) {
  if(vm->opStk != NULL) {
    stk_free(vm->opStk);
  }

  if(vm->frmStk != NULL) {
    frmstk_free(vm->frmStk);
  }

  free(vm);
}
