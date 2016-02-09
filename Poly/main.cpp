#define STANDALONE

// ----Include Files -------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poly.h"

inline unsigned long GetCurrTime()
{
    unsigned long theTick;

    theTick = GetTickCount();
    return theTick;
}

// ----Host API ------------------------------------------------------------------------------

/* 计算平均值 */
static void average(script_env *sc)
{
    int n = Poly_GetParamCount(sc);
    int sum = 0;
    for (int i = 0; i < n; i++)
    {
        sum += Poly_GetParamAsInt(sc, i);
    }
    Poly_ReturnIntFromHost(sc, sum / n);
}

static void poly_pause(script_env *sc)
{
    int nParam = Poly_GetParamCount(sc);
    if (nParam > 0)
    {
        char *prompt = Poly_GetParamAsString(sc, 0);
        printf("%s", prompt);
        system("pause>nul");
    }
    else
    {
        system("pause");
    }
    Poly_ReturnFromHost(sc);
}

static void h_PrintString(script_env *sc)
{
    char* str = Poly_GetParamAsString(sc, 0);
    puts(str);
    Poly_ReturnFromHost(sc);
}

static void h_PrintInt(script_env *sc)
{
    int i = Poly_GetParamAsInt(sc, 0);
    printf("Explode %d!\n", i);
    Poly_ReturnFromHost(sc);
}

static void h_Division(script_env *sc)
{
    float i = Poly_GetParamAsFloat(sc, 0);
    float j = Poly_GetParamAsFloat(sc, 1);
    printf("%f\n", i / j);
    Poly_ReturnFromHost(sc);
}

// ---- Entry Main ----------------------------------------------------------------------------------

int RunScript(char* pstrFilename)
{
    // Initialize the runtime environment
    script_env *sc = Poly_Initialize();

    // 注册宿主api
    Poly_RegisterHostFunc(POLY_GLOBAL_FUNC, "Average", average);
    Poly_RegisterHostFunc(POLY_GLOBAL_FUNC, "Explode", h_PrintInt);
    Poly_RegisterHostFunc(POLY_GLOBAL_FUNC, "pause", poly_pause);
    Poly_RegisterHostFunc(POLY_GLOBAL_FUNC, "Division", h_Division);
    Poly_RegisterHostFunc(POLY_GLOBAL_FUNC, "PrintString", h_PrintString);

    // Load the demo script
    int iErrorCode = Poly_LoadScript(sc, pstrFilename);

    // Check for an error
    if (iErrorCode != POLY_LOAD_OK)
    {
        printf("载入脚本失败\n");
        exit(1);
    }

    unsigned long start = GetCurrTime();

    // Run we're loaded script from Main()

    Poly_RunScript(sc, POLY_INFINITE_TIMESLICE);

    printf("耗时 %fs\n", (GetCurrTime() - start) / 1000.0);

    int iExitCode = Poly_GetExitCode(sc);

    // Free resources and perform general cleanup
    Poly_ShutDown(sc);

    return iExitCode;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("%s: no input files\n", argv[0]);
        exit(0);
    }

    RunScript(argv[1]);
}
