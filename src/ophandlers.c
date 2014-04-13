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
#include "ophandlers.h"
#include "libstr.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

/* define boolean values used in the op_bool_push function */
#define OP_TRUE             1
#define OP_FALSE            0
#define OP_NO_RETURN       -1

/**
 * Pushes an operand onto the operand stack.
 * vm: an instance of vm.
 * data: pointer to the data to push to the stack.
 * dataSize: the size of the operand in bytes.
 * type: the type of this operand.
 * returns: true if success, and false if typestk error occurs. See typestk.c
 * for more info.
 */
static bool opstk_push(VM * vm, void * data, size_t dataSize, VarType type) {

  /* increment ref count for this object */
  if(type == TYPE_LIBDATA) {
    vmlibdata_inc_refcount( *((VMLibData**)data) );
  }

  return typestk_push(vm->opStk, data, dataSize, type);
}

/**
 * Pops an operand from the operand stack.
 * vm: an instance of VM.
 * data: pointer to a buffer to receive the value.
 * dataSize: the size of the data buffer in bytes.
 * type: pointer to a VarType that will receive the type of the operand.
 * returns: true if success, and false if typestk error occurs. See typestk.c.
 */
static bool opstk_pop(VM * vm, void * data, size_t dataSize, VarType * type) {
  bool result = typestk_pop(vm->opStk, data, dataSize, type);
  
  /* decrement ref count for this object */
  if(*type == TYPE_LIBDATA) {
    vmlibdata_dec_refcount( *((VMLibData**)data) );
  }

  return result;
}

/**
 * Peeks an operand from the operand stack.
 * vm: an instance of VM.
 * data: pointer to a buffer to receive the value.
 * dataSize: the size of the data buffer in bytes.
 * type: pointer to a VarType that will receive the type of the operand.
 * returns: true if success, and false if typestk error occurs. See typestk.c.
 */
bool opstk_peek(VM * vm, void * data, size_t dataSize, VarType * type) {

  return typestk_peek(vm->opStk, data, dataSize, type);
}









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
  VMLibData * dataStruct;
  VMLibData * oldDataStruct;
  char data[VM_VAR_SIZE];
  VarType type;
  VarType oldType;

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

  opstk_peek(vm, data, VM_VAR_SIZE, &type);
  opstk_peek(vm, &dataStruct, sizeof(VMLibData*), &type);

  /* increment ref counter for this object */
  if(type == TYPE_LIBDATA) {
    vmlibdata_inc_refcount(dataStruct);
  }

  /* decrement ref counter for previous value if it was an object */
  frmstk_var_read(vm->frmStk, stackDepth, 
		  varArgsIndex, &oldDataStruct, sizeof(VMLibData*), &oldType);
  if(oldType == TYPE_LIBDATA) {
    vmlibdata_dec_refcount(oldDataStruct);
    vmlibdata_check_cleanup(vm, oldDataStruct);
  }

  /* write the value to a variable slot in the frame stack */
  if(!frmstk_var_write(vm->frmStk, stackDepth, 
		      varArgsIndex, data, VM_VAR_SIZE, type)) {
    vm_set_err(vm, VMERR_FRMSTK_VAR_ACCESS_FAILED);
    return false;
  }

  return true;
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
  VMLibData * dataStruct;
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
  if(!frmstk_var_read(vm->frmStk, stackDepth, 
		     varArgsIndex, data, VM_VAR_SIZE, &type)) {
    vm_set_err(vm, VMERR_FRMSTK_VAR_ACCESS_FAILED);
    return false;
  }

  /* increment ref count for objects */
  frmstk_var_read(vm->frmStk, stackDepth, 
		  varArgsIndex, &dataStruct, sizeof(VMLibData*), &type);
  if(type == TYPE_LIBDATA) {
    vmlibdata_inc_refcount(dataStruct);
  }
  

  /* push value to op stack */
  if(!opstk_push(vm, data, VM_VAR_SIZE, type)) {
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
 * OP_CALL_B [number_of_vars_and_args:1] [args:1] [function_address:sizeof(int)]
 * args: the number of values to pop from OP stack to treat as arguments.
 */
bool op_frame_push(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index, bool functionCall) {

  char numVarArgs = 0;
  char args;
  size_t paramSize = (functionCall ? ((2 * sizeof(char)) + sizeof(int)):
			    (2 * sizeof(char)));
  size_t endOfInstruction = *index + paramSize + 1;

  int i = 0;

  /* check there are enough bytes left for the parameters */
  if((byteCodeLen - *index) < paramSize) {
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
  if(!frmstk_push(vm->frmStk, functionCall ? endOfInstruction
		  : OP_NO_RETURN, numVarArgs)) {
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
    for(i = args - 1; i >= 0; i--) {
      char data[VM_VAR_SIZE];
      VarType type;

      opstk_pop(vm, &data, VM_VAR_SIZE, &type);
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
 * a function or logical block is left. If the stack frame has a recorded return
 * value (it was pushed with OP_CALL_B) then the code returns to that point of
 * execution. If isReturn is true, function keeps popping frames until we reach
 * the function frame.
 * OP_FRM_POP
 * OP_RETURN
 */
bool op_frame_pop(VM * vm,  char * byteCode, 
		  size_t byteCodeLen, int * index, bool isReturn) {

  int returnAddr;
  int i = 0;
  VMLibData * arg;
  VarType type;

  /* if this is a return statement, loop until function frame is found, or
   * frame stack is empty
   */
  do {
    /* get function return address from function frame */
    returnAddr = frmstk_ret_addr(vm->frmStk);

    /* decrement refcounters for objects that were variables */
    for(i = 0; frmstk_var_read(vm->frmStk, 0, i, &arg, 
			       sizeof(VMLibData*), &type); i++) {
      if(type == TYPE_LIBDATA) {
	vmlibdata_dec_refcount(arg);
	vmlibdata_check_cleanup(vm, arg);
      }
    }
  
    /* pop frame off of the stack */
    if(!frmstk_pop(vm->frmStk)) {
      vm_set_err(vm, VMERR_FRMSTK_EMPTY);
      return false;
    }

    /* if there is a return address, goto it to end the function */
    if(returnAddr != OP_NO_RETURN) {

      /* check that goto value is within the size of the bytecode */
      if(returnAddr >= byteCodeLen || returnAddr < 0) {
	vm->err = VMERR_INVALID_ADDR;
	return false;
      }

      (*index) = returnAddr;
      return true;
    }

  } while(isReturn);

  /* move cursor to next opcode */
  (*index)++;
  return true;
}

/**
 * Adds two numeric values, concats two strings, or appends a number to the end
 * of a string. Operates on previous two values on the OP stack, pops both, and
 * pushes result.
 * OP_ADD
 */
bool op_add(VM * vm,  char * byteCode, 
	    size_t byteCodeLen, int * index) {
  
  VarType type1;
  VarType type2;

  /* handle not enough items in stack case */
  if(typestk_size(vm->opStk) < 2) {
    vm_set_err(vm, VMERR_STACK_EMPTY);
    return false;
  }

  (*index)++;

  /* check the types of the top two objects on the stack */
  opstk_peek(vm, NULL, sizeof(char*), &type1);
  opstk_peek(vm, NULL, sizeof(char*), &type2);

  /* handle string to string concat operation */
  if(type1 == TYPE_LIBDATA && type2 == TYPE_LIBDATA) {

    VMLibData * data1;
    VMLibData * data2;
    VMLibData * result;

    /* pop topmost libdata structs */
    opstk_pop(vm, &data1, sizeof(VMLibData*), &type1);
    opstk_pop(vm, &data2, sizeof(VMLibData*), &type2);

    /* check to make sure these libdata structs contain strings */
    if(!vmlibdata_is_type(data1, LIBSTR_STRING_TYPE, LIBSTR_STRING_TYPE_LEN)
       || !vmlibdata_is_type(data2, LIBSTR_STRING_TYPE, LIBSTR_STRING_TYPE_LEN)) {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }    

    /* create result string LibData struct */
    result = libstr_string_new(libstr_string_length(data1)
			       + libstr_string_length(data2));
    if(result == NULL) {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }
    vmlibdata_inc_refcount(result);

    /* write strings to new string */
    libstr_string_append(result, libstr_string(data1), 
			 libstr_string_length(data1));
    libstr_string_append(result, libstr_string(data2), 
			 libstr_string_length(data2));

    /* push result to operand stack */
    if(!opstk_push(vm, &result, sizeof(VMLibData*), TYPE_LIBDATA)) {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }

    /* cleanup memory */
    vmlibdata_dec_refcount(data1);
    vmlibdata_dec_refcount(data2);
    vmlibdata_check_cleanup(vm, data1);
    vmlibdata_check_cleanup(vm, data2);

    return true;
  } else if(type1 == TYPE_NUMBER && type2 == TYPE_NUMBER) {
    /* handle add operation: */

    double value1;
    double value2;

    /* pop topmost two double values */
    opstk_pop(vm, &value1, sizeof(double), &type1);
    opstk_pop(vm, &value2, sizeof(double), &type2);
    
    value1 += value2;
    opstk_push(vm, &value1, sizeof(double), type1);

  } else {
    vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
    return false;
  }
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

  opstk_pop(vm, &value2, sizeof(double), &type1);
  opstk_pop(vm, &value1, sizeof(double), &type2);
    
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
    /* check for divide by zero errors */
    if(value2 == 0) {
      vm_set_err(vm, VMERR_DIVIDE_BY_ZERO);
      return false;
    }
    value1 /= value2;
    break;
  case OP_MOD:
    value1 = fmod(value1, value2);
    break;
  default:
    /* TODO: remove in release version */
    printf("\n\nDEBUG: Invalid OPCode received. op_dual_operand_math().\n\n");
    exit(0);
  }

  (*index)++;

  opstk_push(vm, &value1, sizeof(double), TYPE_NUMBER);
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

  opstk_pop(vm, &value2, sizeof(double), &type1);
  opstk_pop(vm, &value1, sizeof(double), &type2);

  /* TODO: implement comparisons between types, and object to object comparisons */
  switch(code) {
  case OP_LT:
    if(type1 == TYPE_NUMBER && type2 == TYPE_NUMBER) {
      result = value1 < value2;
    } else {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }
    break;
  case OP_LTE:
    if(type1 == TYPE_NUMBER && type2 == TYPE_NUMBER) {
      result = value1 <= value2;
    } else {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }
    break;
  case OP_GTE:
    if(type1 == TYPE_NUMBER && type2 == TYPE_NUMBER) {
      result = value1 >= value2;
    } else {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }
    break;
  case OP_GT:
    if(type1 == TYPE_NUMBER && type2 == TYPE_NUMBER) {
      result = value1 > value2;
    } else {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }
    break;
  case OP_EQUALS:
    if(type1 == TYPE_NUMBER && type2 == TYPE_NUMBER) {
      result = value1 == value2;
    } else if ((type1 == TYPE_NULL && type2 != TYPE_NULL)
	       || (type1 != TYPE_NULL && type2 == TYPE_NULL)) {
      result = false;
    } else if (type1 == TYPE_NULL && type2 == TYPE_NULL) {
      result = true;
    } else {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }
    break;
  case OP_NOT_EQUALS:
    if(type1 == TYPE_NUMBER && type2 == TYPE_NUMBER) {
      result = value1 != value2;
    } else if ((type1 == TYPE_NULL && type2 != TYPE_NULL)
	       || (type1 != TYPE_NULL && type2 == TYPE_NULL)) {
      result = true;
    } else if (type1 == TYPE_NULL && type2 == TYPE_NULL) {
      result = false;
    } else {
      vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
      return false;
    }
    break;
  default:
    /* TODO: remove in release version */
    printf("\n\nDEBUG: Invalid OPCode received. op_dual_comparison().\n\n");
    exit(0);
  }

  /* move to next instruction byte */
  (*index)++;

  /* push result */
  opstk_push(vm, &result, sizeof(bool), TYPE_BOOLEAN);
  return true;
}

/**
 * Pops top two values from OP stack and compares them. If first is less than
 * second, pushes true. Otherwise, pushes false.
 * OP_LT
 */
bool op_boolean_logic(VM * vm,  char * byteCode, 
			size_t byteCodeLen, int * index, OpCode code) {
  bool value1;
  bool value2;
  bool result;
  VarType type1;
  VarType type2;

  /* check for enough items in the stack */
  if(typestk_size(vm->opStk) < 2) {
    vm_set_err(vm, VMERR_STACK_EMPTY);
    return false;
  }

  opstk_pop(vm, &value2, sizeof(bool), &type1);
  opstk_pop(vm, &value1, sizeof(bool), &type2);
    
  /* check data types */
  if(type1 != TYPE_BOOLEAN || type2 != TYPE_BOOLEAN) {
    vm_set_err(vm, VMERR_INVALID_TYPE_IN_OPERATION);
    return false;
  }

  switch(code) {
  case OP_AND:
    result = value1 && value2;
    break;
  case OP_OR:
    result = value1 || value2;
    break;
  default:
    /* TODO: remove in release version */
    printf("\n\nDEBUG: Invalid OPCode received. op_dual_comparison().\n\n");
    exit(0);
  }

  /* move to next instruction byte */
  (*index)++;

  /* push result */
  opstk_push(vm, &result, sizeof(bool), TYPE_BOOLEAN);
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

  if(!opstk_push(vm, &value, sizeof(double), TYPE_NUMBER)) {
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

  /* check that there is at least one item in the stack to pop */
  if(typestk_size(vm->opStk) <= 0) {
    vm_set_err(vm, VMERR_STACK_EMPTY);
    return false;
  }

  opstk_pop(vm, &value, sizeof(char*), &type);
  (*index)++;

   /* free objects that were popped and passed */
  if(type == TYPE_LIBDATA) {
    vmlibdata_dec_refcount((VMLibData*)value);
    vmlibdata_check_cleanup(vm, (VMLibData*)value);
  }

  return true;
}

/**
 * Pushes a NULL to the stack.
 * OP_PUSH_NULL
 */
bool op_null_push(VM * vm,  char * byteCode, 
	    size_t byteCodeLen, int * index) {
  double value = 0;

  /* push null to stack */
  if(!opstk_push(vm, &value, sizeof(double), TYPE_NULL)) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }
  (*index)++;

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

  if(!opstk_push(vm, &value, sizeof(bool), TYPE_BOOLEAN)) {
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
  VMLibData * string;

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

  /* create new string buffer */
  string = libstr_string_new((int)strLen);
  if(string == NULL) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* push new string */
  libstr_string_append(string, byteCode + *index, strLen);
  vmlibdata_inc_refcount(string);
  if(!opstk_push(vm, &string, sizeof(VMLibData*), TYPE_LIBDATA)) {
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
  
  opstk_pop(vm, &value, sizeof(bool), &type);

  value = !value;

  (*index)++;

  /* make sure that push doesn't fail */
  if(!opstk_push(vm, &value, sizeof(bool), type)) {
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

  opstk_pop(vm, &value, sizeof(bool), &type);

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
 * Performs goto operation to specified index in the opcode buffer.
 * OP_GOTO [goto_address:sizeof(int)]
 */
bool op_goto(VM * vm, char * byteCode, 
	     size_t byteCodeLen, int * index) {
  int addr;

  (*index)++;

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
  *index += sizeof(int);

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
  for(i = numArgs - 1; i >= 0; i--) {
    opstk_pop(vm, &args[i].data, VM_VAR_SIZE, &args[i].type);
  }

  /* call the callback function
   * if returns false, no return value was given. push a null */
  if(! ((*callback)(vm, args, numArgs)) ) {
    double value = 0;
    opstk_push(vm, &value, sizeof(double), TYPE_NULL);
  }

  /* check for native function errors */
  if(vm->err != VMERR_SUCCESS) {
    return false;
  }

  /* decrement any variable reference counters */
  for(i = 0; i < numArgs; i++) {
    if(args[i].type == TYPE_LIBDATA) {
       vmlibdata_dec_refcount(vmarg_libdata(args[i]));
       vmlibdata_check_cleanup(vm, vmarg_libdata(args[i]));
    }
  }
  return true;
}
