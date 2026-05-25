#pragma once

#include "IdeController.h"
#include "ApiConfigDialog.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::Threading;
using namespace System::ComponentModel;

namespace AiPlcUi
{
    public ref class MainForm : public Form
    {
    public:
        MainForm()
        {
            try
            {
                controller = gcnew IdeController();
                controller->OnLog = gcnew LogHandler(this, &MainForm::AppendLog);
            }
            catch (Exception^ ex)
            {
                MessageBox::Show("控制器初始化错误: " + ex->Message, "AI PLC 工程生成工作台", MessageBoxButtons::OK, MessageBoxIcon::Error);
                controller = gcnew IdeController();
            }
            InitializeComponent();
        }

    private:
        IdeController^ controller;
        BackgroundWorker^ bgwGenerate;
        BackgroundWorker^ bgwRepair;
        BackgroundWorker^ bgwSimulate;
        BackgroundWorker^ bgwImport;
        String^ pendingDslPath;

        MenuStrip^ menuStrip1;
        ToolStrip^ toolStrip1;
        TreeView^ treeProject;
        Panel^ pnlLeft;
        Panel^ pnlRight;
        Panel^ pnlInput;
        TextBox^ txtPrompt;
        Button^ btnGenerate;
        ComboBox^ cmbPhase;
        RichTextBox^ txtLog;
        TabControl^ tabMain;
        TabPage^ tabDsl;
        TabPage^ tabXml;
        TabPage^ tabScl;
        TabPage^ tabStl;
        TabPage^ tabErrors;
        TabPage^ tabSimulation;
        TabPage^ tabDocument;
        TabPage^ tabHmi;
        RichTextBox^ txtDsl;
        RichTextBox^ txtXml;
        RichTextBox^ txtScl;
        RichTextBox^ txtStl;
        RichTextBox^ txtErrors;
        RichTextBox^ txtSimulation;
        RichTextBox^ txtDocument;
        RichTextBox^ txtHmi;
        StatusStrip^ statusStrip1;
        ToolStripStatusLabel^ lblStatus;
        ToolStripButton^ tsbGenerate;
        ToolStripButton^ tsbValidate;
        ToolStripButton^ tsbRepair;
        ToolStripButton^ tsbImport;
        ToolStripButton^ tsbExport;
        ToolStripButton^ tsbSimulate;
        ToolStripSeparator^ tsSep1;
        ToolStripSeparator^ tsSep2;

        void InitializeComponent()
        {
            this->Text = "AI PLC 工程生成工作台";
            this->Width = 1400;
            this->Height = 830;
            this->MinimumSize = Drawing::Size(1000, 700);
            this->StartPosition = FormStartPosition::CenterScreen;
            this->Font = gcnew System::Drawing::Font("Microsoft YaHei UI", 9);
            this->TopMost = true;
            this->Shown += gcnew EventHandler(this, &MainForm::OnFormShown);

            bgwGenerate = gcnew BackgroundWorker();
            bgwGenerate->DoWork += gcnew DoWorkEventHandler(this, &MainForm::BgGenerate_DoWork);
            bgwGenerate->RunWorkerCompleted += gcnew RunWorkerCompletedEventHandler(this, &MainForm::BgGenerate_Completed);

            bgwRepair = gcnew BackgroundWorker();
            bgwRepair->DoWork += gcnew DoWorkEventHandler(this, &MainForm::BgRepair_DoWork);
            bgwRepair->RunWorkerCompleted += gcnew RunWorkerCompletedEventHandler(this, &MainForm::BgRepair_Completed);

            bgwSimulate = gcnew BackgroundWorker();
            bgwSimulate->DoWork += gcnew DoWorkEventHandler(this, &MainForm::BgSimulate_DoWork);
            bgwSimulate->RunWorkerCompleted += gcnew RunWorkerCompletedEventHandler(this, &MainForm::BgSimulate_Completed);

            bgwImport = gcnew BackgroundWorker();
            bgwImport->DoWork += gcnew DoWorkEventHandler(this, &MainForm::BgImport_DoWork);
            bgwImport->RunWorkerCompleted += gcnew RunWorkerCompletedEventHandler(this, &MainForm::BgImport_Completed);

            menuStrip1 = gcnew MenuStrip();
            ToolStripMenuItem^ menuFile = gcnew ToolStripMenuItem("文件");
            ToolStripMenuItem^ menuNew = gcnew ToolStripMenuItem("新建工程");
            ToolStripMenuItem^ menuOpen = gcnew ToolStripMenuItem("打开DSL...");
            ToolStripMenuItem^ menuSave = gcnew ToolStripMenuItem("保存XML...");
            ToolStripMenuItem^ menuExit = gcnew ToolStripMenuItem("退出");
            menuNew->Click += gcnew EventHandler(this, &MainForm::OnMenuNew);
            menuOpen->Click += gcnew EventHandler(this, &MainForm::OnMenuOpen);
            menuSave->Click += gcnew EventHandler(this, &MainForm::OnMenuSave);
            menuExit->Click += gcnew EventHandler(this, &MainForm::OnMenuExit);
            menuFile->DropDownItems->AddRange(gcnew array<ToolStripItem^>{menuNew, menuOpen, menuSave, menuExit});

            ToolStripMenuItem^ menuTool = gcnew ToolStripMenuItem("工具");
            ToolStripMenuItem^ menuValidate = gcnew ToolStripMenuItem("验证");
            ToolStripMenuItem^ menuRepair = gcnew ToolStripMenuItem("自动修复");
            ToolStripMenuItem^ menuSimulate = gcnew ToolStripMenuItem("运行仿真");
            menuValidate->Click += gcnew EventHandler(this, &MainForm::OnValidateClicked);
            menuRepair->Click += gcnew EventHandler(this, &MainForm::OnRepairClicked);
            menuSimulate->Click += gcnew EventHandler(this, &MainForm::OnSimulateClicked);
            menuTool->DropDownItems->AddRange(gcnew array<ToolStripItem^>{menuValidate, menuRepair, menuSimulate});

            ToolStripMenuItem^ menuTia = gcnew ToolStripMenuItem("TIA Portal");
            ToolStripMenuItem^ menuImport = gcnew ToolStripMenuItem("导入到TIA Portal");
            menuImport->Click += gcnew EventHandler(this, &MainForm::OnImportClicked);
            menuTia->DropDownItems->Add(menuImport);

            ToolStripMenuItem^ menuApi = gcnew ToolStripMenuItem("API配置");
            ToolStripMenuItem^ menuApiConfig = gcnew ToolStripMenuItem("配置API密钥...");
            menuApiConfig->Click += gcnew EventHandler(this, &MainForm::OnApiConfigClicked);
            menuApi->DropDownItems->Add(menuApiConfig);

            menuStrip1->Items->AddRange(gcnew array<ToolStripItem^>{menuFile, menuTool, menuTia, menuApi});

            toolStrip1 = gcnew ToolStrip();
            tsbGenerate = gcnew ToolStripButton("生成");
            tsbGenerate->DisplayStyle = ToolStripItemDisplayStyle::Text;
            tsbGenerate->Click += gcnew EventHandler(this, &MainForm::OnGenerateClicked);

            tsbValidate = gcnew ToolStripButton("验证");
            tsbValidate->DisplayStyle = ToolStripItemDisplayStyle::Text;
            tsbValidate->Click += gcnew EventHandler(this, &MainForm::OnValidateClicked);

            tsbRepair = gcnew ToolStripButton("修复");
            tsbRepair->DisplayStyle = ToolStripItemDisplayStyle::Text;
            tsbRepair->Click += gcnew EventHandler(this, &MainForm::OnRepairClicked);

            tsSep1 = gcnew ToolStripSeparator();

            tsbSimulate = gcnew ToolStripButton("仿真");
            tsbSimulate->DisplayStyle = ToolStripItemDisplayStyle::Text;
            tsbSimulate->Click += gcnew EventHandler(this, &MainForm::OnSimulateClicked);

            tsSep2 = gcnew ToolStripSeparator();

            tsbImport = gcnew ToolStripButton("导入TIA");
            tsbImport->DisplayStyle = ToolStripItemDisplayStyle::Text;
            tsbImport->Click += gcnew EventHandler(this, &MainForm::OnImportClicked);

            tsbExport = gcnew ToolStripButton("导出");
            tsbExport->DisplayStyle = ToolStripItemDisplayStyle::Text;
            tsbExport->Click += gcnew EventHandler(this, &MainForm::OnExportClicked);

            toolStrip1->Items->AddRange(gcnew array<ToolStripItem^>{
                tsbGenerate, tsbValidate, tsbRepair, tsSep1,
                tsbSimulate, tsSep2,
                tsbImport, tsbExport});
            toolStrip1->Dock = DockStyle::Top;
            menuStrip1->Dock = DockStyle::Top;

            pnlLeft = gcnew Panel();
            pnlLeft->Dock = DockStyle::Left;
            pnlLeft->Width = 400;

            treeProject = gcnew TreeView();
            treeProject->Dock = DockStyle::Fill;
            treeProject->Font = gcnew System::Drawing::Font("Microsoft YaHei UI", 9);
            TreeNode^ root = treeProject->Nodes->Add("Project", "工程");
            root->Nodes->Add("Prompt", "需求描述");
            root->Nodes->Add("DSL", "DSL");
            root->Nodes->Add("XML", "XML");
            root->Nodes->Add("SCL", "SCL");
            root->Nodes->Add("STL", "STL");
            root->Nodes->Add("Errors", "错误");
            root->Nodes->Add("Simulation", "仿真");
            root->Nodes->Add("HMI", "HMI");
            root->Nodes->Add("Document", "文档");
            root->Nodes->Add("Export", "导出");
            root->ExpandAll();
            treeProject->AfterSelect += gcnew TreeViewEventHandler(this, &MainForm::OnTreeSelect);
            pnlLeft->Controls->Add(treeProject);

            pnlRight = gcnew Panel();
            pnlRight->Dock = DockStyle::Fill;

            pnlInput = gcnew Panel();
            pnlInput->Dock = DockStyle::Top;
            pnlInput->Height = 170;
            pnlInput->Padding = System::Windows::Forms::Padding(10, 6, 10, 6);

            Label^ lblPrompt = gcnew Label();
            lblPrompt->Text = "PLC需求描述:";
            lblPrompt->Location = Point(0, 0);
            lblPrompt->AutoSize = true;
            pnlInput->Controls->Add(lblPrompt);

            txtPrompt = gcnew TextBox();
            txtPrompt->Multiline = true;
            txtPrompt->Location = Point(0, 22);
            txtPrompt->Size = Drawing::Size(960, 70);
            txtPrompt->Anchor = static_cast<AnchorStyles>(AnchorStyles::Top | AnchorStyles::Left | AnchorStyles::Right);
            txtPrompt->Text = "生成双电机互锁控制系统";
            txtPrompt->ScrollBars = ScrollBars::Vertical;
            pnlInput->Controls->Add(txtPrompt);

            cmbPhase = gcnew ComboBox();
            cmbPhase->DropDownStyle = ComboBoxStyle::DropDownList;
            cmbPhase->Location = Point(0, 100);
            cmbPhase->Size = Drawing::Size(140, 25);
            cmbPhase->Items->AddRange(gcnew array<String^>{"快速生成", "标准生成", "优化生成", "完整工程生成"});
            cmbPhase->SelectedIndex = 1;
            pnlInput->Controls->Add(cmbPhase);

            btnGenerate = gcnew Button();
            btnGenerate->Text = "生成PLC工程";
            btnGenerate->Location = Point(150, 98);
            btnGenerate->Size = Drawing::Size(160, 28);
            btnGenerate->BackColor = Color::FromArgb(0, 120, 215);
            btnGenerate->ForeColor = Color::White;
            btnGenerate->FlatStyle = FlatStyle::Flat;
            btnGenerate->Click += gcnew EventHandler(this, &MainForm::OnGenerateClicked);
            pnlInput->Controls->Add(btnGenerate);

            Label^ lblLog = gcnew Label();
            lblLog->Text = "AI生成日志:";
            lblLog->Location = Point(0, 134);
            lblLog->AutoSize = true;
            pnlInput->Controls->Add(lblLog);

            txtLog = gcnew RichTextBox();
            txtLog->Dock = DockStyle::Top;
            txtLog->Height = 280;
            txtLog->ReadOnly = true;
            txtLog->BackColor = Color::FromArgb(30, 30, 30);
            txtLog->ForeColor = Color::FromArgb(200, 255, 200);
            txtLog->Font = gcnew System::Drawing::Font("Consolas", 9);

            tabMain = gcnew TabControl();
            tabMain->Dock = DockStyle::Fill;

            tabDsl = gcnew TabPage("DSL");
            tabXml = gcnew TabPage("XML");
            tabScl = gcnew TabPage("SCL");
            tabStl = gcnew TabPage("STL");
            tabErrors = gcnew TabPage("错误");
            tabSimulation = gcnew TabPage("仿真");
            tabDocument = gcnew TabPage("文档");
            tabHmi = gcnew TabPage("HMI");

            txtDsl = gcnew RichTextBox();
            txtDsl->Dock = DockStyle::Fill;
            txtDsl->ReadOnly = true;
            txtDsl->Font = gcnew System::Drawing::Font("Consolas", 9);
            txtDsl->WordWrap = false;
            tabDsl->Controls->Add(txtDsl);

            txtXml = gcnew RichTextBox();
            txtXml->Dock = DockStyle::Fill;
            txtXml->ReadOnly = true;
            txtXml->Font = gcnew System::Drawing::Font("Consolas", 9);
            txtXml->WordWrap = false;
            tabXml->Controls->Add(txtXml);

            txtScl = gcnew RichTextBox();
            txtScl->Dock = DockStyle::Fill;
            txtScl->ReadOnly = true;
            txtScl->Font = gcnew System::Drawing::Font("Consolas", 9);
            txtScl->WordWrap = false;
            tabScl->Controls->Add(txtScl);

            txtStl = gcnew RichTextBox();
            txtStl->Dock = DockStyle::Fill;
            txtStl->ReadOnly = true;
            txtStl->Font = gcnew System::Drawing::Font("Consolas", 9);
            txtStl->WordWrap = false;
            tabStl->Controls->Add(txtStl);

            txtErrors = gcnew RichTextBox();
            txtErrors->Dock = DockStyle::Fill;
            txtErrors->ReadOnly = true;
            txtErrors->Font = gcnew System::Drawing::Font("Consolas", 9);
            txtErrors->BackColor = Color::FromArgb(255, 240, 240);
            tabErrors->Controls->Add(txtErrors);

            txtSimulation = gcnew RichTextBox();
            txtSimulation->Dock = DockStyle::Fill;
            txtSimulation->ReadOnly = true;
            txtSimulation->Font = gcnew System::Drawing::Font("Consolas", 9);
            tabSimulation->Controls->Add(txtSimulation);

            txtDocument = gcnew RichTextBox();
            txtDocument->Dock = DockStyle::Fill;
            txtDocument->ReadOnly = true;
            txtDocument->Font = gcnew System::Drawing::Font("Microsoft YaHei UI", 9);
            tabDocument->Controls->Add(txtDocument);

            txtHmi = gcnew RichTextBox();
            txtHmi->Dock = DockStyle::Fill;
            txtHmi->ReadOnly = true;
            txtHmi->Font = gcnew System::Drawing::Font("Consolas", 9);
            tabHmi->Controls->Add(txtHmi);

            tabMain->TabPages->AddRange(gcnew array<TabPage^>{
                tabDsl, tabXml, tabScl, tabStl,
                tabErrors, tabSimulation, tabDocument, tabHmi});

            pnlRight->Controls->Add(tabMain);
            pnlRight->Controls->Add(txtLog);
            pnlRight->Controls->Add(pnlInput);

            statusStrip1 = gcnew StatusStrip();
            statusStrip1->SizingGrip = false;
            statusStrip1->Font = gcnew System::Drawing::Font("Microsoft YaHei UI", 9);
            lblStatus = gcnew ToolStripStatusLabel("就绪");
            lblStatus->Spring = true;
            lblStatus->TextAlign = ContentAlignment::MiddleLeft;
            statusStrip1->Items->Add(lblStatus);

            this->Controls->Add(pnlRight);
            this->Controls->Add(pnlLeft);
            this->Controls->Add(statusStrip1);
            this->Controls->Add(toolStrip1);
            this->Controls->Add(menuStrip1);

            this->MainMenuStrip = menuStrip1;
        }

        void AppendLog(String^ message)
        {
            if (txtLog->InvokeRequired)
            {
                txtLog->BeginInvoke(gcnew LogHandler(this, &MainForm::AppendLog), gcnew array<Object^>{message});
                return;
            }
            txtLog->AppendText(message + "\n");
            txtLog->SelectionStart = txtLog->Text->Length;
            txtLog->ScrollToCaret();

            if (message->StartsWith("[ERROR]"))
                lblStatus->Text = "错误";
            else if (message->StartsWith("[SUCCESS]"))
                lblStatus->Text = "完成";
            else if (message->StartsWith("[INFO]"))
                lblStatus->Text = message->Substring(7);
        }

        void UpdateViews()
        {
            txtDsl->Text = controller->Context->DslJson;
            txtXml->Text = controller->Context->Xml;
            txtScl->Text = controller->Context->SclCode;
            txtStl->Text = controller->Context->StlCode;
            txtErrors->Text = controller->Context->Errors;
            txtSimulation->Text = controller->Context->SimulationLog;
            txtDocument->Text = controller->Context->Document;
            txtHmi->Text = controller->Context->HmiXml;
        }

        void SetBusy(bool busy)
        {
            btnGenerate->Enabled = !busy;
            tsbGenerate->Enabled = !busy;
            tsbValidate->Enabled = !busy;
            tsbRepair->Enabled = !busy;
            tsbSimulate->Enabled = !busy;
        }

        void OnGenerateClicked(Object^ sender, EventArgs^ e)
        {
            if (bgwGenerate->IsBusy)
            {
                MessageBox::Show("生成任务正在运行中，请等待完成。", "提示", MessageBoxButtons::OK, MessageBoxIcon::Information);
                return;
            }

            String^ prompt = txtPrompt->Text->Trim();
            if (prompt->Length == 0)
            {
                MessageBox::Show("请输入PLC需求描述。", "输入提示", MessageBoxButtons::OK, MessageBoxIcon::Warning);
                return;
            }

            txtLog->Clear();
            SetBusy(true);
            lblStatus->Text = "正在生成...";
            bgwGenerate->RunWorkerAsync(gcnew array<String^>{prompt, cmbPhase->SelectedItem->ToString()});
        }

        void BgGenerate_DoWork(Object^ sender, DoWorkEventArgs^ e)
        {
            array<String^>^ args = safe_cast<array<String^>^>(e->Argument);

            if (args[0] == "load")
            {
                controller->LoadDslFile(args[1]);
                return;
            }

            String^ prompt = args[0];
            String^ mode = args[1];

            if (mode == "完整工程生成")
                controller->GeneratePhase6(prompt);
            else if (mode == "优化生成")
                controller->GeneratePhase5(prompt);
            else if (mode == "标准生成")
                controller->GeneratePhase4(prompt);
            else
                controller->GeneratePhase3(prompt);
        }

        void BgGenerate_Completed(Object^ sender, RunWorkerCompletedEventArgs^ e)
        {
            SetBusy(false);
            UpdateViews();
        }

        void OnValidateClicked(Object^ sender, EventArgs^ e)
        {
            txtLog->Clear();
            controller->ValidateProject();
            txtErrors->Text = controller->Context->Errors;
        }

        void OnRepairClicked(Object^ sender, EventArgs^ e)
        {
            if (bgwRepair->IsBusy)
            {
                MessageBox::Show("修复任务正在运行中，请等待完成。", "提示", MessageBoxButtons::OK, MessageBoxIcon::Information);
                return;
            }

            txtLog->Clear();
            SetBusy(true);
            lblStatus->Text = "正在修复...";
            bgwRepair->RunWorkerAsync();
        }

        void BgRepair_DoWork(Object^ sender, DoWorkEventArgs^ e)
        {
            controller->RepairProject();
        }

        void BgRepair_Completed(Object^ sender, RunWorkerCompletedEventArgs^ e)
        {
            SetBusy(false);
            UpdateViews();
        }

        void OnSimulateClicked(Object^ sender, EventArgs^ e)
        {
            if (bgwSimulate->IsBusy)
            {
                MessageBox::Show("仿真任务正在运行中，请等待完成。", "提示", MessageBoxButtons::OK, MessageBoxIcon::Information);
                return;
            }

            txtLog->Clear();
            SetBusy(true);
            lblStatus->Text = "正在仿真...";
            bgwSimulate->RunWorkerAsync();
        }

        void BgSimulate_DoWork(Object^ sender, DoWorkEventArgs^ e)
        {
            controller->RunSimulation(50);
        }

        void BgSimulate_Completed(Object^ sender, RunWorkerCompletedEventArgs^ e)
        {
            SetBusy(false);
            txtSimulation->Text = controller->Context->SimulationLog;
            tabMain->SelectedTab = tabSimulation;
        }

        void OnImportClicked(Object^ sender, EventArgs^ e)
        {
            if (bgwImport->IsBusy)
            {
                MessageBox::Show("导入任务正在运行中，请等待完成", "提示", MessageBoxButtons::OK, MessageBoxIcon::Information);
                return;
            }
            txtLog->Clear();
            SetBusy(true);
            tsbImport->Enabled = false;
            bgwImport->RunWorkerAsync();
        }

        void BgImport_DoWork(Object^ sender, DoWorkEventArgs^ e)
        {
            controller->ImportToTia();
        }

        void BgImport_Completed(Object^ sender, RunWorkerCompletedEventArgs^ e)
        {
            SetBusy(false);
            tsbImport->Enabled = true;
        }

        void OnExportClicked(Object^ sender, EventArgs^ e)
        {
            controller->ExportProject();
        }

        void OnTreeSelect(Object^ sender, TreeViewEventArgs^ e)
        {
            if (e->Node == nullptr) return;
            String^ key = e->Node->Name;
            if (key == "DSL") tabMain->SelectedTab = tabDsl;
            else if (key == "XML") tabMain->SelectedTab = tabXml;
            else if (key == "SCL") tabMain->SelectedTab = tabScl;
            else if (key == "STL") tabMain->SelectedTab = tabStl;
            else if (key == "Errors") tabMain->SelectedTab = tabErrors;
            else if (key == "Simulation") tabMain->SelectedTab = tabSimulation;
            else if (key == "Document") tabMain->SelectedTab = tabDocument;
            else if (key == "HMI") tabMain->SelectedTab = tabHmi;
            else if (key == "Prompt")
            {
                txtPrompt->Focus();
            }
        }

        void OnMenuNew(Object^ sender, EventArgs^ e)
        {
            controller->Context->Clear();
            txtPrompt->Clear();
            txtLog->Clear();
            txtDsl->Clear();
            txtXml->Clear();
            txtScl->Clear();
            txtStl->Clear();
            txtErrors->Clear();
            txtSimulation->Clear();
            txtDocument->Clear();
            txtHmi->Clear();
            lblStatus->Text = "就绪";
        }

        void OnMenuOpen(Object^ sender, EventArgs^ e)
        {
            OpenFileDialog^ dlg = gcnew OpenFileDialog();
            dlg->Filter = "DSL JSON|*.dsl.json|JSON|*.json|所有文件|*.*";
            if (dlg->ShowDialog() == System::Windows::Forms::DialogResult::OK)
            {
                pendingDslPath = dlg->FileName;
                txtLog->Clear();
                SetBusy(true);
                lblStatus->Text = "正在加载DSL...";
                bgwGenerate->RunWorkerAsync(gcnew array<String^>{"load", pendingDslPath});
            }
        }

        void OnMenuSave(Object^ sender, EventArgs^ e)
        {
            controller->ExportProject();
        }

        void OnMenuExit(Object^ sender, EventArgs^ e)
        {
            Application::Exit();
        }

        void OnApiConfigClicked(Object^ sender, EventArgs^ e)
        {
            ApiConfigDialog^ dlg = gcnew ApiConfigDialog(controller->Context->Config, controller->Context->ExeDir);
            if (dlg->ShowDialog() == System::Windows::Forms::DialogResult::OK)
            {
                controller->Context->Config = dlg->GetConfig();
                String^ configPath = System::IO::Path::Combine(controller->Context->ExeDir, "phase3_config.json");
                controller->Context->Config->Save(configPath);
                lblStatus->Text = "API配置已保存";
            }
            delete dlg;
        }

        void OnFormShown(Object^ sender, EventArgs^ e)
        {
            this->TopMost = false;
            this->Activate();
        }
    };
}
