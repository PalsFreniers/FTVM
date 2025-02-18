#include <FTVM.hpp>
#include <Logger.hpp>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <sys/syscall.h>
#include <utils.hpp>
#include <iostream>
#include <SharedObject.hpp>

void usage(std::string prog) {
        std::cout << "Usage: " << prog << "<command> [OPTIONS]" << std::endl;
        std::cout << "COMMANDS:" << std::endl;
        std::cout << "      com [file.ftas] -> compile the file \"file.ftas\"" << std::endl;
        std::cout << "      sim [file]      -> execute the file" << std::endl;
        std::cout << "      bug [file]      -> execute the file in debug mode" << std::endl;
        std::cout << "OPTIONS:" << std::endl;
        std::cout << "      lib [funcs.so]  -> executes the initProg() function from funcs.so" << std::endl;
}

int main(int c, char *args[]) {
        if(c != 3 && c != 5) {
                usage(args[0]);
                return 1;
        }
        std::string command = args[1];
        std::string file = args[2];
        SharedObject so = SharedObject();
        if(c == 5) {
                if(args[3] != std::string("lib")) {
                        usage(args[0]);
                        return 1;
                }
                so = SharedObject(args[4]); 
                if(!so) {
                        Logger().log(LOG_ERROR, "unable to load library `/s`", args[4]);
                        return 1;
                }
        }
        FTVM::Program prog;
        if(command == "com") {
                FTVM::compile(file, "out.a");
        } else if(command == "sim") {
                try {
                        prog.load(file);
                        if(so) {
                                void (*func)(FTVM::Program &) = (void (*)(FTVM::Program &))so.get("initProg");
                                func(prog);
                        }
                        prog.launch();
                        while(prog) prog.step();
                } catch(std::exception &e) {
                        Logger().log(LOG_ERROR, "got exception : `/s`", e.what());
                        return 1;
                }
        } else if(command == "bug") {
                try {
                        prog.load(file);
                        if(so) {
                                void (*func)(FTVM::Program &) = (void (*)(FTVM::Program &))so.get("initProg");
                                func(prog);
                        }
                        prog.launch();
                        while(prog) {
                                prog.step();
                                prog.show();
                                getchar();
                        }
                } catch(std::exception &e) {
                        Logger().log(LOG_ERROR, "got exception : `/s`", e.what());
                        return 1;
                }
        } else {
                usage(args[0]);
                return 1;
        }
        return 0;
}
