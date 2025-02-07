#pragma once

#include <string>

namespace FTVM {
        class Program {
        public:
                Program();
                Program(std::string path);
                Program(const Program &other);
                ~Program();
                Program &operator=(const Program &other);
                void load(std::string path);
        private:
                unsigned char *file;
        };
}
