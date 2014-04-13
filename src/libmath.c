/**
 * libmath.c
 * (C) 2014 Christian Gunderman + Kai Smith
 * Modified by: Kai Smith, Austin Kidder
 * Author Email: gundermanc@gmail.com
 * Modifier Email: kjs108@case.edu
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
  vmarg_push_number(vm, fabs(vmarg_number(arg[0], NULL)));

  /* this function does return a value */
  return true;
}

/**
 * VMNative: math_sqrt( value )
 * Accepts one number argument. Returns its square root.
 */
static bool vmn_math_sqrt(VM * vm, VMArg * arg, int argc) {

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
  vmarg_push_number(vm, sqrt(vmarg_number(arg[0], NULL)));

  /* this function does return a value */
  return true;
}

/**
 * VMNative: math_pow( base, power )
 * Accepts one number argument. Returns base^power.
 */
static bool vmn_math_pow(VM * vm, VMArg * arg, int argc) {

  /* check for proper number of arguments */
  if(argc != 2) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument type */
  if(vmarg_type(arg[0]) != TYPE_NUMBER
     || vmarg_type(arg[1]) != TYPE_NUMBER) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* push result */
  vmarg_push_number(vm, pow(vmarg_number(arg[0], NULL), vmarg_number(arg[1], NULL)));

  /* this function does return a value */
  return true;
}

/**
 * VMNative: math_round( value, precision )
 * Accepts one number argument. Returns value rounded to precision decimal places.
 */
static bool vmn_math_round(VM * vm, VMArg * arg, int argc) {

  switch(argc) {
  case 1: /* If user did not specify precision, assume round to int */
   
    /* check argument type */
    if(vmarg_type(arg[0]) != TYPE_NUMBER) {
      vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
      return false;
    }

    /* push result */
    vmarg_push_number(vm, round(vmarg_number(arg[0], NULL)));

    /* this function does return a value */
    return true;
  
  case 2: /* if user specified precision */

    /* check argument type */
    if(vmarg_type(arg[0]) != TYPE_NUMBER
       || vmarg_type(arg[1]) != TYPE_NUMBER) {
      vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
      return false;
    }

    /* push result */
    vmarg_push_number(vm, round(vmarg_number(arg[0], NULL) 
				* pow(10, vmarg_number(arg[1], NULL)))
		      / pow(10, vmarg_number(arg[1], NULL)));

    /* this function does return a value */
    return true;
  }
  
  vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
  return false;
}

/**
 * VMNative: math_sin( value )
 * Accepts one number argument. Returns sin(value).
 */
static bool vmn_math_sin(VM * vm, VMArg * arg, int argc) {

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
  vmarg_push_number(vm, sin(vmarg_number(arg[0], NULL)));

  /* this function does return a value */
  return true;
}

/**
 * VMNative: math_cos( value )
 * Accepts one number argument. Returns cos(value).
 */
static bool vmn_math_cos(VM * vm, VMArg * arg, int argc) {

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
  vmarg_push_number(vm, cos(vmarg_number(arg[0], NULL)));

  /* this function does return a value */
  return true;
}

/**
 * VMNative: math_tan( value )
 * Accepts one number argument. Returns tan(value).
 */
static bool vmn_math_tan(VM * vm, VMArg * arg, int argc) {

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
  vmarg_push_number(vm, tan(vmarg_number(arg[0], NULL)));

  /* this function does return a value */
  return true;
}

/**
 * VMNative: math_asin( value )
 * Accepts one number argument. Returns arcsin(value).
 */
static bool vmn_math_asin(VM * vm, VMArg * arg, int argc) {

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
  vmarg_push_number(vm, asin(vmarg_number(arg[0], NULL)));

  /* this function does return a value */
  return true;
}

/**
 * VMNative: math_acos( value )
 * Accepts one number argument. Returns arccos(value).
 */
static bool vmn_math_acos(VM * vm, VMArg * arg, int argc) {

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
  vmarg_push_number(vm, acos(vmarg_number(arg[0], NULL)));

  /* this function does return a value */
  return true;
}

/**
 * VMNative: math_atan( value )
 * Accepts one number argument. Returns arctan(value).
 */
static bool vmn_math_atan(VM * vm, VMArg * arg, int argc) {

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
  vmarg_push_number(vm, atan(vmarg_number(arg[0], NULL)));

  /* this function does return a value */
  return true;
}

/**
 * VMNative: math_atan2( y_value, x_value )
 * Accepts one number argument. Returns arctan(y_value, x_value).
 */
static bool vmn_math_atan2(VM * vm, VMArg * arg, int argc) {

  /* check for proper number of arguments */
  if(argc != 2) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument type */
  if(vmarg_type(arg[0]) != TYPE_NUMBER
     || vmarg_type(arg[1]) != TYPE_NUMBER) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* push result */
  vmarg_push_number(vm, atan2(vmarg_number(arg[0], NULL), vmarg_number(arg[1], NULL)));

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
		      "math_abs", 8, vmn_math_abs)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "math_sqrt", 9, vmn_math_sqrt)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "math_pow", 8, vmn_math_pow)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "math_round", 10, vmn_math_round)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "math_sin", 8, vmn_math_sin)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "math_cos", 8, vmn_math_cos)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "math_tan", 8, vmn_math_tan)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "math_asin", 9, vmn_math_asin)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "math_acos", 9, vmn_math_acos)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "math_atan", 9, vmn_math_atan)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "math_atan2", 10, vmn_math_atan2)) {
    return false;
  }
  return true;
}
