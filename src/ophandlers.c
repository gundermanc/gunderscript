/**
 * ophandlers.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * The ophandlers.c file contains functions that implement the functionality
 * of the VM opcodes.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gsbool.h"
#include "vm.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define OP_TRUE             1
#define OP_FALSE            0

bool op_var_stor(VM * vm, char * byteCode, 
		   size_t byteCodeLen, int * index) {

  /* check that there are enough tokens in the input */
  if((byteCodeLen - *index) >= 2) {
    char stackDepth = byteCode[++(*index)];
    char varArgsIndex = byteCode[++(*index)];
    char data[VM_VAR_SIZE];
    VarType type;

    *index += 1;

    /* handle empty op stack error case */
    if(!(typestk_size(vm->opStk) > 0)) {
      vm_set_err(vm, VMERR_STACK_EMPTY);
      return false;
    }

    /* handle empty frame stack error case */
    if(!(frmstk_size(vm->frmStk) > 0)) {
      vm_set_err(vm, VMERR_FRMSTK_EMPTY);
      return false;
    }

    typestk_peek(vm->opStk, data, VM_VAR_SIZE, &type);

    if(frmstk_var_write(vm->frmStk, stackDepth, 
			varArgsIndex, data, VM_VAR_SIZE, type)) {
      printf("Var stored!\n");
      return true;
    } else {
      vm_set_err(vm, VMERR_FRMSTK_VAR_ACCESS_FAILED);
      return false;
    }			 
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

 bool op_var_push(VM * vm,  char * byteCode, 
			size_t byteCodeLen, int * index) {
  /* check that there are enough tokens in the input */
  if((byteCodeLen - *index) >= 2) {
    char stackDepth = byteCode[++(*index)];
    char varArgsIndex = byteCode[++(*index)];
    char data[VM_VAR_SIZE];
    VarType type;

    *index += 1;

     /* handle empty frame stack error case */
    if(!(frmstk_size(vm->frmStk) > 0)) {
      vm_set_err(vm, VMERR_FRMSTK_EMPTY);
      return false;
    }

    /* read value */
    if(frmstk_var_read(vm->frmStk, stackDepth, 
			varArgsIndex, data, VM_VAR_SIZE, &type)) {
    } else {
      vm_set_err(vm, VMERR_FRMSTK_VAR_ACCESS_FAILED);
      return false;
    }

    /* push value to op stack */
    if(typestk_push(vm->opStk, data, VM_VAR_SIZE, type)) {
      return true;
    } else {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
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
 bool op_add(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {

  if(typestk_size(vm->opStk) >= 2) {
    char * string1;
    char * string2;
    VarType type1;
    VarType type2;
    char * newString;

    *index += 1;

    /* pop topmost two strings */
    typestk_pop(vm->opStk, &string1, sizeof(char*), &type1);
    typestk_pop(vm->opStk, &string2, sizeof(char*), &type2);

    /* check that top two values are strings */
    if(type1 != TYPE_STRING || type2 != TYPE_STRING) {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }

    /* allocate and push new string, and free old ones */
    newString = calloc(strlen(string1) + strlen(string2) + 1, sizeof(char));
    if(newString == NULL) {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }

    strcpy(newString, string2);
    strcat(newString, string1);
    if(!typestk_push(vm->opStk, &newString, sizeof(char*), TYPE_STRING)) {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }

    free(string1);
    free(string2);
    printf("Concat.\n");
    return true;
  }

  vm_set_err(vm, VMERR_STACK_EMPTY);
  return false;
}

 bool op_sub(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {
  if(typestk_size(vm->opStk) >= 2) {
    double value1;
    double value2;
    VarType type1;
    VarType type2;

    typestk_pop(vm->opStk, &value1, sizeof(double), &type1);
    typestk_pop(vm->opStk, &value2, sizeof(double), &type2);
    
    if(type1 != TYPE_NUMBER || type2 != TYPE_NUMBER) {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }

    value1 -= value2;
    typestk_push(vm->opStk, &value1, sizeof(double), TYPE_NUMBER);
    return true;
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

 bool op_mul(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {
  if(typestk_size(vm->opStk) >= 2) {
    double value1;
    double value2;
    VarType type1;
    VarType type2;

    typestk_pop(vm->opStk, &value1, sizeof(double), &type1);
    typestk_pop(vm->opStk, &value2, sizeof(double), &type2);
    
    if(type1 != TYPE_NUMBER || type2 != TYPE_NUMBER) {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }

    value1 *= value2;
    typestk_push(vm->opStk, &value1, sizeof(double), TYPE_NUMBER);
    return true;
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

 bool op_div(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {
  if(typestk_size(vm->opStk) >= 2) {
    double value1;
    double value2;
    VarType type1;
    VarType type2;

    typestk_pop(vm->opStk, &value1, sizeof(double), &type1);
    typestk_pop(vm->opStk, &value2, sizeof(double), &type2);
    
    /* check data types */
    if(type1 != TYPE_NUMBER || type2 != TYPE_NUMBER) {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }

    /* divide by zero error handler */
    if(value2 == 0) {
      vm_set_err(vm, VMERR_DIVIDE_BY_ZERO);
      return false;
    }

    value1 /= value2;
    typestk_push(vm->opStk, &value1, sizeof(double), TYPE_NUMBER);
    return true;
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

 bool op_mod(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {
  if(typestk_size(vm->opStk) >= 2) {
    double value1;
    double value2;
    VarType type1;
    VarType type2;

    typestk_pop(vm->opStk, &value1, sizeof(double), &type1);
    typestk_pop(vm->opStk, &value2, sizeof(double), &type2);
    
    /* check data types */
    if(type1 != TYPE_NUMBER || type2 != TYPE_NUMBER) {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }

    /* divide by zero error handler */
    /*if(value2 == 0) {
      vm_set_err(vm, VMERR_DIVIDE_BY_ZERO);
      return false;
      }*/

    /* TODO: fix linker issue so that this will actually work */
    /*value1 = fmod(value1, value2);
      typestk_push(vm->opStk, &value1, sizeof(double), TYPE_NUMBER);*/
    return true;
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

 bool op_lt(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {
  if(typestk_size(vm->opStk) >= 2) {
    double value1;
    double value2;
    bool result;
    VarType type1;
    VarType type2;

    typestk_pop(vm->opStk, &value1, sizeof(double), &type1);
    typestk_pop(vm->opStk, &value2, sizeof(double), &type2);
    
    /* check data types */
    if(type1 != TYPE_NUMBER || type2 != TYPE_NUMBER) {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }

    result = value1 < value2;
    typestk_push(vm->opStk, &result, sizeof(bool), TYPE_BOOLEAN);
    return true;
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

 bool op_lte(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {
  if(typestk_size(vm->opStk) >= 2) {
    double value1;
    double value2;
    bool result;
    VarType type1;
    VarType type2;

    typestk_pop(vm->opStk, &value1, sizeof(double), &type1);
    typestk_pop(vm->opStk, &value2, sizeof(double), &type2);
    
    /* check data types */
    if(type1 != TYPE_NUMBER || type2 != TYPE_NUMBER) {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }

    result = value1 <= value2;
    typestk_push(vm->opStk, &result, sizeof(bool), TYPE_BOOLEAN);
    return true;
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

 bool op_gte(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {
  if(typestk_size(vm->opStk) >= 2) {
    double value1;
    double value2;
    bool result;
    VarType type1;
    VarType type2;

    typestk_pop(vm->opStk, &value1, sizeof(double), &type1);
    typestk_pop(vm->opStk, &value2, sizeof(double), &type2);
    
    /* check data types */
    if(type1 != TYPE_NUMBER || type2 != TYPE_NUMBER) {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }

    result = value1 >= value2;
    typestk_push(vm->opStk, &result, sizeof(bool), TYPE_BOOLEAN);
    return true;
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

 bool op_gt(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {
  if(typestk_size(vm->opStk) >= 2) {
    double value1;
    double value2;
    bool result;
    VarType type1;
    VarType type2;

    typestk_pop(vm->opStk, &value1, sizeof(double), &type1);
    typestk_pop(vm->opStk, &value2, sizeof(double), &type2);
    
    /* check data types */
    if(type1 != TYPE_NUMBER || type2 != TYPE_NUMBER) {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }

    result = value1 > value2;
    typestk_push(vm->opStk, &result, sizeof(bool), TYPE_BOOLEAN);
    return true;
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

 bool op_equals(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {
  if(typestk_size(vm->opStk) >= 2) {
    double value1;
    double value2;
    bool result;
    VarType type1;
    VarType type2;

    typestk_pop(vm->opStk, &value1, sizeof(double), &type1);
    typestk_pop(vm->opStk, &value2, sizeof(double), &type2);
    
    /* check data types */
    if(type1 != TYPE_NUMBER || type2 != TYPE_NUMBER) {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }

    result = value1 == value2;
    typestk_push(vm->opStk, &result, sizeof(bool), TYPE_BOOLEAN);
    return true;
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

 bool op_not_equals(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {
  if(typestk_size(vm->opStk) >= 2) {
    double value1;
    double value2;
    bool result;
    VarType type1;
    VarType type2;

    typestk_pop(vm->opStk, &value1, sizeof(double), &type1);
    typestk_pop(vm->opStk, &value2, sizeof(double), &type2);
    
    /* check data types */
    if(type1 != TYPE_NUMBER || type2 != TYPE_NUMBER) {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }

    result = value1 != value2;
    typestk_push(vm->opStk, &result, sizeof(bool), TYPE_BOOLEAN);
    return true;
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

 bool op_num_push(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {

  /* check if there are enough bytes left for a double */
  if((byteCodeLen - *index) >= sizeof(double)) {
    double value;

    *index += 1;
    memcpy(&value, byteCode + *index, sizeof(double));

    if(!typestk_push(vm->opStk, &value, sizeof(double), TYPE_NUMBER)) {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }

    *index += sizeof(double);
    return true;
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

 bool op_pop(VM * vm,  char * byteCode, 
	    size_t byteCodeLen, int * index) {

  if(typestk_size(vm->opStk) > 0) {
    void * value;
    VarType type;

    typestk_pop(vm->opStk, &value, sizeof(char*), &type);
    *index += 1;

    if(type == TYPE_STRING) {
      free(value);
    }
    return true;
  }

  vm_set_err(vm, VMERR_STACK_EMPTY);
  return false;
}

 bool op_bool_push(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {
  if((byteCodeLen - *index) >= 1) {
    bool value;

    *index += 1;

    value = byteCode[*index];

    *index += 1;

    /* check if value is true or false */
    if(value != OP_TRUE && value != OP_FALSE) {
      vm_set_err(vm, VMERR_INVALID_PARAM);
      return false;
    }

    if(!typestk_push(vm->opStk, &value, sizeof(bool), TYPE_BOOLEAN)) {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }

    return true;    
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
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
    if(!typestk_push(vm->opStk, &string, sizeof(char*), TYPE_STRING)) {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }

    *index += strLen;
    return true;
  }

  vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
  return false;
}

 bool op_not(VM * vm, char * byteCode, 
		   size_t byteCodeLen, int * index) {
  if(typestk_size(vm->opStk) >= 1) {
    bool value;
    VarType type;

    typestk_pop(vm->opStk, &value, sizeof(bool), &type);

    value = !value;

    *index += 1;

    if(typestk_push(vm->opStk, &value, sizeof(bool), type)) {
      return true;
    } else {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }
  }

  vm_set_err(vm, VMERR_STACK_EMPTY);
  return false;
}

 bool op_cond_goto(VM * vm, char * byteCode, 
			 size_t byteCodeLen, int * index, bool negGoto) {
  if(typestk_size(vm->opStk) >= 1) {
    bool value;
    int addr;
    VarType type;

    typestk_pop(vm->opStk, &value, sizeof(bool), &type);

    *index += 1;

    /* make sure top item in stack was a boolean */
    if(type != TYPE_BOOLEAN) {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }

    /* check top boolean for if we should skip goto */
    if((!value && !negGoto) || (value && negGoto)) {
      *index += sizeof(int);
      return true;
    }

    /* check that there are enough bytes in byte code for the address */
    if(!((byteCodeLen - *index) >= sizeof(int))) {
      vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
      return false;
    }

    memcpy(&addr, byteCode + *index, sizeof(int));

    if(addr < 0 || addr >= byteCodeLen) {
      printf("Invalid Addr: %i", addr);
      vm_set_err(vm, VMERR_INVALID_ADDR);
      return false;
    }
 
    /* change address */
    *index = addr;

    return true;
  }

  vm_set_err(vm, VMERR_STACK_EMPTY);
  return false;
}

 bool op_call_ptr_n(VM * vm, char * byteCode, 
			 size_t byteCodeLen, int * index) {

  char numArgs = 0;
  VMCallback callback;

  /* handle not enough bytes in bytecode error case */
  if((byteCodeLen - *index) < 9) {
    vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
    return false;
  }

  numArgs = byteCode[++(*index)];
  (*index)++;

  /* handle invalid number of args case*/
  /* TODO: check signedness of char...this might be unneccessary */
  if(numArgs < 0) {
    vm_set_err(vm, VMERR_INVALID_PARAM);
    return false;
  }

  memcpy(&callback, byteCode + *index, sizeof(void*));
  printf("Index: %i", *index);
  *index += sizeof(void*);

  printf("Stored Pointer: %i", callback);
  
  return true;
}
