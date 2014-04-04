'''A shell to run Gunderscript code as you write it.
author: Kai Smith
email: kjs108@case.edu
'''

from subprocess import Popen, PIPE

def run(program, program_name, printout):
    program_file = open(program_name, "w")
    program_file.write(program)
    program_file.close()
    output = Popen(["./gunderscript", "main", program_name], stdout = PIPE).stdout.read()
    if "Compiler Error" in output:
        return False
    if printout:
        print("\n".join(output.split("\n")[3:-2]))
    return True

if __name__ == "__main__":
    program_name = "shell_prog.gxs"
    program = [["function exported main(){"]]
    openparens = 0
    openbrackets = 0
    openbraces = 0
    tabwidth = 2
    inblock = False
    block = []
    while True:
        if not inblock:
            i = raw_input(">>> "+tabwidth*(openparens+openbrackets+openbraces)*" ")
        else:
            i = raw_input("... "+tabwidth*(openparens+openbrackets+openbraces)*" ")
        if i.lower() == "end" and not inblock:
            break
        elif i.lower() == "run" and not inblock:
            run("\n".join(sum(program, []))+"\n}", program_name)
        elif i.lower() == "view block":
            print("\n".join(block))
        elif i.lower() == "reset block":
            block = []
            inblock = False
        elif i[:5].lower() == "save ":
            try:
                save_file = open(i[5:], "w")
                save_file.write("\n".join(sum(program, []))+"\n}")
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
            elif openparens < 0 or openbrackets < 0 or openbraces < 0:
                print("Mismatched parens. Last line not written, enter \"reset block\" to start whole block over.")
            else:
                inblock = False
                block.append(i)
                program.append(block)
                success = run("\n".join(sum(program, []))+"\n}", program_name, "sys_print(" in "".join(block))
                if not success:
                    print("Not valid gunderscript code. Last line of block deleted. Enter \"reset block\" to start whole block over.")
                    block = block[:-1]
                    program = program[:-1]
                else:
                    block = []
                    for n, line in enumerate(program[-1]):
                        if "sys_print(" in line:
                            program[-1][n] = "//"+line

    Popen(["rm", program_name])
