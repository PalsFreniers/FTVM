#pragma once

#include <string>

class SharedObject {
public:
        SharedObject();
        SharedObject(std::string lib);
        SharedObject(const SharedObject &other);
        ~SharedObject();
        SharedObject &operator=(const SharedObject &other);

        operator bool();

        void load(std::string lib);
        void unload();

        void *get(std::string fn);
private:
        std::string _lib;
        void *_obj;
};
