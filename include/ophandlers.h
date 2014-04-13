/**
 * ophandlers.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See ophandlers.c for more information.
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

#ifndef OPHANDLERS__H__
#define OPHANDLERS__H__

bool op_var_stor(VM * vm, char * byteCode, 
			size_t byteCodeLen, int * index);

bool op_var_push(VM * vm,  char * byteCode, 
		  size_t byteCodeLen, int * index);

bool op_frame_push(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index, bool functionCall);

bool op_frame_pop(VM * vm,  char * byteCode, 
		  size_t byteCodeLen, int * index, bool isReturn);

bool op_add(VM * vm,  char * byteCode, 
	     size_t byteCodeLen, int * index);

bool op_dual_operand_math(VM * vm,  char * byteCode, 
			  size_t byteCodeLen, int * index, OpCode code);

bool op_dual_comparison(VM * vm,  char * byteCode, 
			size_t byteCodeLen, int * index, OpCode code);

bool op_boolean_logic(VM * vm,  char * byteCode, 
		      size_t byteCodeLen, int * index, OpCode code);

bool op_num_push(VM * vm,  char * byteCode, 
		  size_t byteCodeLen, int * index);

bool op_pop(VM * vm,  char * byteCode, 
	     size_t byteCodeLen, int * index);

bool op_bool_push(VM * vm,  char * byteCode, 
		   size_t byteCodeLen, int * index);

bool op_str_push(VM * vm, char * byteCode, 
		  size_t byteCodeLen, int * index);

bool op_not(VM * vm, char * byteCode, 
	     size_t byteCodeLen, int * index);

bool op_cond_goto(VM * vm, char * byteCode, 
		   size_t byteCodeLen, int * index, bool negGoto);

bool op_goto(VM * vm, char * byteCode, 
		  size_t byteCodeLen, int * index);

bool op_call_ptr_n(VM * vm, char * byteCode, 
		    size_t byteCodeLen, int * index);

bool op_null_push(VM * vm,  char * byteCode, 
		  size_t byteCodeLen, int * index);
#endif /* OPHANDLERS__H__ */
