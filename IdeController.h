#pragma once

#include "ProjectContext.h"
#include "AiPipeline.h"
#include "AgentSystem.h"
#include "LadConversion.h"
#include "TiaUtils.h"
#include "TiaBridge.h"

using namespace System;
using namespace System::IO;

public delegate void LogHandler(String^ message);

public ref class IdeController
{
public:
    ProjectContext^ Context;
    LogHandler^ OnLog;

    IdeController()
    {
        Context = gcnew ProjectContext();
        Context->ExeDir = Path::GetDirectoryName(
            Assembly::GetExecutingAssembly()->Location);

        String^ configPath = Path::Combine(Context->ExeDir, "phase3_config.json");
        Context->Config = P3Config::Load(configPath);

        String^ tiaVer = DetectTiaVersion();
        Context->HasTiaPortal = (tiaVer != "");
        Context->TiaVersion = tiaVer;

        if (Context->HasTiaPortal)
        {
            try
            {
                SetDllDirectoryForTia(tiaVer);
                AppDomain::CurrentDomain->AssemblyResolve +=
                    gcnew ResolveEventHandler(OnAssemblyResolve);
            }
            catch (Exception^)
            {
                Context->HasTiaPortal = false;
            }
        }
    }

    void GenerateProject(String^ prompt)
    {
        Context->Clear();
        Context->Prompt = prompt;

        Log("[INFO] 开始AI PLC生成...");
        Log("[INFO] 需求: " + prompt);

        if (Context->Config == nullptr || Context->Config->ApiKey == nullptr || Context->Config->ApiKey->Length == 0)
        {
            Log("[ERROR] API密钥未配置。请创建phase3_config.json并设置api_key。");
            Context->Errors = "API密钥未配置。";
            return;
        }

        try
        {
            P3LlmClient::UiLogCallback = gcnew Action<String^>(this, &IdeController::Log);
            P5CompilerLog::UiLogCallback = gcnew Action<String^>(this, &IdeController::Log);
            P4LogicGraphGenerator::LogCallback = gcnew Action<String^>(this, &IdeController::Log);
            P3PipelineResult^ result = P6Pipeline::RunPhase6(
                prompt, Context->Config, Context->TemplateXmlPath);
            P3LlmClient::UiLogCallback = nullptr;
            P5CompilerLog::UiLogCallback = nullptr;
            P4LogicGraphGenerator::LogCallback = nullptr;

            if (result->Success)
            {
                Context->Dsl = result->Dsl;

                if (result->Dsl != nullptr)
                {
                    Context->DslJson = P3DslSerializer::Serialize(result->Dsl);
                }

                Context->Xml = result->Xml;
                Context->TagTableXml = result->TagTableXml;
                Log("[SUCCESS] 生成完成。");
                Log("[INFO] XML长度: " + Context->Xml->Length + " 字符");

                if (Context->TagTableXml != nullptr && Context->TagTableXml->Length > 0)
                    Log("[INFO] 变量表已生成 (" + Context->TagTableXml->Length + " 字符)");

                if (Context->Dsl != nullptr)
                {
                    Log("[INFO] 变量数: " + Context->Dsl->Variables->Count);
                    Log("[INFO] 网络数: " + Context->Dsl->Networks->Count);
                }

                GenerateMultiBackend();
            }
            else
            {
                Context->Errors = result->ErrorMessage;
                Log("[ERROR] " + result->ErrorMessage);
            }
        }
        catch (Exception^ ex)
        {
            P3LlmClient::UiLogCallback = nullptr;
            P5CompilerLog::UiLogCallback = nullptr;
            P4LogicGraphGenerator::LogCallback = nullptr;
            Context->Errors = ex->Message;
            Log("[ERROR] 生成过程异常: " + ex->Message);
            if (ex->InnerException != nullptr)
                Log("[ERROR] 内部异常: " + ex->InnerException->Message);
            Log("[ERROR] 堆栈: " + ex->StackTrace);
        }
    }

    void GeneratePhase3(String^ prompt)
    {
        Context->Clear();
        Context->Prompt = prompt;
        Log("[INFO] Phase 3: 自然语言 -> LLM -> DSL -> XML");

        P3Config^ config = EnsureConfig();
        if (config == nullptr) return;

        P3LlmClient::UiLogCallback = gcnew Action<String^>(this, &IdeController::Log);
        P3PipelineResult^ result = P3Pipeline::Run(prompt, config, Context->TemplateXmlPath);
        P3LlmClient::UiLogCallback = nullptr;
        ProcessResult(result);
    }

    void GeneratePhase4(String^ prompt)
    {
        Context->Clear();
        Context->Prompt = prompt;
        Log("[INFO] Phase 4: 自然语言 -> 需求分析 -> 语义验证 -> 状态机 -> XML");

        P3Config^ config = EnsureConfig();
        if (config == nullptr) return;

        P3LlmClient::UiLogCallback = gcnew Action<String^>(this, &IdeController::Log);
        P3PipelineResult^ result = P3Pipeline::RunPhase4(prompt, config, Context->TemplateXmlPath, gcnew Action<String^>(this, &IdeController::Log));
        P3LlmClient::UiLogCallback = nullptr;
        ProcessResult(result);
    }

    void GeneratePhase5(String^ prompt)
    {
        Context->Clear();
        Context->Prompt = prompt;
        Log("[INFO] Phase 5: 自然语言 -> IR -> 优化 -> 多后端输出");

        P3Config^ config = EnsureConfig();
        if (config == nullptr) return;

        P3LlmClient::UiLogCallback = gcnew Action<String^>(this, &IdeController::Log);
        P3PipelineResult^ result = P5Pipeline::RunPhase5(prompt, config, Context->TemplateXmlPath);
        P3LlmClient::UiLogCallback = nullptr;
        ProcessResult(result);
    }

    void GeneratePhase6(String^ prompt)
    {
        GenerateProject(prompt);
    }

    void ValidateProject()
    {
        if (Context->Dsl == nullptr)
        {
            Log("[ERROR] 没有DSL可验证，请先生成。");
            return;
        }

        Log("[INFO] 正在验证DSL...");

        P3ValidationResult^ v3 = P3SemanticValidator::Validate(Context->Dsl);
        P3ValidationResult^ v5 = P5SemanticValidator::Validate(Context->Dsl);

        String^ errors = "";
        for each (String^ e in v3->Errors) errors += "[P3] " + e + "\n";
        for each (String^ e in v5->Errors) errors += "[P5] " + e + "\n";
        for each (String^ w in v3->Warnings) errors += "[P3-WARN] " + w + "\n";
        for each (String^ w in v5->Warnings) errors += "[P5-WARN] " + w + "\n";

        if (errors->Length == 0)
        {
            Log("[SUCCESS] 验证通过，未发现错误。");
            Context->Errors = "";
        }
        else
        {
            Context->Errors = errors;
            Log("[WARNING] 验证发现以下问题:");
            Log(errors);
        }
    }

    void RepairProject()
    {
        if (Context->Dsl == nullptr)
        {
            Log("[ERROR] 没有DSL可修复，请先生成。");
            return;
        }

        Log("[INFO] 正在自动修复DSL...");

        String^ fixResult = P3SemanticValidator::AutoFix(Context->Dsl);
        if (fixResult->Length > 0) Log("[FIX] " + fixResult);

        P3RepairEngine::RepairUndefinedVariables(Context->Dsl);

        List<String^>^ p6Fixes = P6SelfRepairEngine::SelfRepair(Context->Dsl);
        for each (String^ f in p6Fixes) Log("[AGENT-FIX] " + f);

        String^ p5Fix = P5RepairEngine::AutoRepair(Context->Dsl);
        if (p5Fix != nullptr && p5Fix->Length > 0) Log("[P5-FIX] " + p5Fix);

        Context->DslJson = P3DslSerializer::Serialize(Context->Dsl);

        Log("[INFO] 修复后重新编译...");
        LadDsl^ ladDsl = P3DslConverter::ToLadDsl(Context->Dsl);
        Context->Xml = BuildLadXml(ladDsl, Context->TemplateXmlPath);
        Context->TagTableXml = P3TagTableGenerator::GenerateTagTableXml(Context->Dsl);

        GenerateMultiBackend();

        Log("[SUCCESS] 修复完成。");
    }

    void ImportToTia()
    {
        if (!Context->HasTiaPortal)
        {
            Log("[ERROR] 未检测到TIA Portal。");
            return;
        }

        if (Context->Xml == nullptr || Context->Xml->Length == 0)
        {
            Log("[ERROR] 没有XML可导入，请先生成。");
            return;
        }

        Log("[INFO] 正在连接TIA Portal " + Context->TiaVersion + "...");

        TiaPortalHandle portal = TiaBridge_Connect(Context->TiaVersion);
        if (portal == nullptr)
        {
            Log("[ERROR] 连接TIA Portal失败。");
            return;
        }

        try
        {
            String^ tempDir = Path::Combine(Context->ExeDir, "tia_import_temp");
            if (Directory::Exists(tempDir)) Directory::Delete(tempDir, true);
            Directory::CreateDirectory(tempDir);

            String^ sourceDir = Path::Combine(tempDir, "Source_PLC");
            Directory::CreateDirectory(sourceDir);

            String^ xmlPath = Path::Combine(sourceDir, "Main.xml");
            File::WriteAllText(xmlPath, Context->Xml, gcnew System::Text::UTF8Encoding(false));

            String^ debugDir = Path::Combine(Context->ExeDir, "debug_xml");
            if (!Directory::Exists(debugDir)) Directory::CreateDirectory(debugDir);
            String^ debugXmlPath = Path::Combine(debugDir, "Main_debug.xml");
            File::WriteAllText(debugXmlPath, Context->Xml, gcnew System::Text::UTF8Encoding(false));
            Log("[INFO] XML诊断日志: " + debugXmlPath);

            if (Context->TagTableXml != nullptr && Context->TagTableXml->Length > 0)
            {
                String^ tagDir = Path::Combine(tempDir, "TagTables_PLC");
                Directory::CreateDirectory(tagDir);
                String^ tagPath = Path::Combine(tagDir, "DefaultTagTable.xml");
                File::WriteAllText(tagPath, Context->TagTableXml, gcnew System::Text::UTF8Encoding(false));
                Log("[INFO] 变量表已写入: " + tagPath);
            }

            Log("[INFO] 程序块已写入: " + xmlPath);
            Log("[INFO] 开始导入到TIA Portal...");

            String^ importResult = TiaBridge_DoImportWithPath(portal, tempDir);

            if (importResult != nullptr && importResult->StartsWith("ERROR:")) {
                Log("[ERROR] " + importResult);
            }
            else if (importResult != nullptr && importResult->Length > 0) {
                Log("[SUCCESS] 导入TIA Portal完成 - " + importResult);
            }
            else {
                Log("[SUCCESS] 导入TIA Portal完成。");
            }
        }
        catch (Exception^ e)
        {
            Log("[ERROR] 导入失败: " + e->Message);
        }
        finally
        {
            TiaBridge_Disconnect(portal);
        }
    }

    void ExportProject()
    {
        if (Context->Xml == nullptr || Context->Xml->Length == 0)
        {
            Log("[ERROR] 没有XML可导出，请先生成。");
            return;
        }

        SaveFileDialog^ dlg = gcnew SaveFileDialog();
        dlg->Filter = "XML文件|*.xml|所有文件|*.*";
        dlg->DefaultExt = "xml";
        dlg->FileName = "ai_output";

        if (dlg->ShowDialog() == DialogResult::OK)
        {
            File::WriteAllText(dlg->FileName, Context->Xml,
                gcnew System::Text::UTF8Encoding(false));
            Log("[SUCCESS] XML已导出到: " + dlg->FileName);

            if (Context->Dsl != nullptr)
            {
                String^ dslPath = Path::ChangeExtension(dlg->FileName, ".dsl.json");
                P3Pipeline::SaveDslToFile(Context->Dsl, dslPath);
                Log("[SUCCESS] DSL已导出到: " + dslPath);
            }

            if (Context->TagTableXml != nullptr && Context->TagTableXml->Length > 0)
            {
                String^ tagPath = Path::ChangeExtension(dlg->FileName, ".tagtable.xml");
                File::WriteAllText(tagPath, Context->TagTableXml,
                    gcnew System::Text::UTF8Encoding(false));
                Log("[SUCCESS] 变量表已导出到: " + tagPath);
            }
        }
    }

    void LoadDslFile(String^ dslJsonPath)
    {
        Context->Clear();
        Log("[INFO] 正在加载DSL: " + dslJsonPath);

        P3PipelineResult^ result = P3Pipeline::RunFromDslFile(dslJsonPath, Context->TemplateXmlPath);
        ProcessResult(result);
    }

    void RunSimulation(int maxCycles)
    {
        if (Context->Dsl == nullptr)
        {
            Log("[ERROR] 没有DSL可仿真，请先生成。");
            return;
        }

        Log("[INFO] 正在运行仿真 (" + maxCycles + " 个周期)...");

        Dictionary<String^, bool>^ inputs = gcnew Dictionary<String^, bool>();
        for each (P3Variable^ v in Context->Dsl->Variables)
        {
            if (v->Type == "Bool" && (v->Scope == "Input" || v->Name->StartsWith("I")))
            {
                inputs[v->Name] = true;
            }
        }

        List<String^>^ simLog = P6SimulationEngine::RunSimulation(Context->Dsl, maxCycles, inputs);
        StringBuilder^ sb = gcnew StringBuilder();
        for each (String^ line in simLog) sb->AppendLine(line);
        Context->SimulationLog = sb->ToString();

        Log("[SUCCESS] 仿真完成。");
    }

private:

    void Log(String^ message)
    {
        if (OnLog != nullptr) OnLog(message);
    }

    P3Config^ EnsureConfig()
    {
        if (Context->Config == nullptr || Context->Config->ApiKey == nullptr || Context->Config->ApiKey->Length == 0)
        {
            Log("[ERROR] API密钥未配置。请创建phase3_config.json并设置api_key。");
            Context->Errors = "API密钥未配置。";
            return nullptr;
        }
        return Context->Config;
    }

    void ProcessResult(P3PipelineResult^ result)
    {
        if (result->Success)
        {
            Context->Dsl = result->Dsl;
            if (result->Dsl != nullptr)
                Context->DslJson = P3DslSerializer::Serialize(result->Dsl);
            Context->Xml = result->Xml;
            Context->TagTableXml = result->TagTableXml;

            if (Context->Dsl != nullptr)
                Log("[INFO] DSL网络数: " + Context->Dsl->Networks->Count + ", 变量数: " + Context->Dsl->Variables->Count);

            if (Context->Xml != nullptr && Context->Xml->Length > 0)
            {
                int cuCount = 0;
                int idx = 0;
                while ((idx = Context->Xml->IndexOf("SW.Blocks.CompileUnit", idx)) != -1) {
                    cuCount++;
                    idx++;
                }
                Log("[INFO] XML中CompileUnit数: " + cuCount + ", XML长度: " + Context->Xml->Length + " 字符");
                if (cuCount == 0)
                    Log("[WARNING] XML中没有CompileUnit(网络)，导入后博图将看不到任何网络！");
            }

            Log("[SUCCESS] 生成完成。");
            if (Context->TagTableXml != nullptr && Context->TagTableXml->Length > 0)
                Log("[INFO] 变量表已生成 (" + Context->TagTableXml->Length + " 字符)");
            GenerateMultiBackend();
        }
        else
        {
            Context->Errors = result->ErrorMessage;
            Log("[ERROR] " + result->ErrorMessage);
        }
    }

    void GenerateMultiBackend()
    {
        if (Context->Dsl == nullptr) return;

        try
        {
            P5IRProgram^ ir = P5DslToIrConverter::Convert(Context->Dsl);
            ir = P5Optimizer::Optimize(ir);

            Context->SclCode = (gcnew P5SclBackend())->Generate(ir);
            Context->StlCode = (gcnew P5StlBackend())->Generate(ir);
            Context->FbdCode = (gcnew P5FbdBackend())->Generate(ir);

            P5VisualizationBackend^ vis = gcnew P5VisualizationBackend();
            Context->Visualization = vis->Generate(ir);

            if (Context->Dsl != nullptr)
            {
                List<P6HmiElement^>^ hmi = P6HmiGenerator::Generate(Context->Dsl);
                Context->HmiXml = P6HmiGenerator::GenerateXml(hmi);

                List<P6DocumentSection^>^ docs = P6DocumentGenerator::Generate(Context->Dsl);
                Context->Document = P6DocumentGenerator::GenerateFullDocument(docs);
            }
        }
        catch (Exception^ e)
        {
            Log("[WARN] 多后端生成部分失败: " + e->Message);
        }
    }
};
