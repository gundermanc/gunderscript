'''A shell to run Gunderscript code as you write it.
author: Kai Smith
email: kjs108@case.edu
'''

from subprocess import Popen, PIPE
import readline

def assemble(depends, functions, program):
    p = "\n".join(depends)+"\n"
    p+= "\n".join(sum(functions, []))+"\n"
    p+= "\n".join(sum(program, []))+"\n}"
    return p

def run(program, program_name, prevout):
    program_file = open(program_name, "w")
    program_file.write(program)
    program_file.close()
    output = Popen(["./gunderscript", "main", program_name], stdout = PIPE).stdout.read()
    realout = "\n".join(output.split("\n")[3:-2])
    if prevout == realout[:len(prevout)]:
        realout = realout[len(prevout):]
    if "Compiler Error" in output:
        print("\n".join(output.split("\n")[5:]))
        return (False, False)
    elif len(realout) > 0:
        print(realout)
    return (True, "\n".join(output.split("\n")[3:-2]))

if __name__ == "__main__":
    program_name = "shell_prog.gxs"
    program = [["function exported main(){"]]
    outlines = ""
    openparens = 0
    openbrackets = 0
    openbraces = 0
    tabwidth = 2
    inblock = False
    funcdef = False
    depends= []
    functions = []
    block = []
    while True:
        if not inblock:
            i = raw_input(">>> "+tabwidth*(openparens+openbrackets+openbraces)*" ")
        else:
            i = raw_input("... "+tabwidth*(openparens+openbrackets+openbraces)*" ")
        if i[:9].lower() == "function " and block == []:
            funcdef = True
        if i[:8].lower() == "depends ":
            depends.append(i)
        if i.lower() == "end" and not inblock:
            break
        elif i.lower() == "run" and not inblock:
            run(assemble(depends, functions, program), program_name, outlines)
        elif i.lower() == "view block":
            print("\n".join(block))
        elif i.lower() == "reset block":
            block = []
            inblock = False
            openparens = 0
            openbrackets = 0
            openbraces = 0
        elif i[:5].lower() == "save ":
            try:
                save_file = open(i[5:], "w")
                save_file.write(assemble(depends, functions, program))
                save_file.close()
            except:
                print("Error saving. Likely an invalid file name.")
        else:
            openparens = openparens + i.count("(") - i.count(")")
            openbrackets = openbrackets + i.count("[") - i.count("]")
            openbraces = openbraces + i.count("{") - i.count("}")
            if openparens > 0 or openbrackets > 0 or openbraces > 0:
                inblock = True
                block.append(tabwidth*(openparens+openbrackets+openbraces)*" "+i)
            elif openparens < 0:
                print("Mismatched parentheses. Last line not written, enter \"reset block\" to start whole block over.")
            elif openbrackets < 0:
                print("Mismatched brackets. Last line not written, enter \"reset block\" to start whole block over.")
            elif openbraces < 0:
                print("Mismatched braces. Last line not written, enter \"reset block\" to start whole block over.")
            else:
                inblock = False
                block.append(i)
                if funcdef:
                    functions.append(block)
                else:
                    program.append(block)
                results = run(assemble(depends, functions, program), program_name, outlines)
                success = results[0]
                if results[1] != False:
                    outlines = results[1]
                if not success:
                    block = []
                    inblock = False
                    openparens = 0
                    openbrackets = 0
                    openbraces = 0
                    if funcdef:
                        functions = functions[:-1]
                    else:
                        program = program[:-1]
                else:
                    block = []
                funcdef = False

    Popen(["rm", program_name])
