#pragma once

inline char g_tiaVersion[16] = "";

inline String^ GetTiaVersion() {
    if (g_tiaVersion[0] == '\0') return "";
    return gcnew String(g_tiaVersion);
}

inline void SetTiaVersion(String^ ver) {
    if (ver == nullptr || ver->Length == 0) {
        g_tiaVersion[0] = '\0';
        return;
    }
    for (int i = 0; i < ver->Length && i < 15; i++) {
        g_tiaVersion[i] = (char)ver[i];
    }
    g_tiaVersion[ver->Length < 15 ? ver->Length : 15] = '\0';
}

inline String^ DetectTiaVersion() {
    array<String^>^ versions = gcnew array<String^> { "V20", "V19", "V18", "V17", "V16" };
    for each (String^ ver in versions) {
        String^ dllPath = "C:\\Program Files\\Siemens\\Automation\\Portal " + ver +
            "\\PublicAPI\\" + ver + "\\Siemens.Engineering.dll";
        if (File::Exists(dllPath)) {
            Console::WriteLine("Detected TIA Portal " + ver);
            SetTiaVersion(ver);
            return ver;
        }
    }
    return "";
}

inline Assembly^ OnAssemblyResolve(Object^ sender, ResolveEventArgs^ args) {
    String^ ver = GetTiaVersion();
    if (ver == "") return nullptr;

    String^ argName = args->Name;
    if (!argName->StartsWith("Siemens.Engineering")) return nullptr;

    int commaPos = argName->IndexOf(',');
    String^ simpleName = commaPos > 0 ? argName->Substring(0, commaPos) : argName;
    if (!simpleName->EndsWith(".dll")) {
        simpleName = simpleName + ".dll";
    }

    array<String^>^ searchPaths = gcnew array<String^> {
        "C:\\Program Files\\Siemens\\Automation\\Portal " + ver + "\\PublicAPI\\" + ver,
        "C:\\Program Files\\Siemens\\Automation\\Portal " + ver + "\\Bin\\PublicAPI",
        "C:\\Program Files\\Siemens\\Automation\\Portal " + ver + "\\bin"
    };

    for each (String^ searchPath in searchPaths) {
        String^ dllPath = Path::Combine(searchPath, simpleName);
        if (File::Exists(dllPath)) {
            return Assembly::LoadFrom(dllPath);
        }
    }
    return nullptr;
}

#ifdef HAS_SIEMENS_ENGINEERING

inline void CompileBlock(PlcBlock^ block) {
    try {
        IEngineeringServiceProvider^ provider = dynamic_cast<IEngineeringServiceProvider^>(block);
        if (provider != nullptr) {
            ICompilable^ compilable = dynamic_cast<ICompilable^>(provider->GetService(ICompilable::typeid));
            if (compilable != nullptr) {
                compilable->Compile();
            }
        }
    }
    catch (Exception^) {}
}

inline PlcSoftware^ FindPlcSoftware(Project^ project) {
    if (project == nullptr || project->Devices == nullptr) return nullptr;

    for each (Device^ device in project->Devices) {
        if (device->DeviceItems == nullptr) continue;
        for each (DeviceItem^ devItem in device->DeviceItems) {
            IEngineeringServiceProvider^ serviceProvider = dynamic_cast<IEngineeringServiceProvider^>(devItem);
            if (serviceProvider == nullptr) continue;
            Object^ serviceObj = serviceProvider->GetService(SoftwareContainer::typeid);
            if (serviceObj == nullptr) continue;
            SoftwareContainer^ container = dynamic_cast<SoftwareContainer^>(serviceObj);
            if (container != nullptr && container->Software != nullptr) {
                PlcSoftware^ plcSw = dynamic_cast<PlcSoftware^>(container->Software);
                if (plcSw != nullptr) return plcSw;
            }
        }
    }
    return nullptr;
}

#endif

[DllImport("kernel32.dll", CharSet = CharSet::Auto, SetLastError = true)]
extern "C" bool SetDllDirectory(String^ lpPathName);

inline void SetDllDirectoryForTia(String^ tiaVersion) {
    String^ tiaBaseDir = "C:\\Program Files\\Siemens\\Automation\\Portal " + tiaVersion;
    String^ tiaBinDir = Path::Combine(tiaBaseDir, "bin");

    bool ok = SetDllDirectory(tiaBinDir);

    if (ok) {
        Console::WriteLine("Set DLL directory: " + tiaBinDir);
    }
    else {
        Console::WriteLine("Warning: Failed to set DLL directory: " + tiaBinDir);
    }

    String^ currentPath = Environment::GetEnvironmentVariable("PATH");
    if (!currentPath->Contains(tiaBinDir)) {
        Environment::SetEnvironmentVariable("PATH", tiaBinDir + ";" + currentPath);
        Console::WriteLine("Added to PATH: " + tiaBinDir);
    }
}

inline void PrintInnerExceptions(Exception^ e, int depth) {
    if (e == nullptr) return;
    String^ indent = gcnew String(' ', depth * 2);
    Console::WriteLine(indent + "Exception: " + e->GetType()->Name + " - " + e->Message);
    if (e->InnerException != nullptr) {
        PrintInnerExceptions(e->InnerException, depth + 1);
    }
    if (dynamic_cast<ReflectionTypeLoadException^>(e) != nullptr) {
        ReflectionTypeLoadException^ tle = dynamic_cast<ReflectionTypeLoadException^>(e);
        for each (Exception^ loaderEx in tle->LoaderExceptions) {
            PrintInnerExceptions(loaderEx, depth + 1);
        }
    }
}

inline void ShowMenu(bool tiaAvailable) {
    String^ ver = GetTiaVersion();
    Console::WriteLine();
    Console::WriteLine("========================================");
    Console::WriteLine("  TIA Portal Openness Automation Tool");
    Console::WriteLine("  Detected version: " + (ver != "" ? ver : "N/A"));
    Console::WriteLine("  TIA Portal: " + (tiaAvailable ? "Available" : "Unavailable"));
    Console::WriteLine("========================================");
    Console::WriteLine("  1. Import (from files to TIA Portal)" + (tiaAvailable ? "" : " [requires TIA Portal]"));
    Console::WriteLine("  2. Export (from TIA Portal to files)" + (tiaAvailable ? "" : " [requires TIA Portal]"));
    Console::WriteLine("  3. Round-trip Test (export->import->export->compare)" + (tiaAvailable ? "" : " [requires TIA Portal]"));
    Console::WriteLine("  4. AI Code Generation (JSON DSL -> LAD XML)");
    Console::WriteLine("  5. AI Auto Generate (Natural Language -> LLM -> PLC)");
    Console::WriteLine("  6. AI DSL File Generate (P3 DSL JSON -> LAD XML)");
    Console::WriteLine("  7. AI Phase4 Generate (Multi-Step: Requirement->Plan->Variable->StateMachine->PLC)");
    Console::WriteLine("  8. AI Phase5 Generate (Compiler Platform: CFG->IR->Optimizer->Multi-Backend)");
    Console::WriteLine("  9. AI Phase6 Generate (Agent: Reasoning->Synthesis->Simulation->HMI->Doc)");
    Console::WriteLine("  10. Exit");
    Console::WriteLine("========================================");
    Console::Write("  Please select (1/2/3/4/5/6/7/8/9/10): ");
}