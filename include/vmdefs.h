/**
 * vmdefs.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * Contains definitions for the VM opcodes as well as the various scripting
 * types.
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

#ifndef VMDEFS__H__
#define VMDEFS__H__

/* size allocated for VM variables on the stack:
 * largest field is a double which is 8 bytes
 */
#define VM_VAR_SIZE          sizeof(double)

/* maximum number of arguments for a native
 * function
 */
#define VM_MAX_NARGS         25

/* variable datatypes */
typedef enum {
  TYPE_BOOLEAN,
  TYPE_NUMBER,
  TYPE_STRING,
  TYPE_LIBDATA,
}VarType;

/* VM OP Code values */
typedef enum {
  OP_VAR_PUSH,
  OP_VAR_STOR,
  OP_FRM_PUSH,
  OP_FRM_POP,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_MOD,
  OP_LT,
  OP_GT,
  OP_LTE,
  OP_GTE,
  OP_GOTO,
  OP_BOOL_PUSH,
  OP_NUM_PUSH, /* 15 */
  OP_EQUALS,
  OP_EXIT,
  OP_STR_PUSH,
  OP_CALL_STR_N,
  OP_CALL_PTR_N, /* 20 */
  OP_CALL_B,
  OP_NOT,
  OP_TCOND_GOTO,
  OP_FCOND_GOTO,
  OP_NOT_EQUALS,
  OP_POP,
  OP_AND,
  OP_OR,
} OpCode;

#endif /* VMDEFS__H__ */
