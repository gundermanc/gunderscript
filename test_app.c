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
  printf("Test!");
  return true;
}

/* VM Test code */
int main() {
  VM * vm = vm_new(10000000);
  char foo[45];
  double d = 6;
  double q = 0;
  int a = 0, b = 0;
  bool r;
  VarType type;
  VMCallback vc;

  foo[0] = OP_CALL_PTR_N;
  foo[1] = 0;
  
  printf("Actual Pointer: %p", &test);
  memcpy(foo, &test, sizeof(VMCallback));
  memcpy(&vc, test, sizeof(VMCallback));
  /*vc = &test;*/
  printf("Extracted: %p", vc);
  /*  (*vc)(NULL,NULL,0); */

  printf("Success: %i\n", vm_exec(vm, foo, 10, 0));
  printf("Stack Depth: %i\n", typestk_size(vm->opStk));
  printf("Error: %i\n", vm_get_err(vm));


  /*typestk_peek(vm->opStk, &b, sizeof(int), &type);*/
  /*printf("Output Value: %i ", b);*/

  vm_free(vm);
  return 0;
}
