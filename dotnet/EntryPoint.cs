using System.Diagnostics;
using System.Runtime.InteropServices;

namespace DotnetOsxDebuggingRepro;
public class EntryPoint
{
    [UnmanagedCallersOnly]
    public static void PluginEntry() 
    {
        Console.WriteLine(@"
===========================

Plugin entered successfully

===========================");

        _ = WaitForDebuggerAsync();
    }

    private static async Task WaitForDebuggerAsync()
    {
        Console.WriteLine("Waiting for debugger...");

        while (!Debugger.IsAttached)
            await Task.Delay(500);

        Console.WriteLine("Debugger attached");
    }
}
