#pragma once

#include "AiPipeline.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

ref struct ModelPreset
{
    String^ Name;
    String^ Provider;
    String^ ApiUrl;
    String^ Model;
    String^ KeyHint;
    Color BrandColor;

    ModelPreset(String^ name, String^ provider, String^ apiUrl, String^ model, String^ keyHint, Color brandColor)
    {
        Name = name;
        Provider = provider;
        ApiUrl = apiUrl;
        Model = model;
        KeyHint = keyHint;
        BrandColor = brandColor;
    }

    virtual String^ ToString() override
    {
        return Name;
    }
};

ref class ModelCard : public Panel
{
public:
    ModelPreset^ Preset;
    Button^ btnAction;
    Label^ lblName;
    Label^ lblModel;
    Label^ lblStatus;
    bool IsConfigured;

    ModelCard(ModelPreset^ preset, bool configured)
    {
        Preset = preset;
        IsConfigured = configured;

        this->Size = Drawing::Size(490, 62);
        this->BackColor = Color::FromArgb(45, 45, 48);
        this->Margin = System::Windows::Forms::Padding(0, 0, 0, 6);

        Panel^ colorBar = gcnew Panel();
        colorBar->Size = Drawing::Size(4, 42);
        colorBar->Location = Point(8, 10);
        colorBar->BackColor = preset->BrandColor;
        this->Controls->Add(colorBar);

        lblName = gcnew Label();
        lblName->Text = preset->Name;
        lblName->Font = gcnew System::Drawing::Font("Microsoft YaHei UI", 10, FontStyle::Bold);
        lblName->ForeColor = Color::White;
        lblName->Location = Point(20, 8);
        lblName->AutoSize = true;
        this->Controls->Add(lblName);

        lblModel = gcnew Label();
        lblModel->Text = preset->Model;
        lblModel->Font = gcnew System::Drawing::Font("Consolas", 8);
        lblModel->ForeColor = Color::FromArgb(160, 160, 160);
        lblModel->Location = Point(20, 32);
        lblModel->AutoSize = true;
        this->Controls->Add(lblModel);

        lblStatus = gcnew Label();
        UpdateStatus(configured);
        lblStatus->Font = gcnew System::Drawing::Font("Microsoft YaHei UI", 8);
        lblStatus->Location = Point(340, 20);
        lblStatus->Size = Drawing::Size(60, 20);
        this->Controls->Add(lblStatus);

        btnAction = gcnew Button();
        btnAction->Size = Drawing::Size(70, 28);
        btnAction->Location = Point(408, 17);
        btnAction->FlatStyle = FlatStyle::Flat;
        btnAction->Font = gcnew System::Drawing::Font("Microsoft YaHei UI", 8);
        btnAction->Cursor = Cursors::Hand;
        UpdateButton(configured);
        this->Controls->Add(btnAction);
    }

    void UpdateStatus(bool configured)
    {
        IsConfigured = configured;
        if (configured)
        {
            lblStatus->Text = "已配置";
            lblStatus->ForeColor = Color::FromArgb(87, 227, 128);
        }
        else
        {
            lblStatus->Text = "未配置";
            lblStatus->ForeColor = Color::FromArgb(160, 160, 160);
        }
    }

    void UpdateButton(bool configured)
    {
        if (configured)
        {
            btnAction->Text = "删除";
            btnAction->BackColor = Color::FromArgb(220, 53, 69);
            btnAction->ForeColor = Color::White;
        }
        else
        {
            btnAction->Text = "添加";
            btnAction->BackColor = Color::FromArgb(0, 120, 215);
            btnAction->ForeColor = Color::White;
        }
    }
};

ref class AddKeyForm : public Form
{
private:
    TextBox^ txtKey;
    TextBox^ txtUrl;
    TextBox^ txtModelName;
    ModelPreset^ preset;

public:
    AddKeyForm(ModelPreset^ p) : preset(p)
    {
        this->Text = "配置 " + preset->Name;
        this->Size = Drawing::Size(460, 300);
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
        this->MaximizeBox = false;
        this->MinimizeBox = false;
        this->StartPosition = FormStartPosition::CenterParent;
        this->Font = gcnew System::Drawing::Font("Microsoft YaHei UI", 9);
        this->BackColor = Color::FromArgb(40, 40, 40);
        this->ForeColor = Color::White;

        int y = 20;
        int labelX = 20;
        int controlX = 120;
        int controlW = 300;

        Label^ lblUrl = gcnew Label();
        lblUrl->Text = "API地址:";
        lblUrl->Location = Point(labelX, y + 5);
        lblUrl->AutoSize = true;
        lblUrl->ForeColor = Color::White;
        this->Controls->Add(lblUrl);

        txtUrl = gcnew TextBox();
        txtUrl->Location = Point(controlX, y);
        txtUrl->Size = Drawing::Size(controlW, 26);
        txtUrl->BackColor = Color::FromArgb(50, 50, 50);
        txtUrl->ForeColor = Color::White;
        if (preset->Provider != "自定义")
        {
            txtUrl->Text = preset->ApiUrl;
            txtUrl->ReadOnly = true;
            txtUrl->ForeColor = Color::FromArgb(180, 180, 180);
        }
        this->Controls->Add(txtUrl);
        y += 38;

        Label^ lblModel = gcnew Label();
        lblModel->Text = "模型:";
        lblModel->Location = Point(labelX, y + 5);
        lblModel->AutoSize = true;
        lblModel->ForeColor = Color::White;
        this->Controls->Add(lblModel);

        txtModelName = gcnew TextBox();
        txtModelName->Location = Point(controlX, y);
        txtModelName->Size = Drawing::Size(controlW, 26);
        txtModelName->BackColor = Color::FromArgb(50, 50, 50);
        txtModelName->ForeColor = Color::White;
        if (preset->Provider != "自定义")
        {
            txtModelName->Text = preset->Model;
            txtModelName->ReadOnly = true;
            txtModelName->ForeColor = Color::FromArgb(180, 180, 180);
        }
        this->Controls->Add(txtModelName);
        y += 38;

        Label^ lblKey = gcnew Label();
        lblKey->Text = "API密钥:";
        lblKey->Location = Point(labelX, y + 5);
        lblKey->AutoSize = true;
        lblKey->ForeColor = Color::White;
        this->Controls->Add(lblKey);

        txtKey = gcnew TextBox();
        txtKey->Location = Point(controlX, y);
        txtKey->Size = Drawing::Size(controlW, 26);
        txtKey->UseSystemPasswordChar = true;
        txtKey->BackColor = Color::FromArgb(50, 50, 50);
        txtKey->ForeColor = Color::White;
        this->Controls->Add(txtKey);
        y += 38;

        Label^ lblHint = gcnew Label();
        lblHint->Text = preset->KeyHint;
        lblHint->Location = Point(controlX, y);
        lblHint->AutoSize = true;
        lblHint->Font = gcnew System::Drawing::Font("Microsoft YaHei UI", 8);
        lblHint->ForeColor = Color::FromArgb(120, 120, 120);
        this->Controls->Add(lblHint);
        y += 40;

        Button^ btnConfirm = gcnew Button();
        btnConfirm->Text = "确认添加";
        btnConfirm->Size = Drawing::Size(120, 34);
        btnConfirm->Location = Point(controlX, y);
        btnConfirm->BackColor = Color::FromArgb(0, 120, 215);
        btnConfirm->ForeColor = Color::White;
        btnConfirm->FlatStyle = FlatStyle::Flat;
        btnConfirm->Click += gcnew EventHandler(this, &AddKeyForm::OnConfirm);
        this->Controls->Add(btnConfirm);

        Button^ btnCancel = gcnew Button();
        btnCancel->Text = "取消";
        btnCancel->Size = Drawing::Size(80, 34);
        btnCancel->Location = Point(controlX + 130, y);
        btnCancel->BackColor = Color::FromArgb(60, 60, 60);
        btnCancel->ForeColor = Color::White;
        btnCancel->FlatStyle = FlatStyle::Flat;
        btnCancel->Click += gcnew EventHandler(this, &AddKeyForm::OnCancel);
        this->Controls->Add(btnCancel);
    }

    property String^ InputApiKey
    {
        String^ get() { return txtKey->Text->Trim(); }
    }

    property String^ InputApiUrl
    {
        String^ get() { return txtUrl->Text->Trim(); }
    }

    property String^ InputModelName
    {
        String^ get() { return txtModelName->Text->Trim(); }
    }

private:
    void OnConfirm(Object^ sender, EventArgs^ e)
    {
        if (InputApiKey->Length == 0)
        {
            MessageBox::Show("请输入API密钥。", "提示", MessageBoxButtons::OK, MessageBoxIcon::Warning);
            return;
        }

        if (preset->Provider == "自定义")
        {
            if (InputApiUrl->Length == 0 || InputModelName->Length == 0)
            {
                MessageBox::Show("请填写API地址和模型名称。", "提示", MessageBoxButtons::OK, MessageBoxIcon::Warning);
                return;
            }
        }

        this->DialogResult = System::Windows::Forms::DialogResult::OK;
        this->Close();
    }

    void OnCancel(Object^ sender, EventArgs^ e)
    {
        this->DialogResult = System::Windows::Forms::DialogResult::Cancel;
        this->Close();
    }
};

public ref class ApiConfigDialog : public Form
{
private:
    FlowLayoutPanel^ flowModels;
    P3Config^ currentConfig;
    String^ exeDir;
    System::Collections::Generic::List<ModelCard^>^ cards;

    static array<ModelPreset^>^ presets = gcnew array<ModelPreset^>{
        gcnew ModelPreset("DeepSeek V4 Pro", "国内", "https://api.deepseek.com/chat/completions", "deepseek-v4-pro", "从 platform.deepseek.com 获取", Color::FromArgb(73, 133, 255)),
        gcnew ModelPreset("DeepSeek V4 Flash", "国内", "https://api.deepseek.com/chat/completions", "deepseek-v4-flash", "从 platform.deepseek.com 获取", Color::FromArgb(73, 133, 255)),
        gcnew ModelPreset("通义千问 3.6 Max", "国内", "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions", "qwen3.6-max-preview", "从 dashscope.console.aliyun.com 获取", Color::FromArgb(255, 106, 0)),
        gcnew ModelPreset("通义千问 3.6 Plus", "国内", "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions", "qwen3.6-plus", "从 dashscope.console.aliyun.com 获取", Color::FromArgb(255, 106, 0)),
        gcnew ModelPreset("通义千问 3.6 Flash", "国内", "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions", "qwen3.6-flash", "从 dashscope.console.aliyun.com 获取", Color::FromArgb(255, 106, 0)),
        gcnew ModelPreset("QwQ Plus 推理", "国内", "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions", "qwq-plus", "从 dashscope.console.aliyun.com 获取", Color::FromArgb(255, 140, 0)),
        gcnew ModelPreset("智谱 GLM-5.1", "国内", "https://open.bigmodel.cn/api/paas/v4/chat/completions", "glm-5.1", "从 open.bigmodel.cn 获取", Color::FromArgb(54, 88, 226)),
        gcnew ModelPreset("智谱 GLM-5-Turbo", "国内", "https://open.bigmodel.cn/api/paas/v4/chat/completions", "glm-5-turbo", "从 open.bigmodel.cn 获取", Color::FromArgb(54, 88, 226)),
        gcnew ModelPreset("智谱 GLM-4.7-Flash(免费)", "国内", "https://open.bigmodel.cn/api/paas/v4/chat/completions", "glm-4.7-flash", "从 open.bigmodel.cn 获取", Color::FromArgb(100, 120, 226)),
        gcnew ModelPreset("GPT-5.5", "国外", "https://api.openai.com/v1/chat/completions", "gpt-5.5", "从 platform.openai.com 获取", Color::FromArgb(16, 163, 127)),
        gcnew ModelPreset("GPT-5.4", "国外", "https://api.openai.com/v1/chat/completions", "gpt-5.4", "从 platform.openai.com 获取", Color::FromArgb(16, 163, 127)),
        gcnew ModelPreset("GPT-5.4 Mini", "国外", "https://api.openai.com/v1/chat/completions", "gpt-5.4-mini", "从 platform.openai.com 获取", Color::FromArgb(16, 163, 127)),
        gcnew ModelPreset("Claude Opus 4.7", "国外", "https://api.anthropic.com/v1/messages", "claude-opus-4-7", "从 console.anthropic.com 获取", Color::FromArgb(216, 154, 80)),
        gcnew ModelPreset("Claude Sonnet 4.6", "国外", "https://api.anthropic.com/v1/messages", "claude-sonnet-4-6", "从 console.anthropic.com 获取", Color::FromArgb(216, 154, 80)),
        gcnew ModelPreset("Claude Haiku 4.5", "国外", "https://api.anthropic.com/v1/messages", "claude-haiku-4-5-20251001", "从 console.anthropic.com 获取", Color::FromArgb(216, 154, 80)),
        gcnew ModelPreset("Gemini 3.5 Flash", "国外", "https://generativelanguage.googleapis.com/v1beta/openai/chat/completions", "gemini-3.5-flash", "从 aistudio.google.com 获取", Color::FromArgb(66, 133, 244)),
        gcnew ModelPreset("Gemini 3.1 Pro", "国外", "https://generativelanguage.googleapis.com/v1beta/openai/chat/completions", "gemini-3.1-pro-preview", "从 aistudio.google.com 获取", Color::FromArgb(66, 133, 244)),
        gcnew ModelPreset("Gemini 3.1 Flash-Lite", "国外", "https://generativelanguage.googleapis.com/v1beta/openai/chat/completions", "gemini-3.1-flash-lite", "从 aistudio.google.com 获取", Color::FromArgb(66, 133, 244)),
        gcnew ModelPreset("自定义模型", "自定义", "", "", "输入自定义API地址和模型名", Color::FromArgb(128, 128, 128))
    };

public:
    ApiConfigDialog(P3Config^ config, String^ exeDirectory)
    {
        currentConfig = config;
        exeDir = exeDirectory;
        cards = gcnew System::Collections::Generic::List<ModelCard^>();

        this->Text = "API 模型配置";
        this->Size = Drawing::Size(560, 680);
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::Sizable;
        this->MaximizeBox = false;
        this->MinimizeBox = false;
        this->StartPosition = FormStartPosition::CenterParent;
        this->Font = gcnew System::Drawing::Font("Microsoft YaHei UI", 9);
        this->BackColor = Color::FromArgb(30, 30, 30);
        this->ForeColor = Color::White;

        Label^ lblTitle = gcnew Label();
        lblTitle->Text = "选择要配置的AI模型";
        lblTitle->Font = gcnew System::Drawing::Font("Microsoft YaHei UI", 14, FontStyle::Bold);
        lblTitle->ForeColor = Color::White;
        lblTitle->Location = Point(20, 15);
        lblTitle->AutoSize = true;
        this->Controls->Add(lblTitle);

        Label^ lblHint = gcnew Label();
        lblHint->Text = "点击「添加」输入API密钥，点击「删除」移除密钥";
        lblHint->Font = gcnew System::Drawing::Font("Microsoft YaHei UI", 8);
        lblHint->ForeColor = Color::FromArgb(160, 160, 160);
        lblHint->Location = Point(22, 45);
        lblHint->AutoSize = true;
        this->Controls->Add(lblHint);

        flowModels = gcnew FlowLayoutPanel();
        flowModels->Location = Point(15, 70);
        flowModels->Size = Drawing::Size(520, 540);
        flowModels->FlowDirection = FlowDirection::TopDown;
        flowModels->WrapContents = false;
        flowModels->AutoScroll = true;
        flowModels->BackColor = Color::FromArgb(30, 30, 30);
        this->Controls->Add(flowModels);

        BuildCards();

        Button^ btnClose = gcnew Button();
        btnClose->Text = "关闭";
        btnClose->Size = Drawing::Size(90, 32);
        btnClose->Location = Point(440, 615);
        btnClose->BackColor = Color::FromArgb(60, 60, 60);
        btnClose->ForeColor = Color::White;
        btnClose->FlatStyle = FlatStyle::Flat;
        btnClose->Click += gcnew EventHandler(this, &ApiConfigDialog::OnCloseClicked);
        this->Controls->Add(btnClose);
    }

    P3Config^ GetConfig()
    {
        return currentConfig;
    }

private:
    void BuildCards()
    {
        bool hasAnyKey = (currentConfig->ApiKey != nullptr && currentConfig->ApiKey->Length > 0);
        bool isCustomModel = true;
        if (hasAnyKey)
        {
            for (int i = 0; i < presets->Length; i++)
            {
                if (presets[i]->Provider != "自定义" && currentConfig->Model == presets[i]->Model)
                {
                    isCustomModel = false;
                    break;
                }
            }
        }

        for (int i = 0; i < presets->Length; i++)
        {
            bool configured = false;
            if (hasAnyKey)
            {
                if (presets[i]->Provider == "自定义")
                {
                    configured = isCustomModel;
                }
                else
                {
                    configured = (currentConfig->Model == presets[i]->Model);
                }
            }

            ModelCard^ card = gcnew ModelCard(presets[i], configured);
            card->btnAction->Tag = i;
            card->btnAction->Click += gcnew EventHandler(this, &ApiConfigDialog::OnCardAction);
            cards->Add(card);
            flowModels->Controls->Add(card);
        }
    }

    void OnCardAction(Object^ sender, EventArgs^ e)
    {
        Button^ btn = safe_cast<Button^>(sender);
        int idx = safe_cast<int>(btn->Tag);
        ModelCard^ card = cards[idx];
        ModelPreset^ preset = card->Preset;

        if (card->IsConfigured)
        {
            System::Windows::Forms::DialogResult r = MessageBox::Show(
                "确定删除 " + preset->Name + " 的API密钥？",
                "确认删除", MessageBoxButtons::YesNo, MessageBoxIcon::Warning);

            if (r == System::Windows::Forms::DialogResult::Yes)
            {
                currentConfig->ApiKey = "";
                currentConfig->ApiUrl = "";
                currentConfig->Model = "";

                String^ configPath = System::IO::Path::Combine(exeDir, "phase3_config.json");
                currentConfig->Save(configPath);

                card->UpdateStatus(false);
                card->UpdateButton(false);

                for (int i = 0; i < cards->Count; i++)
                {
                    if (cards[i] != card && cards[i]->IsConfigured)
                    {
                        cards[i]->UpdateStatus(false);
                        cards[i]->UpdateButton(false);
                    }
                }
            }
        }
        else
        {
            AddKeyForm^ dlg = gcnew AddKeyForm(preset);
            if (dlg->ShowDialog() == System::Windows::Forms::DialogResult::OK)
            {
                String^ url = preset->ApiUrl;
                String^ model = preset->Model;

                if (preset->Provider == "自定义")
                {
                    url = dlg->InputApiUrl;
                    model = dlg->InputModelName;
                }

                currentConfig->ApiKey = dlg->InputApiKey;
                currentConfig->ApiUrl = url;
                currentConfig->Model = model;

                String^ configPath = System::IO::Path::Combine(exeDir, "phase3_config.json");
                currentConfig->Save(configPath);

                card->UpdateStatus(true);
                card->UpdateButton(true);

                for (int i = 0; i < cards->Count; i++)
                {
                    if (cards[i] != card && cards[i]->IsConfigured)
                    {
                        cards[i]->UpdateStatus(false);
                        cards[i]->UpdateButton(false);
                    }
                }
            }
            delete dlg;
        }
    }

    void OnCloseClicked(Object^ sender, EventArgs^ e)
    {
        this->DialogResult = System::Windows::Forms::DialogResult::OK;
        this->Close();
    }
};
