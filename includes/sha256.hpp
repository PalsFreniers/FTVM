#pragma once

#include <string>
#include <types.hpp>

void sha256(std::string msg, u32 hash[8]);
std::string sha256str(u32 hash[8]);
