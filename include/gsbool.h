/**
 * gsbool.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * This library is designed to be compatible with older C standards, namely,
 * C89, for cross platform compatibility. As such, we define our own boolean
 * in this header. If you wish to use your own boolean definition, define
 * GSBOOL__H__ in your code to prevent this header from being included.
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

#ifndef GSBOOL__H__
#define GSBOOL__H__



/* Boolean Definitions:
 * Some compilers don't come with stdbool.h, so we go ahead and define our own
 * for this project.
 *
 * If you wish to use another bool definition such as stdbool.h, define:
 * GSBOOL__H__ before including any headers from this project to disable this
 * header.
 */
#ifndef bool
#define bool int
#endif /* bool */

#ifndef true
#define true 1
#endif /* true */

#ifndef false
#define false 0
#endif /* false */

#endif /* GSBOOL__H__*/
