/**
 * opcodes.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * Contains definitions for the VM opcodes.
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

#ifndef OPCODES__H__
#define OPCODES__H__

typedef enum {
  OP_VAR_PUSH,
  OP_VAR_STOR,
  OP_FRM_PUSH,
  OP_FRM_POP,
  OP_ADD,
  OP_CONCAT,
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
  OP_NUM_PUSH,
  OP_EQUALS,
  OP_EXIT,
  OP_STR_PUSH,
  OP_CALL_STR_N,
  OP_CALL_PTR_N,
  OP_CALL_B,
  OP_NOT,
  OP_COND_GOTO,
  OP_NOT_EQUALS,
  OP_POP,
} OpCode;

#endif /* OPCODES__H__ */
