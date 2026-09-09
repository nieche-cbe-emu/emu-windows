
#include <stdio.h>
#include <windows.h>

int main(void)
{
    HMODULE m = LoadLibraryA("nieche.dll");
    if (!m) {
        printf("LoadLibrary 失败: %lu\n", GetLastError());
        return 2;
    }
    unsigned (*abi)(void) = (unsigned (*)(void))GetProcAddress(m, "nieche_abi_version");
    int (*st)(void) = (int (*)(void))GetProcAddress(m, "nieche_selftest");
    if (!abi || !st) {
        printf("符号缺失\n");
        return 3;
    }
    printf("ABI=%u\n", abi());
    fflush(stdout);
    printf("selftest 开始\n");
    fflush(stdout);
    int r = st();
    printf("selftest=%d\n", r);
    fflush(stdout);
    return r == 1 ? 0 : 4;
}
