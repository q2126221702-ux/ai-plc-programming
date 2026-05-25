#pragma once
#include "DataTypes.h"
#include "JsonSerializer.h"
#include "LadXmlParser.h"
#include "TiaUtils.h"

inline void ExportLadDsl(PlcBlock^ block, String^ exportPath) {
    try {
        String^ xmlPath = Path::Combine(exportPath, block->Name + "_template.xml");
        if (File::Exists(xmlPath)) File::Delete(xmlPath);

        Console::WriteLine("    Exporting LAD block XML: " + block->Name);
        block->Export(gcnew FileInfo(xmlPath), ExportOptions::WithDefaults);

        String^ rawXml = File::ReadAllText(xmlPath, gcnew System::Text::UTF8Encoding(false));
        String^ flgStart = "<FlgNet";
        String^ flgEnd = "</FlgNet>";
        int fs = rawXml->IndexOf(flgStart);
        int fe = rawXml->IndexOf(flgEnd);
        if (fs >= 0 && fe >= 0) {
            String^ flgBlock = rawXml->Substring(fs, fe - fs + flgEnd->Length);
            String^ diagPath = Path::Combine(exportPath, block->Name + "_FlgNet_diag.xml");
            File::WriteAllText(diagPath, flgBlock, gcnew System::Text::UTF8Encoding(false));
            Console::WriteLine("      Saved raw FlgNet to: " + diagPath);
        }

        List<LadNetwork^>^ networks = ParseLadFlgNetsFromXml(xmlPath);

        if (networks->Count == 0) {
            Console::WriteLine("      Note: No LAD networks found (empty block)");
            LadDsl^ dsl = gcnew LadDsl();
            dsl->Networks = gcnew List<LadNetwork^>();
            String^ jsonPath = Path::Combine(exportPath, block->Name + ".lad.json");
            if (File::Exists(jsonPath)) File::Delete(jsonPath);
            String^ json = LadDslToJson(dsl);
            File::WriteAllText(jsonPath, json, gcnew System::Text::UTF8Encoding(false));
            Console::WriteLine("      DSL: " + jsonPath);
            return;
        }

        Console::WriteLine("      Parsed " + Convert::ToString(networks->Count) + " network(s)");

        LadDsl^ dsl = gcnew LadDsl();
        dsl->Networks = networks;

        String^ jsonPath = Path::Combine(exportPath, block->Name + ".lad.json");
        if (File::Exists(jsonPath)) File::Delete(jsonPath);
        String^ json = LadDslToJson(dsl);
        File::WriteAllText(jsonPath, json, gcnew System::Text::UTF8Encoding(false));
        Console::WriteLine("      DSL: " + jsonPath);
    }
    catch (Exception^ e) {
        Console::WriteLine("      Failed: " + e->Message);
    }
}

inline void ExportBlockSource(PlcBlockGroup^ blockGroup, String^ exportPath) {
    if (blockGroup == nullptr || blockGroup->Blocks == nullptr) return;

    if (!Directory::Exists(exportPath)) {
        Directory::CreateDirectory(exportPath);
    }

    for each (PlcBlock^ block in blockGroup->Blocks) {
        try {
            CompileBlock(block);
            ProgrammingLanguage lang = block->ProgrammingLanguage;

            if (lang == ProgrammingLanguage::SCL) {
                String^ fileName = block->Name + ".scl";
                String^ filePath = Path::Combine(exportPath, fileName);
                if (File::Exists(filePath)) {
                    File::Delete(filePath);
                }
                Console::WriteLine("    Exporting SCL source: " + block->Name);
                block->Export(gcnew FileInfo(filePath), ExportOptions::WithDefaults);
                Console::WriteLine("      OK: " + filePath);
            }
            else if (lang == ProgrammingLanguage::LAD) {
                ExportLadDsl(block, exportPath);
            }
            else {
                Console::WriteLine("    Skipping block: " + block->Name +
                    " (" + lang.ToString() + ") - not SCL or LAD");
            }
        }
        catch (Exception^ e) {
            Console::WriteLine("      Failed: " + e->Message);
        }
    }

    if (blockGroup->Groups != nullptr) {
        for each (PlcBlockGroup^ subGroup in blockGroup->Groups) {
            String^ subPath = Path::Combine(exportPath, subGroup->Name);
            ExportBlockSource(subGroup, subPath);
        }
    }
}

inline void ExportTagTables(PlcTagTableGroup^ tagTableGroup, String^ exportPath) {
    if (tagTableGroup == nullptr) return;

    if (!Directory::Exists(exportPath)) {
        Directory::CreateDirectory(exportPath);
    }

    if (tagTableGroup->TagTables != nullptr) {
        for each (PlcTagTable^ tagTable in tagTableGroup->TagTables) {
            try {
                String^ fileName = tagTable->Name + ".xml";
                String^ filePath = Path::Combine(exportPath, fileName);
                if (File::Exists(filePath)) {
                    File::Delete(filePath);
                }
                Console::WriteLine("    Exporting tag table: " + tagTable->Name);
                tagTable->Export(gcnew FileInfo(filePath), ExportOptions::WithDefaults);
                Console::WriteLine("      OK: " + filePath);
            }
            catch (Exception^ e) {
                Console::WriteLine("      Failed: " + e->Message);
            }
        }
    }

    if (tagTableGroup->Groups != nullptr) {
        for each (PlcTagTableGroup^ subGroup in tagTableGroup->Groups) {
            String^ subPath = Path::Combine(exportPath, subGroup->Name);
            ExportTagTables(subGroup, subPath);
        }
    }
}

inline void DoExport(TiaPortal^ portal) {
    Project^ project = nullptr;
    if (portal->Projects->Count > 0) {
        project = portal->Projects[0];
    }

    if (project == nullptr) {
        Console::WriteLine("No open project found. Please open a project in TIA Portal first.");
        return;
    }

    Console::WriteLine("Current project: " + project->Name);

    String^ exportPath = "D:\\TIA_Export";
    Console::Write("Enter export path (press Enter for D:\\TIA_Export): ");
    String^ input = Console::ReadLine()->Trim();
    if (input->Length > 0) {
        exportPath = input;
    }

    Console::WriteLine("Export path: " + exportPath);
    Console::WriteLine();
    Console::WriteLine("Starting export...");

    if (project->Devices == nullptr) {
        Console::WriteLine("No devices found in project.");
        return;
    }

    for each (Device^ dev in project->Devices) {
        Console::WriteLine("Found device: " + dev->Name);

        if (dev->DeviceItems == nullptr) continue;
        for each (DeviceItem^ devItem in dev->DeviceItems) {
            IEngineeringServiceProvider^ serviceProvider = dynamic_cast<IEngineeringServiceProvider^>(devItem);
            if (serviceProvider == nullptr) continue;
            Object^ serviceObj = serviceProvider->GetService(SoftwareContainer::typeid);
            if (serviceObj == nullptr) continue;
            SoftwareContainer^ container = dynamic_cast<SoftwareContainer^>(serviceObj);
            if (container == nullptr || container->Software == nullptr) continue;
            PlcSoftware^ plcSw = dynamic_cast<PlcSoftware^>(container->Software);
            if (plcSw == nullptr) continue;

            Console::WriteLine("  Found PLC software: " + plcSw->Name);

            if (plcSw->BlockGroup != nullptr) {
                String^ blockPath = Path::Combine(exportPath, "Source_" + plcSw->Name);
                Console::WriteLine("    Exporting block source code...");
                ExportBlockSource(plcSw->BlockGroup, blockPath);
            }

            if (plcSw->TagTableGroup != nullptr) {
                String^ tagPath = Path::Combine(exportPath, "TagTables_" + plcSw->Name);
                Console::WriteLine("    Exporting tag tables...");
                ExportTagTables(plcSw->TagTableGroup, tagPath);
            }
        }
    }

    Console::WriteLine();
    Console::WriteLine("=== Export completed! ===");
    Console::WriteLine("Files saved to: " + exportPath);
}