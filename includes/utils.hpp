#pragma once

#include <types.hpp>
#include <string>
#include <fstream>
#include <vector>

#define auto __auto_type
#define unused __attribute__((unused))
#define KB(x) ((x) * 1000)
#define MB(x) (KB(x) * 1000)
#define nullptr ((void *)0)

int to_int(char const *s);
u64 scall(u64 number, ...);
std::string slurp(std::ifstream &influx);
std::vector<std::string> split(std::string& s, const std::string& delimiter);
std::string to_string(long x);
