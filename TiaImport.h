#pragma once
#include "DataTypes.h"
#include "JsonSerializer.h"
#include "LadConversion.h"
#include "LadXmlParser.h"
#include "TiaUtils.h"

inline bool ImportLadDsl(PlcSoftware^ plcSw, String^ importPath) {
    bool allOk = true;

    if (plcSw == nullptr || !Directory::Exists(importPath)) {
        Console::WriteLine("    PLC software is null or import path does not exist.");
        return false;
    }

    array<String^>^ jsonFiles = Directory::GetFiles(importPath, "*.lad.json");
    if (jsonFiles->Length == 0) {
        Console::WriteLine("    No .lad.json files found in: " + importPath);
        return false;
    }

    for each (String^ jsonPath in jsonFiles) {
        try {
            String^ baseName = Path::GetFileNameWithoutExtension(jsonPath);
            baseName = Path::GetFileNameWithoutExtension(baseName);

            String^ templatePath = Path::Combine(importPath, baseName + "_template.xml");
            if (!File::Exists(templatePath)) {
                Console::WriteLine("    Template XML not found for: " + baseName + ", skipping.");
                allOk = false;
                continue;
            }

            String^ json = File::ReadAllText(jsonPath, gcnew System::Text::UTF8Encoding(false));
            LadDsl^ dsl = JsonToLadDsl(json);

            if (dsl->Networks->Count == 0) {
                Console::WriteLine("    Empty DSL for: " + baseName + ", skipping.");
                allOk = false;
                continue;
            }

            String^ newXml = BuildLadXml(dsl, templatePath);
            if (newXml == nullptr) { allOk = false; continue; }

            String^ newXmlPath = Path::Combine(importPath, baseName + "_rebuilt.xml");
            if (File::Exists(newXmlPath)) File::Delete(newXmlPath);
            File::WriteAllText(newXmlPath, newXml, gcnew System::Text::UTF8Encoding(false));

            // Save rebuilt XML to disk for debugging
            String^ debugXmlPath = Path::Combine(Path::GetDirectoryName(newXmlPath), 
                Path::GetFileNameWithoutExtension(newXmlPath) + "_debug.xml");
            File::WriteAllText(debugXmlPath, newXml, gcnew System::Text::UTF8Encoding(false));
            Console::WriteLine("    [DEBUG] Saved rebuilt XML to: " + debugXmlPath);

            Console::WriteLine("    Generated LAD XML: " + baseName + "_rebuilt.xml");
            Console::WriteLine("      Networks rebuilt: " + Convert::ToString(dsl->Networks->Count));

            {
                XmlDocument^ diagDoc = gcnew XmlDocument();
                diagDoc->Load(newXmlPath);

                HashSet<String^>^ validUIds = gcnew HashSet<String^>();
                XmlNodeList^ accessNodes = diagDoc->GetElementsByTagName("Access");
                for each (XmlNode^ an in accessNodes) {
                    XmlElement^ ae = dynamic_cast<XmlElement^>(an);
                    if (ae != nullptr) validUIds->Add(ae->GetAttribute("UId"));
                }
                XmlNodeList^ partNodes2 = diagDoc->GetElementsByTagName("Part");
                for each (XmlNode^ pn in partNodes2) {
                    XmlElement^ pe = dynamic_cast<XmlElement^>(pn);
                    if (pe != nullptr) validUIds->Add(pe->GetAttribute("UId"));
                }
                XmlNodeList^ callNodes2 = diagDoc->GetElementsByTagName("Call");
                for each (XmlNode^ cn in callNodes2) {
                    XmlElement^ ce = dynamic_cast<XmlElement^>(cn);
                    if (ce != nullptr) validUIds->Add(ce->GetAttribute("UId"));
                }

                XmlNodeList^ identConNodes = diagDoc->GetElementsByTagName("IdentCon");
                for each (XmlNode^ icn in identConNodes) {
                    XmlElement^ ice = dynamic_cast<XmlElement^>(icn);
                    if (ice != nullptr && !validUIds->Contains(ice->GetAttribute("UId"))) {
                        Console::WriteLine("      [DIAG-ERROR] IdentCon UId={0} has no matching Access/Part/Call!",
                            ice->GetAttribute("UId"));
                    }
                }

                XmlNodeList^ partNodes = diagDoc->GetElementsByTagName("Part");
                for each (XmlNode^ pn in partNodes) {
                    XmlElement^ pe = dynamic_cast<XmlElement^>(pn);
                    if (pe != nullptr) {
                        Console::WriteLine("      [DIAG-PART] Name='{0}' UId={1} Version={2}",
                            pe->GetAttribute("Name"), pe->GetAttribute("UId"), pe->GetAttribute("Version"));
                    }
                }
                XmlNodeList^ instNodes = diagDoc->GetElementsByTagName("Instance");
                for each (XmlNode^ in2 in instNodes) {
                    XmlElement^ ie = dynamic_cast<XmlElement^>(in2);
                    if (ie != nullptr) {
                        Console::WriteLine("      [DIAG-INSTANCE] Type='{0}' Scope='{1}' UId='{2}'",
                            ie->GetAttribute("Type"), ie->GetAttribute("Scope"), ie->GetAttribute("UId"));
                    }
                }
            }

            if (plcSw->BlockGroup != nullptr && plcSw->BlockGroup->Blocks != nullptr) {
                Console::WriteLine("    Importing LAD block: " + baseName);
                plcSw->BlockGroup->Blocks->Import(gcnew FileInfo(newXmlPath), ImportOptions::Override);
                Console::WriteLine("      OK");
            }

        }
        catch (Exception^ e) {
            Console::WriteLine("      Failed: " + e->Message);
            allOk = false;
        }
    }

    return allOk;
}

inline void ImportBlockSource(PlcSoftware^ plcSw, String^ importPath) {
    if (plcSw == nullptr || !Directory::Exists(importPath)) {
        Console::WriteLine("    PLC software is null or import path does not exist.");
        return;
    }

    array<String^>^ files = Directory::GetFiles(importPath, "*.scl");
    int importCount = 0;

    for each (String^ filePath in files) {
        try {
            String^ fileName = Path::GetFileName(filePath);
            String^ blockName = Path::GetFileNameWithoutExtension(fileName);

            Console::WriteLine("    Importing SCL source: " + fileName);
            plcSw->BlockGroup->Blocks->Import(gcnew FileInfo(filePath), ImportOptions::Override);
            Console::WriteLine("      OK");
            importCount++;
        }
        catch (Exception^ e) {
            Console::WriteLine("      Failed: " + e->Message);
            Console::WriteLine("      Hint: Ensure the SCL source is valid and compilable.");
        }
    }

    if (importCount == 0) {
        Console::WriteLine("    No SCL source files found in: " + importPath);
    }

    if (plcSw->BlockGroup != nullptr && plcSw->BlockGroup->Groups != nullptr) {
        for each (PlcBlockGroup^ subGroup in plcSw->BlockGroup->Groups) {
            String^ subPath = Path::Combine(importPath, subGroup->Name);
            if (Directory::Exists(subPath)) {
                ImportBlockSource(plcSw, subPath);
            }
        }
    }
}

inline void ImportTagTables(PlcTagTableGroup^ tagTableGroup, String^ importPath) {
    if (tagTableGroup == nullptr || !Directory::Exists(importPath)) {
        Console::WriteLine("    Tag table group is null or import path does not exist.");
        return;
    }

    array<String^>^ files = Directory::GetFiles(importPath, "*.xml");
    if (files->Length == 0) {
        Console::WriteLine("    No tag table files found in: " + importPath);
        return;
    }

    for each (String^ filePath in files) {
        try {
            String^ fileName = Path::GetFileName(filePath);
            Console::WriteLine("    Importing tag table: " + fileName);
            tagTableGroup->TagTables->Import(gcnew FileInfo(filePath), ImportOptions::Override);
            Console::WriteLine("      OK");
        }
        catch (Exception^ e) {
            Console::WriteLine("      Failed: " + e->Message);
        }
    }

    if (tagTableGroup->Groups != nullptr) {
        for each (PlcTagTableGroup^ subGroup in tagTableGroup->Groups) {
            String^ subPath = Path::Combine(importPath, subGroup->Name);
            if (Directory::Exists(subPath)) {
                ImportTagTables(subGroup, subPath);
            }
        }
    }
}

inline String^ DoImport(TiaPortal^ portal, String^ importPath) {
    Project^ project = nullptr;
    if (portal->Projects->Count > 0) {
        project = portal->Projects[0];
    }

    if (project == nullptr) {
        Console::WriteLine("No open project found. Please open a project in TIA Portal first.");
        return "ERROR: TIA Portal中没有打开的项目，请先打开一个项目。";
    }

    Console::WriteLine("Current project: " + project->Name);

    if (!Directory::Exists(importPath)) {
        Console::WriteLine("Import path does not exist: " + importPath);
        return "ERROR: 导入路径不存在: " + importPath;
    }

    Console::WriteLine("Import path: " + importPath);
    Console::WriteLine();
    Console::WriteLine("Starting import...");

    PlcSoftware^ plcSw = FindPlcSoftware(project);
    if (plcSw == nullptr) {
        Console::WriteLine("No PLC software found in project.");
        return "ERROR: 项目中没有找到PLC设备，请确保项目中包含PLC站。";
    }

    Console::WriteLine("Target PLC software: " + plcSw->Name);

    int blocksImported = 0;
    int tagsImported = 0;
    List<String^>^ errors = gcnew List<String^>();

    array<String^>^ subDirs = Directory::GetDirectories(importPath);
    for each (String^ subDir in subDirs) {
        String^ dirName = Path::GetFileName(subDir);

        if (dirName->StartsWith("Source_")) {
            Console::WriteLine("  Importing from: " + dirName);
            ImportBlockSource(plcSw, subDir);
            if (Directory::GetFiles(subDir, "*.lad.json")->Length > 0) {
                bool ladOk = ImportLadDsl(plcSw, subDir);
                if (ladOk) blocksImported++;
                else errors->Add("LAD DSL导入失败，尝试直接导入XML");
            }
            array<String^>^ xmlFiles = Directory::GetFiles(subDir, "*.xml");
            for each (String^ xmlFile in xmlFiles) {
                String^ fn = Path::GetFileName(xmlFile);
                if (fn->EndsWith("_template.xml") || fn->EndsWith("_rebuilt.xml") || fn->EndsWith("_debug.xml"))
                    continue;
                try {
                    Console::WriteLine("    Importing block XML: " + fn);
                    plcSw->BlockGroup->Blocks->Import(gcnew FileInfo(xmlFile), ImportOptions::Override);
                    Console::WriteLine("      OK");
                    blocksImported++;
                }
                catch (Exception^ e) {
                    Console::WriteLine("      Failed: " + e->Message);
                    errors->Add("程序块导入失败(" + fn + "): " + e->Message);
                }
            }
        }
        else if (dirName->StartsWith("TagTables_")) {
            Console::WriteLine("  Importing tag tables from: " + dirName);
            array<String^>^ tagFiles = Directory::GetFiles(subDir, "*.xml");
            for each (String^ filePath in tagFiles) {
                try {
                    String^ fileName = Path::GetFileName(filePath);
                    Console::WriteLine("    Importing tag table: " + fileName);

                    String^ xmlContent = File::ReadAllText(filePath);

                    PlcTagTable^ targetTable = nullptr;
                    for each (PlcTagTable^ tt in plcSw->TagTableGroup->TagTables) {
                        targetTable = tt;
                        break;
                    }

                    if (targetTable == nullptr) {
                        Console::WriteLine("    No existing tag table found, creating new one.");
                        plcSw->TagTableGroup->TagTables->Import(gcnew FileInfo(filePath), ImportOptions::Override);
                        Console::WriteLine("      OK");
                        tagsImported++;
                        continue;
                    }

                    Console::WriteLine("    Using existing tag table: " + targetTable->Name);

                    List<PlcTag^>^ existingTags = gcnew List<PlcTag^>();
                    for each (PlcTag^ et in targetTable->Tags) {
                        existingTags->Add(et);
                    }
                    for each (PlcTag^ et in existingTags) {
                        try {
                            et->Delete();
                        }
                        catch (Exception^) {}
                    }
                    Console::WriteLine("    Cleared " + existingTags->Count + " existing tags from " + targetTable->Name);

                    XmlDocument^ tagDoc = gcnew XmlDocument();
                    tagDoc->LoadXml(xmlContent);

                    XmlNamespaceManager^ tagNs = gcnew XmlNamespaceManager(tagDoc->NameTable);
                    tagNs->AddNamespace("ns", "http://www.siemens.com/automation/Openness/SW/Tags/v5");

                    XmlNodeList^ tagNodes = tagDoc->SelectNodes("//ns:SW.Tags.PlcTag", tagNs);
                    if (tagNodes->Count == 0) {
                        tagNodes = tagDoc->GetElementsByTagName("SW.Tags.PlcTag");
                    }

                    int tagCount = 0;
                    for each (XmlNode^ tagNode in tagNodes) {
                        XmlElement^ tagElem = dynamic_cast<XmlElement^>(tagNode);
                        if (tagElem == nullptr) continue;

                        String^ tagName = "";
                        String^ tagAddr = "";
                        String^ tagType = "Bool";
                        String^ tagComment = "";

                        XmlNode^ nameNode = tagElem->SelectSingleNode(".//Name");
                        if (nameNode != nullptr) tagName = nameNode->InnerText;

                        XmlNode^ addrNode = tagElem->SelectSingleNode(".//LogicalAddress");
                        if (addrNode != nullptr) tagAddr = addrNode->InnerText;

                        XmlNode^ typeNode = tagElem->SelectSingleNode(".//DataTypeName");
                        if (typeNode == nullptr) typeNode = tagElem->SelectSingleNode(".//DataType");
                        if (typeNode != nullptr) tagType = typeNode->InnerText;

                        XmlNodeList^ commentNodes = tagElem->SelectNodes(".//MultilingualTextItem");
                        for each (XmlNode^ cn in commentNodes) {
                            XmlElement^ ce = dynamic_cast<XmlElement^>(cn);
                            if (ce == nullptr) continue;
                            XmlNode^ textNode = ce->SelectSingleNode(".//Text");
                            if (textNode != nullptr && textNode->InnerText->Length > 0) {
                                tagComment = textNode->InnerText;
                                break;
                            }
                        }

                        if (tagName->Length > 0 && (Char::IsLetter(tagName[0]) || tagName[0] > 0x4E00)) {
                            try {
                                PlcTag^ newTag = targetTable->Tags->Create(tagName, tagType, tagAddr);
                                if (tagComment->Length > 0) {
                                    try {
                                        MultilingualText^ comment = newTag->Comment;
                                        if (comment != nullptr && comment->Items != nullptr && comment->Items->Count > 0) {
                                            for each (MultilingualTextItem^ item in comment->Items) {
                                                item->Text = tagComment;
                                                break;
                                            }
                                        }
                                    }
                                    catch (Exception^ ce) {
                                        Console::WriteLine("      Comment failed: " + ce->Message);
                                    }
                                }
                                tagCount++;
                                Console::WriteLine("      Tag: " + tagName + " " + tagAddr + " " + tagType);
                            }
                            catch (Exception^ te) {
                                Console::WriteLine("      Tag '" + tagName + "' failed: " + te->Message);
                            }
                        }
                        else if (tagName->Length > 0) {
                            Console::WriteLine("      Skipped invalid tag name: '" + tagName + "'");
                        }
                    }

                    Console::WriteLine("      Imported " + tagCount + " tags into " + targetTable->Name);
                    tagsImported++;
                }
                catch (Exception^ e) {
                    Console::WriteLine("      Failed: " + e->Message);
                    errors->Add("变量表导入失败: " + e->Message);
                }
            }
        }
        else if (File::Exists(Path::Combine(subDir, Path::GetFileName(subDir) + ".lad.json")) ||
                 Directory::GetFiles(subDir, "*.lad.json")->Length > 0) {
            Console::WriteLine("  Importing LAD DSL from: " + dirName);
            ImportLadDsl(plcSw, subDir);
            blocksImported++;
        }
    }

    Console::WriteLine();
    Console::WriteLine("Compiling all blocks after import...");
    try {
        if (plcSw->BlockGroup != nullptr) {
            IEngineeringServiceProvider^ provider = dynamic_cast<IEngineeringServiceProvider^>(plcSw->BlockGroup);
            if (provider != nullptr) {
                ICompilable^ compilable = dynamic_cast<ICompilable^>(provider->GetService(ICompilable::typeid));
                if (compilable != nullptr) {
                    compilable->Compile();
                    Console::WriteLine("  Compilation completed.");
                }
            }
        }
    }
    catch (Exception^ e) {
        Console::WriteLine("  Compilation warning: " + e->Message);
    }

    Console::WriteLine();
    Console::WriteLine("=== Import completed! ===");

    String^ result = "项目: " + project->Name + ", PLC: " + plcSw->Name +
        ", 导入程序块: " + blocksImported + ", 导入变量表: " + tagsImported;
    if (errors->Count > 0) {
        result += ", 错误: " + String::Join("; ", errors);
    }
    return result;
}

inline void DoImportInteractive(TiaPortal^ portal) {
    String^ importPath = "D:\\TIA_Export";
    Console::Write("Enter import path (press Enter for D:\\TIA_Export): ");
    String^ input = Console::ReadLine()->Trim();
    if (input->Length > 0) {
        importPath = input;
    }
    DoImport(portal, importPath);
}