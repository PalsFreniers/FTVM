#pragma once

#include <types.hpp>
#include <string>
#include <map>
#include <stack>

#define INSTRUCTION_NOP  0x00
#define INSTRUCTION_PUSH 0x01
#define INSTRUCTION_POP  0x02
#define INSTRUCTION_CALL 0x03
#define INSTRUCTION_ADD  0x04
#define INSTRUCTION_END  0xFF

#define SPEC_IMM 0x01
#define SPEC_REG 0x02

#define REG_R1 0x01
#define REG_R2 0x02
#define REG_R3 0x03
#define REG_R4 0x04
#define REG_R5 0x05
#define REG_R6 0x06

#define getSuperInstruction(insr)    (((insr) >> (8 * 7)) & 0xFF)
#define getInstructionSpec(insr)     (((insr) >> (8 * 6)) & 0xFF)
#define getInstructionRegSpace(insr) ((insr) & 0xFFFFFFFF)
#define getInstructionRegX(insr, x)  (((insr) >> (8 * ((x) - 1))) & 0xFF)
#define getInstructionImm(insr)      ((insr) & 0xFFFFFFFF)

#define buildInstructionComplete(super, spec, _, __, imm, r1, r2, r3, r4) (((u64)(super) << (8 * 7))  | \
                                                                           ((u64)(spec) << (8 * 6))   | \
                                                                           ((u64)(imm) & ((u32)-1))   | \
                                                                           (r1 & 0xFF)              | \
                                                                           ((r2 & 0xFF) << 8)       | \
                                                                           ((r3 & 0xFF) << (8 * 2)) | \
                                                                           ((r4 & 0xFF) << (8 * 3)))

#define buildInstruction(super, spec, imm, r1, r2, r3, r4) buildInstructionComplete(super, spec, 0, 0, imm, r1, r2, r3, r4)
#define buildInstructionImmediate(super, imm)              buildInstruction(super, SPEC_IMM, imm, 0, 0, 0, 0)
#define buildInstruction4R(super, r1, r2, r3, r4)          buildInstruction(super, SPEC_REG, 0, r1, r2, r3, r4)
#define buildInstruction3R(super, r1, r2, r3)              buildInstruction4R(super, r1, r2, r3, 0)
#define buildInstruction2R(super, r1, r2)                  buildInstruction3R(super, r1, r2, 0)
#define buildInstruction1R(super, r1)                      buildInstruction2R(super, r1, 0)
#define buildInstructionNoArg(super)                       buildInstruction(super, 0, 0, 0, 0, 0, 0)

#define buildNOPIstruction()         buildInstructionNoArg(INSTRUCTION_NOP)
#define buildPUSHiInstruction(imm)   buildInstructionImmediate(INSTRUCTION_PUSH, imm)
#define buildPUSHrInstruction(reg)   buildInstruction1R(INSTRUCTION_PUSH, reg)
#define buildPOPInstruction(reg)     buildInstruction1R(INSTRUCTION_POP, reg)
#define buildCALLInstrucion(addr)    buildInstructionImmediate(INSTRUCTION_CALL, addr)
#define buildADDrInstruction(r1, r2) buildInstruction2R(INSTRUCTION_ADD, r1, r2)
#define buildADDiInstruction()       buildInstructionImmediate(INSTRUCTION_ADD, 0)
#define buildENDInstruction()        buildInstructionNoArg(INSTRUCTION_END)

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

        typedef void (*extrn)(Registers &, std::stack<u32> &);

        #pragma pack(push, 1)
        struct ExtrnHash {
                u32 hash[8];
                bool operator<(const ExtrnHash &other) const;
        };
        #pragma pack(pop)

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
        };

        void compile(std::string path, std::string outPath);
}
