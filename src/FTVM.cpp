#include <FTVM.hpp>
#include <Logger.hpp>
#include <csignal>
#include <cstddef>
#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
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
#include <algorithm>

namespace FTVM {
        bool ExtrnHash::operator<(const ExtrnHash &other) const {
                for (int i = 0; i < 8; ++i) {
                        if (hash[i] != other.hash[i])
                                return hash[i] < other.hash[i];
                }
                return false;
        }

        bool ExtrnHash::operator==(const ExtrnHash &other) const {
                for (int i = 0; i < 8; ++i) {
                        if (hash[i] != other.hash[i])
                                return false;
                }
                return true;
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

        Program::Program() : _file(NULL), _size(0), _startOffset(0), _extrnTableOff(NULL), _path(""), _launched(false), _valid(false) {}
        
        Program::Program(std::string path) : _file(NULL), _size(0), _startOffset(0), _extrnTableOff(NULL), _path(""), _launched(false), _valid(false)  {
                load(path);
        }
        
        Program::Program(const Program &other) : _file(NULL), _size(0), _startOffset(0), _extrnTableOff(NULL), _path(""), _launched(false), _valid(false)  {
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
                                        _extrnTableOff = (ExtrnHeader *)(((u8 *)h) + sh->off);
                                        SHEviewed = true;
                                        for(usz j = 0; j < sh->len; j++) {
                                                const ExtrnHash hash = {0};
                                                sha256((char *)(((u8 *)h) + ((ExtrnHeader *)(((u8 *)h) + sh->off))[j].name), (u32 *)hash.hash);
                                                _extrns[hash] = NO_FUNC;
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

        void Program::set(std::string name, extrn func) {
                const ExtrnHash h = {0};
                sha256(name, (u32 *)h.hash);
                _extrns[h] = func;
        }
        
        void Program::launch() {
                if(!_valid) throw std::runtime_error("Program " + _path + " not valid");
                if(_launched) return;
                for(auto it = _extrns.begin(); it != _extrns.end(); it++) if(it->second == NO_FUNC) throw std::runtime_error("extern function not loaded");
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

        u32 Program::_pop() {
                if(_execStack.empty()) throw std::runtime_error("EmptyStackException");
                u32 val = _execStack.top();
                _execStack.pop();
                return val;
        }

        void Program::step() {
                Logger _log = Logger(_path + " step");
                if(!_valid) throw std::runtime_error("Program " + _path + " not valid");
                if(!_launched) throw std::runtime_error("Program " + _path + " not launched");
                u64 instr = *(u64 *)(_file + _startOffset + (_regs.rip * sizeof(u64)));
                switch(getSuperInstruction(instr)) {
                        case INSTRUCTION_NOP:
                                break;
                        case INSTRUCTION_PUSH:
                                if(getInstructionSpec(instr) == SPEC_IMM) _execStack.push(getInstructionImm(instr));
                                else if(getInstructionSpec(instr) == SPEC_REG) _execStack.push(_getRegisterValue(getInstructionRegX(instr, 1)));
                                else throw std::runtime_error("UnknownSpecException");
                                break;
                        case INSTRUCTION_POP:
                                if(getInstructionSpec(instr) == SPEC_REG) _getRegisterValue(getInstructionRegX(instr, 1)) = _pop();
                                else throw std::runtime_error("UnknownSpecException");
                                break;
                        case INSTRUCTION_CALL: {
                                u32 off = getInstructionImm(instr);
                                std::string name = (char *)_file + _extrnTableOff[off].name;
                                const ExtrnHash h = {0};
                                sha256(name, (u32 *)h.hash);
                                auto it = _extrns.find(h);
                                if(it == _extrns.end()) throw std::runtime_error("UnknownExternException");
                                it->second(_regs, _execStack);
                                } break;
                        case INSTRUCTION_ADD:
                        if(getInstructionSpec(instr) == SPEC_REG) {
                                u32 &x = _getRegisterValue(getInstructionRegX(instr, 1));
                                u32 &y = _getRegisterValue(getInstructionRegX(instr, 2));
                                x += y;
                        } else if(getInstructionSpec(instr) == SPEC_IMM) {
                                u32 y = _pop();
                                u32 x = _pop();
                                x += y;
                                _execStack.push(x);
                        } else throw std::runtime_error("UnknownSpecException");
                        break;
                        case INSTRUCTION_SUB:
                        if(getInstructionSpec(instr) == SPEC_REG) {
                                u32 &x = _getRegisterValue(getInstructionRegX(instr, 1));
                                u32 &y = _getRegisterValue(getInstructionRegX(instr, 2));
                                x -= y;
                        } else if(getInstructionSpec(instr) == SPEC_IMM) {
                                u32 y = _pop();
                                u32 x = _pop();
                                x -= y;
                                _execStack.push(x);
                        } else throw std::runtime_error("UnknownSpecException");
                        break;
                        case INSTRUCTION_MUL:
                        if(getInstructionSpec(instr) == SPEC_REG) {
                                u32 &x = _getRegisterValue(getInstructionRegX(instr, 1));
                                u32 &y = _getRegisterValue(getInstructionRegX(instr, 2));
                                x *= y;
                        } else if(getInstructionSpec(instr) == SPEC_IMM) {
                                u32 y = _pop();
                                u32 x = _pop();
                                x *= y;
                                _execStack.push(x);
                        } else throw std::runtime_error("UnknownSpecException");
                        break;
                        case INSTRUCTION_DIV:
                        if(getInstructionSpec(instr) == SPEC_REG) {
                                u32 &x = _getRegisterValue(getInstructionRegX(instr, 1));
                                u32 &y = _getRegisterValue(getInstructionRegX(instr, 2));
                                x /= y;
                        } else if(getInstructionSpec(instr) == SPEC_IMM) {
                                u32 y = _pop();
                                u32 x = _pop();
                                x /= y;
                                _execStack.push(x);
                        } else throw std::runtime_error("UnknownSpecException");
                        break;
                        case INSTRUCTION_MOD:
                        if(getInstructionSpec(instr) == SPEC_REG) {
                                u32 &x = _getRegisterValue(getInstructionRegX(instr, 1));
                                u32 &y = _getRegisterValue(getInstructionRegX(instr, 2));
                                x %= y;
                        } else if(getInstructionSpec(instr) == SPEC_IMM) {
                                u32 y = _pop();
                                u32 x = _pop();
                                x %= y;
                                _execStack.push(x);
                        } else throw std::runtime_error("UnknownSpecException");
                        break;
                        case INSTRUCTION_JNE: {
                        u32 x, y;
                        if(getInstructionSpec(instr) == SPEC_REG) {
                                x = _getRegisterValue(getInstructionRegX(instr, 1));
                                y = _getRegisterValue(getInstructionRegX(instr, 2));
                        } else if(getInstructionSpec(instr) == SPEC_IMM) {
                                y = _pop();
                                x = _pop();
                        } else throw std::runtime_error("UnknownSpecException");
                        if(x != y) _regs.rip = _pop() - 1;
                        } break;
                        case INSTRUCTION_JE: {
                        u32 x, y;
                        if(getInstructionSpec(instr) == SPEC_REG) {
                                x = _getRegisterValue(getInstructionRegX(instr, 1));
                                y = _getRegisterValue(getInstructionRegX(instr, 2));
                        } else if(getInstructionSpec(instr) == SPEC_IMM) {
                                y = _pop();
                                x = _pop();
                        } else throw std::runtime_error("UnknownSpecException");
                        if(x == y) _regs.rip = _pop() - 1;
                        } break;
                        case INSTRUCTION_JG: {
                        u32 x, y;
                        if(getInstructionSpec(instr) == SPEC_REG) {
                                x = _getRegisterValue(getInstructionRegX(instr, 1));
                                y = _getRegisterValue(getInstructionRegX(instr, 2));
                        } else if(getInstructionSpec(instr) == SPEC_IMM) {
                                y = _pop();
                                x = _pop();
                        } else throw std::runtime_error("UnknownSpecException");
                        if(x > y) _regs.rip = _pop() - 1;
                        } break;
                        case INSTRUCTION_JL: {
                        u32 x, y;
                        if(getInstructionSpec(instr) == SPEC_REG) {
                                x = _getRegisterValue(getInstructionRegX(instr, 1));
                                y = _getRegisterValue(getInstructionRegX(instr, 2));
                        } else if(getInstructionSpec(instr) == SPEC_IMM) {
                                y = _pop();
                                x = _pop();
                        } else throw std::runtime_error("UnknownSpecException");
                        if(x < y) _regs.rip = _pop() - 1;
                        } break;
                        case INSTRUCTION_JGE: {
                        u32 x, y;
                        if(getInstructionSpec(instr) == SPEC_REG) {
                                x = _getRegisterValue(getInstructionRegX(instr, 1));
                                y = _getRegisterValue(getInstructionRegX(instr, 2));
                        } else if(getInstructionSpec(instr) == SPEC_IMM) {
                                y = _pop();
                                x = _pop();
                        } else throw std::runtime_error("UnknownSpecException");
                        if(x >= y) _regs.rip = _pop() - 1;
                        } break;
                        case INSTRUCTION_JLE: {
                        u32 x, y;
                        if(getInstructionSpec(instr) == SPEC_REG) {
                                x = _getRegisterValue(getInstructionRegX(instr, 1));
                                y = _getRegisterValue(getInstructionRegX(instr, 2));
                        } else if(getInstructionSpec(instr) == SPEC_IMM) {
                                y = _pop();
                                x = _pop();
                        } else throw std::runtime_error("UnknownSpecException");
                        if(x <= y) _regs.rip = _pop() - 1;
                        } break;
                        case INSTRUCTION_JMP:
                                _regs.rip = getInstructionImm(instr) - 1;
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

        const char *getInstructionName(u64 instr) {
                switch(getSuperInstruction(instr)) {
                        case INSTRUCTION_NOP:  return "NOP";
                        case INSTRUCTION_PUSH: return "PUSH";
                        case INSTRUCTION_POP:  return "POP";
                        case INSTRUCTION_CALL: return "CALL";
                        case INSTRUCTION_ADD:  return "ADD";
                        case INSTRUCTION_SUB:  return "SUB";
                        case INSTRUCTION_MUL:  return "MUL";
                        case INSTRUCTION_DIV:  return "DIV";
                        case INSTRUCTION_MOD:  return "MOD";
                        case INSTRUCTION_JNE:  return "JNE";
                        case INSTRUCTION_JE:   return "JE";
                        case INSTRUCTION_JG:   return "JG";
                        case INSTRUCTION_JL:   return "JL";
                        case INSTRUCTION_JGE:  return "JGE";
                        case INSTRUCTION_JLE:  return "JLE";
                        case INSTRUCTION_JMP:  return "JMP";
                        case INSTRUCTION_END:  return "END";
                        default:               return "UNKNOWN";
                }
        }

        const char *getInstructionSpecName(u64 instr) {
                switch(getInstructionSpec(instr)) {
                        case SPEC_REG: return "REGISTER";
                        case SPEC_IMM: return "IMMEDIATE";
                        default: return "UNKNOWN";
                }
        }

        void Program::show() {
                std::stack<u32> tmp;
                if(!_launched || !_valid) throw std::runtime_error("Program " + _path + " not loaded/launched");
                u64 instr = *(u64 *)(_file + _startOffset + (_regs.rip * sizeof(u64)));
                std::cout << "Instruction: 0x" << std::setfill('0') << std::setw(16) << std::hex << instr << std::dec;
                std::cout << " (" << getInstructionName(instr) << "<" << getInstructionSpecName(instr) << ">" << ")" << std::endl;
                std::cout << "Registers :: {" << std::endl;
                std::cout << "\trip: " << _regs.rip << std::endl;
                std::cout << "\tr1: " << _regs.r1 << std::endl;
                std::cout << "\tr2: " << _regs.r2 << std::endl;
                std::cout << "\tr3: " << _regs.r3 << std::endl;
                std::cout << "\tr4: " << _regs.r4 << std::endl;
                std::cout << "\tr5: " << _regs.r5 << std::endl;
                std::cout << "\tr6: " << _regs.r6 << std::endl;
                std::cout << "}" << std::endl;
                std::cout << "stack :: [ ";
                while(_execStack.size() > 0) {
                        std::cout << _execStack.top() << " ";
                        tmp.push(_execStack.top());
                        _execStack.pop();
                }
                std::cout << "]" << std::endl;
                while(!tmp.empty()) {
                        _execStack.push(tmp.top());
                        tmp.pop();
                }
        }

        u32 parseIntOrReg(std::string arg, int lne, bool &isReg) {
                int val = 0;
                try {
                        val = to_int(arg.c_str());
                } catch(std::exception &e) {
                        isReg = true;
                        if(arg == "R1")       val = REG_R1;
                        else if(arg == "R2")  val = REG_R2;
                        else if(arg == "R3")  val = REG_R3;
                        else if(arg == "R4")  val = REG_R4;
                        else if(arg == "R5")  val = REG_R5;
                        else if(arg == "R6")  val = REG_R6;
                        else if(arg == "RIP") val = REG_RIP;
                        else throw std::runtime_error(compilerError("unable to parse register or value `" + arg + "`"));
                }
                return val;
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
                input.exceptions(std::ifstream::failbit);
                output.exceptions(std::ofstream::failbit);
                int lne = 0;
                std::vector<u64> prog;
                std::map<std::string, u64> labels;
                std::vector<std::string> extrns;
                try {
                        prog.push_back(0L);
                        input.open(path.c_str());
                        std::string l;
                        input.exceptions(std::ifstream::badbit);
                        while(std::getline(input, l)) {
                                lne++;
                                if(l.empty() || l[0] == '\n') continue;
                                if(l.length() >= 2 && l[0] == '-' && l[1] == '-') continue;
                                std::vector<std::string> line = split(split(l, "--")[0], " ");
                                if(line.empty()) continue;
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
                                        if(std::find(extrns.begin(), extrns.end(), line[1]) != extrns.end()) throw std::runtime_error(compilerError("forbidden reloading of extern " + line[1]));
                                        labels[line[1]] = prog.size();
                                } else if(line[0] == "END") {
                                        if(line.size() != 1) throw std::runtime_error(compilerError("end instruction need no arguments but got " + to_string(line.size() - 1)));
                                        prog.push_back(buildENDInstruction());
                                } else if(line[0] == "EXTERN") {
                                        if(line.size() != 2) throw std::runtime_error(compilerError("end instruction need no arguments but got " + to_string(line.size() - 1)));
                                        if(labels.find(line[1]) != labels.end()) throw std::runtime_error(compilerError("forbidden reloading of label " + line[1]));
                                        if(std::find(extrns.begin(), extrns.end(), line[1]) != extrns.end()) throw std::runtime_error(compilerError("forbidden reloading of extern " + line[1]));
                                        extrns.push_back(line[1]);
                                } else if(line[0] == "CALL") {
                                        if(line.size() != 2) throw std::runtime_error(compilerError("end instruction need no arguments but got " + to_string(line.size() - 1)));
                                        if(std::find(extrns.begin(), extrns.end(), line[1]) == extrns.end()) throw std::runtime_error(compilerError("unknown function " + line[1]));
                                        int val = std::find(extrns.begin(), extrns.end(), line[1]) - extrns.begin();
                                        prog.push_back(buildCALLInstrucion(val));
                                } else if(line[0] == "ADD") {
                                        if(line.size() != 3) throw std::runtime_error(compilerError("ADD instruction need no arguments but got " + to_string(line.size() - 1)));
                                        int val1 = 0, val2 = 0;
                                        bool isReg1 = false, isReg2 = false;
                                        val1 = parseIntOrReg(line[1], lne, isReg1);
                                        val2 = parseIntOrReg(line[2], lne, isReg2);
                                        if(isReg1 && isReg2) prog.push_back(buildADDrInstruction(val1, val2));
                                        else {
                                                if(isReg1) prog.push_back(buildPUSHrInstruction(val1));
                                                else prog.push_back(buildPUSHiInstruction(val1));
                                                if(isReg2) prog.push_back(buildPUSHrInstruction(val2));
                                                else prog.push_back(buildPUSHiInstruction(val2));
                                                prog.push_back(buildADDiInstruction());
                                                if(isReg1) prog.push_back(buildPOPInstruction(val1));
                                        }
                                } else if(line[0] == "SUB") {
                                        if(line.size() != 3) throw std::runtime_error(compilerError("SUB instruction need no arguments but got " + to_string(line.size() - 1)));
                                        int val1 = 0, val2 = 0;
                                        bool isReg1 = false, isReg2 = false;
                                        val1 = parseIntOrReg(line[1], lne, isReg1);
                                        val2 = parseIntOrReg(line[2], lne, isReg2);
                                        if(isReg1 && isReg2) prog.push_back(buildSUBrInstruction(val1, val2));
                                        else {
                                                if(isReg1) prog.push_back(buildPUSHrInstruction(val1));
                                                else prog.push_back(buildPUSHiInstruction(val1));
                                                if(isReg2) prog.push_back(buildPUSHrInstruction(val2));
                                                else prog.push_back(buildPUSHiInstruction(val2));
                                                prog.push_back(buildSUBiInstruction());
                                                if(isReg1) prog.push_back(buildPOPInstruction(val1));
                                        }
                                } else if(line[0] == "MUL") {
                                        if(line.size() != 3) throw std::runtime_error(compilerError("MUL instruction need no arguments but got " + to_string(line.size() - 1)));
                                        int val1 = 0, val2 = 0;
                                        bool isReg1 = false, isReg2 = false;
                                        val1 = parseIntOrReg(line[1], lne, isReg1);
                                        val2 = parseIntOrReg(line[2], lne, isReg2);
                                        if(isReg1 && isReg2) prog.push_back(buildMULrInstruction(val1, val2));
                                        else {
                                                if(isReg1) prog.push_back(buildPUSHrInstruction(val1));
                                                else prog.push_back(buildPUSHiInstruction(val1));
                                                if(isReg2) prog.push_back(buildPUSHrInstruction(val2));
                                                else prog.push_back(buildPUSHiInstruction(val2));
                                                prog.push_back(buildMULiInstruction());
                                                if(isReg1) prog.push_back(buildPOPInstruction(val1));
                                        }
                                } else if(line[0] == "DIV") {
                                        if(line.size() != 3) throw std::runtime_error(compilerError("DIV instruction need no arguments but got " + to_string(line.size() - 1)));
                                        int val1 = 0, val2 = 0;
                                        bool isReg1 = false, isReg2 = false;
                                        val1 = parseIntOrReg(line[1], lne, isReg1);
                                        val2 = parseIntOrReg(line[2], lne, isReg2);
                                        if(isReg1 && isReg2) prog.push_back(buildDIVrInstruction(val1, val2));
                                        else {
                                                if(isReg1) prog.push_back(buildPUSHrInstruction(val1));
                                                else prog.push_back(buildPUSHiInstruction(val1));
                                                if(isReg2) prog.push_back(buildPUSHrInstruction(val2));
                                                else prog.push_back(buildPUSHiInstruction(val2));
                                                prog.push_back(buildDIViInstruction());
                                                if(isReg1) prog.push_back(buildPOPInstruction(val1));
                                        }
                                } else if(line[0] == "MOD") {
                                        if(line.size() != 3) throw std::runtime_error(compilerError("MOD instruction need no arguments but got " + to_string(line.size() - 1)));
                                        int val1 = 0, val2 = 0;
                                        bool isReg1 = false, isReg2 = false;
                                        val1 = parseIntOrReg(line[1], lne, isReg1);
                                        val2 = parseIntOrReg(line[2], lne, isReg2);
                                        if(isReg1 && isReg2) prog.push_back(buildMODrInstruction(val1, val2));
                                        else {
                                                if(isReg1) prog.push_back(buildPUSHrInstruction(val1));
                                                else prog.push_back(buildPUSHiInstruction(val1));
                                                if(isReg2) prog.push_back(buildPUSHrInstruction(val2));
                                                else prog.push_back(buildPUSHiInstruction(val2));
                                                prog.push_back(buildMODiInstruction());
                                                if(isReg1) prog.push_back(buildPOPInstruction(val1));
                                        }
                                } else if(line[0] == "JNE") {
                                        if(line.size() != 4) throw std::runtime_error(compilerError("JNE instruction need no arguments but got " + to_string(line.size() - 1)));
                                        if(labels.find(line[1]) == labels.end()) throw std::runtime_error(compilerError("label `" + line[1] + "` not found"));
                                        int val1 = 0, val2 = 0;
                                        bool isReg1 = false, isReg2 = false;
                                        val1 = parseIntOrReg(line[2], lne, isReg1);
                                        val2 = parseIntOrReg(line[3], lne, isReg2);
                                        prog.push_back(buildPUSHiInstruction(labels[line[1]]));
                                        if(isReg1 && isReg2) prog.push_back(buildJNErInstruction(val1, val2));
                                        else {
                                                if(isReg1) prog.push_back(buildPUSHrInstruction(val1));
                                                else prog.push_back(buildPUSHiInstruction(val1));
                                                if(isReg2) prog.push_back(buildPUSHrInstruction(val2));
                                                else prog.push_back(buildPUSHiInstruction(val2));
                                                prog.push_back(buildJNEiInstruction());
                                        }
                                } else if(line[0] == "JE") {
                                        if(line.size() != 4) throw std::runtime_error(compilerError("JE instruction need no arguments but got " + to_string(line.size() - 1)));
                                        if(labels.find(line[1]) == labels.end()) throw std::runtime_error(compilerError("label `" + line[1] + "` not found"));
                                        int val1 = 0, val2 = 0;
                                        bool isReg1 = false, isReg2 = false;
                                        val1 = parseIntOrReg(line[2], lne, isReg1);
                                        val2 = parseIntOrReg(line[3], lne, isReg2);
                                        prog.push_back(buildPUSHiInstruction(labels[line[1]]));
                                        if(isReg1 && isReg2) prog.push_back(buildJErInstruction(val1, val2));
                                        else {
                                                if(isReg1) prog.push_back(buildPUSHrInstruction(val1));
                                                else prog.push_back(buildPUSHiInstruction(val1));
                                                if(isReg2) prog.push_back(buildPUSHrInstruction(val2));
                                                else prog.push_back(buildPUSHiInstruction(val2));
                                                prog.push_back(buildJEiInstruction());
                                        }
                                } else if(line[0] == "JG") {
                                        if(line.size() != 4) throw std::runtime_error(compilerError("JG instruction need no arguments but got " + to_string(line.size() - 1)));
                                        if(labels.find(line[1]) == labels.end()) throw std::runtime_error(compilerError("label `" + line[1] + "` not found"));
                                        int val1 = 0, val2 = 0;
                                        bool isReg1 = false, isReg2 = false;
                                        val1 = parseIntOrReg(line[2], lne, isReg1);
                                        val2 = parseIntOrReg(line[3], lne, isReg2);
                                        prog.push_back(buildPUSHiInstruction(labels[line[1]]));
                                        if(isReg1 && isReg2) prog.push_back(buildJGrInstruction(val1, val2));
                                        else {
                                                if(isReg1) prog.push_back(buildPUSHrInstruction(val1));
                                                else prog.push_back(buildPUSHiInstruction(val1));
                                                if(isReg2) prog.push_back(buildPUSHrInstruction(val2));
                                                else prog.push_back(buildPUSHiInstruction(val2));
                                                prog.push_back(buildJGiInstruction());
                                        }
                                } else if(line[0] == "JL") {
                                        if(line.size() != 4) throw std::runtime_error(compilerError("JL instruction need no arguments but got " + to_string(line.size() - 1)));
                                        if(labels.find(line[1]) == labels.end()) throw std::runtime_error(compilerError("label `" + line[1] + "` not found"));
                                        int val1 = 0, val2 = 0;
                                        bool isReg1 = false, isReg2 = false;
                                        val1 = parseIntOrReg(line[2], lne, isReg1);
                                        val2 = parseIntOrReg(line[3], lne, isReg2);
                                        prog.push_back(buildPUSHiInstruction(labels[line[1]]));
                                        if(isReg1 && isReg2) prog.push_back(buildJLrInstruction(val1, val2));
                                        else {
                                                if(isReg1) prog.push_back(buildPUSHrInstruction(val1));
                                                else prog.push_back(buildPUSHiInstruction(val1));
                                                if(isReg2) prog.push_back(buildPUSHrInstruction(val2));
                                                else prog.push_back(buildPUSHiInstruction(val2));
                                                prog.push_back(buildJLiInstruction());
                                        }
                                } else if(line[0] == "JGE") {
                                        if(line.size() != 4) throw std::runtime_error(compilerError("JGE instruction need no arguments but got " + to_string(line.size() - 1)));
                                        if(labels.find(line[1]) == labels.end()) throw std::runtime_error(compilerError("label `" + line[1] + "` not found"));
                                        int val1 = 0, val2 = 0;
                                        bool isReg1 = false, isReg2 = false;
                                        val1 = parseIntOrReg(line[2], lne, isReg1);
                                        val2 = parseIntOrReg(line[3], lne, isReg2);
                                        prog.push_back(buildPUSHiInstruction(labels[line[1]]));
                                        if(isReg1 && isReg2) prog.push_back(buildJGErInstruction(val1, val2));
                                        else {
                                                if(isReg1) prog.push_back(buildPUSHrInstruction(val1));
                                                else prog.push_back(buildPUSHiInstruction(val1));
                                                if(isReg2) prog.push_back(buildPUSHrInstruction(val2));
                                                else prog.push_back(buildPUSHiInstruction(val2));
                                                prog.push_back(buildJGEiInstruction());
                                        }
                                } else if(line[0] == "JLE") {
                                        if(line.size() != 4) throw std::runtime_error(compilerError("JLE instruction need no arguments but got " + to_string(line.size() - 1)));
                                        if(labels.find(line[1]) == labels.end()) throw std::runtime_error(compilerError("label `" + line[1] + "` not found"));
                                        int val1 = 0, val2 = 0;
                                        bool isReg1 = false, isReg2 = false;
                                        val1 = parseIntOrReg(line[2], lne, isReg1);
                                        val2 = parseIntOrReg(line[3], lne, isReg2);
                                        prog.push_back(buildPUSHiInstruction(labels[line[1]]));
                                        if(isReg1 && isReg2) prog.push_back(buildJLErInstruction(val1, val2));
                                        else {
                                                if(isReg1) prog.push_back(buildPUSHrInstruction(val1));
                                                else prog.push_back(buildPUSHiInstruction(val1));
                                                if(isReg2) prog.push_back(buildPUSHrInstruction(val2));
                                                else prog.push_back(buildPUSHiInstruction(val2));
                                                prog.push_back(buildJLEiInstruction());
                                        }
                                } else if(line[0] == "JMP") {
                                        if(line.size() != 2) throw std::runtime_error(compilerError("JMP instruction need no arguments but got " + to_string(line.size() - 1)));
                                        if(labels.find(line[1]) == labels.end()) throw std::runtime_error(compilerError("label `" + line[1] + "` not found"));
                                        prog.push_back(buildJMPiInstruction(labels[line[1]]));
                                } else {
                                        throw std::runtime_error(compilerError("unknown keyword `" + line[0] + "`"));
                                }
                        }
                        prog.push_back(buildENDInstruction());
                        input.close();
                        Header h = Header();
                        h.segmentsTable = sizeof(Header) + (prog.size() * sizeof(prog[0]));
                        h.segmentNumber = extrns.size() != 0;
                        output.open(outPath.c_str());
                        input.exceptions(std::ifstream::badbit);
                        output.write((const char *)&h, sizeof(h));
                        for(auto it = prog.begin(); it != prog.end(); it++) output.write((const char *)&(*it), sizeof(*it));
                        u64 sgn = extrns.size() != 0;
                        if(extrns.size() > 0) {
                                SegmentHeader sgh;
                                sgh.len = extrns.size();
                                sgh.type = ST_EXTRN;
                                sgh.off = h.segmentsTable + (sgn * sizeof(SegmentHeader));
                                output.write((const char *)&sgh, sizeof(SegmentHeader));
                                u64 acc = sgh.off + (sgh.len * sizeof(ExtrnHeader));
                                for(auto it = extrns.begin(); it != extrns.end(); it++) {
                                       ExtrnHeader eh;
                                       eh.name = acc;
                                       acc += (*it).size() + 1;
                                       output.write((const char *)&eh, sizeof(ExtrnHeader));
                                }
                                for(auto it = extrns.begin(); it != extrns.end(); it++) {
                                       output.write((*it).c_str(), (*it).length());
                                       output << (char)0;
                                }
                        }
                        output.close();
                } catch(std::exception &e) {
                        _log.log(LOG_ERROR, "unable to compile /s: /s", path.c_str(), e.what());
                        throw std::runtime_error("Compile Error");
                }
        }
}
