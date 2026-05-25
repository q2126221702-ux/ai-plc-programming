#pragma once
#include "DataTypes.h"

ref class PartSchemaRegistry {
private:
    Dictionary<int, PartSchema^>^ schemas;
    Dictionary<String^, int>^ nameToType;
    Dictionary<int, String^>^ typeToName;

    void AddSchema(CuPartType pt, String^ flgNetName,
        array<String^>^ inputPins, array<String^>^ outputPins,
        bool requiresInstanceDB, bool requiresOperand,
        bool supportsNegation, bool hasDynamicCardinality,
        String^ presetPinName,
        String^ version,
        array<TemplateValueDef^>^ templateValues,
        String^ instanceType,
        bool useCallStructure,
        bool disabledENO,
        bool requiresInstance) {
        PartSchema^ schema = gcnew PartSchema();
        schema->PartType = pt;
        schema->FlgNetName = flgNetName;
        schema->InputPins = gcnew List<PinDef^>();
        schema->OutputPins = gcnew List<PinDef^>();
        schema->RequiresInstanceDB = requiresInstanceDB;
        schema->RequiresOperand = requiresOperand;
        schema->SupportsNegation = supportsNegation;
        schema->HasDynamicCardinality = hasDynamicCardinality;
        schema->PresetPinName = presetPinName;
        schema->Version = version;
        schema->TemplateValues = gcnew List<TemplateValueDef^>();
        if (templateValues != nullptr) {
            for (int i = 0; i < templateValues->Length; i++) {
                schema->TemplateValues->Add(templateValues[i]);
            }
        }
        schema->InstanceType = instanceType;
        schema->UseCallStructure = useCallStructure;
        schema->DisabledENO = disabledENO;
        schema->RequiresInstance = requiresInstance;

        for (int i = 0; i < inputPins->Length; i++) {
            PinDef^ pd = gcnew PinDef();
            pd->Name = inputPins[i];
            pd->Dir = PIN_DIR_IN;
            pd->Required = true;
            schema->InputPins->Add(pd);
        }
        for (int i = 0; i < outputPins->Length; i++) {
            PinDef^ pd = gcnew PinDef();
            pd->Name = outputPins[i];
            pd->Dir = PIN_DIR_OUT;
            pd->Required = false;
            schema->OutputPins->Add(pd);
        }

        schemas[(int)pt] = schema;
        nameToType[flgNetName] = (int)pt;
        typeToName[(int)pt] = flgNetName;
    }

    TemplateValueDef^ TV(String^ name, String^ type, String^ defaultValue) {
        TemplateValueDef^ tv = gcnew TemplateValueDef();
        tv->Name = name;
        tv->Type = type;
        tv->DefaultValue = defaultValue;
        tv->IsAutomaticTyped = false;
        return tv;
    }

    TemplateValueDef^ ATV(String^ name) {
        TemplateValueDef^ tv = gcnew TemplateValueDef();
        tv->Name = name;
        tv->Type = "";
        tv->DefaultValue = "";
        tv->IsAutomaticTyped = true;
        return tv;
    }

public:
    PartSchemaRegistry() {
        schemas = gcnew Dictionary<int, PartSchema^>();
        nameToType = gcnew Dictionary<String^, int>();
        typeToName = gcnew Dictionary<int, String^>();

        AddSchema(CuPartType::Contact, "Contact",
            gcnew array<String^> { "in", "operand" },
            gcnew array<String^> { "out" },
            false, true, true, false, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::Coil, "Coil",
            gcnew array<String^> { "in", "operand" },
            gcnew array<String^> {},
            false, true, false, false, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::SCoil, "SCoil",
            gcnew array<String^> { "in", "operand" },
            gcnew array<String^> {},
            false, true, false, false, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::RCoil, "RCoil",
            gcnew array<String^> { "in", "operand" },
            gcnew array<String^> {},
            false, true, false, false, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::TON, "TON",
            gcnew array<String^> { "IN", "PT" },
            gcnew array<String^> { "Q", "ET" },
            true, false, false, false, "PT",
            "1.0",
            gcnew array<TemplateValueDef^> { TV("time_type", "Type", "Time") },
            "IEC_TIMER", false, false, true);

        AddSchema(CuPartType::TOF, "TOF",
            gcnew array<String^> { "IN", "PT" },
            gcnew array<String^> { "Q", "ET" },
            true, false, false, false, "PT",
            "1.0",
            gcnew array<TemplateValueDef^> { TV("time_type", "Type", "Time") },
            "IEC_TIMER", false, false, true);

        AddSchema(CuPartType::CTU, "CTU",
            gcnew array<String^> { "CU", "R", "PV" },
            gcnew array<String^> { "Q", "CV" },
            true, false, false, false, "PV",
            "1.0",
            gcnew array<TemplateValueDef^> { TV("value_type", "Type", "Int") },
            "IEC_COUNTER", false, false, true);

        AddSchema(CuPartType::CTD, "CTD",
            gcnew array<String^> { "CD", "LD", "PV" },
            gcnew array<String^> { "Q", "CV" },
            true, false, false, false, "PV",
            "1.0",
            gcnew array<TemplateValueDef^> { TV("value_type", "Type", "Int") },
            "IEC_COUNTER", false, false, true);

        AddSchema(CuPartType::O, "O",
            gcnew array<String^> { "in1", "in2" },
            gcnew array<String^> { "out" },
            false, false, false, true, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::Compare, "Compare",
            gcnew array<String^> { "in1", "in2" },
            gcnew array<String^> { "out" },
            false, false, false, true, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::Math, "Math",
            gcnew array<String^> { "in1", "in2" },
            gcnew array<String^> { "out" },
            true, false, false, true, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::Move, "Move",
            gcnew array<String^> { "en", "in" },
            gcnew array<String^> { "eno", "out1" },
            false, false, false, false, nullptr,
            "",
            gcnew array<TemplateValueDef^> {
                TV("Card", "Cardinality", "1")
            },
            "", false, true, false);

        AddSchema(CuPartType::Junction, "Junction",
            gcnew array<String^> { "in" },
            gcnew array<String^> { "out" },
            false, false, false, false, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::TP, "TP",
            gcnew array<String^> { "IN", "PT" },
            gcnew array<String^> { "Q", "ET" },
            true, false, false, false, "PT",
            "1.0",
            gcnew array<TemplateValueDef^> { TV("time_type", "Type", "Time") },
            "IEC_TIMER", false, false, true);

        AddSchema(CuPartType::CTUD, "CTUD",
            gcnew array<String^> { "CU", "CD", "R", "LD", "PV" },
            gcnew array<String^> { "QU", "QD", "CV" },
            true, false, false, false, "PV",
            "1.0",
            gcnew array<TemplateValueDef^> { TV("value_type", "Type", "Int") },
            "IEC_COUNTER", false, false, true);

        AddSchema(CuPartType::CompareEQ, "Eq",
            gcnew array<String^> { "pre", "in1", "in2" },
            gcnew array<String^> { "out" },
            false, false, false, true, nullptr,
            "",
            gcnew array<TemplateValueDef^> { TV("SrcType", "Type", "Int") },
            "", false, false, false);

        AddSchema(CuPartType::CompareNE, "Ne",
            gcnew array<String^> { "pre", "in1", "in2" },
            gcnew array<String^> { "out" },
            false, false, false, true, nullptr,
            "",
            gcnew array<TemplateValueDef^> { TV("SrcType", "Type", "Int") },
            "", false, false, false);

        AddSchema(CuPartType::CompareGT, "Gt",
            gcnew array<String^> { "pre", "in1", "in2" },
            gcnew array<String^> { "out" },
            false, false, false, true, nullptr,
            "",
            gcnew array<TemplateValueDef^> { TV("SrcType", "Type", "Int") },
            "", false, false, false);

        AddSchema(CuPartType::CompareLT, "Lt",
            gcnew array<String^> { "pre", "in1", "in2" },
            gcnew array<String^> { "out" },
            false, false, false, true, nullptr,
            "",
            gcnew array<TemplateValueDef^> { TV("SrcType", "Type", "Int") },
            "", false, false, false);

        AddSchema(CuPartType::CompareGE, "Ge",
            gcnew array<String^> { "pre", "in1", "in2" },
            gcnew array<String^> { "out" },
            false, false, false, true, nullptr,
            "",
            gcnew array<TemplateValueDef^> { TV("SrcType", "Type", "Int") },
            "", false, false, false);

        AddSchema(CuPartType::CompareLE, "Le",
            gcnew array<String^> { "pre", "in1", "in2" },
            gcnew array<String^> { "out" },
            false, false, false, true, nullptr,
            "",
            gcnew array<TemplateValueDef^> { TV("SrcType", "Type", "Int") },
            "", false, false, false);

        AddSchema(CuPartType::RisingEdge, "PContact",
            gcnew array<String^> { "pre", "operand", "bit" },
            gcnew array<String^> { "out" },
            false, false, false, false, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::FallingEdge, "NContact",
            gcnew array<String^> { "pre", "operand", "bit" },
            gcnew array<String^> { "out" },
            false, false, false, false, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::RBitfield, "RBitfield",
            gcnew array<String^> { "en", "n", "operand" },
            gcnew array<String^> {},
            false, true, false, false, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::SBitfield, "SBitfield",
            gcnew array<String^> { "en", "n", "operand" },
            gcnew array<String^> {},
            false, true, false, false, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::ADD, "Add",
            gcnew array<String^> { "en", "in1", "in2" },
            gcnew array<String^> { "eno", "out" },
            false, false, false, true, nullptr,
            "",
            gcnew array<TemplateValueDef^> {
                TV("Card", "Cardinality", "2"),
                ATV("SrcType")
            },
            "", false, true, false);

        AddSchema(CuPartType::SUB, "Sub",
            gcnew array<String^> { "en", "in1", "in2" },
            gcnew array<String^> { "eno", "out" },
            false, false, false, true, nullptr,
            "",
            gcnew array<TemplateValueDef^> {
                ATV("SrcType")
            },
            "", false, true, false);

        AddSchema(CuPartType::MUL, "Mul",
            gcnew array<String^> { "en", "in1", "in2" },
            gcnew array<String^> { "eno", "out" },
            false, false, false, true, nullptr,
            "",
            gcnew array<TemplateValueDef^> {
                TV("Card", "Cardinality", "2"),
                ATV("SrcType")
            },
            "", false, true, false);

        AddSchema(CuPartType::DIV, "Div",
            gcnew array<String^> { "en", "in1", "in2" },
            gcnew array<String^> { "eno", "out" },
            false, false, false, true, nullptr,
            "",
            gcnew array<TemplateValueDef^> {
                ATV("SrcType")
            },
            "", false, true, false);

        AddSchema(CuPartType::MOD, "Mod",
            gcnew array<String^> { "en", "in1", "in2" },
            gcnew array<String^> { "eno", "out" },
            false, false, false, true, nullptr,
            "",
            gcnew array<TemplateValueDef^> {
                ATV("SrcType")
            },
            "", false, true, false);

        AddSchema(CuPartType::JMP, "JMP",
            gcnew array<String^> { "in", "operand" },
            gcnew array<String^> {},
            false, true, false, false, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::LABEL, "LABEL",
            gcnew array<String^> { "in", "operand" },
            gcnew array<String^> {},
            false, true, false, false, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::RET, "RET",
            gcnew array<String^> { "in" },
            gcnew array<String^> {},
            false, false, false, false, nullptr,
            "", nullptr, "", false, false, false);

        AddSchema(CuPartType::NOP, "NOP",
            gcnew array<String^> { "in" },
            gcnew array<String^> { "out" },
            false, false, false, false, nullptr,
            "", nullptr, "", false, false, false);
    }

    PartSchema^ GetSchema(CuPartType pt) {
        if (schemas->ContainsKey((int)pt)) return schemas[(int)pt];
        return nullptr;
    }

    CuPartType NameToType(String^ flgNetName) {
        if (nameToType->ContainsKey(flgNetName)) return (CuPartType)nameToType[flgNetName];
        return CuPartType::Contact;
    }

    String^ TypeToName(CuPartType pt) {
        if (typeToName->ContainsKey((int)pt)) return typeToName[(int)pt];
        return "Contact";
    }

    String^ GetFirstInputPin(CuPartType pt) {
        PartSchema^ s = GetSchema(pt);
        if (s != nullptr && s->InputPins->Count > 0) return s->InputPins[0]->Name;
        return "in";
    }

    String^ GetFirstOutputPin(CuPartType pt) {
        PartSchema^ s = GetSchema(pt);
        if (s != nullptr && s->OutputPins->Count > 0) return s->OutputPins[0]->Name;
        return "out";
    }

    String^ GetOperandPin(CuPartType pt) {
        PartSchema^ s = GetSchema(pt);
        if (s != nullptr && s->RequiresOperand) {
            for (int i = 0; i < s->InputPins->Count; i++) {
                if (s->InputPins[i]->Name == "operand") return "operand";
            }
        }
        return "";
    }

    List<CuPin^>^ ExpandPins(CuPartType pt, int cardinality) {
        PartSchema^ s = GetSchema(pt);
        List<CuPin^>^ pins = gcnew List<CuPin^>();
        if (s == nullptr) return pins;

        if (s->HasDynamicCardinality && cardinality > 2) {
            for (int i = 1; i <= cardinality; i++) {
                CuPin^ p = gcnew CuPin();
                p->Name = "in" + i.ToString();
                p->Dir = PIN_DIR_IN;
                p->Required = true;
                pins->Add(p);
            }
            for (int i = 0; i < s->OutputPins->Count; i++) {
                CuPin^ p = gcnew CuPin();
                p->Name = s->OutputPins[i]->Name;
                p->Dir = s->OutputPins[i]->Dir;
                p->Required = s->OutputPins[i]->Required;
                pins->Add(p);
            }
        }
        else {
            for (int i = 0; i < s->InputPins->Count; i++) {
                CuPin^ p = gcnew CuPin();
                p->Name = s->InputPins[i]->Name;
                p->Dir = s->InputPins[i]->Dir;
                p->Required = s->InputPins[i]->Required;
                pins->Add(p);
            }
            for (int i = 0; i < s->OutputPins->Count; i++) {
                CuPin^ p = gcnew CuPin();
                p->Name = s->OutputPins[i]->Name;
                p->Dir = s->OutputPins[i]->Dir;
                p->Required = s->OutputPins[i]->Required;
                pins->Add(p);
            }
        }
        return pins;
    }
};

ref class UidAllocator {
private:
    int nextUid;
    HashSet<int>^ usedSet;
public:
    UidAllocator() : nextUid(1), usedSet(gcnew HashSet<int>()) {}

    void ScanTemplate(String^ xml) {
        System::Text::RegularExpressions::Regex^ idRegex =
            gcnew System::Text::RegularExpressions::Regex("ID=\"([0-9A-Fa-f]+)\"");
        for each (System::Text::RegularExpressions::Match^ m in idRegex->Matches(xml)) {
            int val = System::Convert::ToInt32(m->Groups[1]->Value, 16);
            if (val >= nextUid) nextUid = val + 1;
            usedSet->Add(val);
        }

        Console::WriteLine("    UidAllocator: scanned template, {0} existing IDs, max={1}, starting from {2}",
            usedSet->Count, nextUid - 1, nextUid);
    }

    int Alloc() {
        int uid = nextUid++;
        while (usedSet->Contains(uid)) uid = nextUid++;
        usedSet->Add(uid);
        return uid;
    }

    void ForceStartFrom(int start) {
        nextUid = start;
        usedSet->Clear();
    }
};

inline LogicGraph^ BuildLogicGraph(LadNetwork^ network) {
    LogicGraph^ graph = gcnew LogicGraph();
    graph->Nodes = gcnew List<LgNode^>();
    graph->Edges = gcnew List<LgEdge^>();

    int nextId = 1;
    int prevNodeId = -1;

    List<LadElement^>^ elems = network->Elements;
    for (int i = 0; i < elems->Count; i++) {
        LadElement^ el = elems[i];

        if (el->Type == "parallel" && el->Branches != nullptr && el->Branches->Count >= 2) {
            int forkId = prevNodeId;

            if (prevNodeId >= 0) {
                LgNode^ forkNode = gcnew LgNode();
                forkNode->Id = nextId++;
                forkNode->NodeType = CuPartType::Junction;
                forkNode->Tag = "";
                forkNode->NormallyOpen = true;
                graph->Nodes->Add(forkNode);

                LgEdge^ forkEdge = gcnew LgEdge();
                forkEdge->FromId = prevNodeId;
                forkEdge->ToId = forkNode->Id;
                graph->Edges->Add(forkEdge);

                forkId = forkNode->Id;
            }

            List<int>^ branchEndIds = gcnew List<int>();

            for (int bi = 0; bi < el->Branches->Count; bi++) {
                List<LadElement^>^ branch = el->Branches[bi];
                int branchPrevId = forkId;

                for (int bei = 0; bei < branch->Count; bei++) {
                    LadElement^ bel = branch[bei];
                    String^ btype = bel->Type;

                    if (btype == "access") {
                        if (bei + 1 < branch->Count) {
                            LadElement^ nextB = branch[bei + 1];
                            String^ nbt = nextB->Type;
                            if (nbt == "contact") {
                                LgNode^ node = gcnew LgNode();
                                node->Id = nextId++;
                                node->NodeType = CuPartType::Contact;
                                node->Tag = bel->Tag;
                                node->NormallyOpen = nextB->NormallyOpen;
                                graph->Nodes->Add(node);

                                LgEdge^ edge = gcnew LgEdge();
                                edge->FromId = branchPrevId;
                                edge->ToId = node->Id;
                                graph->Edges->Add(edge);

                                branchPrevId = node->Id;
                                bei++;
                            }
                            else if (nbt == "coil" || nbt == "setCoil" || nbt == "resetCoil") {
                                CuPartType coilType = CuPartType::Coil;
                                if (nbt == "setCoil") coilType = CuPartType::SCoil;
                                else if (nbt == "resetCoil") coilType = CuPartType::RCoil;

                                LgNode^ node = gcnew LgNode();
                                node->Id = nextId++;
                                node->NodeType = coilType;
                                node->Tag = bel->Tag;
                                node->NormallyOpen = true;
                                graph->Nodes->Add(node);

                                LgEdge^ edge = gcnew LgEdge();
                                edge->FromId = branchPrevId;
                                edge->ToId = node->Id;
                                graph->Edges->Add(edge);
                                branchPrevId = node->Id;
                                bei++;
                            }
                        }
                    }
                    else if (btype == "timerOnDelay") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::TON;
                        node->Tag = "";
                        node->NormallyOpen = true;
                        node->InstanceName = bel->InstanceName;
                        node->PresetTime = bel->PresetTime;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "timerOffDelay") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::TOF;
                        node->Tag = "";
                        node->NormallyOpen = true;
                        node->InstanceName = bel->InstanceName;
                        node->PresetTime = bel->PresetTime;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "counterUp") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::CTU;
                        node->Tag = "";
                        node->NormallyOpen = true;
                        node->InstanceName = bel->InstanceName;
                        node->PresetTime = bel->PresetTime;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "counterDown") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::CTD;
                        node->Tag = "";
                        node->NormallyOpen = true;
                        node->InstanceName = bel->InstanceName;
                        node->PresetTime = bel->PresetTime;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "timerPulse") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::TP;
                        node->Tag = "";
                        node->NormallyOpen = true;
                        node->InstanceName = bel->InstanceName;
                        node->PresetTime = bel->PresetTime;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "counterUpDown") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::CTUD;
                        node->Tag = "";
                        node->NormallyOpen = true;
                        node->InstanceName = bel->InstanceName;
                        node->PresetTime = bel->PresetTime;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "compareEQ" || btype == "compareNE" || btype == "compareGT" ||
                             btype == "compareLT" || btype == "compareGE" || btype == "compareLE") {
                        CuPartType cmpType = CuPartType::CompareEQ;
                        if (btype == "compareNE") cmpType = CuPartType::CompareNE;
                        else if (btype == "compareGT") cmpType = CuPartType::CompareGT;
                        else if (btype == "compareLT") cmpType = CuPartType::CompareLT;
                        else if (btype == "compareGE") cmpType = CuPartType::CompareGE;
                        else if (btype == "compareLE") cmpType = CuPartType::CompareLE;

                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = cmpType;
                        node->Tag = bel->Tag;
                        node->Tag2 = bel->Tag2;
                        node->NormallyOpen = true;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "risingEdge" || btype == "risingEdgeContact") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::RisingEdge;
                        node->Tag = bel->Tag;
                        node->NormallyOpen = true;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "fallingEdge" || btype == "fallingEdgeContact") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::FallingEdge;
                        node->Tag = bel->Tag;
                        node->NormallyOpen = true;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "move") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::Move;
                        node->Tag = bel->Tag;
                        node->Tag2 = bel->Tag2;
                        node->NormallyOpen = true;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "add" || btype == "sub" || btype == "mul" ||
                             btype == "div" || btype == "mod") {
                        CuPartType mathType = CuPartType::ADD;
                        if (btype == "sub") mathType = CuPartType::SUB;
                        else if (btype == "mul") mathType = CuPartType::MUL;
                        else if (btype == "div") mathType = CuPartType::DIV;
                        else if (btype == "mod") mathType = CuPartType::MOD;

                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = mathType;
                        node->Tag = bel->Tag;
                        node->Tag2 = bel->Tag2;
                        node->Tag3 = bel->Tag3;
                        node->DataType = bel->DataType;
                        node->NormallyOpen = true;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "jmp") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::JMP;
                        node->Tag = bel->Tag;
                        node->NormallyOpen = true;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "label") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::LABEL;
                        node->Tag = bel->Tag;
                        node->NormallyOpen = true;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "ret") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::RET;
                        node->Tag = "";
                        node->NormallyOpen = true;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "nop") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::NOP;
                        node->Tag = "";
                        node->NormallyOpen = true;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);

                        branchPrevId = node->Id;
                    }
                    else if (btype == "contact") {
                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = CuPartType::Contact;
                        node->Tag = bel->Tag;
                        node->NormallyOpen = bel->NormallyOpen;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);
                        branchPrevId = node->Id;
                    }
                    else if (btype == "coil" || btype == "setCoil" || btype == "resetCoil") {
                        CuPartType coilType = CuPartType::Coil;
                        if (btype == "setCoil") coilType = CuPartType::SCoil;
                        else if (btype == "resetCoil") coilType = CuPartType::RCoil;

                        LgNode^ node = gcnew LgNode();
                        node->Id = nextId++;
                        node->NodeType = coilType;
                        node->Tag = bel->Tag;
                        node->NormallyOpen = true;
                        graph->Nodes->Add(node);

                        LgEdge^ edge = gcnew LgEdge();
                        edge->FromId = branchPrevId;
                        edge->ToId = node->Id;
                        graph->Edges->Add(edge);
                        branchPrevId = node->Id;
                    }
                }

                if (branchPrevId != forkId)
                    branchEndIds->Add(branchPrevId);
            }

            int nextIdx = i + 1;
            while (nextIdx < elems->Count && elems[nextIdx]->Type == "access") {
                nextIdx++;
            }

            if (nextIdx < elems->Count) {
                LgNode^ joinNode = gcnew LgNode();
                joinNode->Id = nextId++;
                joinNode->NodeType = CuPartType::Junction;
                joinNode->Tag = "";
                joinNode->NormallyOpen = true;
                joinNode->IsParallelJoin = true;
                graph->Nodes->Add(joinNode);

                for (int ji = 0; ji < branchEndIds->Count; ji++) {
                    LgEdge^ joinEdge = gcnew LgEdge();
                    joinEdge->FromId = branchEndIds[ji];
                    joinEdge->ToId = joinNode->Id;
                    graph->Edges->Add(joinEdge);
                }

                prevNodeId = joinNode->Id;
            } else {
                prevNodeId = -1;
            }
        }
        else if (el->Type == "access" && i + 1 < elems->Count) {
            LadElement^ nextEl = elems[i + 1];

            if (nextEl->Type == "contact") {
                LgNode^ node = gcnew LgNode();
                node->Id = nextId++;
                node->NodeType = CuPartType::Contact;
                node->Tag = el->Tag;
                node->NormallyOpen = nextEl->NormallyOpen;
                graph->Nodes->Add(node);

                LgEdge^ edge = gcnew LgEdge();
                edge->FromId = prevNodeId;
                edge->ToId = node->Id;
                graph->Edges->Add(edge);

                prevNodeId = node->Id;
                i++;
            }
            else if (nextEl->Type == "coil" || nextEl->Type == "setCoil" || nextEl->Type == "resetCoil") {
                CuPartType coilType = CuPartType::Coil;
                if (nextEl->Type == "setCoil") coilType = CuPartType::SCoil;
                else if (nextEl->Type == "resetCoil") coilType = CuPartType::RCoil;

                LgNode^ node = gcnew LgNode();
                node->Id = nextId++;
                node->NodeType = coilType;
                node->Tag = el->Tag;
                node->NormallyOpen = true;
                graph->Nodes->Add(node);

                LgEdge^ edge = gcnew LgEdge();
                edge->FromId = prevNodeId;
                edge->ToId = node->Id;
                graph->Edges->Add(edge);

                prevNodeId = node->Id;
                i++;
            }
        }
        else if (el->Type == "timerOnDelay") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::TON;
            node->Tag = "";
            node->NormallyOpen = true;
            node->InstanceName = el->InstanceName;
            node->PresetTime = el->PresetTime;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "timerOffDelay") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::TOF;
            node->Tag = "";
            node->NormallyOpen = true;
            node->InstanceName = el->InstanceName;
            node->PresetTime = el->PresetTime;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "counterUp") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::CTU;
            node->Tag = "";
            node->NormallyOpen = true;
            node->InstanceName = el->InstanceName;
            node->PresetTime = el->PresetTime;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "counterDown") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::CTD;
            node->Tag = "";
            node->NormallyOpen = true;
            node->InstanceName = el->InstanceName;
            node->PresetTime = el->PresetTime;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "timerPulse") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::TP;
            node->Tag = "";
            node->NormallyOpen = true;
            node->InstanceName = el->InstanceName;
            node->PresetTime = el->PresetTime;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "counterUpDown") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::CTUD;
            node->Tag = "";
            node->NormallyOpen = true;
            node->InstanceName = el->InstanceName;
            node->PresetTime = el->PresetTime;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "compareEQ" || el->Type == "compareNE" || el->Type == "compareGT" ||
                 el->Type == "compareLT" || el->Type == "compareGE" || el->Type == "compareLE") {
            CuPartType cmpType = CuPartType::CompareEQ;
            if (el->Type == "compareNE") cmpType = CuPartType::CompareNE;
            else if (el->Type == "compareGT") cmpType = CuPartType::CompareGT;
            else if (el->Type == "compareLT") cmpType = CuPartType::CompareLT;
            else if (el->Type == "compareGE") cmpType = CuPartType::CompareGE;
            else if (el->Type == "compareLE") cmpType = CuPartType::CompareLE;

            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = cmpType;
            node->Tag = el->Tag;
            node->Tag2 = el->Tag2;
            node->DataType = el->DataType;
            node->NormallyOpen = true;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "risingEdge" || el->Type == "risingEdgeContact") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::RisingEdge;
            node->Tag = el->Tag;
            node->Tag2 = el->Tag2;
            node->NormallyOpen = true;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "fallingEdge" || el->Type == "fallingEdgeContact") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::FallingEdge;
            node->Tag = el->Tag;
            node->Tag2 = el->Tag2;
            node->NormallyOpen = true;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "resetBitfield" || el->Type == "setBitfield") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = (el->Type == "resetBitfield") ? CuPartType::RBitfield : CuPartType::SBitfield;
            node->Tag = el->Tag;
            node->Tag2 = el->Tag2;
            node->NormallyOpen = true;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "move") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::Move;
            node->Tag = el->Tag;
            node->Tag2 = el->Tag2;
            node->DataType = el->DataType;
            node->NormallyOpen = true;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "add" || el->Type == "sub" || el->Type == "mul" ||
                 el->Type == "div" || el->Type == "mod") {
            CuPartType mathType = CuPartType::ADD;
            if (el->Type == "sub") mathType = CuPartType::SUB;
            else if (el->Type == "mul") mathType = CuPartType::MUL;
            else if (el->Type == "div") mathType = CuPartType::DIV;
            else if (el->Type == "mod") mathType = CuPartType::MOD;

            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = mathType;
            node->Tag = el->Tag;
            node->Tag2 = el->Tag2;
            node->Tag3 = el->Tag3;
            node->DataType = el->DataType;
            node->NormallyOpen = true;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "jmp") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::JMP;
            node->Tag = el->Tag;
            node->NormallyOpen = true;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "label") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::LABEL;
            node->Tag = el->Tag;
            node->NormallyOpen = true;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "ret") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::RET;
            node->Tag = "";
            node->NormallyOpen = true;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "nop") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::NOP;
            node->Tag = "";
            node->NormallyOpen = true;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "contact") {
            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = CuPartType::Contact;
            node->Tag = el->Tag;
            node->NormallyOpen = el->NormallyOpen;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
        else if (el->Type == "coil" || el->Type == "setCoil" || el->Type == "resetCoil") {
            CuPartType coilType = CuPartType::Coil;
            if (el->Type == "setCoil") coilType = CuPartType::SCoil;
            else if (el->Type == "resetCoil") coilType = CuPartType::RCoil;

            LgNode^ node = gcnew LgNode();
            node->Id = nextId++;
            node->NodeType = coilType;
            node->Tag = el->Tag;
            node->NormallyOpen = true;
            graph->Nodes->Add(node);

            LgEdge^ edge = gcnew LgEdge();
            edge->FromId = prevNodeId;
            edge->ToId = node->Id;
            graph->Edges->Add(edge);

            prevNodeId = node->Id;
        }
    }

    Console::WriteLine("      GRAPH Nodes: {0}", graph->Nodes->Count);
    for (int ni = 0; ni < graph->Nodes->Count; ni++) {
        Console::WriteLine("        Node[{0}] id={1} type={2} tag={3}", ni, graph->Nodes[ni]->Id, (int)graph->Nodes[ni]->NodeType, graph->Nodes[ni]->Tag);
    }

    return graph;
}

inline ControlFlowGraph^ BuildCfgFromLogicGraph(LogicGraph^ graph) {
    ControlFlowGraph^ cfg = gcnew ControlFlowGraph();
    cfg->Nodes = gcnew List<CfgNode^>();
    cfg->Adjacency = gcnew Dictionary<int, List<int>^>();

    if (graph->Nodes->Count == 0) {
        cfg->EntryId = -1;
        cfg->ExitId = -1;
        return cfg;
    }

    int maxId = 0;
    for (int i = 0; i < graph->Nodes->Count; i++) {
        if (graph->Nodes[i]->Id > maxId) maxId = graph->Nodes[i]->Id;
    }

    int entryId = maxId + 1;
    int exitId = maxId + 2;
    cfg->EntryId = entryId;
    cfg->ExitId = exitId;

    CfgNode^ entryNode = gcnew CfgNode();
    entryNode->Id = entryId;
    entryNode->Type = CfgNodeType::Entry;
    entryNode->Successors = gcnew List<int>();
    entryNode->Predecessors = gcnew List<int>();
    entryNode->DominanceDepth = 0;
    cfg->Nodes->Add(entryNode);
    cfg->Adjacency[entryId] = gcnew List<int>();

    CfgNode^ exitNode = gcnew CfgNode();
    exitNode->Id = exitId;
    exitNode->Type = CfgNodeType::Exit;
    exitNode->Successors = gcnew List<int>();
    exitNode->Predecessors = gcnew List<int>();
    exitNode->DominanceDepth = 0;
    cfg->Nodes->Add(exitNode);
    cfg->Adjacency[exitId] = gcnew List<int>();

    Dictionary<int, int>^ lgIdToCfgIdx = gcnew Dictionary<int, int>();

    for (int i = 0; i < graph->Nodes->Count; i++) {
        LgNode^ n = graph->Nodes[i];
        CfgNode^ cn = gcnew CfgNode();
        cn->Id = n->Id;
        cn->PartType = n->NodeType;
        cn->Tag = n->Tag;
        cn->Tag2 = n->Tag2;
        cn->Tag3 = n->Tag3;
        cn->DataType = n->DataType;
        cn->NormallyOpen = n->NormallyOpen;
        cn->InstanceName = n->InstanceName;
        cn->PresetTime = n->PresetTime;
        cn->Successors = gcnew List<int>();
        cn->Predecessors = gcnew List<int>();
        cn->DominanceDepth = 0;

        if (n->NodeType == CuPartType::Junction) {
            int inCount = 0;
            for (int ei = 0; ei < graph->Edges->Count; ei++) {
                if (graph->Edges[ei]->ToId == n->Id && graph->Edges[ei]->FromId >= 0) inCount++;
            }
            int outCount = 0;
            for (int ei = 0; ei < graph->Edges->Count; ei++) {
                if (graph->Edges[ei]->FromId == n->Id) outCount++;
            }

            if (n->IsParallelJoin && inCount >= 2) {
                cn->Type = CfgNodeType::ParallelJoin;
            }
            else if (inCount >= 2) {
                cn->Type = CfgNodeType::OrJoin;
            }
            else if (outCount >= 2) {
                cn->Type = CfgNodeType::ParallelFork;
            }
            else {
                cn->Type = CfgNodeType::Serial;
            }
        }
        else {
            cn->Type = CfgNodeType::Instruction;
        }

        cfg->Nodes->Add(cn);
        lgIdToCfgIdx[n->Id] = cfg->Nodes->Count - 1;
        cfg->Adjacency[n->Id] = gcnew List<int>();
    }

    for (int i = 0; i < graph->Edges->Count; i++) {
        LgEdge^ e = graph->Edges[i];
        if (e->FromId < 0) {
            cfg->Adjacency[entryId]->Add(e->ToId);
            entryNode->Successors->Add(e->ToId);
            if (lgIdToCfgIdx->ContainsKey(e->ToId)) {
                cfg->Nodes[lgIdToCfgIdx[e->ToId]]->Predecessors->Add(entryId);
            }
        }
        else {
            cfg->Adjacency[e->FromId]->Add(e->ToId);
            if (lgIdToCfgIdx->ContainsKey(e->FromId)) {
                cfg->Nodes[lgIdToCfgIdx[e->FromId]]->Successors->Add(e->ToId);
            }
            if (lgIdToCfgIdx->ContainsKey(e->ToId)) {
                cfg->Nodes[lgIdToCfgIdx[e->ToId]]->Predecessors->Add(e->FromId);
            }
        }
    }

    for (int i = 0; i < cfg->Nodes->Count; i++) {
        CfgNode^ cn = cfg->Nodes[i];
        if (cn->Type == CfgNodeType::Instruction || cn->Type == CfgNodeType::Serial) {
            if (cn->Successors->Count == 0 && cn->Id != exitId) {
                cn->Successors->Add(exitId);
                cfg->Adjacency[cn->Id]->Add(exitId);
                exitNode->Predecessors->Add(cn->Id);
            }
        }
    }

    Dictionary<int, bool>^ visited = gcnew Dictionary<int, bool>();
    List<int>^ queue = gcnew List<int>();
    queue->Add(entryId);
    visited[entryId] = true;

    while (queue->Count > 0) {
        int cur = queue[0];
        queue->RemoveAt(0);

        int curDepth = 0;
        if (cfg->Adjacency->ContainsKey(cur)) {
            for (int pi = 0; pi < cfg->Nodes->Count; pi++) {
                CfgNode^ n = cfg->Nodes[pi];
                if (n->Id == cur) {
                    if (n->Predecessors->Count > 0) {
                        int maxPredDepth = 0;
                        for (int j = 0; j < n->Predecessors->Count; j++) {
                            int predId = n->Predecessors[j];
                            for (int k = 0; k < cfg->Nodes->Count; k++) {
                                if (cfg->Nodes[k]->Id == predId && cfg->Nodes[k]->DominanceDepth > maxPredDepth) {
                                    maxPredDepth = cfg->Nodes[k]->DominanceDepth;
                                }
                            }
                        }
                        curDepth = maxPredDepth + 1;
                    }
                    n->DominanceDepth = curDepth;
                    break;
                }
            }
        }

        if (cfg->Adjacency->ContainsKey(cur)) {
            for (int si = 0; si < cfg->Adjacency[cur]->Count; si++) {
                int succ = cfg->Adjacency[cur][si];
                if (!visited->ContainsKey(succ)) {
                    visited[succ] = true;
                    queue->Add(succ);
                }
            }
        }
    }

    return cfg;
}

inline List<OptimizationResult^>^ OptimizeCfg(ControlFlowGraph^ cfg) {
    List<OptimizationResult^>^ log = gcnew List<OptimizationResult^>();

    bool changed = true;
    int pass = 0;
    while (changed && pass < 10) {
        changed = false;
        pass++;

        List<int>^ toRemove = gcnew List<int>();
        for (int i = 0; i < cfg->Nodes->Count; i++) {
            CfgNode^ n = cfg->Nodes[i];
            if (n->Type == CfgNodeType::Serial && n->Successors->Count == 1 && n->Predecessors->Count == 1) {
                int predId = n->Predecessors[0];
                int succId = n->Successors[0];

                bool predIsSerial = false;
                for (int j = 0; j < cfg->Nodes->Count; j++) {
                    if (cfg->Nodes[j]->Id == predId && cfg->Nodes[j]->Type == CfgNodeType::Serial) {
                        predIsSerial = true;
                        break;
                    }
                }

                if (predIsSerial) {
                    for (int j = 0; j < cfg->Nodes->Count; j++) {
                        if (cfg->Nodes[j]->Id == predId) {
                            cfg->Nodes[j]->Successors->Remove(n->Id);
                            if (!cfg->Nodes[j]->Successors->Contains(succId)) {
                                cfg->Nodes[j]->Successors->Add(succId);
                            }
                            break;
                        }
                    }
                    for (int j = 0; j < cfg->Nodes->Count; j++) {
                        if (cfg->Nodes[j]->Id == succId) {
                            cfg->Nodes[j]->Predecessors->Remove(n->Id);
                            if (!cfg->Nodes[j]->Predecessors->Contains(predId)) {
                                cfg->Nodes[j]->Predecessors->Add(predId);
                            }
                            break;
                        }
                    }

                    if (cfg->Adjacency->ContainsKey(predId)) {
                        cfg->Adjacency[predId]->Remove(n->Id);
                        if (!cfg->Adjacency[predId]->Contains(succId)) {
                            cfg->Adjacency[predId]->Add(succId);
                        }
                    }

                    toRemove->Add(n->Id);

                    OptimizationResult^ r = gcnew OptimizationResult();
                    r->Changed = true;
                    r->Description = "Pass " + pass.ToString() + ": eliminated redundant serial junction " + n->Id.ToString();
                    log->Add(r);
                    changed = true;
                }
            }
        }

        for (int ri = toRemove->Count - 1; ri >= 0; ri--) {
            for (int ni = cfg->Nodes->Count - 1; ni >= 0; ni--) {
                if (cfg->Nodes[ni]->Id == toRemove[ri]) {
                    cfg->Nodes->RemoveAt(ni);
                    break;
                }
            }
            if (cfg->Adjacency->ContainsKey(toRemove[ri])) {
                cfg->Adjacency->Remove(toRemove[ri]);
            }
        }
    }

    if (log->Count == 0) {
        OptimizationResult^ r = gcnew OptimizationResult();
        r->Changed = false;
        r->Description = "No optimizations applicable";
        log->Add(r);
    }

    return log;
}

inline CompileUnitIR^ BuildIrFromCfg(ControlFlowGraph^ cfg, UidAllocator^ uidAlloc) {
    CompileUnitIR^ ir = gcnew CompileUnitIR();
    ir->Parts = gcnew List<CuPart^>();
    ir->Accesses = gcnew List<CuAccess^>();
    ir->Wires = gcnew List<CuWire^>();

    PartSchemaRegistry^ reg = gcnew PartSchemaRegistry();

    auto inferConstType = [](String^ val) -> String^ {
        if (val == nullptr || val->Length == 0) return "DInt";
        if (val->StartsWith("T#")) return "Time";
        if (val->StartsWith("16#")) return "DInt";
        if (val->Contains(".") && !val->StartsWith("DB")) return "Real";
        return "DInt";
    };

    Dictionary<int, int>^ cfgIdToPartUid = gcnew Dictionary<int, int>();
    Dictionary<int, int>^ jToOrUid = gcnew Dictionary<int, int>();
    Dictionary<int, int>^ orInIdx = gcnew Dictionary<int, int>();
    Dictionary<int, int>^ nodeExtraAccessUid = gcnew Dictionary<int, int>();
    String^ prevMathOutputVar = nullptr;

    for (int i = 0; i < cfg->Nodes->Count; i++) {
        CfgNode^ cn = cfg->Nodes[i];
        if (cn->Type != CfgNodeType::Instruction) continue;

        CuPart^ part = gcnew CuPart();
        part->Uid = uidAlloc->Alloc();
        part->PartType = cn->PartType;
        part->Type = reg->TypeToName(cn->PartType);
        part->Cardinality = 0;
        part->NormallyClosed = false;
        part->InstanceName = "";
        part->TimerVersion = "";

        PartSchema^ schema = reg->GetSchema(cn->PartType);

        if (cn->PartType == CuPartType::Contact) {
            part->NormallyClosed = !cn->NormallyOpen;
        }
        else if (schema != nullptr && schema->RequiresInstanceDB) {
            part->TimerVersion = "1.0";
            part->InstanceName = cn->InstanceName;
            part->InstanceUid = uidAlloc->Alloc();
        }

        if (cn->DataType != nullptr && cn->DataType->Length > 0) {
            part->DataType = cn->DataType;
        }

        part->Pins = reg->ExpandPins(part->PartType, part->Cardinality);

        ir->Parts->Add(part);
        cfgIdToPartUid[cn->Id] = part->Uid;

        if (schema != nullptr && schema->RequiresOperand && !schema->RequiresInstanceDB) {
            CuAccess^ access = gcnew CuAccess();
            access->Uid = uidAlloc->Alloc();
            access->Symbol = cn->Tag;
            access->TargetPartUid = part->Uid;
            access->Scope = "GlobalVariable";
            access->ConstantValue = "";
            ir->Accesses->Add(access);
        }

        if (cn->PartType == CuPartType::CompareEQ || cn->PartType == CuPartType::CompareNE ||
            cn->PartType == CuPartType::CompareGT || cn->PartType == CuPartType::CompareLT ||
            cn->PartType == CuPartType::CompareGE || cn->PartType == CuPartType::CompareLE) {
            CuAccess^ in1Access = gcnew CuAccess();
            in1Access->Uid = uidAlloc->Alloc();
            in1Access->Symbol = (cn->Tag != nullptr && cn->Tag->Length > 0) ? cn->Tag : "0";
            in1Access->TargetPartUid = part->Uid;
            bool isConst1 = (cn->Tag != nullptr && cn->Tag->Length > 0 &&
                (Char::IsDigit(cn->Tag[0]) || cn->Tag->StartsWith("-") || cn->Tag->StartsWith("T#") || cn->Tag->StartsWith("16#")));
            in1Access->Scope = isConst1 ? "TypedConstant" : "GlobalVariable";
            in1Access->ConstantValue = isConst1 ? cn->Tag : "";
            in1Access->ConstantType = isConst1 ? inferConstType(cn->Tag) : "";
            ir->Accesses->Add(in1Access);

            CuAccess^ in2Access = gcnew CuAccess();
            in2Access->Uid = uidAlloc->Alloc();
            in2Access->Symbol = (cn->Tag2 != nullptr && cn->Tag2->Length > 0) ? cn->Tag2 : "0";
            in2Access->TargetPartUid = part->Uid;
            bool isConst2 = (cn->Tag2 != nullptr && cn->Tag2->Length > 0 &&
                (Char::IsDigit(cn->Tag2[0]) || cn->Tag2->StartsWith("-") || cn->Tag2->StartsWith("T#") || cn->Tag2->StartsWith("16#")));
            in2Access->Scope = isConst2 ? "TypedConstant" : "GlobalVariable";
            in2Access->ConstantValue = isConst2 ? cn->Tag2 : "";
            in2Access->ConstantType = isConst2 ? inferConstType(cn->Tag2) : "";
            ir->Accesses->Add(in2Access);
        }

        if (cn->PartType == CuPartType::RisingEdge || cn->PartType == CuPartType::FallingEdge) {
            CuAccess^ access = gcnew CuAccess();
            access->Uid = uidAlloc->Alloc();
            access->Symbol = (cn->Tag != nullptr && cn->Tag->Length > 0) ? cn->Tag : "M0.0";
            access->TargetPartUid = part->Uid;
            access->Scope = "GlobalVariable";
            access->ConstantValue = "";
            ir->Accesses->Add(access);

            CuAccess^ bitAccess = gcnew CuAccess();
            bitAccess->Uid = uidAlloc->Alloc();
            bitAccess->Symbol = (cn->Tag2 != nullptr && cn->Tag2->Length > 0) ? cn->Tag2 : "M0.1";
            bitAccess->TargetPartUid = part->Uid;
            bitAccess->Scope = "GlobalVariable";
            bitAccess->ConstantValue = "";
            ir->Accesses->Add(bitAccess);
        }

        if (cn->PartType == CuPartType::RBitfield || cn->PartType == CuPartType::SBitfield) {
            CuAccess^ nAccess = gcnew CuAccess();
            nAccess->Uid = uidAlloc->Alloc();
            String^ nVal = (cn->Tag2 != nullptr && cn->Tag2->Length > 0) ? cn->Tag2 : "1";
            bool isConstN = (nVal->Length > 0 && (Char::IsDigit(nVal[0]) || nVal->StartsWith("-") || nVal->StartsWith("16#")));
            if (isConstN) {
                nAccess->Symbol = nVal;
                nAccess->Scope = "LiteralConstant";
                nAccess->ConstantValue = nVal;
                nAccess->ConstantType = "UInt";
            }
            else {
                nAccess->Symbol = nVal;
                nAccess->Scope = "GlobalVariable";
                nAccess->ConstantValue = "";
            }
            nAccess->TargetPartUid = part->Uid;
            ir->Accesses->Add(nAccess);
        }

        if (cn->PartType == CuPartType::CTU || cn->PartType == CuPartType::CTD || cn->PartType == CuPartType::CTUD) {
            CuAccess^ pvAccess = gcnew CuAccess();
            pvAccess->Uid = uidAlloc->Alloc();
            String^ pvVal = (cn->PresetTime != nullptr && cn->PresetTime->Length > 0) ? cn->PresetTime : "1";
            bool isConstPV = (pvVal->Length > 0 && (Char::IsDigit(pvVal[0]) || pvVal->StartsWith("-") || pvVal->StartsWith("16#")));
            if (isConstPV) {
                pvAccess->Symbol = pvVal;
                pvAccess->Scope = "LiteralConstant";
                pvAccess->ConstantValue = pvVal;
                pvAccess->ConstantType = "Int";
            }
            else {
                pvAccess->Symbol = pvVal;
                pvAccess->Scope = "GlobalVariable";
                pvAccess->ConstantValue = "";
                pvAccess->ConstantType = "";
            }
            pvAccess->TargetPartUid = part->Uid;
            ir->Accesses->Add(pvAccess);
        }

        if (cn->PartType == CuPartType::Move) {
            String^ moveInSymbol = cn->Tag;
            if (moveInSymbol == nullptr || moveInSymbol->Length == 0) {
                moveInSymbol = prevMathOutputVar;
            }
            if (moveInSymbol == nullptr || moveInSymbol->Length == 0) {
                moveInSymbol = "0";
            }

            CuAccess^ inAccess = gcnew CuAccess();
            inAccess->Uid = uidAlloc->Alloc();
            inAccess->Symbol = moveInSymbol;
            inAccess->TargetPartUid = part->Uid;
            bool isConstIn = (moveInSymbol->Length > 0 &&
                (Char::IsDigit(moveInSymbol[0]) || moveInSymbol->StartsWith("-") || moveInSymbol->StartsWith("16#")));
            inAccess->Scope = isConstIn ? "TypedConstant" : "GlobalVariable";
            inAccess->ConstantValue = isConstIn ? moveInSymbol : "";
            inAccess->ConstantType = isConstIn ? inferConstType(moveInSymbol) : "";
            ir->Accesses->Add(inAccess);

            CuAccess^ outAccess = gcnew CuAccess();
            outAccess->Uid = uidAlloc->Alloc();
            outAccess->Symbol = (cn->Tag2 != nullptr && cn->Tag2->Length > 0) ? cn->Tag2 : "MW0";
            outAccess->TargetPartUid = part->Uid;
            outAccess->Scope = "GlobalVariable";
            outAccess->ConstantValue = "";
            ir->Accesses->Add(outAccess);
        }

        if (cn->PartType == CuPartType::ADD || cn->PartType == CuPartType::SUB ||
            cn->PartType == CuPartType::MUL || cn->PartType == CuPartType::DIV ||
            cn->PartType == CuPartType::MOD) {
            CuAccess^ in1Access = gcnew CuAccess();
            in1Access->Uid = uidAlloc->Alloc();
            in1Access->Symbol = (cn->Tag != nullptr && cn->Tag->Length > 0) ? cn->Tag : "0";
            in1Access->TargetPartUid = part->Uid;
            bool isConst1 = (cn->Tag != nullptr && cn->Tag->Length > 0 &&
                (Char::IsDigit(cn->Tag[0]) || cn->Tag->StartsWith("-") || cn->Tag->StartsWith("16#")));
            in1Access->Scope = isConst1 ? "TypedConstant" : "GlobalVariable";
            in1Access->ConstantValue = isConst1 ? cn->Tag : "";
            in1Access->ConstantType = isConst1 ? inferConstType(cn->Tag) : "";
            ir->Accesses->Add(in1Access);

            CuAccess^ in2Access = gcnew CuAccess();
            in2Access->Uid = uidAlloc->Alloc();
            in2Access->Symbol = (cn->Tag2 != nullptr && cn->Tag2->Length > 0) ? cn->Tag2 : "0";
            in2Access->TargetPartUid = part->Uid;
            bool isConst2 = (cn->Tag2 != nullptr && cn->Tag2->Length > 0 &&
                (Char::IsDigit(cn->Tag2[0]) || cn->Tag2->StartsWith("-") || cn->Tag2->StartsWith("16#")));
            in2Access->Scope = isConst2 ? "TypedConstant" : "GlobalVariable";
            in2Access->ConstantValue = isConst2 ? cn->Tag2 : "";
            in2Access->ConstantType = isConst2 ? inferConstType(cn->Tag2) : "";
            ir->Accesses->Add(in2Access);

            CuAccess^ outAccess = gcnew CuAccess();
            outAccess->Uid = uidAlloc->Alloc();
            outAccess->Symbol = (cn->Tag3 != nullptr && cn->Tag3->Length > 0) ? cn->Tag3 : "MW0";
            outAccess->TargetPartUid = part->Uid;
            outAccess->Scope = "GlobalVariable";
            outAccess->ConstantValue = "";
            ir->Accesses->Add(outAccess);

            prevMathOutputVar = (cn->Tag3 != nullptr && cn->Tag3->Length > 0) ? cn->Tag3 : "MW0";
        }

        if (cn->PartType == CuPartType::JMP || cn->PartType == CuPartType::LABEL) {
            CuAccess^ access = gcnew CuAccess();
            access->Uid = uidAlloc->Alloc();
            access->Symbol = (cn->Tag != nullptr && cn->Tag->Length > 0) ? cn->Tag : "M0.0";
            access->TargetPartUid = part->Uid;
            access->Scope = "Label";
            access->ConstantValue = "";
            ir->Accesses->Add(access);
        }

        if (schema != nullptr && schema->RequiresInstanceDB) {
            if (cn->PresetTime != nullptr && cn->PresetTime->Length > 0) {
                CuAccess^ ptAccess = gcnew CuAccess();
                ptAccess->Uid = uidAlloc->Alloc();
                ptAccess->Symbol = cn->PresetTime;
                ptAccess->TargetPartUid = part->Uid;
                ptAccess->Scope = "TypedConstant";
                ptAccess->ConstantValue = cn->PresetTime;
                ptAccess->ConstantType = "Time";
                ir->Accesses->Add(ptAccess);
                nodeExtraAccessUid[cn->Id] = ptAccess->Uid;
            }
        }
    }

    for (int i = 0; i < cfg->Nodes->Count; i++) {
        CfgNode^ cn = cfg->Nodes[i];
        if (cn->Type != CfgNodeType::OrJoin) continue;

        int realIn = cn->Predecessors->Count;
        if (realIn >= 2) {
            CuPart^ orPart = gcnew CuPart();
            orPart->Uid = uidAlloc->Alloc();
            orPart->PartType = CuPartType::O;
            orPart->Type = "O";
            orPart->NormallyClosed = false;
            orPart->Cardinality = realIn;
            orPart->InstanceName = "";
            orPart->TimerVersion = "";
            orPart->Pins = reg->ExpandPins(CuPartType::O, realIn);
            ir->Parts->Add(orPart);
            jToOrUid[cn->Id] = orPart->Uid;
            orInIdx[orPart->Uid] = 1;
        }
    }

    for (int i = 0; i < cfg->Nodes->Count; i++) {
        CfgNode^ cn = cfg->Nodes[i];
        if (cn->Type != CfgNodeType::OrJoin) continue;
        if (!jToOrUid->ContainsKey(cn->Id)) continue;

        int orUid = jToOrUid[cn->Id];

        int orPos = -1;
        for (int pi = 0; pi < ir->Parts->Count; pi++) {
            if (ir->Parts[pi]->Uid == orUid) { orPos = pi; break; }
        }
        if (orPos < 0) continue;

        int minDsPos = ir->Parts->Count;
        for (int si = 0; si < cn->Successors->Count; si++) {
            int succId = cn->Successors[si];
            if (!cfgIdToPartUid->ContainsKey(succId)) continue;
            int dsUid = cfgIdToPartUid[succId];
            for (int pi = 0; pi < ir->Parts->Count; pi++) {
                if (ir->Parts[pi]->Uid == dsUid) {
                    if (pi < minDsPos) minDsPos = pi;
                    break;
                }
            }
        }

        if (minDsPos <= orPos) {
            CuPart^ orPart = ir->Parts[orPos];
            ir->Parts->RemoveAt(orPos);
            int insPos = minDsPos;
            for (int pi = 0; pi < ir->Parts->Count; pi++) {
                if (cfgIdToPartUid->ContainsKey(cn->Successors[0]) && ir->Parts[pi]->Uid == cfgIdToPartUid[cn->Successors[0]]) {
                    insPos = pi;
                    break;
                }
            }
            ir->Parts->Insert(insPos, orPart);
        }
    }

    HashSet<int>^ forkProcessed = gcnew HashSet<int>();

    for (int i = 0; i < cfg->Nodes->Count; i++) {
        CfgNode^ cn = cfg->Nodes[i];

        if (cn->Type == CfgNodeType::Entry) {
            for (int si = 0; si < cn->Successors->Count; si++) {
                int succId = cn->Successors[si];
                CfgNode^ succNode = nullptr;
                for (int j = 0; j < cfg->Nodes->Count; j++) {
                    if (cfg->Nodes[j]->Id == succId) { succNode = cfg->Nodes[j]; break; }
                }
                if (succNode == nullptr) continue;

                if (succNode->Type == CfgNodeType::ParallelFork && succNode->Successors->Count >= 2) {
                    if (forkProcessed->Contains(succId)) continue;
                    forkProcessed->Add(succId);

                    CuWire^ mtWire = gcnew CuWire();
                    mtWire->Uid = uidAlloc->Alloc();
                    mtWire->FromKind = "PowerRail";
                    mtWire->FromUid = -1;
                    mtWire->FromPin = "";
                    mtWire->ExtraTargets = gcnew List<CuWireTarget^>();

                    bool first = true;
                    for (int oi = 0; oi < succNode->Successors->Count; oi++) {
                        int targetId = succNode->Successors[oi];
                        if (!cfgIdToPartUid->ContainsKey(targetId)) continue;
                        CfgNode^ tgt = nullptr;
                        for (int j = 0; j < cfg->Nodes->Count; j++) {
                            if (cfg->Nodes[j]->Id == targetId) { tgt = cfg->Nodes[j]; break; }
                        }
                        CuPartType tgtType = (tgt != nullptr) ? tgt->PartType : CuPartType::Contact;
                        String^ pin = reg->GetFirstInputPin(tgtType);
                        if (first) {
                            mtWire->ToKind = "Part";
                            mtWire->ToUid = cfgIdToPartUid[targetId];
                            mtWire->ToPin = pin;
                            first = false;
                        }
                        else {
                            CuWireTarget^ t = gcnew CuWireTarget();
                            t->Uid = cfgIdToPartUid[targetId];
                            t->Pin = pin;
                            mtWire->ExtraTargets->Add(t);
                        }
                    }
                    ir->Wires->Add(mtWire);
                }
                else {
                    if (!cfgIdToPartUid->ContainsKey(succId)) continue;
                    CuPartType succType = (succNode != nullptr) ? succNode->PartType : CuPartType::Contact;
                    String^ pin = reg->GetFirstInputPin(succType);

                    CuWire^ prWire = gcnew CuWire();
                    prWire->Uid = uidAlloc->Alloc();
                    prWire->FromKind = "PowerRail";
                    prWire->FromUid = -1;
                    prWire->FromPin = "";
                    prWire->ToKind = "Part";
                    prWire->ToUid = cfgIdToPartUid[succId];
                    prWire->ToPin = pin;
                    ir->Wires->Add(prWire);
                }
            }
            continue;
        }

        if (cn->Type == CfgNodeType::Instruction) {
            if (!cfgIdToPartUid->ContainsKey(cn->Id)) continue;

            for (int si = 0; si < cn->Successors->Count; si++) {
                int succId = cn->Successors[si];
                CfgNode^ succNode = nullptr;
                for (int j = 0; j < cfg->Nodes->Count; j++) {
                    if (cfg->Nodes[j]->Id == succId) { succNode = cfg->Nodes[j]; break; }
                }
                if (succNode == nullptr) continue;

                if (succNode->Type == CfgNodeType::ParallelFork && succNode->Successors->Count >= 2) {
                    if (forkProcessed->Contains(succId)) continue;
                    forkProcessed->Add(succId);

                    String^ fromPin = reg->GetFirstOutputPin(cn->PartType);

                    CuWire^ mtWire = gcnew CuWire();
                    mtWire->Uid = uidAlloc->Alloc();
                    mtWire->FromKind = "Part";
                    mtWire->FromUid = cfgIdToPartUid[cn->Id];
                    mtWire->FromPin = fromPin;
                    mtWire->ExtraTargets = gcnew List<CuWireTarget^>();

                    bool first = true;
                    for (int oi = 0; oi < succNode->Successors->Count; oi++) {
                        int targetId = succNode->Successors[oi];
                        if (!cfgIdToPartUid->ContainsKey(targetId)) continue;
                        CfgNode^ tgt = nullptr;
                        for (int j = 0; j < cfg->Nodes->Count; j++) {
                            if (cfg->Nodes[j]->Id == targetId) { tgt = cfg->Nodes[j]; break; }
                        }
                        CuPartType tgtType = (tgt != nullptr) ? tgt->PartType : CuPartType::Contact;
                        String^ toPin = reg->GetFirstInputPin(tgtType);
                        if (first) {
                            mtWire->ToKind = "Part";
                            mtWire->ToUid = cfgIdToPartUid[targetId];
                            mtWire->ToPin = toPin;
                            first = false;
                        }
                        else {
                            CuWireTarget^ t = gcnew CuWireTarget();
                            t->Uid = cfgIdToPartUid[targetId];
                            t->Pin = toPin;
                            mtWire->ExtraTargets->Add(t);
                        }
                    }
                    ir->Wires->Add(mtWire);
                    continue;
                }

                if (succNode->Type == CfgNodeType::OrJoin && jToOrUid->ContainsKey(succId)) {
                    int orUid = jToOrUid[succId];
                    int idx = orInIdx[orUid];

                    String^ fromPin = reg->GetFirstOutputPin(cn->PartType);

                    CuWire^ sigWire = gcnew CuWire();
                    sigWire->Uid = uidAlloc->Alloc();
                    sigWire->FromKind = "Part";
                    sigWire->FromUid = cfgIdToPartUid[cn->Id];
                    sigWire->FromPin = fromPin;
                    sigWire->ToKind = "Part";
                    sigWire->ToUid = orUid;
                    sigWire->ToPin = "in" + idx.ToString();
                    orInIdx[orUid] = idx + 1;
                    ir->Wires->Add(sigWire);
                    continue;
                }

                if (succNode->Type == CfgNodeType::Instruction || succNode->Type == CfgNodeType::Exit) {
                    if (succNode->Type == CfgNodeType::Exit) continue;
                    if (!cfgIdToPartUid->ContainsKey(succId)) continue;

                    String^ fromPin = reg->GetFirstOutputPin(cn->PartType);
                    String^ toPin = reg->GetFirstInputPin(succNode->PartType);

                    CuWire^ sigWire = gcnew CuWire();
                    sigWire->Uid = uidAlloc->Alloc();
                    sigWire->FromKind = "Part";
                    sigWire->FromUid = cfgIdToPartUid[cn->Id];
                    sigWire->FromPin = fromPin;
                    sigWire->ToKind = "Part";
                    sigWire->ToUid = cfgIdToPartUid[succId];
                    sigWire->ToPin = toPin;
                    ir->Wires->Add(sigWire);
                    continue;
                }
            }
        }

        if (cn->Type == CfgNodeType::OrJoin && jToOrUid->ContainsKey(cn->Id)) {
            for (int si = 0; si < cn->Successors->Count; si++) {
                int succId = cn->Successors[si];
                CfgNode^ succNode = nullptr;
                for (int j = 0; j < cfg->Nodes->Count; j++) {
                    if (cfg->Nodes[j]->Id == succId) { succNode = cfg->Nodes[j]; break; }
                }
                if (succNode == nullptr || succNode->Type == CfgNodeType::Exit) continue;
                if (!cfgIdToPartUid->ContainsKey(succId)) continue;

                String^ toPin = reg->GetFirstInputPin(succNode->PartType);

                CuWire^ sigWire = gcnew CuWire();
                sigWire->Uid = uidAlloc->Alloc();
                sigWire->FromKind = "Part";
                sigWire->FromUid = jToOrUid[cn->Id];
                sigWire->FromPin = "out";
                sigWire->ToKind = "Part";
                sigWire->ToUid = cfgIdToPartUid[succId];
                sigWire->ToPin = toPin;
                ir->Wires->Add(sigWire);
            }
        }

        if (cn->Type == CfgNodeType::ParallelJoin && cn->Successors->Count > 0) {
            for (int si = 0; si < cn->Successors->Count; si++) {
                int succId = cn->Successors[si];
                CfgNode^ succNode = nullptr;
                for (int j = 0; j < cfg->Nodes->Count; j++) {
                    if (cfg->Nodes[j]->Id == succId) { succNode = cfg->Nodes[j]; break; }
                }
                if (succNode == nullptr || succNode->Type == CfgNodeType::Exit) continue;
                if (!cfgIdToPartUid->ContainsKey(succId)) continue;

                String^ toPin = reg->GetFirstInputPin(succNode->PartType);

                CuWire^ joinWire = gcnew CuWire();
                joinWire->Uid = uidAlloc->Alloc();
                joinWire->FromKind = "Part";
                joinWire->FromPin = "out";
                joinWire->ExtraTargets = gcnew List<CuWireTarget^>();

                bool firstPred = true;
                for (int pi = 0; pi < cn->Predecessors->Count; pi++) {
                    int predId = cn->Predecessors[pi];
                    if (!cfgIdToPartUid->ContainsKey(predId)) continue;

                    if (firstPred) {
                        joinWire->FromUid = cfgIdToPartUid[predId];
                        firstPred = false;
                    }
                    else {
                        CuWireTarget^ extraFrom = gcnew CuWireTarget();
                        extraFrom->Uid = cfgIdToPartUid[predId];
                        extraFrom->Pin = "out";
                        joinWire->ExtraTargets->Add(extraFrom);
                    }
                }

                joinWire->ToKind = "Part";
                joinWire->ToUid = cfgIdToPartUid[succId];
                joinWire->ToPin = toPin;

                if (joinWire->FromUid > 0) {
                    ir->Wires->Add(joinWire);
                }
            }
        }
    }

    for (int i = 0; i < cfg->Nodes->Count; i++) {
        CfgNode^ cn = cfg->Nodes[i];
        if (cn->Type != CfgNodeType::Instruction) continue;
        if (!cfgIdToPartUid->ContainsKey(cn->Id)) continue;

        int pUid = cfgIdToPartUid[cn->Id];
        PartSchema^ schema = reg->GetSchema(cn->PartType);

        String^ operandPin = reg->GetOperandPin(cn->PartType);
        if (operandPin->Length > 0) {
            int opUid = -1;
            for (int ai = 0; ai < ir->Accesses->Count; ai++) {
                if (ir->Accesses[ai]->TargetPartUid == pUid && (ir->Accesses[ai]->Scope == "GlobalVariable" || ir->Accesses[ai]->Scope == "Label")) {
                    opUid = ir->Accesses[ai]->Uid;
                    break;
                }
            }
            if (opUid >= 0) {
                CuWire^ opWire = gcnew CuWire();
                opWire->Uid = uidAlloc->Alloc();
                opWire->FromKind = "Access";
                opWire->FromUid = opUid;
                opWire->FromPin = "";
                opWire->ToKind = "Part";
                opWire->ToUid = pUid;
                opWire->ToPin = operandPin;
                ir->Wires->Add(opWire);
            }
        }

        if (cn->PartType == CuPartType::CTU || cn->PartType == CuPartType::CTD || cn->PartType == CuPartType::CTUD) {
            List<int>^ ctuAccessUids = gcnew List<int>();
            for (int ai = 0; ai < ir->Accesses->Count; ai++) {
                if (ir->Accesses[ai]->TargetPartUid == pUid) {
                    ctuAccessUids->Add(ir->Accesses[ai]->Uid);
                }
            }
            String^ rPinName = "R";
            String^ pvPinName = "PV";
            if (cn->PartType == CuPartType::CTD) { rPinName = "LD"; }

            CuWire^ rWire = gcnew CuWire();
            rWire->Uid = uidAlloc->Alloc();
            rWire->FromKind = "OpenCon";
            rWire->FromUid = uidAlloc->Alloc();
            rWire->FromPin = "";
            rWire->ToKind = "Part";
            rWire->ToUid = pUid;
            rWire->ToPin = rPinName;
            ir->Wires->Add(rWire);

            if (cn->PartType == CuPartType::CTUD) {
                CuWire^ cdWire = gcnew CuWire();
                cdWire->Uid = uidAlloc->Alloc();
                cdWire->FromKind = "OpenCon";
                cdWire->FromUid = uidAlloc->Alloc();
                cdWire->FromPin = "";
                cdWire->ToKind = "Part";
                cdWire->ToUid = pUid;
                cdWire->ToPin = "CD";
                ir->Wires->Add(cdWire);

                CuWire^ ldWire = gcnew CuWire();
                ldWire->Uid = uidAlloc->Alloc();
                ldWire->FromKind = "OpenCon";
                ldWire->FromUid = uidAlloc->Alloc();
                ldWire->FromPin = "";
                ldWire->ToKind = "Part";
                ldWire->ToUid = pUid;
                ldWire->ToPin = "LD";
                ir->Wires->Add(ldWire);
            }

            if (ctuAccessUids->Count >= 1) {
                CuWire^ pvWire = gcnew CuWire();
                pvWire->Uid = uidAlloc->Alloc();
                pvWire->FromKind = "Access";
                pvWire->FromUid = ctuAccessUids[0];
                pvWire->FromPin = "";
                pvWire->ToKind = "Part";
                pvWire->ToUid = pUid;
                pvWire->ToPin = pvPinName;
                ir->Wires->Add(pvWire);
            }
        }
        else if (schema != nullptr && schema->RequiresInstanceDB && cn->PresetTime != nullptr && cn->PresetTime->Length > 0) {
            int ptUid = -1;
            for (int ai = 0; ai < ir->Accesses->Count; ai++) {
                if (ir->Accesses[ai]->TargetPartUid == pUid && ir->Accesses[ai]->Scope == "TypedConstant") {
                    ptUid = ir->Accesses[ai]->Uid;
                    break;
                }
            }
            if (ptUid >= 0) {
                CuWire^ ptWire = gcnew CuWire();
                ptWire->Uid = uidAlloc->Alloc();
                ptWire->FromKind = "Access";
                ptWire->FromUid = ptUid;
                ptWire->FromPin = "";
                ptWire->ToKind = "Part";
                ptWire->ToUid = pUid;
                ptWire->ToPin = schema->PresetPinName;
                ir->Wires->Add(ptWire);
            }
        }

        if (cn->PartType == CuPartType::CompareEQ || cn->PartType == CuPartType::CompareNE ||
            cn->PartType == CuPartType::CompareGT || cn->PartType == CuPartType::CompareLT ||
            cn->PartType == CuPartType::CompareGE || cn->PartType == CuPartType::CompareLE) {
            List<int>^ cmpAccessUids = gcnew List<int>();
            for (int ai = 0; ai < ir->Accesses->Count; ai++) {
                if (ir->Accesses[ai]->TargetPartUid == pUid) {
                    cmpAccessUids->Add(ir->Accesses[ai]->Uid);
                }
            }
            if (cmpAccessUids->Count >= 1) {
                CuWire^ in1Wire = gcnew CuWire();
                in1Wire->Uid = uidAlloc->Alloc();
                in1Wire->FromKind = "Access";
                in1Wire->FromUid = cmpAccessUids[0];
                in1Wire->FromPin = "";
                in1Wire->ToKind = "Part";
                in1Wire->ToUid = pUid;
                in1Wire->ToPin = "in1";
                ir->Wires->Add(in1Wire);
            }
            if (cmpAccessUids->Count >= 2) {
                CuWire^ in2Wire = gcnew CuWire();
                in2Wire->Uid = uidAlloc->Alloc();
                in2Wire->FromKind = "Access";
                in2Wire->FromUid = cmpAccessUids[1];
                in2Wire->FromPin = "";
                in2Wire->ToKind = "Part";
                in2Wire->ToUid = pUid;
                in2Wire->ToPin = "in2";
                ir->Wires->Add(in2Wire);
            }
        }

        if (cn->PartType == CuPartType::RisingEdge || cn->PartType == CuPartType::FallingEdge) {
            List<int>^ edgeAccessUids = gcnew List<int>();
            for (int ai = 0; ai < ir->Accesses->Count; ai++) {
                if (ir->Accesses[ai]->TargetPartUid == pUid && ir->Accesses[ai]->Scope == "GlobalVariable") {
                    edgeAccessUids->Add(ir->Accesses[ai]->Uid);
                }
            }
            if (edgeAccessUids->Count >= 1) {
                CuWire^ edgeWire = gcnew CuWire();
                edgeWire->Uid = uidAlloc->Alloc();
                edgeWire->FromKind = "Access";
                edgeWire->FromUid = edgeAccessUids[0];
                edgeWire->FromPin = "";
                edgeWire->ToKind = "Part";
                edgeWire->ToUid = pUid;
                edgeWire->ToPin = "operand";
                ir->Wires->Add(edgeWire);
            }
            if (edgeAccessUids->Count >= 2) {
                CuWire^ bitWire = gcnew CuWire();
                bitWire->Uid = uidAlloc->Alloc();
                bitWire->FromKind = "Access";
                bitWire->FromUid = edgeAccessUids[1];
                bitWire->FromPin = "";
                bitWire->ToKind = "Part";
                bitWire->ToUid = pUid;
                bitWire->ToPin = "bit";
                ir->Wires->Add(bitWire);
            }
        }

        if (cn->PartType == CuPartType::RBitfield || cn->PartType == CuPartType::SBitfield) {
            int operandAccessUid = -1;
            int nAccessUid = -1;
            for (int ai = 0; ai < ir->Accesses->Count; ai++) {
                if (ir->Accesses[ai]->TargetPartUid == pUid) {
                    if (ir->Accesses[ai]->Scope == "GlobalVariable" && operandAccessUid < 0) {
                        operandAccessUid = ir->Accesses[ai]->Uid;
                    }
                    else if (nAccessUid < 0) {
                        nAccessUid = ir->Accesses[ai]->Uid;
                    }
                }
            }
            if (nAccessUid >= 0) {
                CuWire^ nWire = gcnew CuWire();
                nWire->Uid = uidAlloc->Alloc();
                nWire->FromKind = "Access";
                nWire->FromUid = nAccessUid;
                nWire->FromPin = "";
                nWire->ToKind = "Part";
                nWire->ToUid = pUid;
                nWire->ToPin = "n";
                nWire->ExtraTargets = gcnew List<CuWireTarget^>();
                ir->Wires->Add(nWire);
            }
        }

        if (cn->PartType == CuPartType::Move) {
            List<int>^ moveAccessUids = gcnew List<int>();
            for (int ai = 0; ai < ir->Accesses->Count; ai++) {
                if (ir->Accesses[ai]->TargetPartUid == pUid) {
                    moveAccessUids->Add(ir->Accesses[ai]->Uid);
                }
            }
            if (moveAccessUids->Count >= 1) {
                CuWire^ inWire = gcnew CuWire();
                inWire->Uid = uidAlloc->Alloc();
                inWire->FromKind = "Access";
                inWire->FromUid = moveAccessUids[0];
                inWire->FromPin = "";
                inWire->ToKind = "Part";
                inWire->ToUid = pUid;
                inWire->ToPin = "in";
                ir->Wires->Add(inWire);
            }
            if (moveAccessUids->Count >= 2) {
                CuWire^ outWire = gcnew CuWire();
                outWire->Uid = uidAlloc->Alloc();
                outWire->FromKind = "Part";
                outWire->FromUid = pUid;
                outWire->FromPin = "out1";
                outWire->ToKind = "Access";
                outWire->ToUid = moveAccessUids[1];
                outWire->ToPin = "";
                ir->Wires->Add(outWire);
            }
        }

        if (cn->PartType == CuPartType::ADD || cn->PartType == CuPartType::SUB ||
            cn->PartType == CuPartType::MUL || cn->PartType == CuPartType::DIV ||
            cn->PartType == CuPartType::MOD) {
            List<int>^ mathAccessUids = gcnew List<int>();
            for (int ai = 0; ai < ir->Accesses->Count; ai++) {
                if (ir->Accesses[ai]->TargetPartUid == pUid) {
                    mathAccessUids->Add(ir->Accesses[ai]->Uid);
                }
            }
            if (mathAccessUids->Count >= 1) {
                CuWire^ in1Wire = gcnew CuWire();
                in1Wire->Uid = uidAlloc->Alloc();
                in1Wire->FromKind = "Access";
                in1Wire->FromUid = mathAccessUids[0];
                in1Wire->FromPin = "";
                in1Wire->ToKind = "Part";
                in1Wire->ToUid = pUid;
                in1Wire->ToPin = "in1";
                ir->Wires->Add(in1Wire);
            }
            if (mathAccessUids->Count >= 2) {
                CuWire^ in2Wire = gcnew CuWire();
                in2Wire->Uid = uidAlloc->Alloc();
                in2Wire->FromKind = "Access";
                in2Wire->FromUid = mathAccessUids[1];
                in2Wire->FromPin = "";
                in2Wire->ToKind = "Part";
                in2Wire->ToUid = pUid;
                in2Wire->ToPin = "in2";
                ir->Wires->Add(in2Wire);
            }
            if (mathAccessUids->Count >= 3) {
                CuWire^ outWire = gcnew CuWire();
                outWire->Uid = uidAlloc->Alloc();
                outWire->FromKind = "Part";
                outWire->FromUid = pUid;
                outWire->FromPin = "out";
                outWire->ToKind = "Access";
                outWire->ToUid = mathAccessUids[2];
                outWire->ToPin = "";
                ir->Wires->Add(outWire);
            }
        }

        if (cn->PartType == CuPartType::JMP || cn->PartType == CuPartType::LABEL) {
            int jmpOpUid = -1;
            for (int ai = 0; ai < ir->Accesses->Count; ai++) {
                if (ir->Accesses[ai]->TargetPartUid == pUid && (ir->Accesses[ai]->Scope == "GlobalVariable" || ir->Accesses[ai]->Scope == "Label")) {
                    jmpOpUid = ir->Accesses[ai]->Uid;
                    break;
                }
            }
            if (jmpOpUid >= 0) {
                CuWire^ jmpWire = gcnew CuWire();
                jmpWire->Uid = uidAlloc->Alloc();
                jmpWire->FromKind = "Access";
                jmpWire->FromUid = jmpOpUid;
                jmpWire->FromPin = "";
                jmpWire->ToKind = "Part";
                jmpWire->ToUid = pUid;
                jmpWire->ToPin = "operand";
                ir->Wires->Add(jmpWire);
            }
        }
    }

    for (int i = 0; i < cfg->Nodes->Count; i++) {
        CfgNode^ cn = cfg->Nodes[i];
        if (cn->Type != CfgNodeType::Instruction) continue;
        if (!cfgIdToPartUid->ContainsKey(cn->Id)) continue;

        PartSchema^ schema = reg->GetSchema(cn->PartType);
        if (schema == nullptr) continue;

        int pUid = cfgIdToPartUid[cn->Id];

        for (int opi = 0; opi < schema->OutputPins->Count; opi++) {
            String^ outPinName = schema->OutputPins[opi]->Name;
            bool isConnected = false;
            for (int wi = 0; wi < ir->Wires->Count; wi++) {
                CuWire^ w = ir->Wires[wi];
                if (w->FromKind == "Part" && w->FromUid == pUid && w->FromPin == outPinName) {
                    isConnected = true;
                    break;
                }
            }

            if (!isConnected) {
                bool skipOpenCon = (outPinName == "Q" || outPinName == "QU" ||
                                    outPinName == "ENO" || outPinName == "eno" || outPinName == "BIT");
                if (skipOpenCon) continue;

                int ocUid = uidAlloc->Alloc();

                CuWire^ ocWire = gcnew CuWire();
                ocWire->Uid = uidAlloc->Alloc();
                ocWire->FromKind = "Part";
                ocWire->FromUid = pUid;
                ocWire->FromPin = outPinName;
                ocWire->ToKind = "OpenCon";
                ocWire->ToUid = ocUid;
                ocWire->ToPin = "";
                ocWire->ExtraTargets = gcnew List<CuWireTarget^>();
                ir->Wires->Add(ocWire);
            }
        }
    }

    Dictionary<String^, List<CuWire^>^>^ fromKeyWires = gcnew Dictionary<String^, List<CuWire^>^>();
    for (int i = 0; i < ir->Wires->Count; i++) {
        CuWire^ w = ir->Wires[i];
        if (w->FromKind == "Part") {
            String^ key = w->FromUid + ":" + w->FromPin;
            if (!fromKeyWires->ContainsKey(key)) fromKeyWires[key] = gcnew List<CuWire^>();
            fromKeyWires[key]->Add(w);
        }
    }

    List<CuWire^>^ mergedWires = gcnew List<CuWire^>();
    HashSet<String^>^ processed = gcnew HashSet<String^>();
    for (int i = 0; i < ir->Wires->Count; i++) {
        CuWire^ w = ir->Wires[i];
        if (w->FromKind != "Part") {
            mergedWires->Add(w);
            continue;
        }
        String^ key = w->FromUid + ":" + w->FromPin;
        if (processed->Contains(key)) continue;
        processed->Add(key);

        List<CuWire^>^ group = fromKeyWires[key];
        if (group->Count <= 1) {
            mergedWires->Add(w);
            continue;
        }

        CuWire^ merged = gcnew CuWire();
        merged->Uid = group[0]->Uid;
        merged->FromKind = group[0]->FromKind;
        merged->FromUid = group[0]->FromUid;
        merged->FromPin = group[0]->FromPin;
        merged->ToKind = group[0]->ToKind;
        merged->ToUid = group[0]->ToUid;
        merged->ToPin = group[0]->ToPin;
        merged->ExtraTargets = gcnew List<CuWireTarget^>();

        if (group[0]->ExtraTargets != nullptr) {
            for each (CuWireTarget^ t in group[0]->ExtraTargets) {
                merged->ExtraTargets->Add(t);
            }
        }

        for (int gi = 1; gi < group->Count; gi++) {
            CuWireTarget^ t = gcnew CuWireTarget();
            t->Uid = group[gi]->ToUid;
            t->Pin = group[gi]->ToPin;
            merged->ExtraTargets->Add(t);

            if (group[gi]->ExtraTargets != nullptr) {
                for each (CuWireTarget^ et in group[gi]->ExtraTargets) {
                    merged->ExtraTargets->Add(et);
                }
            }
        }

        mergedWires->Add(merged);
    }
    ir->Wires = mergedWires;

    return ir;
}

inline bool IrHasOutput(CompileUnitIR^ ir) {
    for (int i = 0; i < ir->Parts->Count; i++) {
        CuPartType pt = ir->Parts[i]->PartType;
        if (pt == CuPartType::Coil || pt == CuPartType::SCoil ||
            pt == CuPartType::RCoil || pt == CuPartType::TON ||
            pt == CuPartType::TOF || pt == CuPartType::TP ||
            pt == CuPartType::CTU || pt == CuPartType::CTD ||
            pt == CuPartType::CTUD || pt == CuPartType::Move ||
            pt == CuPartType::Math ||
            pt == CuPartType::ADD || pt == CuPartType::SUB ||
            pt == CuPartType::MUL || pt == CuPartType::DIV ||
            pt == CuPartType::MOD ||
            pt == CuPartType::CompareEQ || pt == CuPartType::CompareNE ||
            pt == CuPartType::CompareGT || pt == CuPartType::CompareLT ||
            pt == CuPartType::CompareGE || pt == CuPartType::CompareLE ||
            pt == CuPartType::JMP || pt == CuPartType::RET ||
            pt == CuPartType::RBitfield || pt == CuPartType::SBitfield) {
            return true;
        }
    }
    return false;
}

inline bool IsOutputPart(CuPartType pt) {
    return pt == CuPartType::Coil || pt == CuPartType::SCoil || pt == CuPartType::RCoil ||
           pt == CuPartType::TON || pt == CuPartType::TOF || pt == CuPartType::TP ||
           pt == CuPartType::CTU || pt == CuPartType::CTD || pt == CuPartType::CTUD ||
           pt == CuPartType::Move || pt == CuPartType::ADD || pt == CuPartType::SUB ||
           pt == CuPartType::MUL || pt == CuPartType::DIV || pt == CuPartType::MOD ||
           pt == CuPartType::JMP || pt == CuPartType::LABEL || pt == CuPartType::RET ||
           pt == CuPartType::RBitfield || pt == CuPartType::SBitfield;
}

inline bool IsTerminalOutputPart(CuPartType pt) {
    return pt == CuPartType::Coil || pt == CuPartType::SCoil || pt == CuPartType::RCoil ||
           pt == CuPartType::JMP || pt == CuPartType::LABEL || pt == CuPartType::RET ||
           pt == CuPartType::RBitfield || pt == CuPartType::SBitfield;
}

inline void ValidateCompileUnitIR(CompileUnitIR^ ir) {
    if (!IrHasOutput(ir)) {
        Console::WriteLine("    [VALIDATION ERROR] Network has no output element (Coil/SCoil/RCoil/Box required). Skipping.");
        return;
    }

    HashSet<int>^ usedAccessUids = gcnew HashSet<int>();

    for (int i = 0; i < ir->Wires->Count; i++) {
        CuWire^ w = ir->Wires[i];
        if (w->FromKind == "Access") usedAccessUids->Add(w->FromUid);
        if (w->ToKind == "Access") usedAccessUids->Add(w->ToUid);
    }

    for (int i = 0; i < ir->Accesses->Count; i++) {
        CuAccess^ a = ir->Accesses[i];
        if (!usedAccessUids->Contains(a->Uid)) {
            Console::WriteLine("    [VALIDATION ERROR] Access UId={0} symbol={1} is UNUSED (dangling reference)",
                a->Uid, a->Symbol);
        }
    }

    for (int i = 0; i < ir->Wires->Count; i++) {
        CuWire^ w = ir->Wires[i];
        if (w->FromKind == "Access") {
            bool found = false;
            for (int j = 0; j < ir->Accesses->Count; j++) {
                if (ir->Accesses[j]->Uid == w->FromUid) { found = true; break; }
            }
            if (!found) {
                Console::WriteLine("    [VALIDATION ERROR] Wire UId={0} references non-existent Access UId={1}",
                    w->Uid, w->FromUid);
            }
        }
    }

    for (int i = 1; i < ir->Parts->Count; i++) {
        if (ir->Parts[i]->Uid <= ir->Parts[i - 1]->Uid) {
            Console::WriteLine("    [VALIDATION ERROR] Parts not in strictly ascending UId order: [{0}]={1} <= [{2}]={3}",
                i - 1, ir->Parts[i - 1]->Uid, i, ir->Parts[i]->Uid);
        }
    }

    HashSet<int>^ partUids = gcnew HashSet<int>();
    for (int i = 0; i < ir->Parts->Count; i++) {
        partUids->Add(ir->Parts[i]->Uid);
    }

    for (int i = 0; i < ir->Wires->Count; i++) {
        CuWire^ w = ir->Wires[i];
        if (w->FromKind == "Part" && !partUids->Contains(w->FromUid)) {
            Console::WriteLine("    [VALIDATION ERROR] Wire UId={0} references non-existent Part FromUid={1}",
                w->Uid, w->FromUid);
        }
        if (w->ToKind == "Part" && !partUids->Contains(w->ToUid)) {
            Console::WriteLine("    [VALIDATION ERROR] Wire UId={0} references non-existent Part ToUid={1}",
                w->Uid, w->ToUid);
        }
    }

    for (int i = 0; i < ir->Parts->Count; i++) {
        int pUid = ir->Parts[i]->Uid;
        bool hasInputWire = false;
        for (int wi = 0; wi < ir->Wires->Count; wi++) {
            CuWire^ w = ir->Wires[wi];
            if (w->ToKind == "Part" && w->ToUid == pUid) { hasInputWire = true; break; }
            if (w->ExtraTargets != nullptr) {
                for (int ei = 0; ei < w->ExtraTargets->Count; ei++) {
                    if (w->ExtraTargets[ei]->Uid == pUid) { hasInputWire = true; break; }
                }
            }
            if (hasInputWire) break;
        }
        if (!hasInputWire) {
            Console::WriteLine("    [VALIDATION WARNING] Part UId={0} type={1} has no input wire (orphan node)",
                pUid, ir->Parts[i]->Type);
        }
    }

    PartSchemaRegistry^ valReg = gcnew PartSchemaRegistry();

    for (int i = 0; i < ir->Parts->Count; i++) {
        int pUid = ir->Parts[i]->Uid;
        PartSchema^ outSchema = valReg->GetSchema(ir->Parts[i]->PartType);
        if (outSchema == nullptr || outSchema->OutputPins->Count == 0) continue;

        for (int opi = 0; opi < outSchema->OutputPins->Count; opi++) {
            String^ outPin = outSchema->OutputPins[opi]->Name;
            bool hasOutputWire = false;
            for (int wi = 0; wi < ir->Wires->Count; wi++) {
                CuWire^ w = ir->Wires[wi];
                if (w->FromKind == "Part" && w->FromUid == pUid && w->FromPin == outPin) {
                    hasOutputWire = true;
                    break;
                }
            }
            if (!hasOutputWire) {
                Console::WriteLine("    [VALIDATION WARNING] Part UId={0} type={1} output pin '{2}' is dangling (no wire from output)",
                    pUid, ir->Parts[i]->Type, outPin);
            }
        }
    }

    for (int i = 0; i < ir->Parts->Count; i++) {
        CuPart^ p = ir->Parts[i];
        PartSchema^ schema = valReg->GetSchema(p->PartType);
        if (schema == nullptr) continue;

        if (schema->RequiresInstanceDB) {
            if (p->InstanceName == nullptr || p->InstanceName->Length == 0) {
                Console::WriteLine("    [VALIDATION ERROR] IEC Part UId={0} type={1} requires InstanceDB but InstanceName is missing",
                    p->Uid, p->Type);
            }
        }

        Dictionary<String^, bool>^ connectedInputPins = gcnew Dictionary<String^, bool>();
        for (int pi = 0; pi < schema->InputPins->Count; pi++) {
            connectedInputPins[schema->InputPins[pi]->Name] = false;
        }

        for (int wi = 0; wi < ir->Wires->Count; wi++) {
            CuWire^ w = ir->Wires[wi];
            if (w->ToKind == "Part" && w->ToUid == p->Uid && connectedInputPins->ContainsKey(w->ToPin)) {
                connectedInputPins[w->ToPin] = true;
            }
            if (w->ExtraTargets != nullptr) {
                for (int ei = 0; ei < w->ExtraTargets->Count; ei++) {
                    CuWireTarget^ t = w->ExtraTargets[ei];
                    if (t->Uid == p->Uid && connectedInputPins->ContainsKey(t->Pin)) {
                        connectedInputPins[t->Pin] = true;
                    }
                }
            }
        }

        for each (KeyValuePair<String^, bool>^ kv in connectedInputPins) {
            if (kv->Value == false) {
                bool isRequired = false;
                for (int pi = 0; pi < schema->InputPins->Count; pi++) {
                    if (schema->InputPins[pi]->Name == kv->Key && schema->InputPins[pi]->Required) {
                        isRequired = true;
                        break;
                    }
                }
                if (isRequired) {
                    Console::WriteLine("    [VALIDATION ERROR] IEC Part UId={0} type={1} required input pin '{2}' is not connected",
                        p->Uid, p->Type, kv->Key);
                }
            }
        }
    }

    Dictionary<String^, List<int>^>^ coilVarMap = gcnew Dictionary<String^, List<int>^>();
    for (int i = 0; i < ir->Parts->Count; i++) {
        CuPart^ p = ir->Parts[i];
        if (p->PartType == CuPartType::Coil || p->PartType == CuPartType::SCoil ||
            p->PartType == CuPartType::RCoil) {
            String^ coilVar = "";
            for (int ai = 0; ai < ir->Accesses->Count; ai++) {
                CuAccess^ a = ir->Accesses[ai];
                if (a->TargetPartUid == p->Uid) {
                    coilVar = a->Symbol;
                    break;
                }
            }
            if (coilVar->Length > 0) {
                if (!coilVarMap->ContainsKey(coilVar)) {
                    coilVarMap[coilVar] = gcnew List<int>();
                }
                coilVarMap[coilVar]->Add(p->Uid);
            }
        }
    }
    for each (KeyValuePair<String^, List<int>^>^ kv in coilVarMap) {
        if (kv->Value->Count > 1) {
            String^ uidList = "";
            for (int li = 0; li < kv->Value->Count; li++) {
                if (li > 0) uidList += ",";
                uidList += kv->Value[li].ToString();
            }
            Console::WriteLine("    [VALIDATION WARNING] Variable '{0}' is driven by multiple coils: UIds=[{1}] (double coil conflict)",
                kv->Key, uidList);
        }
    }

    for (int i = 0; i < ir->Parts->Count; i++) {
        CuPart^ p = ir->Parts[i];
        if (IsTerminalOutputPart(p->PartType)) {
            for (int wi = 0; wi < ir->Wires->Count; wi++) {
                CuWire^ w = ir->Wires[wi];
                if (w->FromKind == "Part" && w->FromUid == p->Uid) {
                    for (int ti = 0; ti < ir->Parts->Count; ti++) {
                        CuPart^ tp = ir->Parts[ti];
                        if (tp->Uid == w->ToUid && IsOutputPart(tp->PartType)) {
                            Console::WriteLine("    [VALIDATION ERROR] Output element Part UId={0} illegally drives another output element Part UId={1}",
                                p->Uid, tp->Uid);
                        }
                    }
                }
            }
        }
    }

    Dictionary<int, int>^ partInputWireCount = gcnew Dictionary<int, int>();
    for (int i = 0; i < ir->Parts->Count; i++) {
        partInputWireCount[ir->Parts[i]->Uid] = 0;
    }
    for (int i = 0; i < ir->Wires->Count; i++) {
        CuWire^ w = ir->Wires[i];
        if (w->ToKind == "Part" && partInputWireCount->ContainsKey(w->ToUid)) {
            partInputWireCount[w->ToUid]++;
        }
        if (w->ExtraTargets != nullptr) {
            for (int ei = 0; ei < w->ExtraTargets->Count; ei++) {
                CuWireTarget^ t = w->ExtraTargets[ei];
                if (partInputWireCount->ContainsKey(t->Uid)) {
                    partInputWireCount[t->Uid]++;
                }
            }
        }
    }
    for (int i = 0; i < ir->Parts->Count; i++) {
        CuPart^ p = ir->Parts[i];
        if (p->PartType == CuPartType::O && partInputWireCount[p->Uid] < 2) {
            Console::WriteLine("    [VALIDATION ERROR] OrJunction Part UId={0} has only {1} input wire(s), requires >= 2 (branch not closed)",
                p->Uid, partInputWireCount[p->Uid]);
        }
    }

    for (int i = 0; i < ir->Parts->Count; i++) {
        CuPart^ p = ir->Parts[i];
        VariableType expectedType = VariableEngine::InferTypeFromPartType(p->PartType);

        for (int wi = 0; wi < ir->Wires->Count; wi++) {
            CuWire^ w = ir->Wires[wi];
            if (w->ToKind == "Part" && w->ToUid == p->Uid) {
                if (w->FromKind == "Access") {
                    for (int ai = 0; ai < ir->Accesses->Count; ai++) {
                        CuAccess^ a = ir->Accesses[ai];
                        if (a->Uid == w->FromUid) {
                            VariableType accessType = VariableEngine::InferTypeFromSymbol(a->Symbol);
                            if (expectedType == VariableType::Int &&
                                (accessType == VariableType::Timer || accessType == VariableType::Counter)) {
                                Console::WriteLine("    [VALIDATION WARNING] Type mismatch: Part UId={0} expects Int but Access '{1}' is {2}",
                                    p->Uid, a->Symbol, VariableEngine::TypeToString(accessType));
                            }
                            if (expectedType == VariableType::Bool &&
                                (accessType == VariableType::Timer || accessType == VariableType::Counter || accessType == VariableType::Int)) {
                                Console::WriteLine("    [VALIDATION WARNING] Type mismatch: Part UId={0} expects Bool but Access '{1}' is {2}",
                                    p->Uid, a->Symbol, VariableEngine::TypeToString(accessType));
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    Dictionary<int, int>^ uidUsageCount = gcnew Dictionary<int, int>();
    for (int i = 0; i < ir->Parts->Count; i++) {
        int uid = ir->Parts[i]->Uid;
        if (uidUsageCount->ContainsKey(uid)) {
            Console::WriteLine("    [VALIDATION ERROR] Duplicate Part UId={0} found", uid);
        }
        else {
            uidUsageCount[uid] = 0;
        }
    }
    for (int i = 0; i < ir->Accesses->Count; i++) {
        int uid = ir->Accesses[i]->Uid;
        if (uidUsageCount->ContainsKey(uid)) {
            Console::WriteLine("    [VALIDATION ERROR] Duplicate Access UId={0} found (conflicts with Part)", uid);
        }
        else {
            uidUsageCount[uid] = 0;
        }
    }
    for (int i = 0; i < ir->Wires->Count; i++) {
        int uid = ir->Wires[i]->Uid;
        if (uidUsageCount->ContainsKey(uid)) {
            Console::WriteLine("    [VALIDATION ERROR] Duplicate Wire UId={0} found (conflicts with Part/Access)", uid);
        }
        else {
            uidUsageCount[uid] = 0;
        }
    }

    for (int i = 0; i < ir->Wires->Count; i++) {
        CuWire^ w = ir->Wires[i];
        if (w->FromKind == "Part" && !uidUsageCount->ContainsKey(w->FromUid)) {
            Console::WriteLine("    [VALIDATION ERROR] Wire UId={0} references non-existent FromUid={1}",
                w->Uid, w->FromUid);
        }
        if (w->ToKind == "Part" && !uidUsageCount->ContainsKey(w->ToUid)) {
            Console::WriteLine("    [VALIDATION ERROR] Wire UId={0} references non-existent ToUid={1}",
                w->Uid, w->ToUid);
        }
    }

    Dictionary<String^, int>^ labelMap = gcnew Dictionary<String^, int>();
    for (int i = 0; i < ir->Parts->Count; i++) {
        CuPart^ p = ir->Parts[i];
        if (p->PartType == CuPartType::LABEL) {
            for (int ai = 0; ai < ir->Accesses->Count; ai++) {
                CuAccess^ a = ir->Accesses[ai];
                if (a->TargetPartUid == p->Uid) {
                    labelMap[a->Symbol] = p->Uid;
                    break;
                }
            }
        }
    }
    for (int i = 0; i < ir->Parts->Count; i++) {
        CuPart^ p = ir->Parts[i];
        if (p->PartType == CuPartType::JMP) {
            String^ targetLabel = "";
            for (int ai = 0; ai < ir->Accesses->Count; ai++) {
                CuAccess^ a = ir->Accesses[ai];
                if (a->TargetPartUid == p->Uid) {
                    targetLabel = a->Symbol;
                    break;
                }
            }
            if (targetLabel->Length > 0 && !labelMap->ContainsKey(targetLabel)) {
                Console::WriteLine("    [VALIDATION ERROR] JMP Part UId={0} references undefined label '{1}'",
                    p->Uid, targetLabel);
            }
        }
    }

    HashSet<int>^ usedUids = gcnew HashSet<int>();
    for (int i = 0; i < ir->Wires->Count; i++) {
        CuWire^ w = ir->Wires[i];
        if (w->FromKind == "Part") usedUids->Add(w->FromUid);
        if (w->ToKind == "Part") usedUids->Add(w->ToUid);
        if (w->FromKind == "Access") usedUids->Add(w->FromUid);
        if (w->ExtraTargets != nullptr) {
            for (int ei = 0; ei < w->ExtraTargets->Count; ei++) {
                usedUids->Add(w->ExtraTargets[ei]->Uid);
            }
        }
    }
    for (int i = 0; i < ir->Accesses->Count; i++) {
        usedUids->Add(ir->Accesses[i]->Uid);
    }
    for (int i = 0; i < ir->Parts->Count; i++) {
        if (!usedUids->Contains(ir->Parts[i]->Uid)) {
            Console::WriteLine("    [VALIDATION WARNING] Part UId={0} type={1} is not referenced by any wire (unused)",
                ir->Parts[i]->Uid, ir->Parts[i]->Type);
        }
    }
}

inline void FixOutputToOutputWires(CompileUnitIR^ ir, UidAllocator^ uidAlloc) {
    PartSchemaRegistry^ reg = gcnew PartSchemaRegistry();
    bool changed = true;
    while (changed) {
        changed = false;
        for (int wi = 0; wi < ir->Wires->Count; wi++) {
            CuWire^ w = ir->Wires[wi];
            if (w->FromKind != "Part") continue;

            CuPart^ fromPart = nullptr;
            CuPart^ toPart = nullptr;
            for (int pi = 0; pi < ir->Parts->Count; pi++) {
                if (ir->Parts[pi]->Uid == w->FromUid) fromPart = ir->Parts[pi];
                if (ir->Parts[pi]->Uid == w->ToUid && w->ToKind == "Part") toPart = ir->Parts[pi];
            }
            if (fromPart == nullptr || toPart == nullptr) continue;

            bool isIllegal = false;
            if (IsTerminalOutputPart(fromPart->PartType) && IsOutputPart(toPart->PartType)) {
                isIllegal = true;
            }
            if (!isIllegal) continue;

            Console::WriteLine("    [FIX] Breaking Output->Output wire: Part UId={0}({1}) -> Part UId={2}({3})",
                fromPart->Uid, fromPart->Type, toPart->Uid, toPart->Type);

            String^ fromSourceKind = "Powerrail";
            int fromSourceUid = 0;
            String^ fromSourcePin = "";
            for (int si = 0; si < ir->Wires->Count; si++) {
                CuWire^ sw = ir->Wires[si];
                if (sw->FromKind == "Access" || sw->FromKind == "OpenCon") continue;
                if (sw->ToKind == "Part" && sw->ToUid == fromPart->Uid) {
                    fromSourceKind = sw->FromKind;
                    fromSourceUid = sw->FromUid;
                    fromSourcePin = sw->FromPin;
                    break;
                }
                if (sw->ExtraTargets != nullptr) {
                    for (int ei = 0; ei < sw->ExtraTargets->Count; ei++) {
                        if (sw->ExtraTargets[ei]->Uid == fromPart->Uid) {
                            fromSourceKind = sw->FromKind;
                            fromSourceUid = sw->FromUid;
                            fromSourcePin = sw->FromPin;
                            break;
                        }
                    }
                }
            }

            ir->Wires->RemoveAt(wi);
            changed = true;

            CuWire^ newWire = gcnew CuWire();
            newWire->Uid = uidAlloc->Alloc();
            newWire->FromKind = fromSourceKind;
            newWire->FromUid = fromSourceUid;
            newWire->FromPin = fromSourcePin;
            newWire->ToKind = "Part";
            newWire->ToUid = toPart->Uid;
            PartSchema^ toSchema = reg->GetSchema(toPart->PartType);
            newWire->ToPin = (toSchema != nullptr && toSchema->InputPins->Count > 0) ?
                toSchema->InputPins[0]->Name : "in";
            newWire->ExtraTargets = gcnew List<CuWireTarget^>();
            ir->Wires->Add(newWire);

            Console::WriteLine("    [FIX] Created parallel branch: {0} UId={1} -> Part UId={2}({3}) pin={4}",
                fromSourceKind, fromSourceUid, toPart->Uid, toPart->Type, newWire->ToPin);

            break;
        }
    }
}

inline IRProgram^ BuildIRProgram(CompileUnitIR^ ir) {
    IRProgram^ program = gcnew IRProgram();
    program->Nodes = gcnew List<IRNode^>();
    program->Accesses = gcnew List<IRAccess^>();
    program->Wires = gcnew List<IRWire^>();
    program->XmlNs = ir->XmlNs;

    for (int i = 0; i < ir->Accesses->Count; i++) {
        CuAccess^ a = ir->Accesses[i];
        IRAccess^ ia = gcnew IRAccess();
        ia->Uid = a->Uid;
        ia->Symbol = a->Symbol;
        ia->Scope = a->Scope;
        ia->ConstantValue = a->ConstantValue;
        ia->ConstantType = a->ConstantType;
        program->Accesses->Add(ia);
    }

    for (int i = 0; i < ir->Wires->Count; i++) {
        CuWire^ w = ir->Wires[i];
        IRWire^ iw = gcnew IRWire();
        iw->Uid = w->Uid;
        iw->FromKind = w->FromKind;
        iw->FromUid = w->FromUid;
        iw->FromPin = w->FromPin;
        iw->ToKind = w->ToKind;
        iw->ToUid = w->ToUid;
        iw->ToPin = w->ToPin;
        iw->ExtraTargets = gcnew List<IRWireTarget^>();
        if (w->ExtraTargets != nullptr) {
            for (int ei = 0; ei < w->ExtraTargets->Count; ei++) {
                IRWireTarget^ it = gcnew IRWireTarget();
                it->Uid = w->ExtraTargets[ei]->Uid;
                it->Pin = w->ExtraTargets[ei]->Pin;
                iw->ExtraTargets->Add(it);
            }
        }
        program->Wires->Add(iw);
    }

    PartSchemaRegistry^ reg = gcnew PartSchemaRegistry();

    for (int i = 0; i < ir->Parts->Count; i++) {
        CuPart^ p = ir->Parts[i];
        PartSchema^ schema = reg->GetSchema(p->PartType);

        IRNode^ node = gcnew IRNode();
        node->Uid = p->Uid;
        node->Name = p->Type;
        node->Negated = p->NormallyClosed;
        node->Cardinality = p->Cardinality;
        node->DataType = p->DataType;
        node->Parameters = gcnew List<IRParameter^>();

        if (schema != nullptr && schema->RequiresInstanceDB) {
            node->Kind = IRNodeKind::Part;
            node->InstanceDB = p->InstanceName;
            node->InstanceUid = p->InstanceUid;

            for (int pi = 0; pi < schema->InputPins->Count; pi++) {
                IRParameter^ param = gcnew IRParameter();
                param->Name = schema->InputPins[pi]->Name;
                param->Section = "Input";
                param->Datatype = "Bool";
                param->WireUid = 0;
                node->Parameters->Add(param);
            }
            for (int pi = 0; pi < schema->OutputPins->Count; pi++) {
                IRParameter^ param = gcnew IRParameter();
                param->Name = schema->OutputPins[pi]->Name;
                param->Section = "Output";
                param->Datatype = "Bool";
                param->WireUid = 0;
                node->Parameters->Add(param);
            }
        }
        else {
            node->Kind = IRNodeKind::Part;
            node->InstanceDB = "";
            node->InstanceUid = 0;
        }

        program->Nodes->Add(node);
    }

    Dictionary<int, String^>^ accessUidToType = gcnew Dictionary<int, String^>();
    for (int ai = 0; ai < program->Accesses->Count; ai++) {
        IRAccess^ a = program->Accesses[ai];
        String^ inferredType = "";
        if (a->Scope == "TypedConstant" || a->Scope == "LiteralConstant") {
            inferredType = TypeInferenceEngine::InferTypeFromConstant(a->ConstantValue);
        }
        else if (a->Scope == "GlobalVariable") {
            inferredType = TypeInferenceEngine::InferTypeFromSymbol(a->Symbol);
        }
        accessUidToType[a->Uid] = inferredType;
    }

    for (int ni = 0; ni < program->Nodes->Count; ni++) {
        IRNode^ node = program->Nodes[ni];
        if (node->DataType != nullptr && node->DataType->Length > 0) continue;

        bool needsTypeInference = false;
        PartSchema^ nodeSchema = (gcnew PartSchemaRegistry())->GetSchema(
            (gcnew PartSchemaRegistry())->NameToType(node->Name));
        if (nodeSchema != nullptr && nodeSchema->TemplateValues != nullptr) {
            for (int ti = 0; ti < nodeSchema->TemplateValues->Count; ti++) {
                if (nodeSchema->TemplateValues[ti]->Name == "SrcType") {
                    needsTypeInference = true;
                    break;
                }
            }
        }
        if (!needsTypeInference) continue;

        List<String^>^ operandTypes = gcnew List<String^>();
        for (int wi = 0; wi < program->Wires->Count; wi++) {
            IRWire^ w = program->Wires[wi];
            if (w->ToUid == node->Uid) {
                if (w->FromKind == "Access" && accessUidToType->ContainsKey(w->FromUid)) {
                    String^ t = accessUidToType[w->FromUid];
                    if (t != nullptr && t->Length > 0 && t != "Bool") {
                        operandTypes->Add(t);
                    }
                }
            }
        }

        if (operandTypes->Count > 0) {
            String^ inferred = TypeInferenceEngine::InferSrcType(node->Name, operandTypes);
            node->DataType = inferred;
        }
    }

    Dictionary<String^, List<IRWire^>^>^ fanoutMap = gcnew Dictionary<String^, List<IRWire^>^>();
    for (int wi = 0; wi < program->Wires->Count; wi++) {
        IRWire^ w = program->Wires[wi];
        if (w->FromKind == "PowerRail" || w->FromKind == "Access") continue;
        String^ key = w->FromUid.ToString() + ":" + w->FromPin;
        if (!fanoutMap->ContainsKey(key)) {
            fanoutMap[key] = gcnew List<IRWire^>();
        }
        fanoutMap[key]->Add(w);
    }

    List<IRWire^>^ mergedWires = gcnew List<IRWire^>();
    HashSet<int>^ removedUids = gcnew HashSet<int>();

    for each (KeyValuePair<String^, List<IRWire^>^>^ kvp in fanoutMap) {
        if (kvp->Value->Count <= 1) continue;
        IRWire^ first = kvp->Value[0];
        for (int fi = 1; fi < kvp->Value->Count; fi++) {
            IRWire^ dup = kvp->Value[fi];
            IRWireTarget^ t = gcnew IRWireTarget();
            t->Uid = dup->ToUid;
            t->Pin = dup->ToPin;
            first->ExtraTargets->Add(t);
            removedUids->Add(dup->Uid);
        }
    }

    for (int wi = 0; wi < program->Wires->Count; wi++) {
        if (!removedUids->Contains(program->Wires[wi]->Uid)) {
            mergedWires->Add(program->Wires[wi]);
        }
    }
    program->Wires = mergedWires;

    Dictionary<String^, int>^ targetUsage = gcnew Dictionary<String^, int>();
    for (int wi = 0; wi < program->Wires->Count; wi++) {
        IRWire^ w = program->Wires[wi];
        String^ toKey = w->ToUid.ToString() + ":" + w->ToPin;
        if (!targetUsage->ContainsKey(toKey)) targetUsage[toKey] = 0;
        targetUsage[toKey]++;
        if (w->ExtraTargets != nullptr) {
            for (int ei = 0; ei < w->ExtraTargets->Count; ei++) {
                String^ extKey = w->ExtraTargets[ei]->Uid.ToString() + ":" + w->ExtraTargets[ei]->Pin;
                if (!targetUsage->ContainsKey(extKey)) targetUsage[extKey] = 0;
                targetUsage[extKey]++;
            }
        }
    }

    List<String^>^ dupTargets = gcnew List<String^>();
    for each (KeyValuePair<String^, int>^ kvp in targetUsage) {
        if (kvp->Value > 1) dupTargets->Add(kvp->Key + "(" + kvp->Value + ")");
    }
    if (dupTargets->Count > 0) {
        Console::WriteLine("    [WARN] Duplicate wire targets: " + String::Join(", ", dupTargets));
    }

    return program;
}

ref class SchemaVersionMapper {
public:
    static String^ GetNamespace(int tiaVersion) {
        switch (tiaVersion) {
        case 16: return "http://www.siemens.com/automation/Openness/SW/NetworkSource/FlgNet/v3";
        case 17: return "http://www.siemens.com/automation/Openness/SW/NetworkSource/FlgNet/v4";
        case 18: return "http://www.siemens.com/automation/Openness/SW/NetworkSource/FlgNet/v5";
        case 19: return "http://www.siemens.com/automation/Openness/SW/NetworkSource/FlgNet/v5";
        case 20: return "http://www.siemens.com/automation/Openness/SW/NetworkSource/FlgNet/v5";
        default: return "http://www.siemens.com/automation/Openness/SW/NetworkSource/FlgNet/v5";
        }
    }

    static int DetectVersion(String^ ns) {
        if (ns == nullptr || ns->Length == 0) return 18;
        if (ns->Contains("/v3")) return 16;
        if (ns->Contains("/v4")) return 17;
        if (ns->Contains("/v5")) return 18;
        return 18;
    }

    static String^ GetSwBlockName(int tiaVersion) {
        if (tiaVersion <= 16) return "SW.Blocks.FC";
        return "SW.Blocks.FC";
    }

    static bool SupportsFeature(int tiaVersion, String^ feature) {
        if (feature == "CTUD") return tiaVersion >= 17;
        if (feature == "MultiInstance") return tiaVersion >= 17;
        if (feature == "MathBox") return tiaVersion >= 18;
        return true;
    }
};

ref class LADPartWriter {
private:
    String^ ResolveTemplateValue(TemplateValueDef^ tvDef, IRNode^ node) {
        if (tvDef->Name == "Card") {
            if (tvDef->Type == "Cardinality") {
                if (node->Cardinality > 0) return node->Cardinality.ToString();
            }
            return tvDef->DefaultValue;
        }
        if (tvDef->Name == "SrcType" || tvDef->Name == "time_type" || tvDef->Name == "value_type") {
            if (node->DataType != nullptr && node->DataType->Length > 0) return node->DataType;
            return tvDef->DefaultValue;
        }
        return tvDef->DefaultValue;
    }

public:
    void Write(IRProgram^ program, XmlDocument^ doc, XmlElement^ partsElem, String^ ns) {
        PartSchemaRegistry^ reg = gcnew PartSchemaRegistry();
        int counterIdx = 0;
        int timerIdx = 0;

        for (int ai = 0; ai < program->Accesses->Count; ai++) {
            IRAccess^ a = program->Accesses[ai];
            XmlElement^ access = doc->CreateElement("Access", ns);
            access->SetAttribute("Scope", a->Scope);
            access->SetAttribute("UId", a->Uid.ToString());

            if (a->Scope == "TypedConstant") {
                XmlElement^ constant = doc->CreateElement("Constant", ns);
                XmlElement^ cv = doc->CreateElement("ConstantValue", ns);
                cv->InnerText = a->ConstantValue;
                constant->AppendChild(cv);
                access->AppendChild(constant);
            }
            else if (a->Scope == "LiteralConstant") {
                String^ constType = (a->ConstantType != nullptr && a->ConstantType->Length > 0) ? a->ConstantType : "DInt";
                XmlElement^ constant = doc->CreateElement("Constant", ns);
                XmlElement^ ct = doc->CreateElement("ConstantType", ns);
                ct->InnerText = constType;
                constant->AppendChild(ct);
                XmlElement^ cv = doc->CreateElement("ConstantValue", ns);
                cv->InnerText = a->ConstantValue;
                constant->AppendChild(cv);
                access->AppendChild(constant);
            }
            else if (a->Scope == "Label") {
            }
            else {
                XmlElement^ symbol = doc->CreateElement("Symbol", ns);
                XmlElement^ comp = doc->CreateElement("Component", ns);
                comp->SetAttribute("Name", a->Symbol);
                symbol->AppendChild(comp);
                access->AppendChild(symbol);
            }

            partsElem->AppendChild(access);
        }

        for (int ni = 0; ni < program->Nodes->Count; ni++) {
            IRNode^ node = program->Nodes[ni];

            CuPartType pt = reg->NameToType(node->Name);
            PartSchema^ schema = reg->GetSchema(pt);

            XmlElement^ part = doc->CreateElement("Part", ns);
            part->SetAttribute("Name", node->Name);
            part->SetAttribute("UId", node->Uid.ToString());

            if (schema != nullptr && schema->Version != nullptr && schema->Version->Length > 0) {
                part->SetAttribute("Version", schema->Version);
            }

            if (schema != nullptr && schema->RequiresInstance) {
                String^ instanceDB = node->InstanceDB;
                if (instanceDB == nullptr || instanceDB->Length == 0) {
                    if (schema->InstanceType == "IEC_COUNTER") {
                        instanceDB = "IEC_Counter_" + counterIdx + "_DB";
                        counterIdx++;
                    }
                    else {
                        instanceDB = "IEC_Timer_" + timerIdx + "_DB";
                        timerIdx++;
                    }
                }
                else {
                    if (schema->InstanceType == "IEC_COUNTER") {
                        counterIdx++;
                    }
                    else {
                        timerIdx++;
                    }
                }

                XmlElement^ instance = doc->CreateElement("Instance", ns);
                instance->SetAttribute("Scope", "GlobalVariable");
                int instUid = (node->InstanceUid > 0) ? node->InstanceUid : (node->Uid + 1);
                instance->SetAttribute("UId", instUid.ToString());
                XmlElement^ comp = doc->CreateElement("Component", ns);
                comp->SetAttribute("Name", instanceDB);
                instance->AppendChild(comp);
                part->AppendChild(instance);
            }

            if (node->Cardinality > 0 && node->Name == "O") {
                XmlElement^ tv = doc->CreateElement("TemplateValue", ns);
                tv->SetAttribute("Name", "Card");
                tv->SetAttribute("Type", "Cardinality");
                tv->InnerText = node->Cardinality.ToString();
                part->AppendChild(tv);
            }

            if (schema != nullptr && schema->TemplateValues != nullptr && schema->TemplateValues->Count > 0) {
                for (int ti = 0; ti < schema->TemplateValues->Count; ti++) {
                    TemplateValueDef^ tvDef = schema->TemplateValues[ti];
                    if (tvDef->IsAutomaticTyped) {
                        XmlElement^ atv = doc->CreateElement("AutomaticTyped", ns);
                        atv->SetAttribute("Name", tvDef->Name);
                        part->AppendChild(atv);
                    }
                    else {
                        XmlElement^ tv = doc->CreateElement("TemplateValue", ns);
                        tv->SetAttribute("Name", tvDef->Name);
                        tv->SetAttribute("Type", tvDef->Type);
                        tv->InnerText = ResolveTemplateValue(tvDef, node);
                        part->AppendChild(tv);
                    }
                }
            }

            if (schema != nullptr && schema->DisabledENO) {
                part->SetAttribute("DisabledENO", "true");
            }

            if (node->Negated && schema != nullptr && schema->SupportsNegation) {
                XmlElement^ neg = doc->CreateElement("Negated", ns);
                neg->SetAttribute("Name", "operand");
                part->AppendChild(neg);
            }

            partsElem->AppendChild(part);
        }
    }
};

ref class LADCallWriter {
private:
    String^ InferPinType(String^ pinName, CuPartType pt) {
        if (pinName == "IN" || pinName == "CU" || pinName == "CD" ||
            pinName == "R" || pinName == "LD" || pinName == "Q" ||
            pinName == "QU" || pinName == "QD") return "Bool";
        if (pinName == "PT" || pinName == "ET") return "Time";
        if (pinName == "PV" || pinName == "CV") return "Int";
        return "Bool";
    }

    void WriteFBCall(IRNode^ node, CuPartType pt, PartSchema^ schema,
        IRProgram^ program, XmlDocument^ doc, XmlElement^ partsElem,
        String^ ns, int% paramUidCounter) {
        XmlElement^ call = doc->CreateElement("Call", ns);
        call->SetAttribute("UId", node->Uid.ToString());
        call->SetAttribute("Type", "FB");

        XmlElement^ callInfo = doc->CreateElement("CallInfo", ns);
        callInfo->SetAttribute("Name", node->Name);
        callInfo->SetAttribute("BlockType", "FB");

        if (node->InstanceDB != nullptr && node->InstanceDB->Length > 0) {
            XmlElement^ instance = doc->CreateElement("Instance", ns);
            instance->SetAttribute("Scope", "GlobalVariable");

            int instUid = (node->InstanceUid > 0) ? node->InstanceUid : (node->Uid + 10000);
            instance->SetAttribute("UId", instUid.ToString());

            XmlElement^ comp = doc->CreateElement("Component", ns);
            comp->SetAttribute("Name", node->InstanceDB);
            instance->AppendChild(comp);
            callInfo->AppendChild(instance);
        }

        for (int pi = 0; pi < schema->InputPins->Count; pi++) {
            XmlElement^ param = doc->CreateElement("Parameter", ns);
            param->SetAttribute("UId", (paramUidCounter++).ToString());
            param->SetAttribute("Name", schema->InputPins[pi]->Name);
            param->SetAttribute("Section", "Input");
            param->SetAttribute("Type", InferPinType(schema->InputPins[pi]->Name, pt));
            callInfo->AppendChild(param);
        }
        for (int pi = 0; pi < schema->OutputPins->Count; pi++) {
            XmlElement^ param = doc->CreateElement("Parameter", ns);
            param->SetAttribute("UId", (paramUidCounter++).ToString());
            param->SetAttribute("Name", schema->OutputPins[pi]->Name);
            param->SetAttribute("Section", "Output");
            param->SetAttribute("Type", InferPinType(schema->OutputPins[pi]->Name, pt));
            callInfo->AppendChild(param);
        }

        call->AppendChild(callInfo);
        partsElem->AppendChild(call);
    }

public:
    void Write(IRProgram^ program, XmlDocument^ doc, XmlElement^ partsElem, String^ ns) {
        PartSchemaRegistry^ reg = gcnew PartSchemaRegistry();

        int paramUidCounter = 10000;

        for (int ni = 0; ni < program->Nodes->Count; ni++) {
            IRNode^ node = program->Nodes[ni];
            if (node->Kind != IRNodeKind::Call) continue;

            CuPartType pt = reg->NameToType(node->Name);
            PartSchema^ schema = reg->GetSchema(pt);

            if (schema != nullptr && schema->UseCallStructure) {
                WriteFBCall(node, pt, schema, program, doc, partsElem, ns, paramUidCounter);
            }
        }
    }
};

ref class LADWireWriter {
public:
    void Write(IRProgram^ program, XmlDocument^ doc, XmlElement^ wiresElem, String^ ns) {
        for (int wi = 0; wi < program->Wires->Count; wi++) {
            IRWire^ w = program->Wires[wi];
            XmlElement^ wire = doc->CreateElement("Wire", ns);
            wire->SetAttribute("UId", w->Uid.ToString());

            if (w->FromKind == "PowerRail") {
                XmlElement^ pr = doc->CreateElement("Powerrail", ns);
                wire->AppendChild(pr);
            }
            else if (w->FromKind == "Access") {
                XmlElement^ ic = doc->CreateElement("IdentCon", ns);
                ic->SetAttribute("UId", w->FromUid.ToString());
                wire->AppendChild(ic);
            }
            else if (w->FromKind == "OpenCon") {
                XmlElement^ oc = doc->CreateElement("OpenCon", ns);
                oc->SetAttribute("UId", w->FromUid.ToString());
                wire->AppendChild(oc);
            }
            else {
                XmlElement^ nc = doc->CreateElement("NameCon", ns);
                nc->SetAttribute("UId", w->FromUid.ToString());
                nc->SetAttribute("Name", w->FromPin);
                wire->AppendChild(nc);
            }

            if (w->ToKind == "OpenCon") {
                XmlElement^ oc = doc->CreateElement("OpenCon", ns);
                oc->SetAttribute("UId", w->ToUid.ToString());
                wire->AppendChild(oc);
            }
            else if (w->ToKind == "Access") {
                XmlElement^ ic2 = doc->CreateElement("IdentCon", ns);
                ic2->SetAttribute("UId", w->ToUid.ToString());
                wire->AppendChild(ic2);
            }
            else {
                XmlElement^ nc2 = doc->CreateElement("NameCon", ns);
                nc2->SetAttribute("UId", w->ToUid.ToString());
                nc2->SetAttribute("Name", w->ToPin);
                wire->AppendChild(nc2);
            }

            if (w->ExtraTargets != nullptr) {
                for (int ei = 0; ei < w->ExtraTargets->Count; ei++) {
                    IRWireTarget^ t = w->ExtraTargets[ei];
                    XmlElement^ xt = doc->CreateElement("NameCon", ns);
                    xt->SetAttribute("UId", t->Uid.ToString());
                    xt->SetAttribute("Name", t->Pin);
                    wire->AppendChild(xt);
                }
            }

            wiresElem->AppendChild(wire);
        }
    }
};

ref class LADBranchWriter {
public:
    void Write(IRProgram^ program, XmlDocument^ doc, XmlElement^ wiresElem, String^ ns) {
        for (int wi = 0; wi < program->Wires->Count; wi++) {
            IRWire^ w = program->Wires[wi];
            if (w->ExtraTargets == nullptr || w->ExtraTargets->Count == 0) continue;

            for (int ei = 0; ei < w->ExtraTargets->Count; ei++) {
                IRWireTarget^ t = w->ExtraTargets[ei];
            }
        }
    }
};

ref class LadXmlBackend : IBackend {
private:
    LADPartWriter^ partWriter;
    LADCallWriter^ callWriter;
    LADWireWriter^ wireWriter;
    LADBranchWriter^ branchWriter;

    void WriteParts(IRProgram^ program, XmlDocument^ doc, XmlElement^ partsElem, String^ ns) {
        partWriter->Write(program, doc, partsElem, ns);
        callWriter->Write(program, doc, partsElem, ns);
    }

    void WriteWires(IRProgram^ program, XmlDocument^ doc, XmlElement^ wiresElem, String^ ns) {
        wireWriter->Write(program, doc, wiresElem, ns);
        branchWriter->Write(program, doc, wiresElem, ns);
    }

public:
    LadXmlBackend() {
        partWriter = gcnew LADPartWriter();
        callWriter = gcnew LADCallWriter();
        wireWriter = gcnew LADWireWriter();
        branchWriter = gcnew LADBranchWriter();
    }

    virtual String^ Generate(IRProgram^ program) {
        String^ ns = SchemaVersionMapper::GetNamespace(18);
        if (program->XmlNs != nullptr && program->XmlNs->Length > 0)
            ns = program->XmlNs;

        XmlDocument^ doc = gcnew XmlDocument();
        XmlElement^ flgNet = doc->CreateElement("FlgNet", ns);
        doc->AppendChild(flgNet);

        XmlElement^ parts = doc->CreateElement("Parts", ns);
        flgNet->AppendChild(parts);
        WriteParts(program, doc, parts, ns);

        XmlElement^ wires = doc->CreateElement("Wires", ns);
        flgNet->AppendChild(wires);
        WriteWires(program, doc, wires, ns);

        XmlWriterSettings^ settings = gcnew XmlWriterSettings();
        settings->OmitXmlDeclaration = true;
        settings->Indent = true;
        settings->IndentChars = "  ";

        System::Text::StringBuilder^ sb = gcnew System::Text::StringBuilder();
        XmlWriter^ writer = XmlWriter::Create(sb, settings);
        doc->WriteTo(writer);
        writer->Close();

        return sb->ToString();
    }

    void GenerateIntoTemplate(IRProgram^ program, XmlDocument^ doc, XmlElement^ flgNetNode, String^ flgNs) {
        XmlElement^ partsElem = doc->CreateElement("Parts", flgNs);
        WriteParts(program, doc, partsElem, flgNs);
        flgNetNode->AppendChild(partsElem);

        XmlElement^ wiresElem = doc->CreateElement("Wires", flgNs);
        WriteWires(program, doc, wiresElem, flgNs);
        flgNetNode->AppendChild(wiresElem);
    }
};

inline GraphCompilerPipeline^ RunGraphCompiler(LogicGraph^ graph, UidAllocator^ uidAlloc) {
    GraphCompilerPipeline^ pipeline = gcnew GraphCompilerPipeline();
    pipeline->OptimizationLog = gcnew List<OptimizationResult^>();

    Console::WriteLine("  [GraphCompiler] Phase 1: Building CFG from LogicGraph...");
    pipeline->Cfg = BuildCfgFromLogicGraph(graph);
    Console::WriteLine("  [GraphCompiler] CFG nodes: {0}", pipeline->Cfg->Nodes->Count);

    Console::WriteLine("  [GraphCompiler] Phase 2: Optimizing CFG...");
    List<OptimizationResult^>^ optResults = OptimizeCfg(pipeline->Cfg);
    for each (OptimizationResult^ r in optResults) {
        pipeline->OptimizationLog->Add(r);
        Console::WriteLine("    {0}", r->Description);
    }

    Console::WriteLine("  [GraphCompiler] Phase 3: Generating IR from CFG...");
    pipeline->Ir = BuildIrFromCfg(pipeline->Cfg, uidAlloc);
    Console::WriteLine("  [GraphCompiler] IR: {0} parts, {1} accesses, {2} wires",
        pipeline->Ir->Parts->Count, pipeline->Ir->Accesses->Count, pipeline->Ir->Wires->Count);

    return pipeline;
}

inline CompileUnitIR^ BuildCompileUnitIR(LogicGraph^ graph, UidAllocator^ uidAlloc) {
    GraphCompilerPipeline^ pipeline = RunGraphCompiler(graph, uidAlloc);
    return pipeline->Ir;
}

inline void RenumberPartUids(CompileUnitIR^ ir, UidAllocator^ uidAlloc) {
    Dictionary<int, int>^ oldToNew = gcnew Dictionary<int, int>();

    for (int i = 0; i < ir->Accesses->Count; i++) {
        CuAccess^ a = ir->Accesses[i];
        int newUid = uidAlloc->Alloc();
        oldToNew[a->Uid] = newUid;
    }

    for (int i = 0; i < ir->Parts->Count; i++) {
        CuPart^ p = ir->Parts[i];
        int newUid = uidAlloc->Alloc();
        oldToNew[p->Uid] = newUid;

        if (p->InstanceUid > 0) {
            int newInstanceUid = uidAlloc->Alloc();
            oldToNew[p->InstanceUid] = newInstanceUid;
        }
    }

    for (int i = 0; i < ir->Wires->Count; i++) {
        CuWire^ w = ir->Wires[i];
        int newUid = uidAlloc->Alloc();
        oldToNew[w->Uid] = newUid;

        if (w->FromKind == "OpenCon" && w->FromUid > 0 && !oldToNew->ContainsKey(w->FromUid)) {
            int newOcUid = uidAlloc->Alloc();
            oldToNew[w->FromUid] = newOcUid;
        }
        if (w->ToKind == "OpenCon" && w->ToUid > 0 && !oldToNew->ContainsKey(w->ToUid)) {
            int newOcUid = uidAlloc->Alloc();
            oldToNew[w->ToUid] = newOcUid;
        }
    }

    for (int i = 0; i < ir->Accesses->Count; i++) {
        CuAccess^ a = ir->Accesses[i];
        if (oldToNew->ContainsKey(a->Uid)) a->Uid = oldToNew[a->Uid];
        if (oldToNew->ContainsKey(a->TargetPartUid)) a->TargetPartUid = oldToNew[a->TargetPartUid];
    }

    for (int i = 0; i < ir->Parts->Count; i++) {
        CuPart^ p = ir->Parts[i];
        if (oldToNew->ContainsKey(p->Uid)) p->Uid = oldToNew[p->Uid];
        if (p->InstanceUid > 0 && oldToNew->ContainsKey(p->InstanceUid)) p->InstanceUid = oldToNew[p->InstanceUid];
    }

    for (int i = 0; i < ir->Wires->Count; i++) {
        CuWire^ w = ir->Wires[i];
        if (oldToNew->ContainsKey(w->Uid)) w->Uid = oldToNew[w->Uid];
        if (w->FromKind == "Part" && oldToNew->ContainsKey(w->FromUid)) w->FromUid = oldToNew[w->FromUid];
        if (w->FromKind == "Access" && oldToNew->ContainsKey(w->FromUid)) w->FromUid = oldToNew[w->FromUid];
        if (w->FromKind == "OpenCon" && oldToNew->ContainsKey(w->FromUid)) w->FromUid = oldToNew[w->FromUid];
        if (w->ToKind == "Part" && oldToNew->ContainsKey(w->ToUid)) w->ToUid = oldToNew[w->ToUid];
        if (w->ToKind == "Access" && oldToNew->ContainsKey(w->ToUid)) w->ToUid = oldToNew[w->ToUid];
        if (w->ToKind == "OpenCon" && oldToNew->ContainsKey(w->ToUid)) w->ToUid = oldToNew[w->ToUid];
        if (w->ExtraTargets != nullptr) {
            for (int j = 0; j < w->ExtraTargets->Count; j++) {
                CuWireTarget^ t = w->ExtraTargets[j];
                if (oldToNew->ContainsKey(t->Uid)) t->Uid = oldToNew[t->Uid];
            }
        }
    }
}

inline String^ GenerateLadFlgNetXml(LadNetwork^ network, UidAllocator^ uidAlloc) {
    LogicGraph^ graph = BuildLogicGraph(network);
    CompileUnitIR^ ir = BuildCompileUnitIR(graph, uidAlloc);

    if (!IrHasOutput(ir)) {
        Console::WriteLine("    [GenerateLadFlgNetXml] Network has no output element, returning empty.");
        return "";
    }

    ValidateCompileUnitIR(ir);
    FixOutputToOutputWires(ir, uidAlloc);
    RenumberPartUids(ir, uidAlloc);

    IRProgram^ program = BuildIRProgram(ir);
    LadXmlBackend^ backend = gcnew LadXmlBackend();
    return backend->Generate(program);
}

inline String^ GetDefaultLadTemplateXml() {
    return "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<Document>"
        "<Engineering version=\"V20\" />"
        "<DocumentInfo>"
        "<Created>2026-01-01T00:00:00Z</Created>"
        "<ExportSetting>WithDefaults</ExportSetting>"
        "<InstalledProducts>"
        "<Product>"
        "<DisplayName>Totally Integrated Automation Portal</DisplayName>"
        "<DisplayVersion>V20</DisplayVersion>"
        "</Product>"
        "</InstalledProducts>"
        "</DocumentInfo>"
        "<SW.Blocks.OB ID=\"0\">"
        "<AttributeList>"
        "<AutoNumber>true</AutoNumber>"
        "<HeaderAuthor />"
        "<HeaderFamily />"
        "<HeaderName />"
        "<HeaderVersion>0.1</HeaderVersion>"
        "<Interface><Sections xmlns=\"http://www.siemens.com/automation/Openness/SW/Interface/v5\">"
        "<Section Name=\"Input\">"
        "<Member Name=\"Initial_Call\" Datatype=\"Bool\" Accessibility=\"Public\" Informative=\"true\" />"
        "<Member Name=\"Remanence\" Datatype=\"Bool\" Accessibility=\"Public\" Informative=\"true\" />"
        "</Section>"
        "<Section Name=\"Temp\" />"
        "<Section Name=\"Constant\" />"
        "</Sections></Interface>"
        "<IsIECCheckEnabled>false</IsIECCheckEnabled>"
        "<MemoryLayout>Optimized</MemoryLayout>"
        "<Name>Main</Name>"
        "<Namespace />"
        "<Number>1</Number>"
        "<ProgrammingLanguage>LAD</ProgrammingLanguage>"
        "<SecondaryType>ProgramCycle</SecondaryType>"
        "<SetENOAutomatically>false</SetENOAutomatically>"
        "</AttributeList>"
        "<ObjectList>"
        "<MultilingualText ID=\"1\" CompositionName=\"Comment\">"
        "<ObjectList>"
        "<MultilingualTextItem ID=\"2\" CompositionName=\"Items\">"
        "<AttributeList>"
        "<Culture>zh-CN</Culture>"
        "<Text />"
        "</AttributeList>"
        "</MultilingualTextItem>"
        "</ObjectList>"
        "</MultilingualText>"
        "<SW.Blocks.CompileUnit ID=\"3\" CompositionName=\"CompileUnits\">"
        "<AttributeList>"
        "<NetworkSource>"
        "<FlgNet xmlns=\"http://www.siemens.com/automation/Openness/SW/NetworkSource/FlgNet/v5\">"
        "<Parts>"
        "<Access Scope=\"GlobalVariable\" UId=\"21\">"
        "<Symbol>"
        "<Component Name=\"M0.0\" />"
        "</Symbol>"
        "</Access>"
        "<Part Name=\"Contact\" UId=\"22\" />"
        "<Part Name=\"Coil\" UId=\"23\" />"
        "</Parts>"
        "<Wires>"
        "<Wire UId=\"24\">"
        "<Powerrail />"
        "<NameCon UId=\"22\" Name=\"in\" />"
        "</Wire>"
        "<Wire UId=\"25\">"
        "<IdentCon UId=\"21\" />"
        "<NameCon UId=\"22\" Name=\"operand\" />"
        "</Wire>"
        "<Wire UId=\"26\">"
        "<NameCon UId=\"22\" Name=\"out\" />"
        "<NameCon UId=\"23\" Name=\"in\" />"
        "</Wire>"
        "<Wire UId=\"27\">"
        "<IdentCon UId=\"21\" />"
        "<NameCon UId=\"23\" Name=\"operand\" />"
        "</Wire>"
        "</Wires>"
        "</FlgNet>"
        "</NetworkSource>"
        "</AttributeList>"
        "<ObjectList>"
        "<MultilingualText ID=\"5\" CompositionName=\"Comment\">"
        "<ObjectList>"
        "<MultilingualTextItem ID=\"6\" CompositionName=\"Items\">"
        "<AttributeList>"
        "<Culture>zh-CN</Culture>"
        "<Text />"
        "</AttributeList>"
        "</MultilingualTextItem>"
        "</ObjectList>"
        "</MultilingualText>"
        "</ObjectList>"
        "</SW.Blocks.CompileUnit>"
        "</ObjectList>"
        "</SW.Blocks.OB>"
        "</Document>";
}

inline String^ BuildLadXml(LadDsl^ dsl, String^ templateXmlPath) {
    String^ xml = nullptr;
    if (templateXmlPath != nullptr && templateXmlPath->Length > 0 && File::Exists(templateXmlPath)) {
        xml = File::ReadAllText(templateXmlPath, gcnew System::Text::UTF8Encoding(false));
    }
    if (xml == nullptr || xml->Length == 0) {
        Console::WriteLine("    [BuildLadXml] Template file not found or empty, using built-in default template");
        xml = GetDefaultLadTemplateXml();
    }

    XmlDocument^ doc = gcnew XmlDocument();
    doc->LoadXml(xml);

    XmlNamespaceManager^ nsmgr = gcnew XmlNamespaceManager(doc->NameTable);
    nsmgr->AddNamespace("ns", "http://www.siemens.com/automation/Openness/SW/Blocks/v5");

    XmlNodeList^ compileUnits = doc->SelectNodes("//ns:SW.Blocks.CompileUnit", nsmgr);
    if (compileUnits->Count == 0) {
        compileUnits = doc->GetElementsByTagName("SW.Blocks.CompileUnit");
    }

    XmlNode^ parentOfCU = nullptr;
    XmlNode^ templateCUNode = nullptr;

    if (compileUnits->Count > 0) {
        templateCUNode = compileUnits[0];
        parentOfCU = templateCUNode->ParentNode;
    }

    if (parentOfCU == nullptr || templateCUNode == nullptr) {
        Console::WriteLine("    Error: Could not find CompileUnit in template XML");
        return nullptr;
    }

    int maxExistingId = 0;
    System::Text::RegularExpressions::Regex^ idRegex =
        gcnew System::Text::RegularExpressions::Regex("ID=\"([0-9A-Fa-f]+)\"");
    for each (System::Text::RegularExpressions::Match^ m in idRegex->Matches(xml)) {
        int val = System::Convert::ToInt32(m->Groups[1]->Value, 16);
        if (val > maxExistingId) maxExistingId = val;
    }

    List<XmlNode^>^ existingCUs = gcnew List<XmlNode^>();
    for each (XmlNode^ cu in compileUnits) {
        existingCUs->Add(cu);
    }
    for each (XmlNode^ cu in existingCUs) {
        parentOfCU->RemoveChild(cu);
    }

    int nextCUBaseId = maxExistingId + 1;

    Console::WriteLine("    [BuildLadXml] DSL Networks: {0}, Template CUs found: {1}", dsl->Networks->Count, compileUnits->Count);

    for (int i = 0; i < dsl->Networks->Count; i++) {
        LadNetwork^ net = dsl->Networks[i];
        Console::WriteLine("    [BuildLadXml] Network {0}: {1} elements", i + 1, net->Elements->Count);
        if (net->Elements->Count < 1) {
            Console::WriteLine("    [BuildLadXml] Network {0} skipped: no elements", i + 1);
            continue;
        }

        UidAllocator^ netUidAlloc = gcnew UidAllocator();
        netUidAlloc->ForceStartFrom(21);

        XmlNode^ newCU = templateCUNode->CloneNode(true);

        int cuId = nextCUBaseId;
        nextCUBaseId += 5;
        String^ idCU = System::String::Format("{0:X}", cuId);
        String^ idCM = System::String::Format("{0:X}", cuId + 1);
        String^ idCI = System::String::Format("{0:X}", cuId + 2);
        String^ idTM = System::String::Format("{0:X}", cuId + 3);
        String^ idTI = System::String::Format("{0:X}", cuId + 4);

        for each (XmlAttribute^ attr in newCU->Attributes) {
            if (attr->Name == "ID") attr->Value = idCU;
        }

        XmlNodeList^ idAttrs = newCU->SelectNodes("//*[@ID]");
        List<XmlElement^>^ idElems = gcnew List<XmlElement^>();
        for each (XmlNode^ node in idAttrs) {
            XmlElement^ elem = dynamic_cast<XmlElement^>(node);
            if (elem != nullptr) idElems->Add(elem);
        }
        int subIdx = 0;
        array<String^>^ newSubIds = gcnew array<String^> { idCM, idCI, idTM, idTI };
        for each (XmlElement^ elem in idElems) {
            if (elem->GetAttribute("ID") != idCU) {
                if (subIdx < 4) {
                    elem->SetAttribute("ID", newSubIds[subIdx]);
                    subIdx++;
                }
            }
        }

        XmlNode^ flgNetNode = nullptr;
        for each (XmlNode^ child in newCU->ChildNodes) {
            if (child->LocalName == "AttributeList") {
                for each (XmlNode^ attrChild in child->ChildNodes) {
                    if (attrChild->LocalName == "NetworkSource") {
                        for each (XmlNode^ nsChild in attrChild->ChildNodes) {
                            if (nsChild->LocalName == "FlgNet") {
                                flgNetNode = nsChild;
                                break;
                            }
                        }
                        break;
                    }
                }
                break;
            }
        }

        if (flgNetNode != nullptr) {
            List<XmlNode^>^ toRemove = gcnew List<XmlNode^>();
            for each (XmlNode^ fc in flgNetNode->ChildNodes) {
                if (fc->LocalName == "Parts" || fc->LocalName == "Wires") {
                    toRemove->Add(fc);
                }
            }
            for each (XmlNode^ fc in toRemove) {
                flgNetNode->RemoveChild(fc);
            }

            CompileUnitIR^ ir = BuildCompileUnitIR(BuildLogicGraph(net), netUidAlloc);

            if (!IrHasOutput(ir)) {
                Console::WriteLine("    [BuildLadXml] Network {0} has no output element, skipping.", i + 1);
                continue;
            }

            Console::WriteLine("    [BuildLadXml] Network {0}: IR generated OK, appending CompileUnit", i + 1);

            ValidateCompileUnitIR(ir);
            FixOutputToOutputWires(ir, netUidAlloc);

            RenumberPartUids(ir, netUidAlloc);

            IRProgram^ program = BuildIRProgram(ir);
            LadXmlBackend^ backend = gcnew LadXmlBackend();
            XmlElement^ flgNetElem = dynamic_cast<XmlElement^>(flgNetNode);
            String^ flgNs = (flgNetElem != nullptr && flgNetElem->NamespaceURI != nullptr && flgNetElem->NamespaceURI->Length > 0)
                ? flgNetElem->NamespaceURI
                : SchemaVersionMapper::GetNamespace(18);
            backend->GenerateIntoTemplate(program, doc, flgNetElem, flgNs);
        }

        parentOfCU->AppendChild(newCU);
    }

    XmlWriterSettings^ settings = gcnew XmlWriterSettings();
    settings->OmitXmlDeclaration = false;
    settings->Indent = true;
    settings->IndentChars = "  ";
    settings->Encoding = gcnew System::Text::UTF8Encoding(false);

    System::Text::StringBuilder^ sb = gcnew System::Text::StringBuilder();
    XmlWriter^ writer = XmlWriter::Create(sb, settings);
    doc->WriteTo(writer);
    writer->Close();

    return sb->ToString();
}

inline String^ GenerateLadXmlFromJson(String^ json, String^ templateXmlPath) {
    Console::WriteLine("  [AICodeGen] Parsing JSON DSL...");
    LadDsl^ dsl = JsonToLadDsl(json);
    if (dsl == nullptr || dsl->Networks == nullptr || dsl->Networks->Count == 0) {
        Console::WriteLine("  [AICodeGen] Error: Invalid or empty JSON DSL");
        return nullptr;
    }
    Console::WriteLine("  [AICodeGen] Parsed {0} networks from JSON", dsl->Networks->Count);

    Console::WriteLine("  [AICodeGen] Generating LAD XML...");
    String^ xml = BuildLadXml(dsl, templateXmlPath);
    if (xml == nullptr) {
        Console::WriteLine("  [AICodeGen] Error: Failed to generate LAD XML");
        return nullptr;
    }
    Console::WriteLine("  [AICodeGen] LAD XML generated successfully ({0} chars)", xml->Length);

    return xml;
}

inline String^ GenerateLadXmlFromJsonFile(String^ jsonPath, String^ templateXmlPath) {
    if (!File::Exists(jsonPath)) {
        Console::WriteLine("  [AICodeGen] Error: JSON file not found: " + jsonPath);
        return nullptr;
    }
    String^ json = File::ReadAllText(jsonPath, gcnew System::Text::UTF8Encoding(false));
    return GenerateLadXmlFromJson(json, templateXmlPath);
}
