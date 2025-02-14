#pragma once

#include <types.hpp>
#include <string>
#include <map>
#include <stack>
#include <utils.hpp>

#define INSTRUCTION_NOP  0x00
#define INSTRUCTION_PUSH 0x01
#define INSTRUCTION_POP  0x02
#define INSTRUCTION_CALL 0x03
#define INSTRUCTION_RET  0x04
#define INSTRUCTION_ADD  0x05
#define INSTRUCTION_SUB  0x06
#define INSTRUCTION_MUL  0x07
#define INSTRUCTION_DIV  0x08
#define INSTRUCTION_MOD  0x09
#define INSTRUCTION_JNE  0x0a
#define INSTRUCTION_JE   0x0b
#define INSTRUCTION_JL   0x0c
#define INSTRUCTION_JG   0x0d
#define INSTRUCTION_JLE  0x0e
#define INSTRUCTION_JGE  0x0f
#define INSTRUCTION_JMP  0x10
#define INSTRUCTION_AND  0x11
#define INSTRUCTION_OR   0x12
#define INSTRUCTION_NOT  0x13
#define INSTRUCTION_XOR  0x14
#define INSTRUCTION_MOV  0x15
#define INSTRUCTION_SAVE 0x16
#define INSTRUCTION_LOAD 0x17
#define INSTRUCTION_SHL  0x18
#define INSTRUCTION_SHR  0x19
#define INSTRUCTION_END  0xFF

#define SPEC_IMM  0x01
#define SPEC_REG  0x02
#define SPEC_ADDR 0x03

#define NO_REG  0x00
#define REG_R1  0x01
#define REG_R2  0x02
#define REG_R3  0x03
#define REG_R4  0x04
#define REG_R5  0x05
#define REG_R6  0x06
#define REG_RIP 0x07

#define SIZE_BYTE  0x01
#define SIZE_WORD  0x02
#define SIZE_DWORD 0x03

#define getSuperInstruction(insr)    (((insr) >> (8 * 7)) & 0xFF)
#define getInstructionSpec(insr)     (((insr) >> (8 * 6)) & 0xFF)
#define getInstructionSize(insr)     (((insr) >> (8 * 5)) & 0xFF)
#define getInstructionRegSpace(insr) ((insr) & 0xFFFFFFFF)
#define getInstructionRegX(insr, x)  (((insr) >> (8 * ((x) - 1))) & 0xFF)
#define getInstructionImm(insr)      ((insr) & 0xFFFFFFFF)

#define buildInstructionComplete(super, spec, size, __, imm, r1, r2, r3, r4) (((u64)(super) << (8 * 7)) | \
                                                                             ((u64)(spec) << (8 * 6))  | \
                                                                             ((u64)(size) << (8 * 5))  | \
                                                                             ((u64)(imm) & ((u32)-1))  | \
                                                                             (r1 & 0xFF)               | \
                                                                             ((r2 & 0xFF) << 8)        | \
                                                                             ((r3 & 0xFF) << (8 * 2))  | \
                                                                             ((r4 & 0xFF) << (8 * 3)))

#define buildInstruction(super, spec, size, imm, r1, r2, r3, r4) buildInstructionComplete(super, spec, size, 0, imm, r1, r2, r3, r4)
#define buildInstructionImmediate(super, imm)                    buildInstruction(super, SPEC_IMM, 0, imm, 0, 0, 0, 0)
#define buildInstructionAddressed(super, addr)                   buildInstruction(super, SPEC_ADDR, 0, addr, 0, 0, 0, 0)
#define buildInstruction4R(super, r1, r2, r3, r4)                buildInstruction(super, SPEC_REG, 0, 0, r1, r2, r3, r4)
#define buildInstruction3R(super, r1, r2, r3)                    buildInstruction4R(super, r1, r2, r3, 0)
#define buildInstruction2R(super, r1, r2)                        buildInstruction3R(super, r1, r2, 0)
#define buildInstruction1R(super, r1)                            buildInstruction2R(super, r1, 0)
#define buildInstructionSizedImmediate(super, size, imm)         buildInstruction(super, SPEC_IMM, size, imm, 0, 0, 0, 0)
#define buildInstructionSizedAddressed(super, size, addr)        buildInstruction(super, SPEC_ADDR, size, addr, 0, 0, 0, 0)
#define buildInstructionSized4R(super, size, r1, r2, r3, r4)     buildInstruction(super, SPEC_REG, size, 0, r1, r2, r3, r4)
#define buildInstructionSized3R(super, size, r1, r2, r3)         buildInstructionSized4R(super, size, r1, r2, r3, 0)
#define buildInstructionSized2R(super, size, r1, r2)             buildInstructionSized3R(super, size, r1, r2, 0)
#define buildInstructionSized1R(super, size, r1)                 buildInstructionSized2R(super, size, r1, 0)
#define buildInstructionNoArg(super)                             buildInstruction(super, SPEC_IMM, 0, 0, 0, 0, 0, 0)

#define buildNOPIstruction()                buildInstructionNoArg(INSTRUCTION_NOP)
#define buildPUSHiInstruction(imm)          buildInstructionImmediate(INSTRUCTION_PUSH, imm)
#define buildPUSHrInstruction(reg)          buildInstruction1R(INSTRUCTION_PUSH, reg)
#define buildPOPInstruction(reg)            buildInstruction1R(INSTRUCTION_POP, reg)
#define buildCALLeInstrucion(addr)          buildInstructionImmediate(INSTRUCTION_CALL, addr)
#define buildCALLaInstrucion(addr)          buildInstructionAddressed(INSTRUCTION_CALL, addr)
#define buildCALLrInstrucion(reg)           buildInstruction1R(INSTRUCTION_CALL, reg)
#define buildRETInstruction()               buildInstructionNoArg(INSTRUCTION_RET)
#define buildADDrInstruction(r1, r2)        buildInstruction2R(INSTRUCTION_ADD, r1, r2)
#define buildADDiInstruction()              buildInstructionNoArg(INSTRUCTION_ADD)
#define buildSUBrInstruction(r1, r2)        buildInstruction2R(INSTRUCTION_SUB, r1, r2)
#define buildSUBiInstruction()              buildInstructionNoArg(INSTRUCTION_SUB)
#define buildMULrInstruction(r1, r2)        buildInstruction2R(INSTRUCTION_MUL, r1, r2)
#define buildMULiInstruction()              buildInstructionNoArg(INSTRUCTION_MUL)
#define buildDIVrInstruction(r1, r2)        buildInstruction2R(INSTRUCTION_DIV, r1, r2)
#define buildDIViInstruction()              buildInstructionNoArg(INSTRUCTION_DIV)
#define buildMODrInstruction(r1, r2)        buildInstruction2R(INSTRUCTION_MOD, r1, r2)
#define buildMODiInstruction()              buildInstructionNoArg(INSTRUCTION_MOD)
#define buildJNErInstruction(r1, r2)        buildInstruction2R(INSTRUCTION_JNE, r1, r2)
#define buildJNEiInstruction()              buildInstructionNoArg(INSTRUCTION_JNE)
#define buildJErInstruction(r1, r2)         buildInstruction2R(INSTRUCTION_JE, r1, r2)
#define buildJEiInstruction()               buildInstructionNoArg(INSTRUCTION_JE)
#define buildJLrInstruction(r1, r2)         buildInstruction2R(INSTRUCTION_JL, r1, r2)
#define buildJLiInstruction()               buildInstructionNoArg(INSTRUCTION_JL)
#define buildJGrInstruction(r1, r2)         buildInstruction2R(INSTRUCTION_JG, r1, r2)
#define buildJGiInstruction()               buildInstructionNoArg(INSTRUCTION_JG)
#define buildJLErInstruction(r1, r2)        buildInstruction2R(INSTRUCTION_JLE, r1, r2)
#define buildJLEiInstruction()              buildInstructionNoArg(INSTRUCTION_JLE)
#define buildJGErInstruction(r1, r2)        buildInstruction2R(INSTRUCTION_JGE, r1, r2)
#define buildJGEiInstruction()              buildInstructionNoArg(INSTRUCTION_JGE)
#define buildJMPiInstruction(addr)          buildInstructionImmediate(INSTRUCTION_JMP, addr)
#define buildANDrInstruction(r1, r2)        buildInstruction2R(INSTRUCTION_AND, r1, r2)
#define buildANDiInstruction()              buildInstructionNoArg(INSTRUCTION_AND)
#define buildORrInstruction(r1, r2)         buildInstruction2R(INSTRUCTION_OR, r1, r2)
#define buildORiInstruction()               buildInstructionNoArg(INSTRUCTION_OR)
#define buildNOTrInstruction(r1)            buildInstruction1R(INSTRUCTION_NOT, r1)
#define buildNOTiInstruction()              buildInstructionNoArg(INSTRUCTION_NOT)
#define buildXORrInstruction(r1, r2)        buildInstruction2R(INSTRUCTION_XOR, r1, r2)
#define buildXORiInstruction()              buildInstructionNoArg(INSTRUCTION_XOR)
#define buildMOVrInstruction(r1, r2)        buildInstruction2R(INSTRUCTION_MOV, r1, r2)
#define buildSAVErInstruction(size, r1, r2) buildInstructionSized2R(INSTRUCTION_SAVE, size, r1, r2)
#define buildSAVEiInstruction(size)         buildInstructionSizedAddressed(INSTRUCTION_SAVE, size, 0)
#define buildLOADInstruction(size, r1, r2)  buildInstructionSized2R(INSTRUCTION_LOAD, size, r1, r2)
#define buildSHLrInstruction(r1, r2)        buildInstruction2R(INSTRUCTION_SHL, r1, r2)
#define buildSHLiInstruction()              buildInstructionNoArg(INSTRUCTION_SHL)
#define buildSHRrInstruction(r1, r2)        buildInstruction2R(INSTRUCTION_SHR, r1, r2)
#define buildSHRiInstruction()              buildInstructionNoArg(INSTRUCTION_SHR)
#define buildENDInstruction()               buildInstructionNoArg(INSTRUCTION_END)

#define NO_FUNC ((FTVM::extrn)-1L)

#define compilerError(msg) ("line : " + to_string(lne) + ", " + (msg))

namespace FTVM {
        struct Registers {
                u32 rip;
                u32 r1;
                u32 r2;
                u32 r3;
                u32 r4;
                u32 r5;
                u32 r6;
        };

        #pragma pack(push, 1)
        struct Header {
                u8 magic[4];
                u32 entry;
                u32 segmentsTable;
                u32 segmentNumber;

                Header();
        };
        #pragma pack(pop)

        enum SegmentType {
                ST_EXTRN,
        };

        #pragma pack(push, 1)
        struct SegmentHeader {
                u8 type;
                u32 off;
                u32 len;
        };
        #pragma pack(pop)

        #pragma pack(push, 1)
        struct ExtrnHeader {
                u32 name;
        };
        #pragma pack(pop)

        typedef void (*extrn)(Registers &, std::stack<u32> &, u8[MB(1)]);

        struct ExtrnHash {
                u32 hash[8];
                bool operator<(const ExtrnHash &other) const;
                bool operator==(const ExtrnHash &other) const;
        };

        class Program {
        public:
                Program();
                Program(std::string path);
                Program(const Program &other);
                ~Program();
                Program &operator=(const Program &other);
                void load(std::string path);
                void unload();
                void set(std::string name, extrn func);
                void launch();
                void step();
                void show();

                operator bool();
        private:
                u32 &_getRegisterValue(u8 reg);
                u32 _pop();

                u8 *_file;
                usz _size;
                usz _startOffset;
                ExtrnHeader *_extrnTableOff;
                std::string _path;
                std::map<ExtrnHash, extrn> _extrns;
                std::stack<u32> _execStack;
                Registers _regs;
                bool _launched;
                bool _valid;
                u8 _memory[MB(1)];
        };

        void compile(std::string path, std::string outPath);
}
