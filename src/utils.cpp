#include <cstdarg>
#include <utils.hpp>
#include <sstream>

u64 scall(u64 number, ...) {
        va_list lst;
        va_start(lst, number);
        u64 r1, r2, r3, r4, r5, r6;
        r1 = va_arg(lst, u64);
        r2 = va_arg(lst, u64);
        r3 = va_arg(lst, u64);
        r4 = va_arg(lst, u64);
        r5 = va_arg(lst, u64);
        r6 = va_arg(lst, u64);
        u64 ret;

	asm volatile ("mov %1, %%rax \n"
		      "mov %2, %%rdi \n"
		      "mov %3, %%rsi \n"
		      "mov %4, %%rdx \n"
		      "mov %5, %%r10 \n"
		      "mov %6, %%r8  \n"
		      "mov %7, %%r9  \n"
		      "syscall       \n"
		      "mov %%rax, %0 \n"
		      : "=r"(ret)
		      : "r"(number), "r"(r1),
		        "r"(r2), "r"(r3),
		        "r"(r4), "r"(r5),
		        "r"(r6)
		      : "%rax", "%rdi", "%rsi", "%rdx", "%r10", "%r8", "%r9",
		        "memory");
        return ret;
}

std::string slurp(std::ifstream &influx) {
        std::ostringstream tmp;
        tmp << influx.rdbuf();
        return tmp.str();
}
