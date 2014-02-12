/**
 * strcodeparser.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See strcodeparser.c for description.
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

#ifndef STRCODEPARSER__H__
#define STRCODEPARSER__H__

#include <stdlib.h>
#include "compcommon.h"
#include "gsbool.h"
#include "typestk.h"
bool write_operators_from_stack(Compiler * c, TypeStk * opStk, 
				Stk * opLenStk, bool parenthExpected,
				bool popParenth);


#endif /* STRCODEPARSER__H__ */
