#if 1

#define STANDALONE

// ----Include Files -------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvm.h"

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
    case XVM_LOAD_ERROR_FILE_IO:
        printf("File I/O error");
        break;

    case XVM_LOAD_ERROR_INVALID_XSE:
        printf("Invalid .XSE file");
        break;

    case XVM_LOAD_ERROR_UNSUPPORTED_VERS:
        printf("Unsupported .XSE version");
        break;

    case XVM_LOAD_ERROR_OUT_OF_MEMORY:
        printf("Out of memory");
        break;

    case XVM_LOAD_ERROR_OUT_OF_THREADS:
        printf("Out of threads");
        break;
    }

    printf(".\n");
}

// ----Host API ------------------------------------------------------------------------------

/* 打印平均值 */
static void average(XVM_State* vm)
{
    int n = XVM_GetParamCount(vm);
    int sum = 0;
    for (int i = 0; i < n; i++)
    {
        sum += XVM_GetParamAsInt(vm, i);
    }
    XVM_ReturnIntFromHost(vm, sum / n);
}

static void h_PrintString(XVM_State* vm)
{
	char* str = XVM_GetParamAsString(vm, 0);
	puts(str);
	XVM_ReturnFromHost(vm);
}

static void h_PrintInt(XVM_State* vm)
{
	int i = XVM_GetParamAsInt(vm, 0);
	printf("Explode %d!\n", i);
	XVM_ReturnFromHost(vm);
}

static void h_Division(XVM_State* vm)
{
	float i = XVM_GetParamAsFloat(vm, 0);
	float j = XVM_GetParamAsFloat(vm, 1);
	printf("%f\n", i / j);
	XVM_ReturnFromHost(vm);
}

// ----XVM Entry Main ----------------------------------------------------------------------------------

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
		XVM_CompileScript(inputFilename, ExecFileName);
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
    XVM_State* vm = XVM_Create();

    // 注册宿主api
	XVM_RegisterHostFunc(XVM_GLOBAL_FUNC, "Explode", h_PrintInt);
	XVM_RegisterHostFunc(XVM_GLOBAL_FUNC, "Division", h_Division);
	if (!XVM_RegisterHostFunc(XVM_GLOBAL_FUNC, "PrintString", h_PrintString))
    {
        printf("Register Host API Failed!");
        exit(1);
    }

    // An error code
    int iErrorCode;

    // Load the demo script
    iErrorCode = XVM_LoadXSE(vm, ExecFileName);

    // Check for an error
    if (iErrorCode != XVM_LOAD_OK)
    {
        print_error_message(iErrorCode);
        exit(1);
    }

    // Run we're loaded script from Main()
    XVM_RunScript(vm, XVM_INFINITE_TIMESLICE);

    int iExitCode = XVM_GetExitCode(vm);

    // Free resources and perform general cleanup
    XVM_ShutDown(vm);

    return iExitCode;
}


int main(int argc, char* argv[])
{
	unsigned long start = GetCurrTime();
	if (argc < 2) 
	{
		printf("XVM: No input files\n");
		exit(0);
	}

	RunScript(argv[1]);

	printf("耗时 %fs\n", (GetCurrTime()-start)/1000.0);
}

#endif
