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
#include "compiler.h"
#include "ht.h"
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

/*
bool test(VM * vm, VMArg * arg, int argc) {
  printf("String: %s\n", vmarg_string(arg[0]));
  return true;
}

VM Test code
int main() {
  long b;
  char code[200];
  int index = 11;
  VM * vm = vm_new(10000000, 24);

  code[0] = OP_BOOL_PUSH;
  code[1] = 0;
  code[2] = OP_BOOL_PUSH;
  code[3] = 0;
  code[4] = OP_CALL_B;
  code [5] = 2;
  code [6] = 2;

  memcpy(code + 7, &index, sizeof(int));
  code[11] = OP_POP;

  printf("VM Success: %i\n", vm_exec(vm, code, 12, 0));
  printf("StackDepth: %i\n", typestk_size(vm->opStk)); 
  printf("FrmStackDepth: %i\n", frmstk_size(vm->frmStk));
  printf("VM ERR index: %i\n", vm->index);
  printf("VM ERR: %i\n", vm_get_err(vm));

  printf("Read success: %i\n", 
	 frmstk_var_read(vm->frmStk, 0, 1, &b, sizeof(bool), NULL));
  printf("Stored: %i", b);
  vm_free(vm);
  return 0;
}
*/

/* compiler test code */
int main() {
  Compiler * c = compiler_new();
  char * foo = "(3 + 3) * 2";
  compiler_build(c, foo, strlen(foo));
  printf("FunctionHTSize: %i\n", ht_size(c->functionHT));
  printf("SymTblStk Size: %i\n", stk_size(c->symTableStk));
  printf("Output Length: %i\n", sb_size(c->outBuffer));
  printf("Compiler Err: %i\n", c->err);
  compiler_free(c);
  return 0;
}
