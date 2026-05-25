#using <mscorlib.dll>
#using <System.dll>
#using <System.Core.dll>
#using <System.Xml.dll>
#using <System.Windows.Forms.dll>
#using <System.Drawing.dll>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Xml;
using namespace System::IO;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;
using namespace System::Windows::Forms;

#include "DataTypes.h"
#include "TiaUtils.h"
#include "JsonSerializer.h"
#include "LadXmlParser.h"
#include "LadConversion.h"
#include "TiaBridge.h"
#include "Dsl.h"
#include "AiPipeline.h"
#include "AgentSystem.h"
#include "ProjectContext.h"
#include "IdeController.h"
#include "MainForm.h"

void RunConsoleMode(String^ exeDir);
void ShowMenu(bool tiaDetected);

[STAThreadAttribute]
int main(array<String^>^ args) {
    String^ exeDir = Path::GetDirectoryName(
        System::Reflection::Assembly::GetExecutingAssembly()->Location);

    if (args->Length >= 2 && args[0] == "-import") {
        String^ importPath = (args->Length >= 3) ? args[2] : "D:\\TIA_Export";
        String^ logPath = Path::Combine(exeDir, "import_log.txt");
        StreamWriter^ logWriter = gcnew StreamWriter(logPath, false, System::Text::Encoding::UTF8);
        Console::SetOut(logWriter);

        String^ tiaVer = DetectTiaVersion();
        if (tiaVer == "") {
            Console::WriteLine("TIA Portal not detected.");
            logWriter->Close();
            return 1;
        }
        SetDllDirectoryForTia(tiaVer);
        AppDomain::CurrentDomain->AssemblyResolve += gcnew ResolveEventHandler(OnAssemblyResolve);
        TiaPortalHandle portal = TiaBridge_Connect(tiaVer);
        if (portal == nullptr) {
            Console::WriteLine("Failed to connect to TIA Portal.");
            logWriter->Close();
            return 1;
        }
        TiaBridge_DoImportWithPath(portal, importPath);
        TiaBridge_Disconnect(portal);
        logWriter->Close();
        return 0;
    }

    if (args->Length >= 4 && args[0] == "-g") {
        String^ jsonPath = args[1];
        String^ templatePath = args[2];
        String^ outputPath = args[3];
        String^ xml = GenerateLadXmlFromJsonFile(jsonPath, templatePath);
        if (xml != nullptr) {
            File::WriteAllText(outputPath, xml, gcnew System::Text::UTF8Encoding(false));
            Console::WriteLine("AI-generated LAD XML saved to: " + outputPath);
        }
        else {
            Console::WriteLine("Failed to generate LAD XML.");
        }
        return 0;
    }

    if (args->Length >= 3 && args[0] == "-ai") {
        String^ problem = args[1];
        String^ outputPath = args[2];
        String^ templatePath = (args->Length >= 4) ? args[3] : "";
        P3Config^ config = P3Config::Load(Path::Combine(exeDir, "phase3_config.json"));
        if (config->ApiKey == nullptr || config->ApiKey->Length == 0) {
            Console::WriteLine("ERROR: API key not configured. Create phase3_config.json with api_key.");
            return 1;
        }
        P3PipelineResult^ result = P3Pipeline::Run(problem, config, templatePath);
        if (result->Success) {
            File::WriteAllText(outputPath, result->Xml, gcnew System::Text::UTF8Encoding(false));
            Console::WriteLine("AI-generated LAD XML saved to: " + outputPath);
            if (result->Dsl != nullptr) {
                String^ dslOutputPath = Path::ChangeExtension(outputPath, ".dsl.json");
                P3Pipeline::SaveDslToFile(result->Dsl, dslOutputPath);
            }
        }
        else {
            Console::WriteLine("ERROR: " + result->ErrorMessage);
        }
        return 0;
    }

    if (args->Length >= 3 && args[0] == "-p3") {
        String^ dslPath = args[1];
        String^ outputPath = args[2];
        String^ templatePath = (args->Length >= 4) ? args[3] : "";
        P3PipelineResult^ result = P3Pipeline::RunFromDslFile(dslPath, templatePath);
        if (result->Success) {
            File::WriteAllText(outputPath, result->Xml, gcnew System::Text::UTF8Encoding(false));
            Console::WriteLine("AI-generated LAD XML saved to: " + outputPath);
        }
        else {
            Console::WriteLine("ERROR: " + result->ErrorMessage);
        }
        return 0;
    }

    if (args->Length >= 3 && args[0] == "-p4") {
        String^ problem = args[1];
        String^ outputPath = args[2];
        String^ templatePath = (args->Length >= 4) ? args[3] : "";
        P3Config^ config = P3Config::Load(Path::Combine(exeDir, "phase3_config.json"));
        if (config->ApiKey == nullptr || config->ApiKey->Length == 0) {
            Console::WriteLine("ERROR: API key not configured. Create phase3_config.json with api_key.");
            return 1;
        }
        P3PipelineResult^ result = P3Pipeline::RunPhase4(problem, config, templatePath);
        if (result->Success) {
            File::WriteAllText(outputPath, result->Xml, gcnew System::Text::UTF8Encoding(false));
            Console::WriteLine("AI-generated LAD XML saved to: " + outputPath);
            if (result->Dsl != nullptr) {
                String^ dslOutputPath = Path::ChangeExtension(outputPath, ".dsl.json");
                P3Pipeline::SaveDslToFile(result->Dsl, dslOutputPath);
            }
        }
        else {
            Console::WriteLine("ERROR: " + result->ErrorMessage);
        }
        return 0;
    }

    if (args->Length >= 3 && args[0] == "-p5") {
        String^ problem = args[1];
        String^ outputPath = args[2];
        String^ templatePath = (args->Length >= 4) ? args[3] : "";
        P3Config^ config = P3Config::Load(Path::Combine(exeDir, "phase3_config.json"));
        if (config->ApiKey == nullptr || config->ApiKey->Length == 0) {
            Console::WriteLine("ERROR: API key not configured. Create phase3_config.json with api_key.");
            return 1;
        }
        P3PipelineResult^ result = P5Pipeline::RunPhase5(problem, config, templatePath);
        if (result->Success) {
            File::WriteAllText(outputPath, result->Xml, gcnew System::Text::UTF8Encoding(false));
            Console::WriteLine("AI-generated LAD XML saved to: " + outputPath);
            if (result->Dsl != nullptr) {
                String^ dslOutputPath = Path::ChangeExtension(outputPath, ".dsl.json");
                P3Pipeline::SaveDslToFile(result->Dsl, dslOutputPath);
            }
        }
        else {
            Console::WriteLine("ERROR: " + result->ErrorMessage);
        }
        return 0;
    }

    if (args->Length >= 3 && args[0] == "-p6") {
        String^ problem = args[1];
        String^ outputPath = args[2];
        String^ templatePath = (args->Length >= 4) ? args[3] : "";
        P3Config^ config = P3Config::Load(Path::Combine(exeDir, "phase3_config.json"));
        if (config->ApiKey == nullptr || config->ApiKey->Length == 0) {
            Console::WriteLine("ERROR: API key not configured. Create phase3_config.json with api_key.");
            return 1;
        }
        P3PipelineResult^ result = P6Pipeline::RunPhase6(problem, config, templatePath);
        if (result->Success) {
            File::WriteAllText(outputPath, result->Xml, gcnew System::Text::UTF8Encoding(false));
            Console::WriteLine("AI-generated LAD XML saved to: " + outputPath);
            if (result->Dsl != nullptr) {
                String^ dslOutputPath = Path::ChangeExtension(outputPath, ".dsl.json");
                P3Pipeline::SaveDslToFile(result->Dsl, dslOutputPath);
            }
        }
        else {
            Console::WriteLine("ERROR: " + result->ErrorMessage);
        }
        return 0;
    }

    if (args->Length >= 1 && args[0] == "-cli") {
        RunConsoleMode(exeDir);
        return 0;
    }

    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);
    try
    {
        Application::Run(gcnew AiPlcUi::MainForm());
    }
    catch (Exception^ ex)
    {
        String^ logPath = Path::Combine(exeDir, "ui_error.log");
        File::WriteAllText(logPath, ex->ToString(), gcnew System::Text::UTF8Encoding(false));
        MessageBox::Show("界面错误: " + ex->Message, "AI PLC 工程生成工作台",
            MessageBoxButtons::OK, MessageBoxIcon::Error);
    }
    return 0;
}

void RunConsoleMode(String^ exeDir) {
    String^ tiaVer = DetectTiaVersion();
    bool tiaDetected = (tiaVer != "");

    if (!tiaDetected) {
        Console::WriteLine("TIA Portal not detected on this computer.");
        Console::WriteLine("TIA Portal features (Import/Export/RoundTrip) will be unavailable.");
        Console::WriteLine("Standalone features (AI Code Generation) are fully available.");
        Console::WriteLine();
    }

    TiaPortalHandle portal = nullptr;

    if (tiaDetected) {
        try {
            SetDllDirectoryForTia(tiaVer);
            AppDomain::CurrentDomain->AssemblyResolve += gcnew ResolveEventHandler(OnAssemblyResolve);
            Console::WriteLine("Connecting to TIA Portal " + tiaVer + "...");
            portal = TiaBridge_Connect(tiaVer);
        }
        catch (FileNotFoundException^ e) {
            Console::WriteLine("Siemens.Engineering assembly not found: " + e->Message);
            Console::WriteLine("This is normal if TIA Portal is not installed on this machine.");
            Console::WriteLine("TIA Portal features will be disabled, standalone features remain available.");
            Console::WriteLine();
            tiaDetected = false;
        }
        catch (Exception^ e) {
            Console::WriteLine("Failed to initialize TIA Portal: " + e->Message);
            Console::WriteLine("TIA Portal features will be disabled.");
            Console::WriteLine();
            tiaDetected = false;
        }
    }

    try {
        while (true) {
            ShowMenu(tiaDetected);
            String^ choice = Console::ReadLine()->Trim();

            if (choice == "1") {
                if (!tiaDetected || portal == nullptr) {
                    Console::WriteLine("TIA Portal is not available. This feature requires TIA Portal.");
                    Console::WriteLine();
                    continue;
                }
                Console::WriteLine();
                Console::WriteLine("--- Import Mode ---");
                TiaBridge_DoImport(portal);
            }
            else if (choice == "2") {
                if (!tiaDetected || portal == nullptr) {
                    Console::WriteLine("TIA Portal is not available. This feature requires TIA Portal.");
                    Console::WriteLine();
                    continue;
                }
                Console::WriteLine();
                Console::WriteLine("--- Export Mode ---");
                TiaBridge_DoExport(portal);
            }
            else if (choice == "3") {
                if (!tiaDetected || portal == nullptr) {
                    Console::WriteLine("TIA Portal is not available. This feature requires TIA Portal.");
                    Console::WriteLine();
                    continue;
                }
                Console::WriteLine();
                Console::WriteLine("--- Round-trip Test ---");
                TiaBridge_DoRoundTripTest(portal);
            }
            else if (choice == "4") {
                Console::WriteLine();
                Console::WriteLine("--- AI Code Generation ---");
                Console::Write("  Enter JSON DSL file path: ");
                String^ jsonPath = Console::ReadLine()->Trim();
                Console::Write("  Enter template XML path: ");
                String^ templatePath = Console::ReadLine()->Trim();
                Console::Write("  Enter output XML path: ");
                String^ outputPath = Console::ReadLine()->Trim();

                String^ xml = GenerateLadXmlFromJsonFile(jsonPath, templatePath);
                if (xml != nullptr) {
                    File::WriteAllText(outputPath, xml, gcnew System::Text::UTF8Encoding(false));
                    Console::WriteLine("  AI-generated LAD XML saved to: " + outputPath);
                }
            }
            else if (choice == "5") {
                Console::WriteLine();
                Console::WriteLine("--- AI Auto Generate (Natural Language -> PLC) ---");
                Console::WriteLine("  Describe your PLC control problem in natural language:");
                Console::Write("  > ");
                String^ problem = Console::ReadLine()->Trim();
                if (problem->Length == 0) {
                    Console::WriteLine("  No problem description entered.");
                    continue;
                }
                Console::Write("  Enter template XML path (optional, press Enter to skip): ");
                String^ templatePath = Console::ReadLine()->Trim();
                Console::Write("  Enter output XML path: ");
                String^ outputPath = Console::ReadLine()->Trim();
                if (outputPath->Length == 0) outputPath = Path::Combine(exeDir, "ai_output.xml");

                P3Config^ config = P3Pipeline::EnsureConfig(exeDir);
                Console::WriteLine();
                P3PipelineResult^ result = P3Pipeline::Run(problem, config, templatePath);
                if (result->Success) {
                    File::WriteAllText(outputPath, result->Xml, gcnew System::Text::UTF8Encoding(false));
                    Console::WriteLine();
                    Console::WriteLine("AI-generated LAD XML saved to: " + outputPath);

                    if (result->Dsl != nullptr) {
                        String^ dslOutputPath = Path::ChangeExtension(outputPath, ".dsl.json");
                        P3Pipeline::SaveDslToFile(result->Dsl, dslOutputPath);
                    }
                }
                else {
                    Console::WriteLine();
                    Console::WriteLine("ERROR: " + result->ErrorMessage);
                }
            }
            else if (choice == "6") {
                Console::WriteLine();
                Console::WriteLine("--- AI DSL File Generate (P3 DSL JSON -> LAD XML) ---");
                Console::Write("  Enter P3 DSL JSON file path: ");
                String^ dslPath = Console::ReadLine()->Trim();
                Console::Write("  Enter template XML path (optional, press Enter to skip): ");
                String^ templatePath = Console::ReadLine()->Trim();
                Console::Write("  Enter output XML path: ");
                String^ outputPath = Console::ReadLine()->Trim();
                if (outputPath->Length == 0) outputPath = Path::Combine(exeDir, "ai_output.xml");

                P3PipelineResult^ result = P3Pipeline::RunFromDslFile(dslPath, templatePath);
                if (result->Success) {
                    File::WriteAllText(outputPath, result->Xml, gcnew System::Text::UTF8Encoding(false));
                    Console::WriteLine();
                    Console::WriteLine("AI-generated LAD XML saved to: " + outputPath);
                }
                else {
                    Console::WriteLine();
                    Console::WriteLine("ERROR: " + result->ErrorMessage);
                }
            }
            else if (choice == "7") {
                Console::WriteLine();
                Console::WriteLine("--- AI Phase4 Generate (Multi-Step Reasoning) ---");
                Console::WriteLine("  Describe your PLC control problem in natural language:");
                Console::Write("  > ");
                String^ problem = Console::ReadLine()->Trim();
                if (problem->Length == 0) {
                    Console::WriteLine("  No problem description entered.");
                    continue;
                }
                Console::Write("  Enter template XML path (optional, press Enter to skip): ");
                String^ templatePath = Console::ReadLine()->Trim();
                Console::Write("  Enter output XML path: ");
                String^ outputPath = Console::ReadLine()->Trim();
                if (outputPath->Length == 0) outputPath = Path::Combine(exeDir, "ai_output_p4.xml");

                P3Config^ config = P3Pipeline::EnsureConfig(exeDir);
                Console::WriteLine();
                P3PipelineResult^ result = P3Pipeline::RunPhase4(problem, config, templatePath);
                if (result->Success) {
                    File::WriteAllText(outputPath, result->Xml, gcnew System::Text::UTF8Encoding(false));
                    Console::WriteLine();
                    Console::WriteLine("AI-generated LAD XML saved to: " + outputPath);

                    if (result->Dsl != nullptr) {
                        String^ dslOutputPath = Path::ChangeExtension(outputPath, ".dsl.json");
                        P3Pipeline::SaveDslToFile(result->Dsl, dslOutputPath);
                    }
                }
                else {
                    Console::WriteLine();
                    Console::WriteLine("ERROR: " + result->ErrorMessage);
                }
            }
            else if (choice == "8") {
                Console::WriteLine();
                Console::WriteLine("--- AI Phase5 Generate (Compiler Platform) ---");
                Console::WriteLine("  Describe your PLC control problem in natural language:");
                Console::Write("  > ");
                String^ problem = Console::ReadLine()->Trim();
                if (problem->Length == 0) {
                    Console::WriteLine("  No problem description entered.");
                    continue;
                }
                Console::Write("  Enter template XML path (optional, press Enter to skip): ");
                String^ templatePath = Console::ReadLine()->Trim();
                Console::Write("  Enter output XML path: ");
                String^ outputPath = Console::ReadLine()->Trim();
                if (outputPath->Length == 0) outputPath = Path::Combine(exeDir, "ai_output_p5.xml");

                P3Config^ config = P3Pipeline::EnsureConfig(exeDir);
                Console::WriteLine();
                P3PipelineResult^ result = P5Pipeline::RunPhase5(problem, config, templatePath);
                if (result->Success) {
                    File::WriteAllText(outputPath, result->Xml, gcnew System::Text::UTF8Encoding(false));
                    Console::WriteLine();
                    Console::WriteLine("AI-generated LAD XML saved to: " + outputPath);

                    if (result->Dsl != nullptr) {
                        String^ dslOutputPath = Path::ChangeExtension(outputPath, ".dsl.json");
                        P3Pipeline::SaveDslToFile(result->Dsl, dslOutputPath);
                    }
                }
                else {
                    Console::WriteLine();
                    Console::WriteLine("ERROR: " + result->ErrorMessage);
                }
            }
            else if (choice == "9") {
                Console::WriteLine();
                Console::WriteLine("--- AI Phase6 Generate (Agent System) ---");
                Console::WriteLine("  Describe your PLC control problem in natural language:");
                Console::Write("  > ");
                String^ problem = Console::ReadLine()->Trim();
                if (problem->Length == 0) {
                    Console::WriteLine("  No problem description entered.");
                    continue;
                }
                Console::Write("  Enter template XML path (optional, press Enter to skip): ");
                String^ templatePath = Console::ReadLine()->Trim();
                Console::Write("  Enter output XML path: ");
                String^ outputPath = Console::ReadLine()->Trim();
                if (outputPath->Length == 0) outputPath = Path::Combine(exeDir, "ai_output_p6.xml");

                P3Config^ config = P3Pipeline::EnsureConfig(exeDir);
                Console::WriteLine();
                P3PipelineResult^ result = P6Pipeline::RunPhase6(problem, config, templatePath);
                if (result->Success) {
                    File::WriteAllText(outputPath, result->Xml, gcnew System::Text::UTF8Encoding(false));
                    Console::WriteLine();
                    Console::WriteLine("AI-generated LAD XML saved to: " + outputPath);

                    if (result->Dsl != nullptr) {
                        String^ dslOutputPath = Path::ChangeExtension(outputPath, ".dsl.json");
                        P3Pipeline::SaveDslToFile(result->Dsl, dslOutputPath);
                    }
                }
                else {
                    Console::WriteLine();
                    Console::WriteLine("ERROR: " + result->ErrorMessage);
                }
            }
            else if (choice == "10") {
                Console::WriteLine("Exiting...");
                break;
            }
            else {
                Console::WriteLine("Invalid choice. Please enter 1-10.");
            }
        }
    }
    catch (Exception^ e) {
        Console::WriteLine("Operation failed: " + e->Message);
    }
    finally {
        if (portal != nullptr) {
            TiaBridge_Disconnect(portal);
        }
    }
}
