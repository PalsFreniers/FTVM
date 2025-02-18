#include <FTVM.hpp>
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <iostream>

void SYSCALL(FTVM::Registers &reg, unused std::stack<u32> &stk, u8 mem[MB(1)]) {
        if(reg.r1 == 1) {
                int fd = reg.r2;
                char *mem2 = (char *)(&mem[reg.r3]);
                usz len = reg.r4;
                for(usz i = 0; i < len; i++) {
                        if(fd == 1) std::cout << (char)mem2[i];
                        else if(fd == 2) std::cerr << (char)mem2[i];
                        else throw std::runtime_error("unable to find fd");
                }
        } else {
                throw std::runtime_error("unable to find syscall");
        }
}

void READ(FTVM::Registers &reg, unused std::stack<u32> &stk, unused u8 mem[MB(1)]) {
        std::cin >> reg.r1;
}

void RAND(FTVM::Registers &reg, unused std::stack<u32> &stk, unused u8 mem[MB(1)]) {
        reg.r1 = std::rand() % 100 + 1;
}

extern "C" {
        void initProg(FTVM::Program &prog) {
                std::srand(std::time(NULL));
                prog.set("SYSCALL", SYSCALL);
                prog.set("READ", READ);
                prog.set("RAND", RAND);
        }
}
