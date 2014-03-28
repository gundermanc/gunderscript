/**
 * libstr.c
 * (C) 2014 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * Defines the Gunderscript functions and types for modifying strings.
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
#include "vm.h"
#include <string.h>

/**
 * VMNative: string_equals( string1, string2 )
 * Accepts two string arguments. Returns true if they are the same, and false if
 * not.
 */
static bool vmn_str_equals(VM * vm, VMArg * arg, int argc) {

  /* check for proper number of arguments */
  if(argc != 2) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument type */
  if(!vmarg_is_string(arg[0]) || !vmarg_is_string(arg[1])) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* push result */
  vmarg_push_boolean(vm, strcmp(vmarg_string(arg[0]), 
				vmarg_string(arg[1])) == 0);

  /* this function does return a value */
  return true;
}

/**
 * Installs the Libstr library in the given instance of Gunderscript.
 * gunderscript: the instance to receive the library.
 * returns: true upon success, and false upon failure. If failure occurs,
 * you probably did not allocate enough callbacks space in the call to 
 * gunderscript_new().
 */
bool libstr_install(Gunderscript * gunderscript) {
  if(!vm_reg_callback(gunderscript_vm(gunderscript), 
		      "string_equals", 13, vmn_str_equals)) {
    return false;
  }
  return true;
}
