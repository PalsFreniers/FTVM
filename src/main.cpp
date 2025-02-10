#include <FTVM.hpp>
#include <Logger.hpp>
#include <exception>
#include <utils.hpp>
#include <iostream>

void test(FTVM::Registers &r, unused std::stack<u32> &s) {
        std::cout << r.r1 << std::endl;
}

int main() {
        try {
                FTVM::compile("test.ftas", "test");
                FTVM::Program p = FTVM::Program("test");
                p.set("putd", test);
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
