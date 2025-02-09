#include <FTVM.hpp>
#include <Logger.hpp>
#include <csignal>
#include <cstddef>
#include <exception>
#include <fstream>
#include <stack>
#include <stdexcept>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <utils.hpp>
#include <sha256.hpp>
#include <vector>
#include <iostream>

namespace FTVM {
        bool ExtrnHash::operator<(const ExtrnHash &other) const {
                return hash[0] < other.hash[0] &&
                       hash[1] < other.hash[1] &&
                       hash[2] < other.hash[2] &&
                       hash[3] < other.hash[3] &&
                       hash[4] < other.hash[4] &&
                       hash[5] < other.hash[5] &&
                       hash[6] < other.hash[6] &&
                       hash[7] < other.hash[7];
        }

        Header::Header() {
                magic[0] = 'F';
                magic[1] = 'T';
                magic[2] = 'V';
                magic[3] = 'M';
                segmentNumber = 1;
                segmentsTable = 0;
                entry = sizeof(Header);
        }

        Program::Program() : _file(NULL), _size(0), _startOffset(0), _path(""), _launched(false), _valid(false) {}
        
        Program::Program(std::string path) : _file(NULL), _size(0), _startOffset(0), _path(""), _launched(false), _valid(false)  {
                load(path);
        }
        
        Program::Program(const Program &other) : _file(NULL), _size(0), _startOffset(0), _path(""), _launched(false), _valid(false)  {
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

        Program::operator bool() {
                return _launched && _valid;
        }
        
        void segvHandler(int n) {
                (void)n;
                throw std::runtime_error("SegementationFaultException");
        }
        
        void Program::load(std::string path) {
                Logger _log = Logger(path + " FTVM");
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
        
                void *tmp = (void *)scall(9L, NULL, buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0L);
                close(fd);
        
                if(tmp == MAP_FAILED || tmp == NULL) throw std::runtime_error("unable to create memory mappings for file " + path);
                
                _file = (u8 *)tmp;
                _size = buf.st_size;
                _path = path;
        
                std::signal(SIGSEGV, segvHandler);
                Header *h = (Header *)_file;
                try {
                        if(h->magic[0] != 'F' || h->magic[1] != 'T' || h->magic[2] != 'V' || h->magic[3] != 'M') throw std::runtime_error("BadFileFormatMagicException");
                        if(h->segmentNumber != 0 && h->segmentsTable < sizeof(Header)) throw std::runtime_error("BadSectionsDefinitionException");
                        if(h->entry < sizeof(Header)) throw std::runtime_error("NoEntryException");
                        _startOffset = h->entry;
                        bool SHEviewed = false;
                        for(usz i = 0; i < h->segmentNumber; i++) {
                                SegmentHeader *sh = &((SegmentHeader *)(((u8 *)h) + h->segmentsTable))[i];
                                if(sh->type == ST_EXTRN) {
                                        if(SHEviewed) throw std::runtime_error("MultipleSegmentDefinitionException");
                                        SHEviewed = true;
                                        for(usz j = 0; j < sh->len; j++) {
                                                const ExtrnHash hash = {0};
                                                sha256((char *)(((u8 *)h) + ((ExtrnHeader *)(((u8 *)h) + sh->off))->name), (u32 *)hash.hash);
                                                _extrns[hash] = NULL;
                                        }
                                }
                        }
                } catch(std::exception &e) {
                        unload();
                        _log.log(LOG_ERROR, "unable to load FTVM bytecode : `/s`", e.what());
                        std::signal(SIGSEGV, SIG_DFL);
                        throw std::runtime_error("unable to load " + path);
                }
                std::signal(SIGSEGV, SIG_DFL);
                _valid = true;
        }
        
        void Program::launch() {
                if(!_valid) throw std::runtime_error("Program " + _path + " not loaded");
                if(_launched) return;
                for(auto it = _extrns.begin(); it != _extrns.end(); it++) if(it->second == NULL) throw std::runtime_error("extern function not loaded");
                _regs.rip = 0;
                _regs.r1 = 0;
                _regs.r2 = 0;
                _regs.r3 = 0;
                _regs.r4 = 0;
                _regs.r5 = 0;
                _regs.r6 = 0;
                _launched = true;
        }

        u32 &Program::_getRegisterValue(u8 reg) {
                switch(reg) {
                        case REG_R1: return _regs.r1;
                        case REG_R2: return _regs.r2;
                        case REG_R3: return _regs.r3;
                        case REG_R4: return _regs.r4;
                        case REG_R5: return _regs.r5;
                        case REG_R6: return _regs.r6;
                        case REG_RIP: return _regs.rip;
                        default: throw std::runtime_error("UnknownRegisterException");
                }
        }

        void Program::step() {
                Logger _log = Logger(_path + " step");
                if(!_launched || !_valid) throw std::runtime_error("Program " + _path + " not loaded/launched");
                u64 instr = *(u64 *)(_file + _startOffset + (_regs.rip * sizeof(u64)));
                switch(getSuperInstruction(instr)) {
                        case INSTRUCTION_NOP:
                                break;
                        case INSTRUCTION_PUSH:
                                if(getInstructionSpec(instr) == SPEC_IMM) _execStack.push(getInstructionImm(instr));
                                else if(getInstructionSpec(instr) == SPEC_REG) _execStack.push(_getRegisterValue(getInstructionRegX(instr, 1)));
                                else throw std::runtime_error("UnknownInstructionException");
                                break;
                        case INSTRUCTION_POP:
                                if(getInstructionSpec(instr) == SPEC_REG) {
                                        _getRegisterValue(getInstructionRegX(instr, 1)) = _execStack.top();
                                        _execStack.pop();
                                }
                                else throw std::runtime_error("UnknownInstructionException");
                                break;
                        case INSTRUCTION_END:
                                _launched = false;
                                break;
                        default:
                                throw std::runtime_error("UnknownInstructionException");
                }
                _regs.rip++;
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
                while(_execStack.size() > 0) _execStack.pop();
                _launched = false;
                _valid = false;
        }

        void Program::show() {
                std::stack<u32> tmp;
                std::cout << "Registers : {" << std::endl;
                std::cout << "\trip: " << _regs.rip << std::endl;
                std::cout << "\tr1: " << _regs.r1 << std::endl;
                std::cout << "\tr2: " << _regs.r2 << std::endl;
                std::cout << "\tr3: " << _regs.r3 << std::endl;
                std::cout << "\tr4: " << _regs.r4 << std::endl;
                std::cout << "\tr5: " << _regs.r5 << std::endl;
                std::cout << "\tr6: " << _regs.r6 << std::endl;
                std::cout << "\tstack: [ ";
                while(_execStack.size() > 0) {
                        std::cout << _execStack.top() << " ";
                        tmp.push(_execStack.top());
                        _execStack.pop();
                }
                std::cout << "]" << std::endl;
                std::cout << "}" << std::endl;
                while(!tmp.empty()) {
                        _execStack.push(tmp.top());
                        tmp.pop();
                }
        }

        void compile(std::string path, unused std::string outPath) {
                Logger _log("FTVMCompiler");
                if(path.length() < 5 ||
                                path[path.length() - 1] != 's' ||
                                path[path.length() - 2] != 'a' ||
                                path[path.length() - 3] != 't' ||
                                path[path.length() - 4] != 'f' ||
                                path[path.length() - 5] != '.') throw std::runtime_error("BadExtentionException");
                if(outPath.empty()) throw std::runtime_error("EmptyFileException");
                std::ifstream input;
                std::ofstream output;
                input.exceptions(std::ifstream::badbit);
                output.exceptions(std::ofstream::badbit);
                int lne = 0;
                std::vector<u64> prog;
                std::map<std::string, u64> labels;
                try {
                        prog.push_back(0L);
                        input.open(path.c_str());
                        std::string l;
                        while(std::getline(input, l)) {
                                lne++;
                                if(l.empty() || l[0] == '\n') continue;
                                std::vector<std::string> line = split(split(l, "--")[0], " ");
                                if(line[0] == "PUSH") {
                                        if(line.size() != 2) throw std::runtime_error(compilerError("push imm instruction need 1 arguments but got " + to_string(line.size() - 1)));
                                        try {
                                                int val = to_int(line[1].c_str());
                                                prog.push_back(buildPUSHiInstruction(val));
                                        } catch(std::exception &e) {
                                                if(line[1] == "R1")       prog.push_back(buildPUSHrInstruction(REG_R1));
                                                else if(line[1] == "R2")  prog.push_back(buildPUSHrInstruction(REG_R2));
                                                else if(line[1] == "R3")  prog.push_back(buildPUSHrInstruction(REG_R3));
                                                else if(line[1] == "R4")  prog.push_back(buildPUSHrInstruction(REG_R4));
                                                else if(line[1] == "R5")  prog.push_back(buildPUSHrInstruction(REG_R5));
                                                else if(line[1] == "R6")  prog.push_back(buildPUSHrInstruction(REG_R6));
                                                else if(line[1] == "RIP") prog.push_back(buildPUSHrInstruction(REG_RIP));
                                                else throw std::runtime_error(compilerError("unable to parse register or value `" + line[1] + "`"));
                                        }
                                } else if(line[0] == "POP") {
                                        if(line.size() != 2) throw std::runtime_error(compilerError("pop instruction need 1 arguments but got " + to_string(line.size() - 1)));
                                        if(line[1] == "R1")       prog.push_back(buildPOPInstruction(REG_R1));
                                        else if(line[1] == "R2")  prog.push_back(buildPOPInstruction(REG_R2));
                                        else if(line[1] == "R3")  prog.push_back(buildPOPInstruction(REG_R3));
                                        else if(line[1] == "R4")  prog.push_back(buildPOPInstruction(REG_R4));
                                        else if(line[1] == "R5")  prog.push_back(buildPOPInstruction(REG_R5));
                                        else if(line[1] == "R6")  prog.push_back(buildPOPInstruction(REG_R6));
                                        else if(line[1] == "RIP") std::runtime_error("unable to override RIP, use JUMP* instructions instead");
                                        else throw std::runtime_error(compilerError("unable to get register `" + line[1] + "`"));
                                } else if(line[0] == "LABEL") {
                                        if(line.size() != 2) throw std::runtime_error(compilerError("label instruction need 1 arguments but got " + to_string(line.size() - 1)));
                                        if(labels.find(line[1]) != labels.end()) throw std::runtime_error(compilerError("forbidden reloading of label " + line[1]));
                                        labels[line[1]] = prog.size();
                                } else if(line[0] == "END") {
                                        if(line.size() != 1) throw std::runtime_error(compilerError("end instruction need no arguments but got " + to_string(line.size() - 1)));
                                        prog.push_back(buildENDInstruction());
                                } else {
                                        throw std::runtime_error(compilerError("unknown keyword `" + line[0] + "`"));
                                }
                        }
                        prog.push_back(buildENDInstruction());
                        input.close();
                        Header h = Header();
                        h.segmentsTable = sizeof(Header) + (prog.size() * sizeof(prog[0]));
                        h.segmentNumber = 0;
                        output.open(outPath.c_str());
                        output.write((const char *)&h, sizeof(h));
                        for(auto it = prog.begin(); it != prog.end(); it++) output.write((const char *)&(*it), sizeof(*it));
                        output.close();
                } catch(std::exception &e) {
                        _log.log(LOG_ERROR, "unable to compile /s: /s", path.c_str(), e.what());
                        throw std::runtime_error("Compile Error");
                }
        }
}
