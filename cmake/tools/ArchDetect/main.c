#include <stdio.h>

// Based on https://sourceforge.net/p/predef/wiki/Architectures/?SetFreedomCookie

int main() {
	#if defined(__arm__) || defined(__TARGET_ARCH_ARM)
		printf("arm");
		return 3;
	#elif defined(__aarch64__)
		printf("arm64");
		return 4;
	#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
		printf("i386");
		return 1;

	#elif defined(__amd64__) || defined(__amd64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_AMD64) || defined(_M_X64)
		printf("amd64");
		return 2;
	#endif
	printf("UNKNOW");
	return 0;
}
