#include <FTVM.hpp>
#include <Logger.hpp>
#include <csignal>
#include <cstddef>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <utils.hpp>
#include <sha256.hpp>

namespace FTVM {
        Program::Program() : _file(NULL), _size(0), _startOffset(0), _path("") {}
        
        Program::Program(std::string path) : _file(NULL), _size(0), _startOffset(0), _path("") {
                load(path);
        }
        
        Program::Program(const Program &other) : _file(NULL), _size(0), _startOffset(0), _path("") {
                *this = other;
        }
        
        Program::~Program() {
                unload();
        }
        
        Program &FTVM::Program::operator=(const Program &other) {
                unload();
                load(other._path);
                return *this;
        }
        
        void segvHandler(int n) {
                (void)n;
                throw std::runtime_error("SegementationFaultException");
        }
        
        void Program::load(std::string path) {
                Logger _log = Logger("FTVM");
                if(path.empty()) throw std::invalid_argument("empty file path");
        
                int fd = scall(2, path.c_str(), O_RDONLY, 0);
                if(fd < 0) throw std::domain_error("unable to open file " + path);
        
                struct stat buf;
                if(scall(5, fd, &buf) < 0) {
                        close(fd);
                        throw std::runtime_error("unable to stat " + path);
                }
                if(buf.st_size <= (long)sizeof(Header)) {
                        close(fd);
                        throw std::length_error("file does not contain enough data to be loaded");
                }
        
                void *tmp = (void *)scall(9, NULL, buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
                close(fd);
        
                if(tmp == MAP_FAILED) throw std::runtime_error("unable to create memory mappings for file " + path);
                
                _file = (u8 *)tmp;
                _size = buf.st_size;
                _path = path;
        
                std::signal(SIGSEGV, segvHandler);
                Header *h = (Header *)_file;
                try {
                        if(h->magic[0] != 'F' || h->magic[1] != 'T' || h->magic[2] != 'V' || h->magic[3] != 'M') throw std::runtime_error("BadFileFormatMagicException");
                        bool SHEviewed = false, SHFviewed = false;
                        for(usz i = 0; i < h->segmentNumber; i++) {
                                SegmentHeader *sh = &((SegmentHeader *)(((u8 *)h) + h->segmentsTable))[i];
                                if(sh->type == SegmentType::FUNCT) {
                                        if(SHFviewed) throw std::runtime_error("MultipleSegmentDefinitionException");
                                        _startOffset = ((FunctHeader *)(((u8 *)h) + sh->off))[h->entry].off;
                                        SHFviewed = true;
                                }
                                if(sh->type == SegmentType::EXTRN) {
                                        if(SHEviewed) throw std::runtime_error("MultipleSegmentDefinitionException");
                                        SHEviewed = true;
                                        for(usz j = 0; j < sh->len; j++) {
                                                ExtrnHash hash = {0};
                                                sha256((char *)(((u8 *)h) + ((ExtrnHeader *)(((u8 *)h) + sh->off))->name), hash.hash);
                                                _extrns[hash] = NULL;
                                        }
                                }
                        }
                        if(_startOffset == 0) throw std::runtime_error("NoEntryException");
                } catch(std::exception &e) {
                        unload();
                        _log.log(ERROR, "unable to load FTVM bytecode : `/s`", e.what());
                        std::signal(SIGSEGV, SIG_DFL);
                        throw std::runtime_error("unable to load " + path);
                }
        }
        
        void Program::launch() {
                for(auto it = _extrns.begin(); it != _extrns.end(); it++) {
                        if(it->second == NULL) throw std::runtime_error("extern function not loaded");
                }
        }
        
        void Program::unload() {
                if(_file) {
                        scall(11, _file, _size);
                        _file = NULL;
                }
                _size = 0;
                _path = "";
                _startOffset = 0;
                _extrns.clear();
        }

        void compile(std::string path, std::string outPath) {
                Logger _log("FTVMCompiler");
                std::ifstream input;
                try {
                        input.open(path);
                } catch(std::exception &e) {

                }
        }
}
