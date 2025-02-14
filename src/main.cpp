#include <FTVM.hpp>
#include <Logger.hpp>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <stdexcept>
#include <sys/syscall.h>
#include <utils.hpp>
#include <iostream>

void SYSCALL(FTVM::Registers &r, unused std::stack<u32> &s, u8 mem[MB(1)]) {
        switch (r.r1) {
                case SYS_write: {
                        int fd = r.r2;
                        void *addr = &(mem[r.r3]);
                        int len = r.r4;
                        scall(SYS_write, fd, addr, len);
                        } break;
                default: throw std::runtime_error("todo: syscall `" + to_string(r.r1) + "`");
        }
}

void READ(FTVM::Registers &r, unused std::stack<u32> &s, unused u8 mem[MB(1)]) {
        std::cin >> r.r1;
}

void RAND(FTVM::Registers &r, unused std::stack<u32> &s, unused u8 mem[MB(1)]) {
        r.r1 = rand() % 100 + 1;
}

int main() {
        std::srand(std::time(NULL));
        try {
                FTVM::compile("test2.ftas", "test");
                FTVM::Program p = FTVM::Program("test");
                p.set("SYSCALL", SYSCALL);
                p.set("READ", READ);
                p.set("RAND", RAND);
                p.launch();
                while(p) {
                        p.step();
                        //p.show();
                        //getchar();
                }
                //p.show();
        } catch(std::exception &e) {
                Logger().log(LOG_ERROR, "catched exception /s", e.what());
        }
        return 0;
}
