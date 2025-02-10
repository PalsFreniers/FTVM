#include <FTVM.hpp>
#include <Logger.hpp>
#include <exception>
#include <utils.hpp>
#include <iostream>

void test(unused FTVM::Registers r, unused std::stack<u32> &s) {
        std::cout << "Hello World!" << std::endl;
}

int main() {
        try {
                FTVM::compile("test.ftas", "test");
                FTVM::Program p = FTVM::Program("test");
                p.set("test", test);
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
