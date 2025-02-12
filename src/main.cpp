#include <FTVM.hpp>
#include <Logger.hpp>
#include <cstdio>
#include <exception>
#include <utils.hpp>
#include <iostream>

// Fonction externe TEST : affiche la valeur de R1
void TEST(FTVM::Registers &r, unused std::stack<u32> &s) {
        std::cout << "[EXTERN TEST] R1 = " << r.r1 << std::endl;
}

// Fonction externe PRINT : affiche la valeur de R3
void PRINT(FTVM::Registers &r, unused std::stack<u32> &s) {
        std::cout << "[EXTERN PRINT] R3 = " << r.r3 << std::endl;
}

int main() {
        try {
                FTVM::compile("test.ftas", "test");
                FTVM::Program p = FTVM::Program("test");
                p.set("TEST", TEST);
                p.set("PRINT", PRINT);
                p.launch();
                while(p) {
                        p.step();
                }
                p.show();
        } catch(std::exception &e) {
                Logger().log(LOG_ERROR, "catched exception /s", e.what());
        }
        return 0;
}
