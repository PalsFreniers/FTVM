#pragma once

#include <types.hpp>
#include <string>
#include <fstream>
#include <vector>

#define auto __auto_type

int to_int(char const *s);
u64 scall(u64 number, ...);
std::string slurp(std::ifstream &influx);
std::vector<std::string> split(std::string& s, const std::string& delimiter);
