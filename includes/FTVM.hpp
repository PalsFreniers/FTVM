#pragma once

#include <types.hpp>
#include <string>
#include <unordered_map>

#define getSuperInstruction(insr)    (((insr) >> (8 * 7)) & 0xFF)
#define getInstructionSpec(insr)     (((insr) >> (8 * 6)) & 0xFF)
#define getInstructionRegSpace(insr) ((insr) & 0xFFFFFFFF)
#define getInstructionRegX(insr, x)  (((insr) >> (8 * ((x) - 1))) & 0xFF)
#define getInstructionImm(insr)      ((insr) & 0xFFFFFFFF)

#define buildInstructionComplete(super, spec, _, __, imm, r1, r2, r3, r4) ((super) | (spec) | (_) | (__) | (imm) | (r1) | ((r2) << 8) | ((r3) << (8 * 2)) | ((r4) << (8 * 3)))
#define buildInstruction(super, spec, imm, r1, r2, r3, r4) buildInstructionComplete(super, spec, 0L, 0L, imm, r1, r2, r3, r4)
#define buildInstructionImmediate(super, spec, imm) buildInstruction(super, spec, imm, 0, 0, 0, 0)
#define buildInstruction4R(super, spec, r1) buildInstruction()

namespace FTVM {
        enum FTVMSetBase {
                PUSH = 0xFF00000000000000,
                POP  = 0xFE00000000000000,
                IMM  = 0x00FF000000000000,
        };

        struct Header {
                u8 magic[4];
                u32 entry;
                u32 segmentsTable;
                u32 segmentNumber;
        };

        enum SegmentType {
                FUNCT,
                EXTRN,
        };

        struct SegmentHeader {
                u8 type;
                u32 off;
                u32 len;
        };

        struct ExtrnHeader {
                u32 name;
        };

        struct FunctHeader {
                u32 off;
                u32 name;
        };

        typedef void (*extrn)();

        struct ExtrnHash {
                u32 hash[8];
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
                void launch();
                bool step();
        private:
                u8 *_file;
                usz _size;
                usz _startOffset;
                std::string _path;
                std::unordered_map<ExtrnHash, extrn> _extrns;
        };

        void compile(std::string path, std::string outPath);
}
