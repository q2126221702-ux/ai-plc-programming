#pragma once

using namespace System;
using namespace System::Collections::Generic;

public ref class ProjectContext
{
public:
    String^ Prompt;
    String^ DslJson;
    String^ Xml;
    String^ Logs;
    String^ Errors;
    String^ SclCode;
    String^ StlCode;
    String^ FbdCode;
    String^ Visualization;
    String^ SimulationLog;
    String^ HmiXml;
    String^ Document;
    String^ TagTableXml;
    P3Dsl^ Dsl;
    P3Config^ Config;
    bool HasTiaPortal;
    String^ TiaVersion;
    String^ ExeDir;
    String^ TemplateXmlPath;

    ProjectContext()
    {
        Prompt = "";
        DslJson = "";
        Xml = "";
        Logs = "";
        Errors = "";
        SclCode = "";
        StlCode = "";
        FbdCode = "";
        Visualization = "";
        SimulationLog = "";
        HmiXml = "";
        Document = "";
        TagTableXml = "";
        Dsl = nullptr;
        Config = nullptr;
        HasTiaPortal = false;
        TiaVersion = "";
        ExeDir = "";
        TemplateXmlPath = "";
    }

    void Clear()
    {
        DslJson = "";
        Xml = "";
        Logs = "";
        Errors = "";
        SclCode = "";
        StlCode = "";
        FbdCode = "";
        Visualization = "";
        SimulationLog = "";
        HmiXml = "";
        Document = "";
        TagTableXml = "";
        Dsl = nullptr;
    }
};
