#pragma once

#include <types.hpp>
#include <string>
#include <fstream>

#define auto __auto_type

u64 scall(u64 number, ...);

std::string slurp(std::ifstream &influx);
