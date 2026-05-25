#define HAS_SIEMENS_ENGINEERING

#using <mscorlib.dll>
#using <System.dll>
#using <System.Core.dll>
#using <System.Xml.dll>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Xml;
using namespace System::IO;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;
using namespace Siemens::Engineering;
using namespace Siemens::Engineering::HW;
using namespace Siemens::Engineering::HW::Features;
using namespace Siemens::Engineering::SW;
using namespace Siemens::Engineering::Compiler;
using namespace Siemens::Engineering::SW::Blocks;
using namespace Siemens::Engineering::SW::Tags;

#include "TiaBridge.h"
#include "TiaUtils.h"
#include "TiaExport.h"
#include "TiaImport.h"
#include "RoundTrip.h"

static bool g_tiaAvailable = false;

bool TiaBridge_IsAvailable() {
    return g_tiaAvailable;
}

TiaPortalHandle TiaBridge_Connect(String^ tiaVer) {
    TiaPortal^ portal = nullptr;
    try {
        IList<TiaPortalProcess^>^ processes = TiaPortal::GetProcesses();
        if (processes->Count > 0) {
            portal = processes[0]->Attach();
            Console::WriteLine("Attached to running TIA Portal instance!");
        }
        else {
            Console::WriteLine("No running TIA Portal found. Starting new instance...");
            portal = gcnew TiaPortal(TiaPortalMode::WithUserInterface);
            Console::WriteLine("Connected successfully!");
        }
        g_tiaAvailable = true;
    }
    catch (Exception^ e) {
        Console::WriteLine("Failed to connect to TIA Portal: " + e->Message);
        PrintInnerExceptions(e, 1);
        Console::WriteLine("Please ensure:");
        Console::WriteLine("  1. TIA Portal " + tiaVer + " is installed");
        Console::WriteLine("  2. Run this program as administrator");
        Console::WriteLine("  3. User is in 'Siemens TIA Openness' user group");
        g_tiaAvailable = false;
    }
    return portal;
}

void TiaBridge_Disconnect(TiaPortalHandle portal) {
    if (portal != nullptr) {
        TiaPortal^ tiaPortal = dynamic_cast<TiaPortal^>(portal);
        delete tiaPortal;
    }
}

void TiaBridge_DoImport(TiaPortalHandle portal) {
    if (portal == nullptr) return;
    TiaPortal^ tiaPortal = dynamic_cast<TiaPortal^>(portal);
    DoImportInteractive(tiaPortal);
}

String^ TiaBridge_DoImportWithPath(TiaPortalHandle portal, String^ importPath) {
    if (portal == nullptr) return "ERROR: portal is null";
    TiaPortal^ tiaPortal = dynamic_cast<TiaPortal^>(portal);
    return DoImport(tiaPortal, importPath);
}

void TiaBridge_DoExport(TiaPortalHandle portal) {
    if (portal == nullptr) return;
    TiaPortal^ tiaPortal = dynamic_cast<TiaPortal^>(portal);
    DoExport(tiaPortal);
}

void TiaBridge_DoRoundTripTest(TiaPortalHandle portal) {
    if (portal == nullptr) return;
    TiaPortal^ tiaPortal = dynamic_cast<TiaPortal^>(portal);
    DoRoundTripTest(tiaPortal);
}