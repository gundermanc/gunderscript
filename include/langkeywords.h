/**
 * langkeywords.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * Defines keywords and tokens used by the language.
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

#include <stdlib.h>
#include <stdio.h>
#include "gsbool.h"

#ifndef LANGKEYWORDS__H__
#define LANGKEYWORDS__H__

#define LANG_FUNCTION   "function"
#define LANG_FUNCTION_LEN 8
#define LANG_EXPORTED   "exported"
#define LANG_EXPORTED_LEN 8
#define LANG_OPARENTH   "("
#define LANG_OPARENTH_LEN 1
#define LANG_CPARENTH   ")"
#define LANG_CPARENTH_LEN 1
#define LANG_ARGDELIM   ","
#define LANG_ARGDELIM_LEN 1
#define LANG_OBRACKET   "{"
#define LANG_OBRACKET_LEN 1
#define LANG_CBRACKET   "}"
#define LANG_CBRACKET_LEN 1
#define LANG_VAR_DECL   "var"
#define LANG_VAR_DECL_LEN 3

#define LANG_OP_ADD     "+"
#define LANG_OP_ADD_LEN   1
#define LANG_OP_SUB     "-"
#define LANG_OP_SUB_LEN   1
#define LANG_OP_MUL     "*"
#define LANG_OP_MUL_LEN   1
#define LANG_OP_DIV     "/"
#define LANG_OP_DIV_LEN   1
#endif /* LANGKEYWORDS__H__ */
