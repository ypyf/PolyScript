#if 1

#define STANDALONE
#define _CRT_SECURE_NO_WARNINGS

// ----Include Files -------------------------------------------------------------------------
#include <stdio.h>
#include <iostream>
#include <string>
#include <functional>
#include <algorithm>
#include <conio.h>
#include <windows.h>
#include <imagehlp.h>

#include "xvm.h"


void PrintUsage()
{
    printf("Usage:\tXASM Source.XASM [Executable.XSE]\n");
    printf("\n");
    printf("\t- File extensions are not required.\n");
    printf("\t- Executable name is optional; source name is used by default.\n");
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
static void average(int iThreadIndex)
{
    int n = XVM_GetParamCount(iThreadIndex);
    int sum = 0;
    for (int i = 0; i < n; i++)
    {
        sum += XVM_GetParamAsInt(iThreadIndex, i);
    }
    XVM_ReturnIntFromHost(iThreadIndex, sum / n);
}

// ----XVM Entry Main ----------------------------------------------------------------------------------

int RunScript(const char* filename)
{
    // Initialize the runtime environment
    XVM_Init();

    // 注册宿主api
    if (!XVM_RegisterCFunction(XVM_GLOBAL_FUNC, "average", average))
    {
        printf("Register Host API Failed!");
        ExitProcess(1);
    }

    // Declare the thread indices
    int iThreadIndex;

    // An error code
    int iErrorCode;

    // Load the demo script
    iErrorCode = XVM_LoadScript(filename, iThreadIndex, XVM_THREAD_PRIORITY_USER);

    // Check for an error
    if (iErrorCode != XVM_LOAD_OK)
    {
        print_error_message(iErrorCode);
        exit(1);
    }

    // 开始由线程索引指定的脚本
    XVM_StartScript(iThreadIndex);

    // 运行脚本
    XVM_RunScript(XVM_INFINITE_TIMESLICE);

    int iExitCode = XVM_GetExitCode(iThreadIndex);

    // Free resources and perform general cleanup
    XVM_ShutDown();

    return iExitCode;
}

int main(int argc, char* argv[])
{
    char SrcFileName[MAX_PATH] = {0};
    char ExecFileName[MAX_PATH] = {0};

    SetCurrentDirectory(TEXT("../Debug"));

    if (argc < 2)
    {
        PrintUsage();
        return 0;
    }

    // 根据参数构造源文件名
    strcpy(SrcFileName, argv[1]);
    if (!strstr(SrcFileName, SOURCE_FILE_EXT))
    {
        strcat(SrcFileName, SOURCE_FILE_EXT);
    }

    // 如果指定了输出文件名（字节码文件）
    if (argv[2])
    {
        strcpy(ExecFileName, argv[2]);

        // Check for ( the presence of the .MP extension and add it if it's not there
        if (!strstr(ExecFileName, EXEC_FILE_EXT))
        {
            strcat(ExecFileName, EXEC_FILE_EXT);
        }
    }
    else
    {
        // 如果没有指定输出文件名，则根据输入文件名进行构造
        // 将扩展名前的部分作为输出文件名并添加后缀
        int ExtOffset = strrchr(SrcFileName, '.') - SrcFileName;
        strncpy(ExecFileName, SrcFileName, ExtOffset);
        ExecFileName[ExtOffset] = '\0';
        strcat(ExecFileName, EXEC_FILE_EXT);
    }

    // 运行脚本并返回
    int iExitCode = RunScript(SrcFileName);

    printf("退出代码 (%i)\n", iExitCode);
}

#endif
