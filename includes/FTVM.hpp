#pragma once

#include <types.hpp>
#include <string>
#include <unordered_map>

namespace FTVM {
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
