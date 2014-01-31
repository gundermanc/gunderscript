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

#include "gsbool.h"
#include "vm.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* define boolean values used in the op_bool_push function */
#define OP_TRUE             1
#define OP_FALSE            0
#define OP_NO_RETURN       -1

/**
 * All OP functions have more or less the same arguments. To save space
 * commenting, they are all commented here:
 * vm: An instance of the virtual machine.
 * byteCode: a char array containing the raw bytecode.
 * byteCodeLen: the number of bytes/chars in the char array.
 * index: a pointer to the current index value. Must be dereferenced before
 * use. Below each function comment is a diagram of the intended usage of the
 * associated OP code. It is in the format:
 * OPCODE [data:number_of_bytes] [next_data:number_of_bytes] ....
 */



/**
 * Handles OP_VAR_STOR opcode. Stores the top value from the op stack in the
 * frmstk at the specified stack depth and the specified index.
 * OP_VAR_STOR [stack_depth:1] [arg_index: 1]
 */
bool op_var_stor(VM * vm, char * byteCode, 
		   size_t byteCodeLen, int * index) {

  char stackDepth = byteCode[++(*index)];
  char varArgsIndex = byteCode[++(*index)];
  char data[VM_VAR_SIZE];
  VarType type;

  /* check that there are enough tokens in the input */
  if((byteCodeLen - *index) < 2) {
    vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
    return false;
  }

  /* advance to next byte */
  (*index)++;

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

  /* write the value to a variable slot in the frame stack */
  if(frmstk_var_write(vm->frmStk, stackDepth, 
		      varArgsIndex, data, VM_VAR_SIZE, type)) {
    return true;
  } else {
    vm_set_err(vm, VMERR_FRMSTK_VAR_ACCESS_FAILED);
    return false;
  }			 
}

/**
 * Reads a variable from the specified stack frame depth and index and pushes
 * it into the op stack.
 * OP_VAR_PUSH [stack_depth:1] [arg_index:1]
 */
bool op_var_push(VM * vm,  char * byteCode, 
			size_t byteCodeLen, int * index) {
  char stackDepth = byteCode[++(*index)];
  char varArgsIndex = byteCode[++(*index)];
  char data[VM_VAR_SIZE];
  VarType type;  

  /* check that there are enough tokens in the input */
  if((byteCodeLen - *index) < 2) {
    vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
    return false;
  }

  /* move to next byte */
  (*index)++;

  /* handle empty frame stack error case */
  if(!(frmstk_size(vm->frmStk) > 0)) {
    vm_set_err(vm, VMERR_FRMSTK_EMPTY);
    return false;
  }

  /* read value from framestack variable slot */
  if(frmstk_var_read(vm->frmStk, stackDepth, 
		     varArgsIndex, data, VM_VAR_SIZE, &type)) {
  } else {
    vm_set_err(vm, VMERR_FRMSTK_VAR_ACCESS_FAILED);
    return false;
  }

  /* push value to op stack */
  if(!typestk_push(vm->opStk, data, VM_VAR_SIZE, type)) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  return true;
}

/**
 * Pushes a frame onto the frame stack. This operation is used at the start
 * of each function, logical block to enforce a change in scope.
 * functionCall: if true, this frame push acts as a function call instead.
 * OP_FRM_PUSH [number_of_vars_and_args:1]
 * OP_CALL_B [number_of_vars_and_args:1] [args:1]
 * args: the number of values to pop from OP stack to treat as arguments.
 */
bool op_frame_push(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index, bool functionCall) {

  char numVarArgs = 0;
  char args;
  int i = 0;

  /* check there are enough bytes left for the parameters */
  if((byteCodeLen - *index) < 
     (functionCall ? (2 * sizeof(char)):((2 * sizeof(char)) + sizeof(int)))) {
    vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
    return false;
  }

  (*index)++;
  numVarArgs = byteCode[*index];
  (*index)++;

  /* get number of arguments to pop off of the stack */
  if(functionCall) {
    args = byteCode[*index];
    (*index)++;
  }

  /* push new frame with current index as return val */
  if(!frmstk_push(vm->frmStk, functionCall ? *index:OP_NO_RETURN, numVarArgs)) {
     vm_set_err(vm, VMERR_STACK_OVERFLOW);
     return false;
  }

  /* set argument values */
  if(functionCall) {
    int addr;

    /* check for enough stack items to do call */
    if(typestk_size(vm->opStk) < args) {
      vm_set_err(vm, VMERR_STACK_EMPTY);
      return false;
    }

    /* there are more parameters than there is memory allocated in the frame */
    if(args > numVarArgs) {
      vm_set_err(vm, VMERR_INVALID_PARAM);
      return false;
    }

    /* pop arguments and put them in them in variables on the new frame */
    for(i = 0; i < args; i++) {
      char data[VM_VAR_SIZE];
      VarType type;

      typestk_pop(vm->opStk, &data, VM_VAR_SIZE, &type);
      frmstk_var_write(vm->frmStk, FRMSTK_TOP, i, &data, VM_VAR_SIZE, type);
    }

    /* get goto address */
    memcpy(&addr, byteCode + *index, sizeof(int));

    /* check address is in valid range */
    if(addr < 0 || addr > byteCodeLen) {
      vm_set_err(vm, VMERR_INVALID_ADDR);
      return false;
    }

    /* perform goto */
    *index = addr;
  }

  return true;
}

/**
 * Pops a stack from from the top of the frame stack. This is called every time
 * a function or logical block is left.
 */
bool op_frame_pop(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {

  /* push new frame with current index as return val */
  if(frmstk_pop(vm->frmStk)) {
    (*index)++;
    return true;
  }

  vm_set_err(vm, VMERR_STACK_EMPTY);
  return false;
}

/**
 * Adds two numeric values, concats two strings, or appends a number to the end
 * of a string. Operates on previous two values on the OP stack, pops both, and
 * pushes result.
 * OP_ADD
 */
bool op_add(VM * vm,  char * byteCode, 
	    size_t byteCodeLen, int * index) {

  char * string1;
  char * string2;
  VarType type1;
  VarType type2;
  char * newString;

  /* handle not enough items in stack case */
  if(typestk_size(vm->opStk) < 2) {
    vm_set_err(vm, VMERR_STACK_EMPTY);
    return false;
  }

  (*index)++;

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

  /* handle typestk realloc error */
  if(!typestk_push(vm->opStk, &newString, sizeof(char*), TYPE_STRING)) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  free(string1);
  free(string2);
  return true;
}

/**
 * Pops previous two values on the OP stack, performs the requested math
 * operation and pushes the result.
 */
bool op_dual_operand_math(VM * vm,  char * byteCode, 
			  size_t byteCodeLen, int * index, OpCode code) {

  double value1;
  double value2;
  VarType type1;
  VarType type2;

  /* make sure that there are at least two values on the stack */
  if(typestk_size(vm->opStk) < 2) {
    vm_set_err(vm, VMERR_STACK_EMPTY);
    return false;
  }

  typestk_pop(vm->opStk, &value1, sizeof(double), &type1);
  typestk_pop(vm->opStk, &value2, sizeof(double), &type2);
    
  /* check that both operands are numbers..fail other types */
  if(type1 != TYPE_NUMBER || type2 != TYPE_NUMBER) {
    vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
    return false;
  }

  switch(code) {
  case OP_SUB:
    value1 -= value2;
    break;
  case OP_MUL:
    value1 *= value2;
    break;
  case OP_DIV:
    value1 /= value2;
    break;
  case OP_MOD:
    /* TODO: fix linker issue. */
    /*value1 = fmod(value1, value2);*/
    printf("\n\nDEBUG: OP_MOD Not yet implemented.\n\n");
    exit(0);
    break;
  default:
    /* TODO: remove in release version */
    printf("\n\nDEBUG: Invalid OPCode received. op_dual_operand_math().\n\n");
    exit(0);
  }

  (*index)++;

  typestk_push(vm->opStk, &value1, sizeof(double), TYPE_NUMBER);
  return true;
}

/**
 * Pops top two values from OP stack and compares them. If first is less than
 * second, pushes true. Otherwise, pushes false.
 * OP_LT
 */
bool op_dual_comparison(VM * vm,  char * byteCode, 
			size_t byteCodeLen, int * index, OpCode code) {

  double value1;
  double value2;
  bool result;
  VarType type1;
  VarType type2;

  /* check for enough items in the stack */
  if(typestk_size(vm->opStk) < 2) {
    vm_set_err(vm, VMERR_STACK_EMPTY);
    return false;
  }

  typestk_pop(vm->opStk, &value1, sizeof(double), &type1);
  typestk_pop(vm->opStk, &value2, sizeof(double), &type2);
    
  /* check data types */
  if(type1 != TYPE_NUMBER || type2 != TYPE_NUMBER) {
    vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
    return false;
  }

  switch(code) {
  case OP_LT:
    result = value1 < value2;
    break;
  case OP_LTE:
    result = value1 <= value2;
    break;
  case OP_GTE:
    result = value1 >= value2;
    break;
  case OP_GT:
    result = value1 > value2;
    break;
  case OP_EQUALS:
    result = value1 == value2;
    break;
  case OP_NOT_EQUALS:
    result = value1 != value2;
    break;
  default:
    /* TODO: remove in release version */
    printf("\n\nDEBUG: Invalid OPCode received. op_dual_comparison().\n\n");
    exit(0);
  }
  typestk_push(vm->opStk, &result, sizeof(bool), TYPE_BOOLEAN);
  return true;
}

/**
 * Pushes a number value to the OP stack.
 * OP_NUM_PUSH [double_number_value:sizeof(double)]
 */
bool op_num_push(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index) {

  double value;

  /* check if there are enough bytes left for a double */
  if((byteCodeLen - *index) < sizeof(double)) {
    vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
    return false;
  }

  (*index)++;
  memcpy(&value, byteCode + *index, sizeof(double));

  if(!typestk_push(vm->opStk, &value, sizeof(double), TYPE_NUMBER)) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  *index += sizeof(double);
  return true;
}

/**
 * Pops the top value off of the OP stack. This instruction is used at the end
 * of each statement to clear the ununsed data off of the stack.
 * OP_POP
 */
bool op_pop(VM * vm,  char * byteCode, 
	    size_t byteCodeLen, int * index) {

  void * value;
  VarType type;

  printf("Popped.");
  /* check that there is at least one item in the stack to pop */
  if(typestk_size(vm->opStk) <= 0) {
    vm_set_err(vm, VMERR_STACK_EMPTY);
    return false;
  }

  typestk_pop(vm->opStk, &value, sizeof(char*), &type);
  (*index)++;

  /* strings are dynamically allocated. free them */
  if(type == TYPE_STRING) {
    free(value);
  }

  return true;
}

/**
 * Pushes a boolean value to the stack. 
 * OP_BOOL_PUSH [true_or_false:1]
 */
bool op_bool_push(VM * vm,  char * byteCode, 
		  size_t byteCodeLen, int * index) {

  bool value;
   
  (*index)++;
  if((byteCodeLen - *index) < 1) {
    vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
    return false;
  }

  value = byteCode[*index];

  (*index)++;

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

/**
 * Pushes a string to the OP stack.
 * OP_STR_PUSH [string_length:1] [string_characters:string_length]
 */
/* TODO: This whole function seems to have semi-redundant checks.
 * clean up and optimize before file is moved to release candidate.
 */
bool op_str_push(VM * vm, char * byteCode, 
		   size_t byteCodeLen, int * index) {

  char strLen;
  char * string;

  /* check there is at least one byte left for the string length */
  if((byteCodeLen - *index) <= 0) {
    vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
    return false;
  }

  (*index)++;
  strLen = byteCode[*index];
  (*index)++;

  /* make sure there are enough bytes left to store the string */
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

/**
 * Pops a boolean value from the top of the OP stack and inverts it.
 * OP_NOT
 */
bool op_not(VM * vm, char * byteCode, 
		   size_t byteCodeLen, int * index) {

  bool value;
  VarType type;

  /* make sure that there is at least one item in the stack */
  if(typestk_size(vm->opStk) < 1) {
    vm_set_err(vm, VMERR_STACK_EMPTY);
    return false;
  }
  
  typestk_pop(vm->opStk, &value, sizeof(bool), &type);

  value = !value;

  (*index)++;

  /* make sure that push doesn't fail */
  if(!typestk_push(vm->opStk, &value, sizeof(bool), type)) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  return true;
}

/**
 * Pops top value from OP stack. If true and negGoto is false, or false and
 * negGoto is true, performs goto operation to specified index in the opcode
 * buffer. Otherwise, continues to the next instruction without goto-ing.
 * OP_COND_GOTO [goto_address:sizeof(int)]
 */
bool op_cond_goto(VM * vm, char * byteCode, 
			 size_t byteCodeLen, int * index, bool negGoto) {
  bool value;
  int addr;
  VarType type;

  /* check for a value on the stack that tells us to proceed */
  if(typestk_size(vm->opStk) < 1) {
    vm_set_err(vm, VMERR_STACK_EMPTY);
    return false;
  }

  typestk_pop(vm->opStk, &value, sizeof(bool), &type);

  (*index)++;

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
    vm_set_err(vm, VMERR_INVALID_ADDR);
    return false;
  }
 
  /* change address */
  *index = addr;

  return true;
}

/**
 * Calls the specified native function, using the specified number of values
 * from the top of the stack as arguments. callback_index is the index where
 * the desired function is stored in the callbacks array.
 * OP_CALL_PTR_N [args:1] [callback_index:sizeof(int)]
 */
bool op_call_ptr_n(VM * vm, char * byteCode, 
		   size_t byteCodeLen, int * index) {

  char numArgs = 0;
  int callbackIndex, i;
  VMArg args[VM_MAX_NARGS];
  VMCallback callback;

  /* handle not enough bytes in bytecode error case */
  if((byteCodeLen - *index) < (sizeof(char) + sizeof(int))) {
    vm_set_err(vm, VMERR_UNEXPECTED_END_OF_OPCODES);
    return false;
  }

  numArgs = byteCode[++(*index)];
  (*index)++;

  /* copy callback pointer index from the bytecode */
  memcpy(&callbackIndex, byteCode + *index, sizeof(int));
  *index += sizeof(void*);

  /* handle invalid number of args case*/
  /* TODO: check signedness of char...this might be unneccessary */
  if(numArgs < 0 || callbackIndex < 0) {
    vm_set_err(vm, VMERR_INVALID_PARAM);
    return false;
  }

  /* lookup callback function pointer */
  callback = vm_callback_from_index(vm, callbackIndex);
  
  /* verify callback function */
  if(callback == NULL) {
    vm_set_err(vm, VMERR_CALLBACK_NOT_EXIST);
    return false;
  }

  /* check there are enough items on stack for args array */
  if(typestk_size(vm->opStk) < numArgs) {
    vm_set_err(vm, VMERR_STACK_EMPTY);
    return false;
  }

  /* create array of arguments */
  for(i = 0; i < numArgs; i++) {
    typestk_pop(vm->opStk, &args[i].data, VM_VAR_SIZE, &args[i].type);
  }

  /* call the callback function */
  (*callback)(vm, args, numArgs);
  
  return true;
}
