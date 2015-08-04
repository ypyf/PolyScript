#if 1

#define STANDALONE

// ----Include Files -------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poly.h"

#define MAX_PATH    260

inline unsigned long GetCurrTime()
{
	unsigned long theTick;

	theTick = GetTickCount();
	return theTick;
}

void print_error_message(int iErrorCode)
{
    // Print the error based on the code
    printf("Error: ");

    switch (iErrorCode)
    {
    case POLY_LOAD_ERROR_FILE_IO:
        printf("File I/O error");
        break;

    case POLY_LOAD_ERROR_INVALID_XSE:
        printf("Invalid .PE file");
        break;

    case POLY_LOAD_ERROR_UNSUPPORTED_VERS:
        printf("Unsupported .PE version");
        break;

    case POLY_LOAD_ERROR_OUT_OF_MEMORY:
        printf("Out of memory");
        break;

    case POLY_LOAD_ERROR_OUT_OF_THREADS:
        printf("Out of threads");
        break;
    }

    printf(".\n");
}

// ----Host API ------------------------------------------------------------------------------

/* 计算平均值 */
static void average(ScriptContext *sc)
{
    int n = Poly_GetParamCount(sc);
    int sum = 0;
    for (int i = 0; i < n; i++)
    {
        sum += Poly_GetParamAsInt(sc, i);
    }
    Poly_ReturnIntFromHost(sc, sum / n);
}

static void poly_pause(ScriptContext *sc)
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

static void h_PrintString(ScriptContext *sc)
{
	char* str = Poly_GetParamAsString(sc, 0);
	puts(str);
	Poly_ReturnFromHost(sc);
}

static void h_PrintInt(ScriptContext *sc)
{
	int i = Poly_GetParamAsInt(sc, 0);
	printf("Explode %d!\n", i);
	Poly_ReturnFromHost(sc);
}

static void h_Division(ScriptContext *sc)
{
	float i = Poly_GetParamAsFloat(sc, 0);
	float j = Poly_GetParamAsFloat(sc, 1);
	printf("%f\n", i / j);
	Poly_ReturnFromHost(sc);
}

// ----CRL Entry Main ----------------------------------------------------------------------------------

int RunScript(char* pstrFilename)
{
    // Initialize the runtime environment
    ScriptContext *sc = Poly_CreateInterp();

    // 注册宿主api
	Poly_RegisterHostFunc(POLY_GLOBAL_FUNC, "Average", average);
	Poly_RegisterHostFunc(POLY_GLOBAL_FUNC, "Explode", h_PrintInt);
	Poly_RegisterHostFunc(POLY_GLOBAL_FUNC, "pause", poly_pause);
	Poly_RegisterHostFunc(POLY_GLOBAL_FUNC, "Division", h_Division);
	if (!Poly_RegisterHostFunc(POLY_GLOBAL_FUNC, "PrintString", h_PrintString))
    {
        printf("Register Host API Failed!");
        exit(1);
    }

    // An error code
    int iErrorCode;

    // Load the demo script
    iErrorCode = Poly_LoadScript(sc, pstrFilename);

    // Check for an error
    if (iErrorCode != POLY_LOAD_OK)
    {
        print_error_message(iErrorCode);
        exit(1);
    }

    // Run we're loaded script from Main()
    Poly_RunScript(sc, POLY_INFINITE_TIMESLICE);

    int iExitCode = Poly_GetExitCode(sc);

    // Free resources and perform general cleanup
    Poly_ShutDown(sc);

    return iExitCode;
}


int main(int argc, char* argv[])
{
	unsigned long start = GetCurrTime();
	if (argc < 2) 
	{
		printf("CRL: No input files\n");
		exit(0);
	}

	RunScript(argv[1]);

	printf("耗时 %fs\n", (GetCurrTime()-start)/1000.0);
}

#endif
