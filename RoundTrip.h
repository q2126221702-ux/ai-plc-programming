#pragma once
#include "DataTypes.h"
#include "TiaUtils.h"
#include "TiaExport.h"
#include "TiaImport.h"
#include "LadXmlParser.h"
#include "JsonSerializer.h"

ref struct RoundTripDiff {
    String^ BlockName;
    bool Pass;
    String^ Details;
};

inline bool CompareLadElements(LadElement^ a, LadElement^ b) {
    if (a->Type != b->Type) return false;
    if (a->Tag != b->Tag) return false;
    if (a->NormallyOpen != b->NormallyOpen) return false;
    if (a->InstanceName != b->InstanceName) return false;
    if (a->PresetTime != b->PresetTime) return false;
    if (a->Tag2 != b->Tag2) return false;
    if (a->Tag3 != b->Tag3) return false;
    if (a->DataType != b->DataType) return false;

    if (a->Type == "parallel") {
        if (a->Branches == nullptr && b->Branches == nullptr) return true;
        if (a->Branches == nullptr || b->Branches == nullptr) return false;
        if (a->Branches->Count != b->Branches->Count) return false;
        for (int i = 0; i < a->Branches->Count; i++) {
            List<LadElement^>^ ba = a->Branches[i];
            List<LadElement^>^ bb = b->Branches[i];
            if (ba->Count != bb->Count) return false;
            for (int j = 0; j < ba->Count; j++) {
                if (!CompareLadElements(ba[j], bb[j])) return false;
            }
        }
    }
    return true;
}

inline RoundTripDiff^ CompareLadDsl(LadDsl^ dsl1, LadDsl^ dsl2, String^ blockName) {
    RoundTripDiff^ diff = gcnew RoundTripDiff();
    diff->BlockName = blockName;
    diff->Pass = true;
    diff->Details = "";

    System::Text::StringBuilder^ details = gcnew System::Text::StringBuilder();

    if (dsl1->Networks->Count != dsl2->Networks->Count) {
        diff->Pass = false;
        details->AppendFormat("Network count: DSL1={0}, DSL2={1}\n",
            dsl1->Networks->Count, dsl2->Networks->Count);
    }

    int minNets = Math::Min(dsl1->Networks->Count, dsl2->Networks->Count);
    for (int ni = 0; ni < minNets; ni++) {
        LadNetwork^ n1 = dsl1->Networks[ni];
        LadNetwork^ n2 = dsl2->Networks[ni];

        if (n1->Elements->Count != n2->Elements->Count) {
            diff->Pass = false;
            details->AppendFormat("  Net[{0}] element count: {1} vs {2}\n",
                ni, n1->Elements->Count, n2->Elements->Count);
        }

        int minElems = Math::Min(n1->Elements->Count, n2->Elements->Count);
        for (int ei = 0; ei < minElems; ei++) {
            LadElement^ e1 = n1->Elements[ei];
            LadElement^ e2 = n2->Elements[ei];

            if (!CompareLadElements(e1, e2)) {
                diff->Pass = false;
                details->AppendFormat("  Net[{0}].Elem[{1}]: type={2} vs {3}",
                    ni, ei, e1->Type, e2->Type);
                if (e1->Type == e2->Type) {
                    if (e1->Tag != e2->Tag)
                        details->AppendFormat(" tag={0} vs {1}", e1->Tag, e2->Tag);
                    if (e1->NormallyOpen != e2->NormallyOpen)
                        details->AppendFormat(" NO={0} vs {1}", e1->NormallyOpen, e2->NormallyOpen);
                    if (e1->InstanceName != e2->InstanceName)
                        details->AppendFormat(" inst={0} vs {1}", e1->InstanceName, e2->InstanceName);
                    if (e1->PresetTime != e2->PresetTime)
                        details->AppendFormat(" pt={0} vs {1}", e1->PresetTime, e2->PresetTime);
                    if (e1->Tag2 != e2->Tag2)
                        details->AppendFormat(" tag2={0} vs {1}", e1->Tag2, e2->Tag2);
                    if (e1->Tag3 != e2->Tag3)
                        details->AppendFormat(" tag3={0} vs {1}", e1->Tag3, e2->Tag3);
                    if (e1->DataType != e2->DataType)
                        details->AppendFormat(" dataType={0} vs {1}", e1->DataType, e2->DataType);
                }
                details->Append("\n");
            }
        }
    }

    diff->Details = details->ToString();
    return diff;
}

inline void DoRoundTripTest(TiaPortal^ portal) {
    Project^ project = nullptr;
    if (portal->Projects->Count > 0) {
        project = portal->Projects[0];
    }
    if (project == nullptr) {
        Console::WriteLine("No open project found.");
        return;
    }

    Console::WriteLine("Current project: " + project->Name);

    PlcSoftware^ plcSw = FindPlcSoftware(project);
    if (plcSw == nullptr) {
        Console::WriteLine("No PLC software found.");
        return;
    }

    String^ workDir = "D:\\TIA_RoundTrip";
    Console::Write("Enter work directory (press Enter for D:\\TIA_RoundTrip): ");
    String^ input = Console::ReadLine()->Trim();
    if (input->Length > 0) workDir = input;

    String^ pass1Dir = Path::Combine(workDir, "Pass1_Export");
    String^ pass2Dir = Path::Combine(workDir, "Pass2_Export");

    if (!Directory::Exists(pass1Dir)) Directory::CreateDirectory(pass1Dir);
    if (!Directory::Exists(pass2Dir)) Directory::CreateDirectory(pass2Dir);

    Console::WriteLine();
    Console::WriteLine("=== Phase 1: Export from TIA (DSL1) ===");
    Console::WriteLine("Export path: " + pass1Dir);

    if (plcSw->BlockGroup != nullptr) {
        String^ blockPath = Path::Combine(pass1Dir, "Source_" + plcSw->Name);
        ExportBlockSource(plcSw->BlockGroup, blockPath);
    }

    Console::WriteLine();
    Console::WriteLine("=== Phase 2: Import DSL1 into TIA ===");
    bool importOk = ImportLadDsl(plcSw, Path::Combine(pass1Dir, "Source_" + plcSw->Name));

    if (!importOk) {
        Console::WriteLine();
        Console::WriteLine("!!! Phase 2 IMPORT FAILED - cannot verify round-trip !!!");
        Console::WriteLine("!!! DSL comparison will be skipped !!!");
    }

    Console::WriteLine();
    Console::WriteLine("=== Phase 3: Export again from TIA (DSL2) ===");
    Console::WriteLine("Export path: " + pass2Dir);

    if (plcSw->BlockGroup != nullptr) {
        String^ blockPath = Path::Combine(pass2Dir, "Source_" + plcSw->Name);
        ExportBlockSource(plcSw->BlockGroup, blockPath);
    }

    Console::WriteLine();
    Console::WriteLine("=== Phase 4: Compare DSL1 vs DSL2 ===");

    if (!importOk) {
        Console::WriteLine("  [FAIL] Import failed in Phase 2 - round-trip test FAILED");
        Console::WriteLine();
        Console::WriteLine("=== Round-trip Test Results ===");
        Console::WriteLine("  PASS: 0");
        Console::WriteLine("  FAIL: 1 (import failure)");
        Console::WriteLine("  Result: IMPORT FAILED - fix import errors first");
        return;
    }

    String^ pass1BlockDir = Path::Combine(pass1Dir, "Source_" + plcSw->Name);
    String^ pass2BlockDir = Path::Combine(pass2Dir, "Source_" + plcSw->Name);

    if (!Directory::Exists(pass1BlockDir) || !Directory::Exists(pass2BlockDir)) {
        Console::WriteLine("Missing export directories. Cannot compare.");
        return;
    }

    array<String^>^ pass1Jsons = Directory::GetFiles(pass1BlockDir, "*.lad.json");
    int totalPass = 0;
    int totalFail = 0;

    for each (String^ p1Json in pass1Jsons) {
        String^ baseName = Path::GetFileNameWithoutExtension(p1Json);
        baseName = Path::GetFileNameWithoutExtension(baseName);

        String^ p2Json = Path::Combine(pass2BlockDir, baseName + ".lad.json");

        if (!File::Exists(p2Json)) {
            Console::WriteLine("  [MISSING] " + baseName + " not found in Pass2");
            totalFail++;
            continue;
        }

        String^ json1 = File::ReadAllText(p1Json, gcnew System::Text::UTF8Encoding(false));
        String^ json2 = File::ReadAllText(p2Json, gcnew System::Text::UTF8Encoding(false));

        LadDsl^ dsl1 = JsonToLadDsl(json1);
        LadDsl^ dsl2 = JsonToLadDsl(json2);

        RoundTripDiff^ diff = CompareLadDsl(dsl1, dsl2, baseName);

        if (diff->Pass) {
            Console::WriteLine("  [PASS] " + baseName);
            totalPass++;
        }
        else {
            Console::WriteLine("  [FAIL] " + baseName);
            Console::Write(diff->Details);
            totalFail++;
        }
    }

    Console::WriteLine();
    Console::WriteLine("=== Round-trip Test Results ===");
    Console::WriteLine("  PASS: " + totalPass);
    Console::WriteLine("  FAIL: " + totalFail);
    Console::WriteLine("  Total: " + (totalPass + totalFail));

    if (totalFail == 0 && totalPass > 0) {
        Console::WriteLine("  Result: ALL TESTS PASSED - DSL1 == DSL2");
    }
    else if (totalFail > 0) {
        Console::WriteLine("  Result: SOME TESTS FAILED - DSL1 != DSL2");
    }
    else {
        Console::WriteLine("  Result: No LAD blocks found to test");
    }
}
