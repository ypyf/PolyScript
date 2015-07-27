﻿#if 1

#define STANDALONE

// ----Include Files -------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elfvm.h"

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
    case ELF_LOAD_ERROR_FILE_IO:
        printf("File I/O error");
        break;

    case ELF_LOAD_ERROR_INVALID_XSE:
        printf("Invalid .XSE file");
        break;

    case ELF_LOAD_ERROR_UNSUPPORTED_VERS:
        printf("Unsupported .XSE version");
        break;

    case ELF_LOAD_ERROR_OUT_OF_MEMORY:
        printf("Out of memory");
        break;

    case ELF_LOAD_ERROR_OUT_OF_THREADS:
        printf("Out of threads");
        break;
    }

    printf(".\n");
}

// ----Host API ------------------------------------------------------------------------------

/* 打印平均值 */
static void average(ScriptContext *sc)
{
    int n = ELF_GetParamCount(sc);
    int sum = 0;
    for (int i = 0; i < n; i++)
    {
        sum += ELF_GetParamAsInt(sc, i);
    }
    ELF_ReturnIntFromHost(sc, sum / n);
}

static void h_PrintString(ScriptContext *sc)
{
	char* str = ELF_GetParamAsString(sc, 0);
	puts(str);
	ELF_ReturnFromHost(sc);
}

static void h_PrintInt(ScriptContext *sc)
{
	int i = ELF_GetParamAsInt(sc, 0);
	printf("Explode %d!\n", i);
	ELF_ReturnFromHost(sc);
}

static void h_Division(ScriptContext *sc)
{
	float i = ELF_GetParamAsFloat(sc, 0);
	float j = ELF_GetParamAsFloat(sc, 1);
	printf("%f\n", i / j);
	ELF_ReturnFromHost(sc);
}

// ----CRL Entry Main ----------------------------------------------------------------------------------

int RunScript(char* pstrFilename)
{
    char ExecFileName[MAX_PATH];
	char inputFilename[MAX_PATH];

	strcpy(inputFilename, pstrFilename);
	strupr(inputFilename);

    // 构造 .XSE 文件名
    if (strstr(inputFilename, XSS_FILE_EXT))
    {
        int ExtOffset = strrchr(inputFilename, '.') - inputFilename;
        strncpy(ExecFileName, inputFilename, ExtOffset);
        ExecFileName[ExtOffset] = '\0';
        strcat(ExecFileName, XSE_FILE_EXT);

        // 编译
		ELF_CompileScript(inputFilename, ExecFileName);
    }
    else if (strstr(inputFilename, XSE_FILE_EXT))
    {
        strcpy(ExecFileName, inputFilename);
    }
    else
    {
        printf("unexpected script file name.\n");
        return FALSE;
    }

    // Initialize the runtime environment
    ScriptContext *sc = ELF_Create();

    // 注册宿主api
	ELF_RegisterHostFunc(ELF_GLOBAL_FUNC, "Explode", h_PrintInt);
	ELF_RegisterHostFunc(ELF_GLOBAL_FUNC, "Division", h_Division);
	if (!ELF_RegisterHostFunc(ELF_GLOBAL_FUNC, "PrintString", h_PrintString))
    {
        printf("Register Host API Failed!");
        exit(1);
    }

    // An error code
    int iErrorCode;

    // Load the demo script
    iErrorCode = ELF_LoadXSE(sc, ExecFileName);

    // Check for an error
    if (iErrorCode != ELF_LOAD_OK)
    {
        print_error_message(iErrorCode);
        exit(1);
    }

    // Run we're loaded script from Main()
    ELF_RunScript(sc, ELF_INFINITE_TIMESLICE);

    int iExitCode = ELF_GetExitCode(sc);

    // Free resources and perform general cleanup
    ELF_ShutDown(sc);

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
