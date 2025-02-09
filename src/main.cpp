#include <FTVM.hpp>
#include <Logger.hpp>
#include <exception>

int main() {
        try {
                FTVM::compile("test.ftas", "test");
                FTVM::Program p = FTVM::Program("test");
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
