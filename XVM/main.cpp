﻿#if 1

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

#include "xasm.h"
#include "xvm.h"


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

int RunScript(char* pstrFilename)
{
    char ExecFileName[MAX_PATH];

    // 构造 .XSE 文件名
    if (strstr(pstrFilename, XASM_SRC_FILE_EXT))
    {
        int ExtOffset = strrchr(pstrFilename, '.') - pstrFilename;
        strncpy(ExecFileName, pstrFilename, ExtOffset);
        ExecFileName[ExtOffset] = '\0';
        strcat(ExecFileName, XVM_EXEC_FILE_EXT);

        // 编译
        XASM_Assembly(pstrFilename, ExecFileName);
    }
    else if (strstr(pstrFilename, XVM_EXEC_FILE_EXT))
    {
        strcpy(ExecFileName, pstrFilename);
    }
    else
    {
        printf("unexpected script file name.\n");
        return FALSE;
    }

    // Initialize the runtime environment
    XVM_Init();

    // 注册宿主api
    if (!XVM_RegisterHostFunction(XVM_GLOBAL_FUNC, "average", average))
    {
        printf("Register Host API Failed!");
        ExitProcess(1);
    }

    // Declare the thread indices
    int iThreadIndex;

    // An error code
    int iErrorCode;

    // Load the demo script
    iErrorCode = XVM_LoadScript(ExecFileName, iThreadIndex, XVM_THREAD_PRIORITY_USER);

    // Check for an error
    if (iErrorCode != XVM_LOAD_OK)
    {
        print_error_message(iErrorCode);
        exit(1);
    }

    // 开始由线程索引指定的脚本
    //XVM_StartScript(iThreadIndex);

    // Run we're loaded script from Main()
    XVM_RunScript(iThreadIndex, XVM_INFINITE_TIMESLICE);

    int iExitCode = XVM_GetExitCode(iThreadIndex);

    // Free resources and perform general cleanup
    XVM_ShutDown();

    return iExitCode;
}

int main(int argc, char* argv[])
{
    int iExitCode;

    if (argc < 2)
    {
        printf("XVM: No input files\n");
        exit(0);
    }

    // 运行脚本并返回
    iExitCode = RunScript(argv[1]);

    printf("退出代码 (%i)\n", iExitCode);

    // keep window open
    getchar();
}

#endif
