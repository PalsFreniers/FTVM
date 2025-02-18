#include <SharedObject.hpp>
#include <stdexcept>
#include <utils.hpp>
namespace std {
#include <dlfcn.h>
}

SharedObject::SharedObject() : _lib(""), _obj(nullptr) {}

SharedObject::SharedObject(std::string lib) : _lib(lib), _obj(nullptr) {
        load(lib);
}

SharedObject::SharedObject(const SharedObject &other) : _lib(other._lib), _obj(nullptr) {
        *this = other;
}

SharedObject::~SharedObject() {
        unload();
}

SharedObject &SharedObject::operator=(const SharedObject &other) {
        unload();
        load(other._lib);
        return *this;
}

void SharedObject::load(std::string lib) {
        _obj = std::dlopen(lib.c_str(), RTLD_LAZY);
        _lib = lib;
}

void SharedObject::unload() {
        if(_obj) std::dlclose(_obj);

}

void *SharedObject::get(std::string fn) {
        if(_obj == nullptr) throw std::runtime_error("object not loaded");
        void *tmp = std::dlsym(_obj, fn.c_str());
        if(tmp == nullptr) throw std::runtime_error("unable to find `" + fn + "`");
        return tmp;
}

SharedObject::operator bool() {
        return _obj != nullptr;
}
