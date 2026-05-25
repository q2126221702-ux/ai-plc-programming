#pragma once
#include "DataTypes.h"
#include "JsonSerializer.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Text;
using namespace System::Text::RegularExpressions;

ref struct P3Variable {
    String^ Name;
    String^ Type;
    String^ Scope;
    String^ Comment;
    String^ TimerType;
    String^ CounterType;
    String^ Preset;
};

ref struct P3Node {
    String^ NodeType;
    String^ Tag;
    bool NormallyOpen;
    String^ Instance;
    String^ Pt;
    String^ Pv;
    String^ Tag2;
    String^ Tag3;
    String^ DataType;
    String^ Label;
};

ref struct P3Branch {
    List<P3Node^>^ Nodes;
};

ref struct P3Parallel {
    List<P3Branch^>^ Branches;
};

ref struct P3Network {
    String^ Title;
    List<Object^>^ Items;
};

ref struct P3Step {
    String^ Name;
    String^ Action;
    String^ Transition;
    String^ NextStep;
};

ref struct P3TimerDecl {
    String^ Name;
    String^ TimerType;
    String^ Preset;
    String^ Comment;
};

ref struct P3CounterDecl {
    String^ Name;
    String^ CounterType;
    String^ Preset;
    String^ Comment;
};

ref struct P3Dsl {
    String^ DslVersion;
    List<P3Variable^>^ Variables;
    List<P3Network^>^ Networks;
    List<P3Step^>^ Steps;
    List<P3TimerDecl^>^ Timers;
    List<P3CounterDecl^>^ Counters;
    bool AiFallbackNeeded;

    P3Dsl() {
        AiFallbackNeeded = false;
    }
};

ref class P3DslParser {
public:
    static String^ ExtractString(String^ json, String^ key) {
        String^ pattern = "\"" + Regex::Escape(key) + "\"\\s*:\\s*\"((?:[^\"\\\\]|\\\\.)*)\"";
        Match^ m = Regex::Match(json, pattern);
        if (m->Success) {
            String^ val = m->Groups[1]->Value;
            val = val->Replace("\\\"", "\"")->Replace("\\\\", "\\")->Replace("\\n", "\n")->Replace("\\r", "\r")->Replace("\\t", "\t");
            return val;
        }
        String^ boolPattern = "\"" + Regex::Escape(key) + "\"\\s*:\\s*(true|false)";
        Match^ bm = Regex::Match(json, boolPattern);
        if (bm->Success) return bm->Groups[1]->Value;
        String^ numPattern = "\"" + Regex::Escape(key) + "\"\\s*:\\s*(-?\\d+)";
        Match^ nm = Regex::Match(json, numPattern);
        if (nm->Success) return nm->Groups[1]->Value;
        return nullptr;
    }

    static int ExtractInt(String^ json, String^ key) {
        String^ pattern = "\"" + Regex::Escape(key) + "\"\\s*:\\s*(-?\\d+)";
        Match^ m = Regex::Match(json, pattern);
        if (!m->Success) return 0;
        int result;
        if (Int32::TryParse(m->Groups[1]->Value->Trim(), result)) return result;
        return 0;
    }

    static bool ExtractBool(String^ json, String^ key) {
        String^ pattern = "\"" + Regex::Escape(key) + "\"\\s*:\\s*(true|false)";
        Match^ m = Regex::Match(json, pattern);
        if (!m->Success) return false;
        return (m->Groups[1]->Value == "true");
    }

    static List<String^>^ SplitJsonArray(String^ json) {
        List<String^>^ items = gcnew List<String^>();
        if (json == nullptr || json->Length < 2 || json[0] != '[') return items;
        int depth = 0;
        int objStart = -1;
        bool inString = false;
        for (int i = 1; i < json->Length; i++) {
            wchar_t c = json[i];
            if (inString) {
                if (c == L'\\' && i + 1 < json->Length) {
                    i++;
                }
                else if (c == L'"') {
                    inString = false;
                }
            }
            else {
                if (c == L'"') {
                    inString = true;
                }
                else if (c == L'{') {
                    if (depth == 0) objStart = i;
                    depth++;
                }
                else if (c == L'}') {
                    depth--;
                    if (depth == 0 && objStart >= 0) {
                        items->Add(json->Substring(objStart, i - objStart + 1));
                        objStart = -1;
                    }
                }
            }
        }
        return items;
    }

    static String^ ExtractArray(String^ json, String^ key) {
        String^ pattern = "\"" + Regex::Escape(key) + "\"\\s*:\\s*\\[";
        Match^ m = Regex::Match(json, pattern);
        if (!m->Success) return nullptr;
        int arrStart = m->Index + m->Length - 1;
        int depth = 0;
        bool inString = false;
        for (int i = arrStart; i < json->Length; i++) {
            wchar_t c = json[i];
            if (inString) {
                if (c == L'\\' && i + 1 < json->Length) {
                    i++;
                }
                else if (c == L'"') {
                    inString = false;
                }
            }
            else {
                if (c == L'"') {
                    inString = true;
                }
                else if (c == L'[') {
                    depth++;
                }
                else if (c == L']') {
                    depth--;
                    if (depth == 0) return json->Substring(arrStart, i - arrStart + 1);
                }
            }
        }
        return nullptr;
    }

    static P3Variable^ ParseVariable(String^ json) {
        P3Variable^ v = gcnew P3Variable();
        v->Name = ExtractString(json, "name");
        v->Type = ExtractString(json, "type");
        v->Scope = ExtractString(json, "scope");
        v->Comment = ExtractString(json, "comment");
        v->TimerType = ExtractString(json, "timer_type");
        v->CounterType = ExtractString(json, "counter_type");
        v->Preset = ExtractString(json, "preset");
        if (v->Scope == nullptr) v->Scope = "internal";
        if (v->Type == nullptr) v->Type = "Bool";
        return v;
    }

    static P3Node^ ParseNode(String^ json) {
        P3Node^ n = gcnew P3Node();
        n->NodeType = ExtractString(json, "type");
        n->Tag = ExtractString(json, "tag");
        n->NormallyOpen = ExtractBool(json, "normallyOpen");
        if (!ExtractBool(json, "normally_open")) n->NormallyOpen = false;
        if (ExtractBool(json, "normally_open")) n->NormallyOpen = true;
        n->Instance = ExtractString(json, "instance");
        n->Pt = ExtractString(json, "pt");
        n->Pv = ExtractString(json, "pv");
        n->Tag2 = ExtractString(json, "tag2");
        n->Tag3 = ExtractString(json, "tag3");
        n->DataType = ExtractString(json, "data_type");
        if (n->DataType == nullptr) n->DataType = ExtractString(json, "dataType");
        n->Label = ExtractString(json, "label");
        if (n->NormallyOpen == false && !json->Contains("normallyOpen") && !json->Contains("normally_open")) {
            n->NormallyOpen = true;
        }
        return n;
    }

    static P3Parallel^ ParseParallel(String^ json) {
        P3Parallel^ p = gcnew P3Parallel();
        p->Branches = gcnew List<P3Branch^>();
        String^ branchesArr = ExtractArray(json, "branches");
        if (branchesArr == nullptr) return p;
        List<String^>^ branchStrs = SplitJsonArray(branchesArr);
        for each (String^ bs in branchStrs) {
            P3Branch^ branch = gcnew P3Branch();
            branch->Nodes = gcnew List<P3Node^>();
            String^ nodesArr = ExtractArray(bs, "nodes");
            if (nodesArr == nullptr) {
                List<String^>^ nodeStrs = SplitJsonArray(bs);
                for each (String^ ns in nodeStrs) {
                    branch->Nodes->Add(ParseNode(ns));
                }
            }
            else {
                List<String^>^ nodeStrs = SplitJsonArray(nodesArr);
                for each (String^ ns in nodeStrs) {
                    branch->Nodes->Add(ParseNode(ns));
                }
            }
            p->Branches->Add(branch);
        }
        return p;
    }

    static P3Network^ ParseNetwork(String^ json) {
        P3Network^ net = gcnew P3Network();
        net->Title = ExtractString(json, "title");
        if (net->Title == nullptr) net->Title = "";
        net->Items = gcnew List<Object^>();
        String^ itemsArr = ExtractArray(json, "nodes");
        if (itemsArr == nullptr) itemsArr = ExtractArray(json, "items");
        if (itemsArr == nullptr) itemsArr = ExtractArray(json, "elements");
        if (itemsArr != nullptr) {
            List<String^>^ itemStrs = SplitJsonArray(itemsArr);
            for each (String^ is2 in itemStrs) {
                if (is2->Contains("\"branches\"") || is2->Contains("\"type\":\"parallel\"") || is2->Contains("\"type\": \"parallel\"")) {
                    net->Items->Add(ParseParallel(is2));
                }
                else {
                    net->Items->Add(ParseNode(is2));
                }
            }
        }
        return net;
    }

public:
    static P3Dsl^ Parse(String^ json) {
        P3Dsl^ dsl = gcnew P3Dsl();
        dsl->DslVersion = ExtractString(json, "dsl_version");
        if (dsl->DslVersion == nullptr) dsl->DslVersion = "1.0";

        dsl->Variables = gcnew List<P3Variable^>();
        String^ varsArr = ExtractArray(json, "variables");
        if (varsArr != nullptr) {
            List<String^>^ varStrs = SplitJsonArray(varsArr);
            for each (String^ vs in varStrs) {
                dsl->Variables->Add(ParseVariable(vs));
            }
        }

        dsl->Networks = gcnew List<P3Network^>();
        String^ netsArr = ExtractArray(json, "networks");
        if (netsArr != nullptr) {
            List<String^>^ netStrs = SplitJsonArray(netsArr);
            for each (String^ ns in netStrs) {
                dsl->Networks->Add(ParseNetwork(ns));
            }
        }

        dsl->Steps = gcnew List<P3Step^>();
        String^ stepsArr = ExtractArray(json, "steps");
        if (stepsArr != nullptr) {
            List<String^>^ stepStrs = SplitJsonArray(stepsArr);
            for each (String^ ss in stepStrs) {
                P3Step^ step = gcnew P3Step();
                step->Name = ExtractString(ss, "name");
                step->Action = ExtractString(ss, "action");
                step->Transition = ExtractString(ss, "transition");
                step->NextStep = ExtractString(ss, "next_step");
                dsl->Steps->Add(step);
            }
        }

        dsl->Timers = gcnew List<P3TimerDecl^>();
        String^ timersArr = ExtractArray(json, "timers");
        if (timersArr != nullptr) {
            List<String^>^ timerStrs = SplitJsonArray(timersArr);
            for each (String^ ts in timerStrs) {
                P3TimerDecl^ td = gcnew P3TimerDecl();
                td->Name = ExtractString(ts, "name");
                td->TimerType = ExtractString(ts, "timer_type");
                td->Preset = ExtractString(ts, "preset");
                td->Comment = ExtractString(ts, "comment");
                dsl->Timers->Add(td);
            }
        }

        dsl->Counters = gcnew List<P3CounterDecl^>();
        String^ countersArr = ExtractArray(json, "counters");
        if (countersArr != nullptr) {
            List<String^>^ counterStrs = SplitJsonArray(countersArr);
            for each (String^ cs in counterStrs) {
                P3CounterDecl^ cd = gcnew P3CounterDecl();
                cd->Name = ExtractString(cs, "name");
                cd->CounterType = ExtractString(cs, "counter_type");
                cd->Preset = ExtractString(cs, "preset");
                cd->Comment = ExtractString(cs, "comment");
                dsl->Counters->Add(cd);
            }
        }

        return dsl;
    }

    static String^ ExtractJsonFromLlmResponse(String^ response) {
        if (response == nullptr || response->Trim()->Length == 0) return "";
        String^ s = response->Trim();

        String^ bestJson = nullptr;
        int bestLen = 0;

        for (int start = 0; start < s->Length; start++) {
            if (s[start] != L'{') continue;
            if (start > 0) {
                wchar_t prev = s[start - 1];
                if (prev != L' ' && prev != L'\n' && prev != L'\r' && prev != L'\t'
                    && prev != L'[' && prev != L',' && prev != L':') continue;
            }

            int depth = 0;
            bool inString = false;
            int end = -1;
            for (int i = start; i < s->Length; i++) {
                wchar_t c = s[i];
                if (inString) {
                    if (c == L'\\' && i + 1 < s->Length) {
                        i++;
                    }
                    else if (c == L'"') {
                        inString = false;
                    }
                }
                else {
                    if (c == L'"') {
                        inString = true;
                    }
                    else if (c == L'{') {
                        depth++;
                    }
                    else if (c == L'}') {
                        depth--;
                        if (depth == 0) { end = i; break; }
                    }
                }
            }
            if (end < 0) continue;

            String^ candidate = s->Substring(start, end - start + 1);
            bool hasStates = candidate->Contains("\"states\"");
            bool hasInitialState = candidate->Contains("\"initial_state\"");

            if (hasStates && candidate->Length > bestLen) {
                bestJson = candidate;
                bestLen = candidate->Length;
            }
            else if (hasInitialState && bestJson == nullptr) {
                bestJson = candidate;
                bestLen = candidate->Length;
            }
        }

        if (bestJson != nullptr) return bestJson;

        for (int start = 0; start < s->Length; start++) {
            if (s[start] != L'{') continue;
            int depth = 0;
            bool inString = false;
            int end = -1;
            for (int i = start; i < s->Length; i++) {
                wchar_t c = s[i];
                if (inString) {
                    if (c == L'\\' && i + 1 < s->Length) i++;
                    else if (c == L'"') inString = false;
                }
                else {
                    if (c == L'"') inString = true;
                    else if (c == L'{') depth++;
                    else if (c == L'}') { depth--; if (depth == 0) { end = i; break; } }
                }
            }
            if (end >= 0) return s->Substring(start, end - start + 1);
        }

        return "";
    }
};

ref class P3DslSerializer {
public:
    static String^ Serialize(P3Dsl^ dsl) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->Append("{\"dsl_version\":\"");
        sb->Append(dsl->DslVersion);
        sb->Append("\",\"variables\":[");
        for (int i = 0; i < dsl->Variables->Count; i++) {
            if (i > 0) sb->Append(",");
            P3Variable^ v = dsl->Variables[i];
            sb->Append("{\"name\":");
            sb->Append(EscapeJson(v->Name));
            sb->Append(",\"type\":");
            sb->Append(EscapeJson(v->Type));
            sb->Append(",\"scope\":");
            sb->Append(EscapeJson(v->Scope));
            if (v->Comment != nullptr && v->Comment->Length > 0) {
                sb->Append(",\"comment\":");
                sb->Append(EscapeJson(v->Comment));
            }
            if (v->TimerType != nullptr && v->TimerType->Length > 0) {
                sb->Append(",\"timer_type\":");
                sb->Append(EscapeJson(v->TimerType));
            }
            if (v->CounterType != nullptr && v->CounterType->Length > 0) {
                sb->Append(",\"counter_type\":");
                sb->Append(EscapeJson(v->CounterType));
            }
            if (v->Preset != nullptr && v->Preset->Length > 0) {
                sb->Append(",\"preset\":");
                sb->Append(EscapeJson(v->Preset));
            }
            sb->Append("}");
        }
        sb->Append("],\"networks\":[");
        for (int i = 0; i < dsl->Networks->Count; i++) {
            if (i > 0) sb->Append(",");
            P3Network^ net = dsl->Networks[i];
            sb->Append("{\"title\":");
            sb->Append(EscapeJson(net->Title));
            sb->Append(",\"nodes\":[");
            for (int j = 0; j < net->Items->Count; j++) {
                if (j > 0) sb->Append(",");
                Object^ item = net->Items[j];
                P3Node^ node = dynamic_cast<P3Node^>(item);
                P3Parallel^ par = dynamic_cast<P3Parallel^>(item);
                if (node != nullptr) {
                    sb->Append(SerializeNode(node));
                }
                else if (par != nullptr) {
                    sb->Append(SerializeParallel(par));
                }
            }
            sb->Append("]}");
        }

        if (dsl->Steps != nullptr && dsl->Steps->Count > 0) {
            sb->Append(",\"steps\":[");
            for (int i = 0; i < dsl->Steps->Count; i++) {
                if (i > 0) sb->Append(",");
                P3Step^ s = dsl->Steps[i];
                sb->Append("{\"name\":");
                sb->Append(EscapeJson(s->Name));
                if (s->Action != nullptr && s->Action->Length > 0) {
                    sb->Append(",\"action\":");
                    sb->Append(EscapeJson(s->Action));
                }
                if (s->Transition != nullptr && s->Transition->Length > 0) {
                    sb->Append(",\"transition\":");
                    sb->Append(EscapeJson(s->Transition));
                }
                if (s->NextStep != nullptr && s->NextStep->Length > 0) {
                    sb->Append(",\"next_step\":");
                    sb->Append(EscapeJson(s->NextStep));
                }
                sb->Append("}");
            }
            sb->Append("]");
        }

        if (dsl->Timers != nullptr && dsl->Timers->Count > 0) {
            sb->Append(",\"timers\":[");
            for (int i = 0; i < dsl->Timers->Count; i++) {
                if (i > 0) sb->Append(",");
                P3TimerDecl^ td = dsl->Timers[i];
                sb->Append("{\"name\":");
                sb->Append(EscapeJson(td->Name));
                if (td->TimerType != nullptr && td->TimerType->Length > 0) {
                    sb->Append(",\"timer_type\":");
                    sb->Append(EscapeJson(td->TimerType));
                }
                if (td->Preset != nullptr && td->Preset->Length > 0) {
                    sb->Append(",\"preset\":");
                    sb->Append(EscapeJson(td->Preset));
                }
                if (td->Comment != nullptr && td->Comment->Length > 0) {
                    sb->Append(",\"comment\":");
                    sb->Append(EscapeJson(td->Comment));
                }
                sb->Append("}");
            }
            sb->Append("]");
        }

        if (dsl->Counters != nullptr && dsl->Counters->Count > 0) {
            sb->Append(",\"counters\":[");
            for (int i = 0; i < dsl->Counters->Count; i++) {
                if (i > 0) sb->Append(",");
                P3CounterDecl^ cd = dsl->Counters[i];
                sb->Append("{\"name\":");
                sb->Append(EscapeJson(cd->Name));
                if (cd->CounterType != nullptr && cd->CounterType->Length > 0) {
                    sb->Append(",\"counter_type\":");
                    sb->Append(EscapeJson(cd->CounterType));
                }
                if (cd->Preset != nullptr && cd->Preset->Length > 0) {
                    sb->Append(",\"preset\":");
                    sb->Append(EscapeJson(cd->Preset));
                }
                if (cd->Comment != nullptr && cd->Comment->Length > 0) {
                    sb->Append(",\"comment\":");
                    sb->Append(EscapeJson(cd->Comment));
                }
                sb->Append("}");
            }
            sb->Append("]");
        }

        sb->Append("}");
        return sb->ToString();
    }

private:
    static String^ SerializeNode(P3Node^ n) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->Append("{\"type\":");
        sb->Append(EscapeJson(n->NodeType));
        if (n->Tag != nullptr && n->Tag->Length > 0) {
            sb->Append(",\"tag\":");
            sb->Append(EscapeJson(n->Tag));
        }
        if (n->NodeType == "contact" || n->NodeType == "negated_contact") {
            sb->Append(",\"normallyOpen\":");
            sb->Append(n->NormallyOpen ? "true" : "false");
        }
        if (n->Instance != nullptr && n->Instance->Length > 0) {
            sb->Append(",\"instance\":");
            sb->Append(EscapeJson(n->Instance));
        }
        if (n->Pt != nullptr && n->Pt->Length > 0) {
            sb->Append(",\"pt\":");
            sb->Append(EscapeJson(n->Pt));
        }
        if (n->Pv != nullptr && n->Pv->Length > 0) {
            sb->Append(",\"pv\":");
            sb->Append(EscapeJson(n->Pv));
        }
        if (n->Tag2 != nullptr && n->Tag2->Length > 0) {
            sb->Append(",\"tag2\":");
            sb->Append(EscapeJson(n->Tag2));
        }
        if (n->Tag3 != nullptr && n->Tag3->Length > 0) {
            sb->Append(",\"tag3\":");
            sb->Append(EscapeJson(n->Tag3));
        }
        if (n->DataType != nullptr && n->DataType->Length > 0) {
            sb->Append(",\"data_type\":");
            sb->Append(EscapeJson(n->DataType));
        }
        if (n->Label != nullptr && n->Label->Length > 0) {
            sb->Append(",\"label\":");
            sb->Append(EscapeJson(n->Label));
        }
        sb->Append("}");
        return sb->ToString();
    }

    static String^ SerializeParallel(P3Parallel^ p) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->Append("{\"type\":\"parallel\",\"branches\":[");
        for (int i = 0; i < p->Branches->Count; i++) {
            if (i > 0) sb->Append(",");
            P3Branch^ branch = p->Branches[i];
            sb->Append("{\"nodes\":[");
            for (int j = 0; j < branch->Nodes->Count; j++) {
                if (j > 0) sb->Append(",");
                sb->Append(SerializeNode(branch->Nodes[j]));
            }
            sb->Append("]}");
        }
        sb->Append("]}");
        return sb->ToString();
    }
};

ref class P3DslConverter {
public:
    static LadDsl^ ToLadDsl(P3Dsl^ p3) {
        LadDsl^ lad = gcnew LadDsl();
        lad->Networks = gcnew List<LadNetwork^>();
        for (int i = 0; i < p3->Networks->Count; i++) {
            P3Network^ p3net = p3->Networks[i];
            LadNetwork^ ladNet = gcnew LadNetwork();
            ladNet->Number = i + 1;
            ladNet->Title = p3net->Title;
            ladNet->Elements = gcnew List<LadElement^>();
            for each (Object^ item in p3net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                P3Parallel^ par = dynamic_cast<P3Parallel^>(item);
                if (node != nullptr) {
                    LadElement^ el = ConvertNode(node);
                    if (el != nullptr) ladNet->Elements->Add(el);
                }
                else if (par != nullptr) {
                    LadElement^ el = gcnew LadElement();
                    el->Type = "parallel";
                    el->Branches = gcnew List<List<LadElement^>^>();
                    for each (P3Branch^ branch in par->Branches) {
                        List<LadElement^>^ ladBranch = gcnew List<LadElement^>();
                        for each (P3Node^ bn in branch->Nodes) {
                            LadElement^ bel = ConvertNode(bn);
                            if (bel != nullptr) ladBranch->Add(bel);
                        }
                        el->Branches->Add(ladBranch);
                    }
                    ladNet->Elements->Add(el);
                }
            }
            lad->Networks->Add(ladNet);
        }
        return lad;
    }

private:
    static LadElement^ ConvertNode(P3Node^ n) {
        LadElement^ el = gcnew LadElement();
        String^ t = n->NodeType->ToLower()->Trim();

        if (t == "contact" || t == "negated_contact") {
            el->Type = "contact";
            el->Tag = n->Tag;
            el->NormallyOpen = n->NormallyOpen;
            if (t == "negated_contact") el->NormallyOpen = false;
        }
        else if (t == "coil") {
            el->Type = "coil";
            el->Tag = n->Tag;
        }
        else if (t == "set" || t == "setcoil" || t == "s_coil" || t == "scoil") {
            el->Type = "setCoil";
            el->Tag = n->Tag;
        }
        else if (t == "reset" || t == "resetcoil" || t == "r_coil" || t == "rcoil") {
            el->Type = "resetCoil";
            el->Tag = n->Tag;
        }
        else if (t == "ton" || t == "timerondelay" || t == "timer_on_delay") {
            el->Type = "timerOnDelay";
            el->InstanceName = n->Instance;
            el->PresetTime = n->Pt;
            if (el->PresetTime == nullptr || el->PresetTime->Length == 0) el->PresetTime = n->Pv;
        }
        else if (t == "tof" || t == "timeroffdelay" || t == "timer_off_delay") {
            el->Type = "timerOffDelay";
            el->InstanceName = n->Instance;
            el->PresetTime = n->Pt;
            if (el->PresetTime == nullptr || el->PresetTime->Length == 0) el->PresetTime = n->Pv;
        }
        else if (t == "tp" || t == "timerpulse" || t == "timer_pulse") {
            el->Type = "timerPulse";
            el->InstanceName = n->Instance;
            el->PresetTime = n->Pt;
            if (el->PresetTime == nullptr || el->PresetTime->Length == 0) el->PresetTime = n->Pv;
        }
        else if (t == "ctu" || t == "counterup" || t == "counter_up") {
            el->Type = "counterUp";
            el->InstanceName = n->Instance;
            el->PresetTime = n->Pv;
            if (el->PresetTime == nullptr || el->PresetTime->Length == 0) el->PresetTime = n->Pt;
        }
        else if (t == "ctd" || t == "counterdown" || t == "counter_down") {
            el->Type = "counterDown";
            el->InstanceName = n->Instance;
            el->PresetTime = n->Pv;
            if (el->PresetTime == nullptr || el->PresetTime->Length == 0) el->PresetTime = n->Pt;
        }
        else if (t == "ctud" || t == "counterupdown" || t == "counter_up_down") {
            el->Type = "counterUpDown";
            el->InstanceName = n->Instance;
            el->PresetTime = n->Pv;
            if (el->PresetTime == nullptr || el->PresetTime->Length == 0) el->PresetTime = n->Pt;
        }
        else if (t == "compareeq" || t == "eq" || t == "compare_eq") {
            el->Type = "compareEQ";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
            el->DataType = n->DataType;
        }
        else if (t == "comparene" || t == "ne" || t == "compare_ne") {
            el->Type = "compareNE";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
            el->DataType = n->DataType;
        }
        else if (t == "comparegt" || t == "gt" || t == "compare_gt") {
            el->Type = "compareGT";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
            el->DataType = n->DataType;
        }
        else if (t == "comparelt" || t == "lt" || t == "compare_lt") {
            el->Type = "compareLT";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
            el->DataType = n->DataType;
        }
        else if (t == "comparege" || t == "ge" || t == "compare_ge") {
            el->Type = "compareGE";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
            el->DataType = n->DataType;
        }
        else if (t == "comparele" || t == "le" || t == "compare_le") {
            el->Type = "compareLE";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
            el->DataType = n->DataType;
        }
        else if (t == "add") {
            el->Type = "add";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
            el->Tag3 = n->Tag3;
            el->DataType = n->DataType;
        }
        else if (t == "sub") {
            el->Type = "sub";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
            el->Tag3 = n->Tag3;
            el->DataType = n->DataType;
        }
        else if (t == "mul") {
            el->Type = "mul";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
            el->Tag3 = n->Tag3;
            el->DataType = n->DataType;
        }
        else if (t == "div") {
            el->Type = "div";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
            el->Tag3 = n->Tag3;
            el->DataType = n->DataType;
        }
        else if (t == "mod") {
            el->Type = "mod";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
            el->Tag3 = n->Tag3;
            el->DataType = n->DataType;
        }
        else if (t == "move") {
            el->Type = "move";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
            el->DataType = n->DataType;
        }
        else if (t == "risingedge" || t == "rising_edge" || t == "pcontact") {
            el->Type = "risingEdgeContact";
            el->Tag = n->Tag;
        }
        else if (t == "fallingedge" || t == "falling_edge" || t == "ncontact") {
            el->Type = "fallingEdgeContact";
            el->Tag = n->Tag;
        }
        else if (t == "jmp" || t == "jump") {
            el->Type = "jmp";
            el->Tag = n->Label != nullptr ? n->Label : n->Tag;
        }
        else if (t == "label") {
            el->Type = "label";
            el->Tag = n->Label != nullptr ? n->Label : n->Tag;
        }
        else if (t == "step") {
            el->Type = "nop";
            el->Tag = n->Tag;
        }
        else if (t == "transition") {
            el->Type = "nop";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
        }
        else if (t == "ret" || t == "return") {
            el->Type = "ret";
        }
        else if (t == "nop") {
            el->Type = "nop";
        }
        else if (t == "resetbitfield" || t == "r_bitfield" || t == "rbitfield") {
            el->Type = "resetBitfield";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
        }
        else if (t == "setbitfield" || t == "s_bitfield" || t == "sbitfield") {
            el->Type = "setBitfield";
            el->Tag = n->Tag;
            el->Tag2 = n->Tag2;
        }
        else {
            return nullptr;
        }
        return el;
    }
};

ref struct P4SignalDecl {
    String^ Name;
    String^ Comment;
};

ref struct P4TimerRequirement {
    String^ Name;
    String^ Preset;
};

ref struct P4CounterRequirement {
    String^ Name;
    String^ Preset;
};

ref struct P4Requirement {
    List<P4SignalDecl^>^ Inputs;
    List<P4SignalDecl^>^ Outputs;
    List<P4TimerRequirement^>^ Timers;
    List<P4CounterRequirement^>^ Counters;
    String^ ControlType;
    String^ Description;
    List<String^>^ ControlPatterns;
    List<String^>^ InterlockPairs;
    List<String^>^ SequentialOrder;

    P4Requirement() {
        Inputs = gcnew List<P4SignalDecl^>();
        Outputs = gcnew List<P4SignalDecl^>();
        Timers = gcnew List<P4TimerRequirement^>();
        Counters = gcnew List<P4CounterRequirement^>();
        ControlType = "";
        Description = "";
        ControlPatterns = gcnew List<String^>();
        InterlockPairs = gcnew List<String^>();
        SequentialOrder = gcnew List<String^>();
    }
};

ref struct P4State {
    String^ Name;
    String^ Action;
    String^ TransitionCondition;
    String^ NextState;
    String^ Comment;

    P4State() {
        Name = "";
        Action = "";
        TransitionCondition = "";
        NextState = "";
        Comment = "";
    }
};

ref struct P4SemanticPlan {
    List<P4State^>^ States;
    String^ InitialState;
    bool IsSequential;
    bool IsCyclic;
    int CycleCount;
    bool HasInterlock;
    List<String^>^ InterlockPairs;
    String^ FlowDescription;
    bool HasSelfHold;
    List<String^>^ SelfHoldOutputs;
    bool HasStarDelta;
    bool HasLimitSwitch;
    List<String^>^ LimitOutputs;
    bool HasAutoRoundTrip;
    bool HasTimerDelay;
    bool HasCounter;

    P4SemanticPlan() {
        States = gcnew List<P4State^>();
        InitialState = "";
        IsSequential = false;
        IsCyclic = false;
        CycleCount = 0;
        HasInterlock = false;
        InterlockPairs = gcnew List<String^>();
        FlowDescription = "";
        HasSelfHold = false;
        SelfHoldOutputs = gcnew List<String^>();
        HasStarDelta = false;
        HasLimitSwitch = false;
        LimitOutputs = gcnew List<String^>();
        HasAutoRoundTrip = false;
        HasTimerDelay = false;
        HasCounter = false;
    }
};

ref struct P4VariablePlan {
    List<P3Variable^>^ Variables;
    Dictionary<String^, String^>^ AddressMap;

    P4VariablePlan() {
        Variables = gcnew List<P3Variable^>();
        AddressMap = gcnew Dictionary<String^, String^>();
    }
};
