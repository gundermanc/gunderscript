/**
 * test_app.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * Test program entry point that can be used for conveniently trying out
 * test cases or experimenting with the library.
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

#include "lexer.h"
#include "frmstk.h"
#include "vm.h"
#include "vmdefs.h"
#include "typestk.h"
#include <string.h>

/*
char * print_next_token(Lexer * l) {
  size_t tokenLen;
  int i = 0;
  
  char * token = lexer_next(l, &tokenLen);

  if(token == NULL) {
    printf("NULL\n");
    return token;
  }

  for(i = 0; i < tokenLen; i++) {
    printf("%c", token[i]);
  }
  printf("\n\t");

  printf("Token type: %i", lexer_token_type(token, tokenLen, false));
  printf("\n");

  return token;
}

int main() {
  char * foo = "function main(args, argsLen) {\n"
               "  print(\"Hello\"); "
               "  return 0;"
               "}"

               "function print(text) {"
               "  out(text);"
               "  return true;"
               "}";
 
    
  Lexer * l = lexer_new(foo, strlen(foo));
 
  while(print_next_token(l) != NULL);


  printf("Error code: %i", l->err);
  printf("\nLine: %i\n", lexer_line_num(l));

  lexer_free(l);
  return 0;
}
*/

bool test(VM * vm, VMArg * arg, int argc) {
  printf("String: %s\n", vmarg_string(arg[0]));
  return true;
}

/* VM Test code */
int main() {
  char code[200];
  int index = 0;
  VM * vm = vm_new(10000000, 24);
  printf("To storage: %p\n", test);
  printf("Register Succeed: %i\n", vm_reg_callback(vm, "Hello", 5, test));
  printf("VM Err: %i\n", vm_get_err(vm));
  printf("VM # functions: %i\n", vm_num_callbacks(vm));

  code [0] = OP_STR_PUSH;
  code [1] = 2;
  code [2] = 'H';
  code [3] = 'i';
  code [4] = OP_CALL_PTR_N;
  code [5] = 1;

  memcpy(code + 6, &index, sizeof(int));
  
  printf("VM EXEC: %i", vm_exec(vm, code, 10, 0));
  
  vm_free(vm);
  return 0;
}
