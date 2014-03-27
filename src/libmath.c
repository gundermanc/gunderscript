/**
 * libmath.c
 * (C) 2014 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * Defines the Gunderscript functions and types for interfacing with
 * the standard math.h C functions.
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

#include "gunderscript.h"
#include "libmath.h"
#include "vm.h"
#include <string.h>
#include <math.h>


/**
 * VMNative: math_abs( value )
 * Accepts one number argument. Returns its absolute value.
 */
static bool vmn_math_abs(VM * vm, VMArg * arg, int argc) {
  int i = 0;

  /* check for proper number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument type */
  if(vmarg_type(arg[0]) != TYPE_NUMBER) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* push result */
  vmarg_push_number(vm, abs(vmarg_number(arg[0], NULL)));

  /* this function does return a value */
  return true;
}

/**
 * Installs the Libmath library in the given instance of Gunderscript.
 * gunderscript: the instance to receive the library.
 * returns: true upon success, and false upon failure. If failure occurs,
 * you probably did not allocate enough callbacks space in the call to 
 * gunderscript_new().
 */
bool libmath_install(Gunderscript * gunderscript) {

  if(!vm_reg_callback(gunderscript_vm(gunderscript), 
		      "math_abs", 8, vmn_math_abs)) {
    return false;
  }
  return true;
}
