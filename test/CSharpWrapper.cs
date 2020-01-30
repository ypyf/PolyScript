using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using System.IO;   // DllImport

namespace TileEngine
{
    unsafe class ScriptLoader
    {
        #region Poly.dll 接口声明
        unsafe public struct ScriptContext { };

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void POLY_HOST_FUNCTION(ScriptContext* sc);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static ScriptContext* Poly_Initialize();

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_ShutDown(ScriptContext* sc);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_CompileScript(string pstrFilename, string pstrExecFilename);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static int Poly_LoadPE(ScriptContext* sc, string pstrFilename);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_UnloadScript(ScriptContext* sc);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_ResetInterp(ScriptContext* sc);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_RunScript(ScriptContext* sc, int iTimesliceDur);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_StartScript(ScriptContext* sc);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_StopScript(ScriptContext* sc);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_PauseScript(ScriptContext* sc, int iDur);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_ResumeScript(ScriptContext* sc);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_PassIntParam(ScriptContext* sc, int iInt);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_PassFloatParam(ScriptContext* sc, float fFloat);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_PassStringParam(ScriptContext* sc, string pstrString);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static int Poly_CallScriptFunc(ScriptContext* sc, string pstrName);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_CallScriptFuncSync(ScriptContext* sc, string pstrName);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static int Poly_GetReturnValueAsInt(ScriptContext* sc);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static float Poly_GetReturnValueAsFloat(ScriptContext* sc);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static string Poly_GetReturnValueAsString(ScriptContext* sc);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static int Poly_RegisterHostFunc(ScriptContext* sc, string pstrName, POLY_HOST_FUNCTION fnFunc);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static int Poly_GetParamAsInt(ScriptContext* sc, int iParamIndex);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static float Poly_GetParamAsFloat(ScriptContext* sc, int iParamIndex);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static IntPtr Poly_GetParamAsString(ScriptContext* sc, int iParamIndex);

        //[DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        //extern static Value Poly_GetParam(ScriptContext *sc, int iParamIndex);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_ReturnFromHost(ScriptContext* sc);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_ReturnIntFromHost(ScriptContext* sc, int iInt);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_ReturnFloatFromHost(ScriptContext* sc, float iFloat);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static void Poly_ReturnStringFromHost(ScriptContext* sc, string pstrString);

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static int Poly_GetParamCount(ScriptContext* sc);       // 获取传递给函数的参数个数

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static int Poly_IsScriptStop(ScriptContext* sc);        // 脚本是否已经停止

        [DllImport("poly_d32.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static int Poly_GetExitCode(ScriptContext* sc);         // 脚本退出代码

        #endregion

        static readonly ScriptContext* POLY_GLOBAL_FUNC = null;

        private ScriptContext* m_pScript;

        public ScriptLoader()
        {
            m_pScript = Poly_CreateInterp();
        }

        ~ScriptLoader()
        {
            Poly_ShutDown(m_pScript);
        }

        public void loadScript(string pstrFilename)
        {
            // 构造可执行文件名
            string ExecFileName = Path.ChangeExtension(pstrFilename, ".pe");
            Poly_CompileScript(pstrFilename, ExecFileName);

            // An error code
            int iErrorCode = Poly_LoadPE(m_pScript, ExecFileName);

            // TODO  Check for an error

            // 开始由线程索引指定的脚本
            Poly_StartScript(m_pScript);
        }

        public void registerHostFunc(string pstrName, POLY_HOST_FUNCTION fnFunc)
        {
            int i = Poly_RegisterHostFunc(POLY_GLOBAL_FUNC, pstrName, fnFunc);
            if (i != 1)
            {
                Console.WriteLine("注册函数 {0} 失败", pstrName);
            }
        }

        public void callScriptFunc(string pstrName)
        {
            Poly_CallScriptFunc(m_pScript, pstrName);
        }

        public void runScript()
        {
            Poly_RunScript(m_pScript, 100);
        }

        public string getStringParameter(int iIndex)
        {
            var x = Poly_GetParamAsString(m_pScript, iIndex);
            return Marshal.PtrToStringAnsi(x);
        }

        public void returnFromHost()
        {
            Poly_ReturnFromHost(m_pScript);
        }
    }
}
