#pragma once
#include "AiPipeline.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::IO;
using namespace System::Text;

ref class P6ControlReasoningEngine {
public:
    static List<P6ControlAction^>^ ReasonSequentialControl(List<String^>^ steps) {
        P5CompilerLog::Info("REASON", "Reasoning sequential control from " + steps->Count + " steps");
        List<P6ControlAction^>^ actions = gcnew List<P6ControlAction^>();
        for (int i = 0; i < steps->Count; i++) {
            P6ControlAction^ act = gcnew P6ControlAction();
            act->Type = "sequential";
            act->Target = steps[i];
            act->Condition = (i > 0) ? steps[i - 1] + "_done" : "Start";
            act->Parameter = "";
            actions->Add(act);
        }
        return actions;
    }

    static List<P6SafetyRule^>^ ReasonInterlock(List<String^>^ forwardTags, List<String^>^ reverseTags) {
        P5CompilerLog::Info("REASON", "Reasoning interlock rules");
        List<P6SafetyRule^>^ rules = gcnew List<P6SafetyRule^>();
        for each (String^ fwd in forwardTags) {
            for each (String^ rev in reverseTags) {
                P6SafetyRule^ rule = gcnew P6SafetyRule();
                rule->RuleType = "interlock";
                rule->Description = fwd + " and " + rev + " must not run simultaneously";
                rule->TargetTag = fwd + "," + rev;
                rule->Applied = false;
                rules->Add(rule);
            }
        }
        return rules;
    }

    static List<P6SafetyRule^>^ ReasonSafety(P3Dsl^ dsl) {
        P5CompilerLog::Info("REASON", "Reasoning safety rules");
        List<P6SafetyRule^>^ rules = gcnew List<P6SafetyRule^>();

        P6SafetyRule^ estop = gcnew P6SafetyRule();
        estop->RuleType = "emergency_stop";
        estop->Description = "All outputs must be OFF when E-Stop is active";
        estop->TargetTag = "E_Stop";
        estop->Applied = false;
        rules->Add(estop);

        P6SafetyRule^ faultStop = gcnew P6SafetyRule();
        faultStop->RuleType = "fault_stop";
        faultStop->Description = "All motion must stop on fault";
        faultStop->TargetTag = "Fault";
        faultStop->Applied = false;
        rules->Add(faultStop);

        P6SafetyRule^ timeout = gcnew P6SafetyRule();
        timeout->RuleType = "timeout_protection";
        timeout->Description = "Each motion must have timeout protection";
        timeout->TargetTag = "";
        timeout->Applied = false;
        rules->Add(timeout);

        P6SafetyRule^ interlock = gcnew P6SafetyRule();
        interlock->RuleType = "interlock";
        interlock->Description = "Forward and reverse outputs must be interlocked";
        interlock->TargetTag = "Fwd,Rev";
        interlock->Applied = false;
        rules->Add(interlock);

        P6SafetyRule^ autoReset = gcnew P6SafetyRule();
        autoReset->RuleType = "auto_reset";
        autoReset->Description = "All SET outputs must have corresponding RESET logic";
        autoReset->TargetTag = "";
        autoReset->Applied = false;
        rules->Add(autoReset);

        return rules;
    }

    static List<P6ControlAction^>^ ReasonTimeLogic(P3Dsl^ dsl) {
        P5CompilerLog::Info("REASON", "Reasoning time logic");
        List<P6ControlAction^>^ actions = gcnew List<P6ControlAction^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "ton" || t == "tof" || t == "tp") {
                        P6ControlAction^ act = gcnew P6ControlAction();
                        act->Type = "timer_delay";
                        act->Target = (node->Instance != nullptr) ? node->Instance : "T1";
                        act->Condition = node->Tag;
                        act->Parameter = (node->Pt != nullptr) ? node->Pt : "T#1S";
                        actions->Add(act);
                    }
                    else if (t == "keep" || t == "sr" || t == "rs") {
                        P6ControlAction^ act = gcnew P6ControlAction();
                        act->Type = "timer_keep";
                        act->Target = (node->Tag != nullptr) ? node->Tag : "Keep1";
                        act->Condition = node->Tag;
                        act->Parameter = "KEEP";
                        actions->Add(act);
                    }
                    else if (t == "pls" || t == "plf" || t == "pulse") {
                        P6ControlAction^ act = gcnew P6ControlAction();
                        act->Type = "timer_pulse";
                        act->Target = (node->Instance != nullptr) ? node->Instance : "PLS1";
                        act->Condition = node->Tag;
                        act->Parameter = "PULSE";
                        actions->Add(act);
                    }
                    else if (t == "flash" || t == "blink") {
                        P6ControlAction^ act = gcnew P6ControlAction();
                        act->Type = "timer_flash";
                        act->Target = (node->Instance != nullptr) ? node->Instance : "Flash1";
                        act->Condition = node->Tag;
                        act->Parameter = "FLASH";
                        actions->Add(act);
                    }
                    else if (t == "cycle" || t == "repeat") {
                        P6ControlAction^ act = gcnew P6ControlAction();
                        act->Type = "timer_cycle";
                        act->Target = (node->Instance != nullptr) ? node->Instance : "Cycle1";
                        act->Condition = node->Tag;
                        act->Parameter = "CYCLE";
                        actions->Add(act);
                    }
                }
            }
        }
        return actions;
    }

    static List<P4State^>^ ReasonStateMachine(List<String^>^ steps) {
        P5CompilerLog::Info("REASON", "Reasoning state machine from " + steps->Count + " steps");
        List<P4State^>^ states = gcnew List<P4State^>();
        for (int i = 0; i < steps->Count; i++) {
            P4State^ state = gcnew P4State();
            state->Name = "Step" + (i + 1) + "_" + steps[i];
            state->TransitionCondition = (i > 0) ? steps[i - 1] + "_sensor" : "Start_Btn";
            state->Action = steps[i];
            state->NextState = (i < steps->Count - 1) ? "Step" + (i + 2) + "_" + steps[i + 1] : "Idle";
            states->Add(state);
        }
        return states;
    }
};

ref class P6TaskPlanner {
public:
    static List<P6Task^>^ Plan(String^ problem) {
        P5CompilerLog::Info("TASK", "Planning tasks for: " + problem);
        List<P6Task^>^ tasks = gcnew List<P6Task^>();

        array<P6TaskType>^ defaultOrder = {
            P6TaskType::AnalyzeDevices,
            P6TaskType::ExtractIO,
            P6TaskType::DeriveActions,
            P6TaskType::GenerateStateMachine,
            P6TaskType::GenerateProgram,
            P6TaskType::VerifyInterlock,
            P6TaskType::GenerateAlarm,
            P6TaskType::GenerateHMIVars,
            P6TaskType::ExportProject
        };

        array<String^>^ descriptions = {
            "Analyze devices and equipment",
            "Extract IO variables",
            "Derive action flow",
            "Generate state machine",
            "Generate PLC program",
            "Verify interlock safety",
            "Generate alarm logic",
            "Generate HMI variables",
            "Export project files"
        };

        for (int i = 0; i < defaultOrder->Length; i++) {
            P6Task^ task = gcnew P6Task();
            task->Id = i + 1;
            task->Type = defaultOrder[i];
            task->Description = descriptions[i];
            task->Status = "pending";
            tasks->Add(task);
        }

        P5CompilerLog::Info("TASK", "Planned " + tasks->Count + " tasks");
        return tasks;
    }

    static String^ GetTaskDescription(P6TaskType type) {
        switch (type) {
        case P6TaskType::AnalyzeDevices: return "Analyze devices";
        case P6TaskType::ExtractIO: return "Extract IO";
        case P6TaskType::DeriveActions: return "Derive actions";
        case P6TaskType::GenerateStateMachine: return "Generate state machine";
        case P6TaskType::GenerateProgram: return "Generate program";
        case P6TaskType::VerifyInterlock: return "Verify interlock";
        case P6TaskType::GenerateAlarm: return "Generate alarm";
        case P6TaskType::GenerateHMIVars: return "Generate HMI vars";
        case P6TaskType::ExportProject: return "Export project";
        default: return "Unknown";
        }
    }
};

ref class P6SemanticEngine {
public:
    static List<P6ControlAction^>^ ParseNaturalLanguage(String^ text) {
        P5CompilerLog::Info("SEMANTIC", "Parsing natural language: " + text);
        List<P6ControlAction^>^ actions = gcnew List<P6ControlAction^>();

        array<String^>^ sentences = text->Split(gcnew array<Char>{',', '\n', ';'},
            StringSplitOptions::RemoveEmptyEntries);

        for each (String^ sentence in sentences) {
            String^ s = sentence->Trim();
            if (s->Length == 0) continue;

            P6ControlAction^ act = gcnew P6ControlAction();
            String^ sl = s->ToLower();

            if (sl->Contains("extend") || sl->Contains("push") || sl->Contains("forward") ||
                sl->Contains("advance")) {
                act->Type = "cylinder_extend";
                act->Target = ExtractTarget(s, "A");
            }
            else if (sl->Contains("retract") || sl->Contains("return") || sl->Contains("pull")) {
                act->Type = "cylinder_retract";
                act->Target = ExtractTarget(s, "A");
            }
            else if (sl->Contains("clamp") || sl->Contains("grip")) {
                act->Type = "cylinder_clamp";
                act->Target = ExtractTarget(s, "B");
            }
            else if (sl->Contains("release") || sl->Contains("unclamp")) {
                act->Type = "cylinder_release";
                act->Target = ExtractTarget(s, "B");
            }
            else if (sl->Contains("start") || sl->Contains("begin")) {
                act->Type = "start";
                act->Target = "System";
            }
            else if (sl->Contains("stop") || sl->Contains("halt")) {
                act->Type = "stop";
                act->Target = "System";
            }
            else if (sl->Contains("delay") || sl->Contains("wait") || sl->Contains("timer")) {
                act->Type = "timer_delay";
                act->Target = "T1";
                act->Parameter = "T#2S";
            }
            else if (sl->Contains("detect") || sl->Contains("sensor") || sl->Contains("check")) {
                act->Type = "sensor_check";
                act->Target = ExtractTarget(s, "SQ");
            }
            else if (sl->Contains("forward") || sl->Contains("cw") || sl->Contains("rise")) {
                act->Type = "motor_forward";
                act->Target = ExtractTarget(s, "M1");
            }
            else if (sl->Contains("reverse") || sl->Contains("ccw") || sl->Contains("lower")) {
                act->Type = "motor_reverse";
                act->Target = ExtractTarget(s, "M1");
            }
            else {
                act->Type = "action";
                act->Target = s;
            }

            if (sl->Contains("after") || sl->Contains("then") || sl->Contains("next")) {
                act->Condition = "previous_action_done";
            }

            actions->Add(act);
        }

        P5CompilerLog::Info("SEMANTIC", "Parsed " + actions->Count + " control actions");
        return actions;
    }

private:
    static String^ ExtractTarget(String^ text, String^ defaultTarget) {
        for each (Char c in text) {
            if (c >= 'A' && c <= 'Z') return c.ToString();
        }
        return defaultTarget;
    }
};

ref class P6ProgramSynthesisEngine {
public:
    static P3Dsl^ Synthesize(List<P6ControlAction^>^ actions, List<P3Variable^>^ variables) {
        P5CompilerLog::Info("SYNTH", "Synthesizing program from " + actions->Count + " actions");
        P3Dsl^ dsl = gcnew P3Dsl();
        dsl->Variables = gcnew List<P3Variable^>(variables);
        dsl->Networks = gcnew List<P3Network^>();

        for each (P6ControlAction^ act in actions) {
            P3Network^ net = gcnew P3Network();
            net->Title = act->Type + "_" + act->Target;
            net->Items = gcnew List<Object^>();

            if (act->Condition != nullptr && act->Condition->Length > 0 && act->Condition != "previous_action_done") {
                P3Node^ cond = gcnew P3Node();
                cond->NodeType = "contact";
                cond->Tag = act->Condition;
                cond->NormallyOpen = true;
                net->Items->Add(cond);
            }

            if (act->Type == "cylinder_extend" || act->Type == "motor_forward") {
                P3Node^ coil = gcnew P3Node();
                coil->NodeType = "set";
                coil->Tag = act->Target + "_Fwd";
                net->Items->Add(coil);
            }
            else if (act->Type == "cylinder_retract" || act->Type == "motor_reverse") {
                P3Node^ resetCoil = gcnew P3Node();
                resetCoil->NodeType = "reset";
                resetCoil->Tag = act->Target + "_Fwd";
                net->Items->Add(resetCoil);

                P3Node^ setCoil = gcnew P3Node();
                setCoil->NodeType = "set";
                setCoil->Tag = act->Target + "_Rev";
                net->Items->Add(setCoil);
            }
            else if (act->Type == "cylinder_clamp") {
                P3Node^ coil = gcnew P3Node();
                coil->NodeType = "set";
                coil->Tag = act->Target + "_Clamp";
                net->Items->Add(coil);
            }
            else if (act->Type == "cylinder_release") {
                P3Node^ coil = gcnew P3Node();
                coil->NodeType = "reset";
                coil->Tag = act->Target + "_Clamp";
                net->Items->Add(coil);
            }
            else if (act->Type == "timer_delay") {
                P3Node^ timer = gcnew P3Node();
                timer->NodeType = "ton";
                timer->Instance = act->Target;
                timer->Pt = (act->Parameter != nullptr && act->Parameter->Length > 0) ? act->Parameter : "T#1S";
                net->Items->Add(timer);
            }
            else if (act->Type == "start") {
                P3Node^ coil = gcnew P3Node();
                coil->NodeType = "set";
                coil->Tag = "System_Running";
                net->Items->Add(coil);
            }
            else if (act->Type == "stop") {
                P3Node^ coil = gcnew P3Node();
                coil->NodeType = "reset";
                coil->Tag = "System_Running";
                net->Items->Add(coil);
            }
            else {
                P3Node^ coil = gcnew P3Node();
                coil->NodeType = "coil";
                coil->Tag = act->Target;
                net->Items->Add(coil);
            }

            if (net->Items->Count > 0) {
                dsl->Networks->Add(net);
            }
        }

        P5CompilerLog::Info("SYNTH", "Synthesized " + dsl->Networks->Count + " networks");
        return dsl;
    }

    static List<String^>^ GenerateAlarmLogic(P3Dsl^ dsl) {
        P5CompilerLog::Info("SYNTH", "Generating alarm logic");
        List<String^>^ alarms = gcnew List<String^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr && node->Tag != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "ton" && node->Instance != nullptr) {
                        alarms->Add("Alarm: " + node->Instance + " timeout");
                    }
                }
            }
        }
        return alarms;
    }

    static List<String^>^ GenerateHMIVariables(P3Dsl^ dsl) {
        P5CompilerLog::Info("SYNTH", "Generating HMI variables");
        List<String^>^ hmiVars = gcnew List<String^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr) hmiVars->Add(v->Name);
        }
        return hmiVars;
    }

    static List<Object^>^ GenerateLogicGraph(List<P6ControlAction^>^ actions) {
        P5CompilerLog::Info("SYNTH", "Generating LogicGraph");
        List<Object^>^ graph = gcnew List<Object^>();
        for each (P6ControlAction^ act in actions) {
            P3Node^ node = gcnew P3Node();
            node->Tag = act->Target;
            if (act->Type == "cylinder_extend" || act->Type == "motor_forward") {
                node->NodeType = "set";
            }
            else if (act->Type == "cylinder_retract" || act->Type == "motor_reverse") {
                node->NodeType = "reset";
            }
            else if (act->Type == "timer_delay") {
                node->NodeType = "ton";
                node->Instance = act->Target;
                node->Pt = (act->Parameter != nullptr) ? act->Parameter : "T#1S";
            }
            else {
                node->NodeType = "coil";
            }
            graph->Add(node);
        }
        return graph;
    }

    static List<P3Network^>^ SplitNetworks(P3Dsl^ dsl) {
        P5CompilerLog::Info("SYNTH", "Splitting large networks");
        List<P3Network^>^ result = gcnew List<P3Network^>();
        for each (P3Network^ net in dsl->Networks) {
            if (net->Items->Count > 8) {
                P3Network^ part1 = gcnew P3Network();
                part1->Title = net->Title + "_Part1";
                part1->Items = gcnew List<Object^>();
                P3Network^ part2 = gcnew P3Network();
                part2->Title = net->Title + "_Part2";
                part2->Items = gcnew List<Object^>();
                int half = net->Items->Count / 2;
                for (int i = 0; i < net->Items->Count; i++) {
                    if (i < half) part1->Items->Add(net->Items[i]);
                    else part2->Items->Add(net->Items[i]);
                }
                result->Add(part1);
                result->Add(part2);
            }
            else {
                result->Add(net);
            }
        }
        return result;
    }

    static P3Network^ GenerateParallelLogic(List<String^>^ conditions, String^ outputTag) {
        P5CompilerLog::Info("SYNTH", "Generating parallel logic for " + outputTag);
        P3Network^ net = gcnew P3Network();
        net->Title = "Parallel_" + outputTag;
        net->Items = gcnew List<Object^>();
        for each (String^ cond in conditions) {
            P3Node^ contact = gcnew P3Node();
            contact->NodeType = "contact";
            contact->Tag = cond;
            contact->NormallyOpen = true;
            net->Items->Add(contact);
        }
        P3Node^ coil = gcnew P3Node();
        coil->NodeType = "coil";
        coil->Tag = outputTag;
        net->Items->Add(coil);
        return net;
    }

    static P3Network^ GenerateCounterLogic(String^ instance, String^ countTag, int presetValue) {
        P5CompilerLog::Info("SYNTH", "Generating counter logic for " + instance);
        P3Network^ net = gcnew P3Network();
        net->Title = "Counter_" + instance;
        net->Items = gcnew List<Object^>();

        P3Node^ contact = gcnew P3Node();
        contact->NodeType = "contact";
        contact->Tag = countTag;
        contact->NormallyOpen = true;
        net->Items->Add(contact);

        P3Node^ counter = gcnew P3Node();
        counter->NodeType = "ctu";
        counter->Instance = instance;
        counter->Pv = presetValue.ToString();
        net->Items->Add(counter);

        return net;
    }

    static List<P3Network^>^ GenerateSequentialLogic(List<String^>^ steps) {
        P5CompilerLog::Info("SYNTH", "Generating sequential logic from " + steps->Count + " steps");
        List<P3Network^>^ networks = gcnew List<P3Network^>();
        for (int i = 0; i < steps->Count; i++) {
            P3Network^ net = gcnew P3Network();
            net->Title = "Seq_Step" + (i + 1);
            net->Items = gcnew List<Object^>();

            if (i > 0) {
                P3Node^ prevSensor = gcnew P3Node();
                prevSensor->NodeType = "contact";
                prevSensor->Tag = steps[i - 1] + "_done";
                prevSensor->NormallyOpen = true;
                net->Items->Add(prevSensor);
            }
            else {
                P3Node^ startBtn = gcnew P3Node();
                startBtn->NodeType = "contact";
                startBtn->Tag = "Start_Btn";
                startBtn->NormallyOpen = true;
                net->Items->Add(startBtn);
            }

            P3Node^ stepCoil = gcnew P3Node();
            stepCoil->NodeType = "set";
            stepCoil->Tag = steps[i] + "_active";
            net->Items->Add(stepCoil);

            if (i > 0) {
                P3Node^ resetPrev = gcnew P3Node();
                resetPrev->NodeType = "reset";
                resetPrev->Tag = steps[i - 1] + "_active";
                net->Items->Add(resetPrev);
            }

            networks->Add(net);
        }
        return networks;
    }

    static P3Network^ GenerateFaultLogic(String^ faultTag, String^ outputTag) {
        P5CompilerLog::Info("SYNTH", "Generating fault logic for " + faultTag);
        P3Network^ net = gcnew P3Network();
        net->Title = "Fault_" + faultTag;
        net->Items = gcnew List<Object^>();

        P3Node^ faultContact = gcnew P3Node();
        faultContact->NodeType = "contact";
        faultContact->Tag = faultTag;
        faultContact->NormallyOpen = true;
        net->Items->Add(faultContact);

        P3Node^ faultCoil = gcnew P3Node();
        faultCoil->NodeType = "set";
        faultCoil->Tag = outputTag + "_Fault";
        net->Items->Add(faultCoil);

        P3Node^ stopCoil = gcnew P3Node();
        stopCoil->NodeType = "reset";
        stopCoil->Tag = outputTag;
        net->Items->Add(stopCoil);

        return net;
    }
};

ref class P6SelfRepairEngine {
public:
    static List<String^>^ SelfRepair(P3Dsl^ dsl) {
        P5CompilerLog::Info("SELFREPAIR", "Running self-repair engine");
        List<String^>^ repairs = gcnew List<String^>();

        repairs->Add(DiscoverAndFixErrors(dsl));
        repairs->Add(RestructureProgram(dsl));
        repairs->Add(OptimizeStructure(dsl));
        repairs->Add(CompleteMissingLogic(dsl));
        repairs->Add(AutoAddEStop(dsl));
        repairs->Add(AutoAddInterlock(dsl));

        List<String^>^ validRepairs = gcnew List<String^>();
        for each (String^ r in repairs) {
            if (r != nullptr && r->Length > 0) validRepairs->Add(r);
        }
        P5CompilerLog::Info("SELFREPAIR", "Self-repair complete: " + validRepairs->Count + " repairs");
        return validRepairs;
    }

private:
    static String^ DiscoverAndFixErrors(P3Dsl^ dsl) {
        int fixed_ = 0;
        for each (P3Network^ net in dsl->Networks) {
            bool hasOutput = false;
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "coil" || t == "set" || t == "reset") hasOutput = true;
                }
            }
            if (!hasOutput && net->Items->Count > 0) {
                P3Node^ lastNode = dynamic_cast<P3Node^>(net->Items[net->Items->Count - 1]);
                if (lastNode != nullptr && lastNode->Tag != nullptr) {
                    P3Node^ coil = gcnew P3Node();
                    coil->NodeType = "coil";
                    coil->Tag = lastNode->Tag;
                    net->Items->Add(coil);
                    fixed_++;
                }
            }
        }
        if (fixed_ > 0) return "Fixed " + fixed_ + " networks without output";
        return "";
    }

    static String^ RestructureProgram(P3Dsl^ dsl) {
        int restructured = 0;
        for (int i = dsl->Networks->Count - 1; i >= 0; i--) {
            P3Network^ net = dsl->Networks[i];
            if (net->Items->Count > 10) {
                P3Network^ part1 = gcnew P3Network();
                part1->Title = net->Title + "_Part1";
                part1->Items = gcnew List<Object^>();
                P3Network^ part2 = gcnew P3Network();
                part2->Title = net->Title + "_Part2";
                part2->Items = gcnew List<Object^>();
                int half = net->Items->Count / 2;
                for (int j = 0; j < net->Items->Count; j++) {
                    if (j < half) part1->Items->Add(net->Items[j]);
                    else part2->Items->Add(net->Items[j]);
                }
                dsl->Networks->RemoveAt(i);
                dsl->Networks->Insert(i, part2);
                dsl->Networks->Insert(i, part1);
                restructured++;
            }
        }
        if (restructured > 0) return "Restructured " + restructured + " large networks into smaller parts";
        return "";
    }

    static String^ OptimizeStructure(P3Dsl^ dsl) {
        P5RepairEngine::AutoRepair(dsl);
        return "Applied Phase 5 optimizations";
    }

    static String^ CompleteMissingLogic(P3Dsl^ dsl) {
        int added = 0;
        HashSet<String^>^ hasTimeout = gcnew HashSet<String^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr && node->NodeType->ToLower() == "ton" && node->Instance != nullptr) {
                    hasTimeout->Add(node->Instance);
                }
            }
        }
        List<P3Network^>^ newNets = gcnew List<P3Network^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr && node->Tag != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "set" || t == "scoil") && node->Tag->Contains("Fwd")) {
                        String^ timerName = "T_" + node->Tag;
                        if (!hasTimeout->Contains(timerName)) {
                            P3Network^ timeoutNet = gcnew P3Network();
                            timeoutNet->Title = "Timeout_" + node->Tag;
                            timeoutNet->Items = gcnew List<Object^>();

                            P3Node^ contact = gcnew P3Node();
                            contact->NodeType = "contact";
                            contact->Tag = node->Tag;
                            contact->NormallyOpen = true;
                            timeoutNet->Items->Add(contact);

                            P3Node^ timer = gcnew P3Node();
                            timer->NodeType = "ton";
                            timer->Instance = timerName;
                            timer->Pt = "T#5S";
                            timeoutNet->Items->Add(timer);

                            newNets->Add(timeoutNet);
                            added++;
                        }
                    }
                }
            }
        }
        for each (P3Network^ n in newNets) dsl->Networks->Add(n);
        if (added > 0) return "Added " + added + " timeout protections";
        return "";
    }

    static String^ AutoAddEStop(P3Dsl^ dsl) {
        bool hasEStop = false;
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr) {
                String^ n = v->Name->ToLower();
                if (n->Contains("e_stop") || n->Contains("estop") || n->Contains("emergency")) hasEStop = true;
            }
        }
        if (!hasEStop) {
            P3Variable^ eStopVar = gcnew P3Variable();
            eStopVar->Name = "E_Stop";
            eStopVar->Type = "Bool";
            eStopVar->Scope = "input";
            eStopVar->Comment = "Emergency Stop";
            dsl->Variables->Add(eStopVar);

            for each (P3Network^ net in dsl->Networks) {
                bool hasOutput = false;
                for each (Object^ item in net->Items) {
                    P3Node^ node = dynamic_cast<P3Node^>(item);
                    if (node != nullptr) {
                        String^ t = node->NodeType->ToLower();
                        if (t == "coil" || t == "set") hasOutput = true;
                    }
                }
                if (hasOutput) {
                    P3Node^ eStopNC = gcnew P3Node();
                    eStopNC->NodeType = "contact";
                    eStopNC->Tag = "E_Stop";
                    eStopNC->NormallyOpen = false;
                    net->Items->Insert(0, eStopNC);
                }
            }
            return "Auto-added E_Stop variable and NC contacts to all output networks";
        }
        return "";
    }

    static String^ AutoAddInterlock(P3Dsl^ dsl) {
        int added = 0;
        List<String^>^ fwdVars = gcnew List<String^>();
        List<String^>^ revVars = gcnew List<String^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr && v->Scope == "output") {
                String^ n = v->Name->ToLower();
                if (n->Contains("fwd") || n->Contains("forward")) fwdVars->Add(v->Name);
                if (n->Contains("rev") || n->Contains("reverse")) revVars->Add(v->Name);
            }
        }
        if (fwdVars->Count > 0 && revVars->Count > 0) {
            for each (P3Network^ net in dsl->Networks) {
                List<P3Node^>^ toInsert = gcnew List<P3Node^>();
                int insertPos = -1;
                for (int i = 0; i < net->Items->Count; i++) {
                    P3Node^ node = dynamic_cast<P3Node^>(net->Items[i]);
                    if (node != nullptr && node->Tag != nullptr) {
                        String^ tagLower = node->Tag->ToLower();
                        String^ t = node->NodeType->ToLower();
                        if ((t == "set" || t == "coil") && tagLower->Contains("fwd")) {
                            for each (String^ rev in revVars) {
                                P3Node^ interlockNC = gcnew P3Node();
                                interlockNC->NodeType = "contact";
                                interlockNC->Tag = rev;
                                interlockNC->NormallyOpen = false;
                                toInsert->Add(interlockNC);
                                added++;
                            }
                            insertPos = i;
                        }
                        else if ((t == "set" || t == "coil") && tagLower->Contains("rev")) {
                            for each (String^ fwd in fwdVars) {
                                P3Node^ interlockNC = gcnew P3Node();
                                interlockNC->NodeType = "contact";
                                interlockNC->Tag = fwd;
                                interlockNC->NormallyOpen = false;
                                toInsert->Add(interlockNC);
                                added++;
                            }
                            insertPos = i;
                        }
                    }
                }
                if (toInsert->Count > 0 && insertPos >= 0) {
                    for each (P3Node^ n in toInsert) {
                        net->Items->Insert(insertPos, n);
                        insertPos++;
                    }
                }
            }
        }
        if (added > 0) return "Auto-added " + added + " interlock NC contacts";
        return "";
    }
};

ref class P6SimulationEngine {
public:
    static P6SimulationState^ CreateInitialState(P3Dsl^ dsl) {
        P6SimulationState^ state = gcnew P6SimulationState();
        state->CycleCount = 0;

        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr) {
                if (v->Type == "Bool" || v->Type == "Bool") {
                    state->BoolVars[v->Name] = false;
                }
                else if (v->Type == "Int" || v->Type == "DInt") {
                    state->IntVars[v->Name] = 0;
                }
            }
        }
        return state;
    }

    static P6SimulationState^ ExecuteScanCycle(P3Dsl^ dsl, P6SimulationState^ state) {
        state->CycleCount++;
        state->TraceLog->Add("=== Cycle " + state->CycleCount + " ===");

        for each (P3Network^ net in dsl->Networks) {
            bool condition = true;
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr && node->Tag != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "contact") {
                        bool val = false;
                        if (state->BoolVars->ContainsKey(node->Tag)) val = state->BoolVars[node->Tag];
                        condition = condition && (node->NormallyOpen ? val : !val);
                    }
                    else if (t == "coil" && condition) {
                        state->BoolVars[node->Tag] = true;
                        state->TraceLog->Add("  " + node->Tag + " := TRUE");
                    }
                    else if ((t == "set" || t == "scoil") && condition) {
                        state->BoolVars[node->Tag] = true;
                        state->TraceLog->Add("  " + node->Tag + " := SET");
                    }
                    else if ((t == "reset" || t == "rcoil") && condition) {
                        state->BoolVars[node->Tag] = false;
                        state->TraceLog->Add("  " + node->Tag + " := RESET");
                    }
                    else if (t == "ton" && condition && node->Instance != nullptr) {
                        state->TimerStates[node->Instance] = "running";
                        state->TraceLog->Add("  " + node->Instance + " TON started");
                    }
                    else if ((t == "ctu" || t == "ctd") && condition && node->Instance != nullptr) {
                        if (!state->CounterValues->ContainsKey(node->Instance))
                            state->CounterValues[node->Instance] = 0;
                        state->CounterValues[node->Instance]++;
                        state->TraceLog->Add("  " + node->Instance + " = " + state->CounterValues[node->Instance]);
                    }
                }
            }
        }
        return state;
    }

    static List<String^>^ RunSimulation(P3Dsl^ dsl, int maxCycles, Dictionary<String^, bool>^ inputs) {
        P5CompilerLog::Info("SIM", "Running simulation for " + maxCycles + " cycles");
        P6SimulationState^ state = CreateInitialState(dsl);

        for each (KeyValuePair<String^, bool>^ kv in inputs) {
            state->BoolVars[kv->Key] = kv->Value;
        }

        for (int i = 0; i < maxCycles; i++) {
            state = ExecuteScanCycle(dsl, state);
        }

        P5CompilerLog::Info("SIM", "Simulation complete: " + state->CycleCount + " cycles");
        return state->TraceLog;
    }

    static Dictionary<String^, bool>^ SimulateIO(P3Dsl^ dsl, Dictionary<String^, bool>^ inputs) {
        P6SimulationState^ state = CreateInitialState(dsl);
        for each (KeyValuePair<String^, bool>^ kv in inputs) {
            state->BoolVars[kv->Key] = kv->Value;
        }
        state = ExecuteScanCycle(dsl, state);

        Dictionary<String^, bool>^ outputs = gcnew Dictionary<String^, bool>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Scope == "output" && v->Name != nullptr && state->BoolVars->ContainsKey(v->Name)) {
                outputs[v->Name] = state->BoolVars[v->Name];
            }
        }
        return outputs;
    }

    static Dictionary<String^, String^>^ SimulateStateMachine(P3Dsl^ dsl, List<String^>^ stepNames, int maxCycles) {
        P5CompilerLog::Info("SIM", "Simulating state machine with " + stepNames->Count + " steps");
        Dictionary<String^, String^>^ stateTransitions = gcnew Dictionary<String^, String^>();
        P6SimulationState^ state = CreateInitialState(dsl);

        for each (String^ step in stepNames) {
            state->BoolVars[step + "_active"] = false;
        }
        if (stepNames->Count > 0) {
            state->BoolVars[stepNames[0] + "_active"] = true;
        }

        for (int i = 0; i < maxCycles; i++) {
            state = ExecuteScanCycle(dsl, state);
            String^ activeStep = "Idle";
            for each (String^ step in stepNames) {
                String^ key = step + "_active";
                if (state->BoolVars->ContainsKey(key) && state->BoolVars[key]) {
                    activeStep = step;
                }
            }
            stateTransitions["Cycle" + (i + 1)] = activeStep;
        }

        return stateTransitions;
    }

    static List<String^>^ PlaybackTiming(List<P6SimulationState^>^ history) {
        P5CompilerLog::Info("SIM", "Playing back timing from " + history->Count + " snapshots");
        List<String^>^ timeline = gcnew List<String^>();
        for each (P6SimulationState^ snap in history) {
            String^ entry = "Cycle " + snap->CycleCount + ": ";
            for each (KeyValuePair<String^, bool>^ kv in snap->BoolVars) {
                if (kv->Value) entry += kv->Key + "=ON ";
            }
            timeline->Add(entry);
        }
        return timeline;
    }
};

ref class P6ScanCycleEngine {
public:
    static List<String^>^ RunCycles(P3Dsl^ dsl, int cycleCount, Dictionary<String^, bool>^ inputs) {
        P5CompilerLog::Info("SCANCYCLE", "Running " + cycleCount + " scan cycles");
        List<String^>^ log = gcnew List<String^>();

        P6SimulationState^ state = P6SimulationEngine::CreateInitialState(dsl);
        for each (KeyValuePair<String^, bool>^ kv in inputs) {
            state->BoolVars[kv->Key] = kv->Value;
        }

        for (int i = 0; i < cycleCount; i++) {
            log->Add("--- Scan Cycle " + (i + 1) + " ---");
            log->Add("Read Inputs:");
            for each (KeyValuePair<String^, bool>^ kv in inputs) {
                log->Add("  " + kv->Key + " = " + (kv->Value ? "TRUE" : "FALSE"));
            }

            state = P6SimulationEngine::ExecuteScanCycle(dsl, state);

            log->Add("Write Outputs:");
            for each (P3Variable^ v in dsl->Variables) {
                if (v->Scope == "output" && v->Name != nullptr && state->BoolVars->ContainsKey(v->Name)) {
                    log->Add("  " + v->Name + " = " + (state->BoolVars[v->Name] ? "TRUE" : "FALSE"));
                }
            }
        }

        P5CompilerLog::Info("SCANCYCLE", "Scan cycle engine complete");
        return log;
    }
};

ref class P6KnowledgeBase {
public:
    static List<P6KnowledgeTemplate^>^ GetTemplates() {
        List<P6KnowledgeTemplate^>^ templates = gcnew List<P6KnowledgeTemplate^>();

        templates->Add(CreateMotorControlTemplate());
        templates->Add(CreateCylinderControlTemplate());
        templates->Add(CreateConveyorTemplate());
        templates->Add(CreateManipulatorTemplate());
        templates->Add(CreateTrafficLightTemplate());
        templates->Add(CreateSequentialTemplate());
        templates->Add(CreatePIDTemplate());
        templates->Add(CreateSafetyTemplate());
        templates->Add(CreateAlarmTemplate());

        return templates;
    }

    static P6KnowledgeTemplate^ FindTemplate(String^ keyword) {
        for each (P6KnowledgeTemplate^ t in GetTemplates()) {
            if (t->Name->ToLower()->Contains(keyword->ToLower()) ||
                t->Category->ToLower()->Contains(keyword->ToLower()) ||
                t->Description->ToLower()->Contains(keyword->ToLower())) {
                return t;
            }
        }
        return nullptr;
    }

private:
    static P6KnowledgeTemplate^ CreateMotorControlTemplate() {
        P6KnowledgeTemplate^ t = gcnew P6KnowledgeTemplate();
        t->Name = "MotorControl";
        t->Category = "motor";
        t->Description = "Forward/reverse motor control with interlock";
        t->RequiredInputs = gcnew List<String^>(); t->RequiredInputs->Add("Start_Btn"); t->RequiredInputs->Add("Stop_Btn");
        t->RequiredOutputs = gcnew List<String^>(); t->RequiredOutputs->Add("Motor_Fwd"); t->RequiredOutputs->Add("Motor_Rev");
        t->DslTemplate = "motor_control";
        return t;
    }

    static P6KnowledgeTemplate^ CreateCylinderControlTemplate() {
        P6KnowledgeTemplate^ t = gcnew P6KnowledgeTemplate();
        t->Name = "CylinderControl";
        t->Category = "cylinder";
        t->Description = "Cylinder extend/retract with sensor feedback";
        t->RequiredInputs = gcnew List<String^>(); t->RequiredInputs->Add("Extend_Btn"); t->RequiredInputs->Add("Retract_Btn");
        t->RequiredOutputs = gcnew List<String^>(); t->RequiredOutputs->Add("Cyl_Extend"); t->RequiredOutputs->Add("Cyl_Retract");
        t->DslTemplate = "cylinder_control";
        return t;
    }

    static P6KnowledgeTemplate^ CreateConveyorTemplate() {
        P6KnowledgeTemplate^ t = gcnew P6KnowledgeTemplate();
        t->Name = "ConveyorLine";
        t->Category = "conveyor";
        t->Description = "Conveyor line with start/stop and jam detection";
        t->RequiredInputs = gcnew List<String^>(); t->RequiredInputs->Add("Start"); t->RequiredInputs->Add("Stop"); t->RequiredInputs->Add("Jam_Sensor");
        t->RequiredOutputs = gcnew List<String^>(); t->RequiredOutputs->Add("Conveyor_Run");
        t->DslTemplate = "conveyor";
        return t;
    }

    static P6KnowledgeTemplate^ CreateManipulatorTemplate() {
        P6KnowledgeTemplate^ t = gcnew P6KnowledgeTemplate();
        t->Name = "Manipulator";
        t->Category = "manipulator";
        t->Description = "Robot arm pick and place sequence";
        t->RequiredInputs = gcnew List<String^>(); t->RequiredInputs->Add("Pick_Sensor"); t->RequiredInputs->Add("Place_Sensor");
        t->RequiredOutputs = gcnew List<String^>(); t->RequiredOutputs->Add("Arm_Up"); t->RequiredOutputs->Add("Arm_Down"); t->RequiredOutputs->Add("Gripper");
        t->DslTemplate = "manipulator";
        return t;
    }

    static P6KnowledgeTemplate^ CreateTrafficLightTemplate() {
        P6KnowledgeTemplate^ t = gcnew P6KnowledgeTemplate();
        t->Name = "TrafficLight";
        t->Category = "traffic";
        t->Description = "Traffic light with timer-based sequence";
        t->RequiredInputs = gcnew List<String^>(); t->RequiredInputs->Add("Enable");
        t->RequiredOutputs = gcnew List<String^>(); t->RequiredOutputs->Add("Red"); t->RequiredOutputs->Add("Yellow"); t->RequiredOutputs->Add("Green");
        t->DslTemplate = "traffic_light";
        return t;
    }

    static P6KnowledgeTemplate^ CreateSequentialTemplate() {
        P6KnowledgeTemplate^ t = gcnew P6KnowledgeTemplate();
        t->Name = "SequentialControl";
        t->Category = "sequential";
        t->Description = "Generic sequential control with step transitions";
        t->RequiredInputs = gcnew List<String^>(); t->RequiredInputs->Add("Start"); t->RequiredInputs->Add("Step_Sensors");
        t->RequiredOutputs = gcnew List<String^>(); t->RequiredOutputs->Add("Step_Outputs");
        t->DslTemplate = "sequential";
        return t;
    }

    static P6KnowledgeTemplate^ CreatePIDTemplate() {
        P6KnowledgeTemplate^ t = gcnew P6KnowledgeTemplate();
        t->Name = "PIDControl";
        t->Category = "pid";
        t->Description = "PID closed-loop control";
        t->RequiredInputs = gcnew List<String^>(); t->RequiredInputs->Add("Setpoint"); t->RequiredInputs->Add("Feedback");
        t->RequiredOutputs = gcnew List<String^>(); t->RequiredOutputs->Add("Output");
        t->DslTemplate = "pid";
        return t;
    }

    static P6KnowledgeTemplate^ CreateSafetyTemplate() {
        P6KnowledgeTemplate^ t = gcnew P6KnowledgeTemplate();
        t->Name = "SafetyCircuit";
        t->Category = "safety";
        t->Description = "Emergency stop and safety interlock circuit";
        t->RequiredInputs = gcnew List<String^>(); t->RequiredInputs->Add("E_Stop"); t->RequiredInputs->Add("Safety_Gate");
        t->RequiredOutputs = gcnew List<String^>(); t->RequiredOutputs->Add("Safety_OK");
        t->DslTemplate = "safety";
        return t;
    }

    static P6KnowledgeTemplate^ CreateAlarmTemplate() {
        P6KnowledgeTemplate^ t = gcnew P6KnowledgeTemplate();
        t->Name = "AlarmSystem";
        t->Category = "alarm";
        t->Description = "Alarm monitoring and annunciation";
        t->RequiredInputs = gcnew List<String^>(); t->RequiredInputs->Add("Fault_Inputs");
        t->RequiredOutputs = gcnew List<String^>(); t->RequiredOutputs->Add("Alarm_Light"); t->RequiredOutputs->Add("Alarm_Horn");
        t->DslTemplate = "alarm";
        return t;
    }
};

ref class P6TemplateEngine {
public:
    static P3Dsl^ ApplyTemplate(P6KnowledgeTemplate^ template_, Dictionary<String^, String^>^ parameters) {
        P5CompilerLog::Info("TEMPLATE", "Applying template: " + template_->Name);
        P3Dsl^ dsl = gcnew P3Dsl();
        dsl->Variables = gcnew List<P3Variable^>();
        dsl->Networks = gcnew List<P3Network^>();

        for each (String^ input in template_->RequiredInputs) {
            P3Variable^ v = gcnew P3Variable();
            v->Name = ResolveParameter(input, parameters);
            v->Type = "Bool";
            v->Scope = "input";
            dsl->Variables->Add(v);
        }
        for each (String^ output in template_->RequiredOutputs) {
            P3Variable^ v = gcnew P3Variable();
            v->Name = ResolveParameter(output, parameters);
            v->Type = "Bool";
            v->Scope = "output";
            dsl->Variables->Add(v);
        }

        P3Network^ startNet = gcnew P3Network();
        startNet->Title = template_->Name + "_Start";
        startNet->Items = gcnew List<Object^>();

        if (template_->RequiredInputs->Count > 0) {
            P3Node^ startContact = gcnew P3Node();
            startContact->NodeType = "contact";
            startContact->Tag = ResolveParameter(template_->RequiredInputs[0], parameters);
            startContact->NormallyOpen = true;
            startNet->Items->Add(startContact);
        }
        if (template_->RequiredOutputs->Count > 0) {
            P3Node^ outputCoil = gcnew P3Node();
            outputCoil->NodeType = "coil";
            outputCoil->Tag = ResolveParameter(template_->RequiredOutputs[0], parameters);
            startNet->Items->Add(outputCoil);
        }

        if (startNet->Items->Count > 0) dsl->Networks->Add(startNet);

        P5CompilerLog::Info("TEMPLATE", "Template applied: " + dsl->Networks->Count + " networks");
        return dsl;
    }

private:
    static String^ ResolveParameter(String^ name, Dictionary<String^, String^>^ parameters) {
        if (parameters != nullptr && parameters->ContainsKey(name)) return parameters[name];
        return name;
    }
};

ref class P6IndustrialRuleEngine {
public:
    static List<P6SafetyRule^>^ ApplyRules(P3Dsl^ dsl) {
        P5CompilerLog::Info("RULE", "Applying industrial rules");
        List<P6SafetyRule^>^ appliedRules = gcnew List<P6SafetyRule^>();

        List<P6SafetyRule^>^ safetyRules = ApplySafetyRules(dsl);
        List<P6SafetyRule^>^ interlockRules = ApplyInterlockRules(dsl);
        List<P6SafetyRule^>^ estopRules = ApplyEStopRules(dsl);
        List<P6SafetyRule^>^ resetRules = ApplyAutoResetRules(dsl);
        List<P6SafetyRule^>^ misactionRules = ApplyMisactionPreventionRules(dsl);
        List<P6SafetyRule^>^ alarmRules = ApplyAlarmRules(dsl);
        List<P6SafetyRule^>^ timeoutRules = ApplyTimeoutRules(dsl);

        for each (P6SafetyRule^ r in safetyRules) appliedRules->Add(r);
        for each (P6SafetyRule^ r in interlockRules) appliedRules->Add(r);
        for each (P6SafetyRule^ r in estopRules) appliedRules->Add(r);
        for each (P6SafetyRule^ r in resetRules) appliedRules->Add(r);
        for each (P6SafetyRule^ r in misactionRules) appliedRules->Add(r);
        for each (P6SafetyRule^ r in alarmRules) appliedRules->Add(r);
        for each (P6SafetyRule^ r in timeoutRules) appliedRules->Add(r);

        P5CompilerLog::Info("RULE", "Applied " + appliedRules->Count + " industrial rules");
        return appliedRules;
    }

private:
    static List<P6SafetyRule^>^ ApplySafetyRules(P3Dsl^ dsl) {
        List<P6SafetyRule^>^ rules = gcnew List<P6SafetyRule^>();
        bool hasEStop = false;
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr) {
                String^ n = v->Name->ToLower();
                if (n->Contains("e_stop") || n->Contains("estop") || n->Contains("emergency")) hasEStop = true;
            }
        }
        if (!hasEStop) {
            P6SafetyRule^ rule = gcnew P6SafetyRule();
            rule->RuleType = "safety";
            rule->Description = "Missing E-Stop input - recommended to add";
            rule->TargetTag = "E_Stop";
            rule->Applied = false;
            rules->Add(rule);
        }
        return rules;
    }

    static List<P6SafetyRule^>^ ApplyInterlockRules(P3Dsl^ dsl) {
        List<P6SafetyRule^>^ rules = gcnew List<P6SafetyRule^>();
        List<String^>^ fwdVars = gcnew List<String^>();
        List<String^>^ revVars = gcnew List<String^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr && v->Scope == "output") {
                String^ n = v->Name->ToLower();
                if (n->Contains("fwd") || n->Contains("forward")) fwdVars->Add(v->Name);
                if (n->Contains("rev") || n->Contains("reverse")) revVars->Add(v->Name);
            }
        }
        if (fwdVars->Count > 0 && revVars->Count > 0) {
            P6SafetyRule^ rule = gcnew P6SafetyRule();
            rule->RuleType = "interlock";
            rule->Description = "Forward/reverse interlock required";
            rule->TargetTag = String::Join(",", fwdVars) + " <-> " + String::Join(",", revVars);
            rule->Applied = true;
            rules->Add(rule);
        }
        return rules;
    }

    static List<P6SafetyRule^>^ ApplyEStopRules(P3Dsl^ dsl) {
        List<P6SafetyRule^>^ rules = gcnew List<P6SafetyRule^>();
        for each (P3Network^ net in dsl->Networks) {
            bool hasOutput = false;
            bool hasEStopNC = false;
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "coil" || t == "set") hasOutput = true;
                    if (t == "contact" && !node->NormallyOpen && node->Tag != nullptr &&
                        (node->Tag->ToLower()->Contains("e_stop") || node->Tag->ToLower()->Contains("estop"))) {
                        hasEStopNC = true;
                    }
                }
            }
            if (hasOutput && !hasEStopNC) {
                P6SafetyRule^ rule = gcnew P6SafetyRule();
                rule->RuleType = "estop";
                rule->Description = "Network '" + net->Title + "' has output but no E-Stop NC contact";
                rule->TargetTag = net->Title;
                rule->Applied = false;
                rules->Add(rule);
            }
        }
        return rules;
    }

    static List<P6SafetyRule^>^ ApplyAutoResetRules(P3Dsl^ dsl) {
        List<P6SafetyRule^>^ rules = gcnew List<P6SafetyRule^>();
        HashSet<String^>^ setVars = gcnew HashSet<String^>();
        HashSet<String^>^ resetVars = gcnew HashSet<String^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr && node->Tag != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "set" || t == "scoil") setVars->Add(node->Tag);
                    if (t == "reset" || t == "rcoil") resetVars->Add(node->Tag);
                }
            }
        }
        for each (String^ s in setVars) {
            if (!resetVars->Contains(s)) {
                P6SafetyRule^ rule = gcnew P6SafetyRule();
                rule->RuleType = "auto_reset";
                rule->Description = "Variable '" + s + "' is SET but never RESET";
                rule->TargetTag = s;
                rule->Applied = false;
                rules->Add(rule);
            }
        }
        return rules;
    }

    static List<P6SafetyRule^>^ ApplyMisactionPreventionRules(P3Dsl^ dsl) {
        List<P6SafetyRule^>^ rules = gcnew List<P6SafetyRule^>();
        for each (P3Network^ net in dsl->Networks) {
            int outputCount = 0;
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "coil" || t == "set" || t == "reset") outputCount++;
                }
            }
            if (outputCount > 3) {
                P6SafetyRule^ rule = gcnew P6SafetyRule();
                rule->RuleType = "misaction_prevention";
                rule->Description = "Network '" + net->Title + "' has " + outputCount + " outputs (risk of misaction)";
                rule->TargetTag = net->Title;
                rule->Applied = false;
                rules->Add(rule);
            }
        }
        return rules;
    }

    static List<P6SafetyRule^>^ ApplyAlarmRules(P3Dsl^ dsl) {
        List<P6SafetyRule^>^ rules = gcnew List<P6SafetyRule^>();
        bool hasAlarm = false;
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr && v->Name->ToLower()->Contains("alarm")) hasAlarm = true;
        }
        if (!hasAlarm) {
            P6SafetyRule^ rule = gcnew P6SafetyRule();
            rule->RuleType = "alarm";
            rule->Description = "No alarm variables defined - recommended to add fault alarms";
            rule->TargetTag = "";
            rule->Applied = false;
            rules->Add(rule);
        }
        return rules;
    }

    static List<P6SafetyRule^>^ ApplyTimeoutRules(P3Dsl^ dsl) {
        List<P6SafetyRule^>^ rules = gcnew List<P6SafetyRule^>();
        HashSet<String^>^ motionVars = gcnew HashSet<String^>();
        HashSet<String^>^ timerInstances = gcnew HashSet<String^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "set" || t == "scoil") && node->Tag != nullptr) motionVars->Add(node->Tag);
                    if (t == "ton" && node->Instance != nullptr) timerInstances->Add(node->Instance);
                }
            }
        }
        if (motionVars->Count > 0 && timerInstances->Count == 0) {
            P6SafetyRule^ rule = gcnew P6SafetyRule();
            rule->RuleType = "timeout";
            rule->Description = "Motion outputs exist but no timeout timers - recommended to add";
            rule->TargetTag = "";
            rule->Applied = false;
            rules->Add(rule);
        }
        return rules;
    }
};

ref class P6HmiGenerator {
public:
    static List<P6HmiElement^>^ Generate(P3Dsl^ dsl) {
        P5CompilerLog::Info("HMI", "Generating HMI elements");
        List<P6HmiElement^>^ elements = gcnew List<P6HmiElement^>();
        int x = 50, y = 50;

        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name == nullptr) continue;

            P6HmiElement^ elem = gcnew P6HmiElement();
            String^ n = v->Name->ToLower();

            if (v->Scope == "input") {
                if (n->Contains("btn") || n->Contains("button") || n->Contains("start") || n->Contains("stop")) {
                    elem->Type = "Button";
                }
                else {
                    elem->Type = "Indicator";
                }
            }
            else if (v->Scope == "output") {
                if (n->Contains("motor") || n->Contains("valve") || n->Contains("cyl")) {
                    elem->Type = "Indicator";
                }
                else {
                    elem->Type = "Indicator";
                }
            }
            else {
                elem->Type = "StatusDisplay";
            }

            elem->Tag = v->Name;
            elem->Label = v->Name;
            elem->X = x;
            elem->Y = y;
            elem->Width = 100;
            elem->Height = 40;
            elements->Add(elem);

            y += 60;
            if (y > 600) { y = 50; x += 200; }
        }

        P6HmiElement^ alarmWindow = gcnew P6HmiElement();
        alarmWindow->Type = "AlarmWindow";
        alarmWindow->Tag = "Alarms";
        alarmWindow->Label = "Alarm Window";
        alarmWindow->X = 50;
        alarmWindow->Y = y + 60;
        alarmWindow->Width = 400;
        alarmWindow->Height = 200;
        elements->Add(alarmWindow);

        P6HmiElement^ paramPanel = gcnew P6HmiElement();
        paramPanel->Type = "ParameterPanel";
        paramPanel->Tag = "Parameters";
        paramPanel->Label = "Parameter Settings";
        paramPanel->X = x;
        paramPanel->Y = 50;
        paramPanel->Width = 300;
        paramPanel->Height = 400;
        elements->Add(paramPanel);

        P6HmiElement^ trendChart = gcnew P6HmiElement();
        trendChart->Type = "TrendChart";
        trendChart->Tag = "Trends";
        trendChart->Label = "Trend View";
        trendChart->X = x + 350;
        trendChart->Y = 50;
        trendChart->Width = 500;
        trendChart->Height = 300;
        elements->Add(trendChart);

        P6HmiElement^ ioStatus = gcnew P6HmiElement();
        ioStatus->Type = "IOStatusPanel";
        ioStatus->Tag = "IO_Status";
        ioStatus->Label = "IO Status Overview";
        ioStatus->X = x + 350;
        ioStatus->Y = 400;
        ioStatus->Width = 500;
        ioStatus->Height = 200;
        elements->Add(ioStatus);

        P5CompilerLog::Info("HMI", "Generated " + elements->Count + " HMI elements");
        return elements;
    }

    static String^ GenerateXml(List<P6HmiElement^>^ elements) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        sb->AppendLine("<HMI_Screen>");
        for each (P6HmiElement^ e in elements) {
            sb->AppendLine("  <Element type=\"" + e->Type + "\" tag=\"" + e->Tag + "\">");
            sb->AppendLine("    <Label>" + e->Label + "</Label>");
            sb->AppendLine("    <Position x=\"" + e->X + "\" y=\"" + e->Y + "\" w=\"" + e->Width + "\" h=\"" + e->Height + "\"/>");
            sb->AppendLine("  </Element>");
        }
        sb->AppendLine("</HMI_Screen>");
        return sb->ToString();
    }
};

ref class P6DocumentGenerator {
public:
    static List<P6DocumentSection^>^ Generate(P3Dsl^ dsl) {
        P5CompilerLog::Info("DOC", "Generating project documentation");
        List<P6DocumentSection^>^ sections = gcnew List<P6DocumentSection^>();

        sections->Add(GenerateIOTable(dsl));
        sections->Add(GenerateVariableTable(dsl));
        sections->Add(GenerateTimingDiagram(dsl));
        sections->Add(GenerateStateMachineDiagram(dsl));
        sections->Add(GenerateControlDescription(dsl));
        sections->Add(GenerateAlarmDescription(dsl));
        sections->Add(GeneratePointDescription(dsl));

        P5CompilerLog::Info("DOC", "Generated " + sections->Count + " document sections");
        return sections;
    }

    static String^ GenerateFullDocument(List<P6DocumentSection^>^ sections) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("=== PLC Project Documentation ===");
        sb->AppendLine();
        for each (P6DocumentSection^ s in sections) {
            sb->AppendLine("--- " + s->Title + " ---");
            sb->AppendLine(s->Content);
            sb->AppendLine();
        }
        return sb->ToString();
    }

private:
    static P6DocumentSection^ GenerateIOTable(P3Dsl^ dsl) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("Type | Name | Comment");
        sb->AppendLine("-----|------|--------");
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Scope == "input" || v->Scope == "output") {
                sb->AppendLine(v->Scope + " | " + v->Name + " | " + (v->Comment != nullptr ? v->Comment : ""));
            }
        }
        P6DocumentSection^ s = gcnew P6DocumentSection();
        s->Title = "IO Table";
        s->Content = sb->ToString();
        return s;
    }

    static P6DocumentSection^ GenerateVariableTable(P3Dsl^ dsl) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("Name | Type | Scope | Comment");
        sb->AppendLine("-----|------|-------|--------");
        for each (P3Variable^ v in dsl->Variables) {
            sb->AppendLine(v->Name + " | " + v->Type + " | " + v->Scope + " | " + (v->Comment != nullptr ? v->Comment : ""));
        }
        P6DocumentSection^ s = gcnew P6DocumentSection();
        s->Title = "Variable Table";
        s->Content = sb->ToString();
        return s;
    }

    static P6DocumentSection^ GenerateTimingDiagram(P3Dsl^ dsl) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("Network | Elements");
        sb->AppendLine("--------|---------");
        for each (P3Network^ net in dsl->Networks) {
            sb->Append(net->Title + " | ");
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) sb->Append(node->NodeType + ":" + node->Tag + " ");
            }
            sb->AppendLine();
        }
        P6DocumentSection^ s = gcnew P6DocumentSection();
        s->Title = "Timing Diagram";
        s->Content = sb->ToString();
        return s;
    }

    static P6DocumentSection^ GenerateStateMachineDiagram(P3Dsl^ dsl) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("digraph StateMachine {");
        sb->AppendLine("  rankdir=LR;");
        for each (P3Network^ net in dsl->Networks) {
            sb->AppendLine("  \"" + net->Title + "\";");
        }
        for (int i = 0; i < dsl->Networks->Count - 1; i++) {
            sb->AppendLine("  \"" + dsl->Networks[i]->Title + "\" -> \"" + dsl->Networks[i + 1]->Title + "\";");
        }
        sb->AppendLine("}");
        P6DocumentSection^ s = gcnew P6DocumentSection();
        s->Title = "State Machine Diagram";
        s->Content = sb->ToString();
        return s;
    }

    static P6DocumentSection^ GenerateControlDescription(P3Dsl^ dsl) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("Control Sequence:");
        int step = 1;
        for each (P3Network^ net in dsl->Networks) {
            sb->AppendLine("  Step " + step + ": " + net->Title);
            step++;
        }
        P6DocumentSection^ s = gcnew P6DocumentSection();
        s->Title = "Control Description";
        s->Content = sb->ToString();
        return s;
    }

    static P6DocumentSection^ GenerateAlarmDescription(P3Dsl^ dsl) {
        StringBuilder^ sb = gcnew StringBuilder();
        int alarmCount = 0;
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr && v->Name->ToLower()->Contains("alarm")) {
                sb->AppendLine("  Alarm: " + v->Name + " - " + (v->Comment != nullptr ? v->Comment : ""));
                alarmCount++;
            }
        }
        if (alarmCount == 0) sb->AppendLine("  No alarm variables defined.");
        P6DocumentSection^ s = gcnew P6DocumentSection();
        s->Title = "Alarm Description";
        s->Content = sb->ToString();
        return s;
    }

    static P6DocumentSection^ GeneratePointDescription(P3Dsl^ dsl) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("Point | Type | Description");
        sb->AppendLine("------|------|------------");
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Scope == "input" || v->Scope == "output") {
                sb->AppendLine(v->Name + " | " + v->Scope + " | " + (v->Comment != nullptr ? v->Comment : ""));
            }
        }
        P6DocumentSection^ s = gcnew P6DocumentSection();
        s->Title = "Point Description";
        s->Content = sb->ToString();
        return s;
    }
};

ref class P6DebugAI {
public:
    static String^ Diagnose(String^ question, P3Dsl^ dsl) {
        P5CompilerLog::Info("DEBUGAI", "Diagnosing: " + question);
        StringBuilder^ sb = gcnew StringBuilder();

        String^ q = question->ToLower();

        if (q->Contains("not return") || q->Contains("no return") || q->Contains("stuck forward")) {
            sb->AppendLine("Diagnosis: Return condition not met");
            sb->AppendLine();
            sb->AppendLine("Possible causes:");
            sb->AppendLine("  1. Return sensor signal not connected");
            sb->AppendLine("  2. Forward output not reset before return");
            sb->AppendLine("  3. Interlock preventing return motion");
            sb->AppendLine();
            sb->AppendLine("Checking DSL:");
            bool hasReturnReset = false;
            for each (P3Network^ net in dsl->Networks) {
                for each (Object^ item in net->Items) {
                    P3Node^ node = dynamic_cast<P3Node^>(item);
                    if (node != nullptr && node->Tag != nullptr) {
                        String^ t = node->NodeType->ToLower();
                        if ((t == "reset" || t == "rcoil") &&
                            (node->Tag->ToLower()->Contains("fwd") || node->Tag->ToLower()->Contains("forward"))) {
                            hasReturnReset = true;
                        }
                    }
                }
            }
            if (!hasReturnReset) {
                sb->AppendLine("FOUND: No RESET for forward output before return!");
                sb->AppendLine("Fix: Add reset for forward output in return network");
            }
        }
        else if (q->Contains("not move") || q->Contains("no motion") || q->Contains("not start")) {
            sb->AppendLine("Diagnosis: Motion not starting");
            sb->AppendLine();
            sb->AppendLine("Possible causes:");
            sb->AppendLine("  1. Start condition not met");
            sb->AppendLine("  2. E-Stop active");
            sb->AppendLine("  3. Safety interlock preventing start");
        }
        else if (q->Contains("timeout") || q->Contains("time out") || q->Contains("overtime")) {
            sb->AppendLine("Diagnosis: Timeout issue");
            sb->AppendLine();
            sb->AppendLine("Possible causes:");
            sb->AppendLine("  1. Sensor not detecting position");
            sb->AppendLine("  2. Timer PT value too short");
            sb->AppendLine("  3. Mechanical jam");
        }
        else {
            sb->AppendLine("Diagnosis: General analysis");
            sb->AppendLine();
            sb->AppendLine("Program stats:");
            sb->AppendLine("  Variables: " + dsl->Variables->Count);
            sb->AppendLine("  Networks: " + dsl->Networks->Count);
        }

        return sb->ToString();
    }
};

ref class P6SoftPlcRuntime {
public:
    static List<String^>^ Run(P3Dsl^ dsl, int maxCycles, Dictionary<String^, bool>^ initialInputs) {
        P5CompilerLog::Info("SOFTPLC", "Starting SoftPLC runtime for " + maxCycles + " cycles");
        List<String^>^ runtimeLog = gcnew List<String^>();

        P6SimulationState^ state = P6SimulationEngine::CreateInitialState(dsl);

        for each (KeyValuePair<String^, bool>^ kv in initialInputs) {
            state->BoolVars[kv->Key] = kv->Value;
        }

        runtimeLog->Add("SoftPLC Runtime Started");
        runtimeLog->Add("Variables: " + dsl->Variables->Count);
        runtimeLog->Add("Networks: " + dsl->Networks->Count);
        runtimeLog->Add("---");

        for (int cycle = 1; cycle <= maxCycles; cycle++) {
            runtimeLog->Add("[Cycle " + cycle + "]");

            runtimeLog->Add("  READ INPUTS:");
            for each (P3Variable^ v in dsl->Variables) {
                if (v->Scope == "input" && v->Name != nullptr && state->BoolVars->ContainsKey(v->Name)) {
                    runtimeLog->Add("    " + v->Name + " = " + (state->BoolVars[v->Name] ? "1" : "0"));
                }
            }

            state = P6SimulationEngine::ExecuteScanCycle(dsl, state);

            runtimeLog->Add("  EXECUTE + WRITE OUTPUTS:");
            for each (P3Variable^ v in dsl->Variables) {
                if (v->Scope == "output" && v->Name != nullptr && state->BoolVars->ContainsKey(v->Name)) {
                    runtimeLog->Add("    " + v->Name + " = " + (state->BoolVars[v->Name] ? "1" : "0"));
                }
            }

            for each (KeyValuePair<String^, String^>^ kv in state->TimerStates) {
                runtimeLog->Add("    Timer " + kv->Key + " = " + kv->Value);
            }
            for each (KeyValuePair<String^, int>^ kv in state->CounterValues) {
                runtimeLog->Add("    Counter " + kv->Key + " = " + kv->Value);
            }

            runtimeLog->Add("---");
        }

        runtimeLog->Add("SoftPLC Runtime Stopped after " + maxCycles + " cycles");
        P5CompilerLog::Info("SOFTPLC", "SoftPLC runtime complete");
        return runtimeLog;
    }

    static Dictionary<String^, bool>^ GetOutputs(P3Dsl^ dsl, Dictionary<String^, bool>^ inputs) {
        P6SimulationState^ state = P6SimulationEngine::CreateInitialState(dsl);
        for each (KeyValuePair<String^, bool>^ kv in inputs) {
            state->BoolVars[kv->Key] = kv->Value;
        }
        state = P6SimulationEngine::ExecuteScanCycle(dsl, state);

        Dictionary<String^, bool>^ outputs = gcnew Dictionary<String^, bool>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Scope == "output" && v->Name != nullptr && state->BoolVars->ContainsKey(v->Name)) {
                outputs[v->Name] = state->BoolVars[v->Name];
            }
        }
        return outputs;
    }

    static String^ ExportRuntimeState(P6SimulationState^ state) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("=== SoftPLC Runtime State ===");
        sb->AppendLine("Cycle: " + state->CycleCount);
        sb->AppendLine("Bool Variables:");
        for each (KeyValuePair<String^, bool>^ kv in state->BoolVars) {
            sb->AppendLine("  " + kv->Key + " = " + (kv->Value ? "TRUE" : "FALSE"));
        }
        sb->AppendLine("Int Variables:");
        for each (KeyValuePair<String^, int>^ kv in state->IntVars) {
            sb->AppendLine("  " + kv->Key + " = " + kv->Value);
        }
        sb->AppendLine("Timer States:");
        for each (KeyValuePair<String^, String^>^ kv in state->TimerStates) {
            sb->AppendLine("  " + kv->Key + " = " + kv->Value);
        }
        sb->AppendLine("Counter Values:");
        for each (KeyValuePair<String^, int>^ kv in state->CounterValues) {
            sb->AppendLine("  " + kv->Key + " = " + kv->Value);
        }
        return sb->ToString();
    }
};

ref class P6Pipeline {
public:
    static P3PipelineResult^ RunPhase6(String^ problem, P3Config^ config, String^ templateXmlPath) {
        Console::WriteLine("=== Phase 6 AI Agent Pipeline ===");
        Console::WriteLine();
        P5CompilerLog::Info("AGENT", "Problem: " + problem);

        try
        {
        P5CompilerLog::Info("TASK", "Step 1: Task Planning...");
        List<P6Task^>^ tasks = P6TaskPlanner::Plan(problem);
        for each (P6Task^ task in tasks) {
            P5CompilerLog::Info("TASK", "  [" + task->Id + "] " + task->Description);
        }

        P5CompilerLog::Info("SEMANTIC", "Step 2: Semantic Parsing...");
        List<P6ControlAction^>^ actions = P6SemanticEngine::ParseNaturalLanguage(problem);
        for each (P6ControlAction^ act in actions) {
            P5CompilerLog::Info("SEMANTIC", "  " + act->Type + " -> " + act->Target);
        }

        P5CompilerLog::Info("REASON", "Step 3: Control Reasoning...");
        List<String^>^ stepNames = gcnew List<String^>();
        for each (P6ControlAction^ act in actions) {
            stepNames->Add(act->Target);
        }
        List<P4State^>^ states = P6ControlReasoningEngine::ReasonStateMachine(stepNames);
        List<P6SafetyRule^>^ safetyRules = P6ControlReasoningEngine::ReasonSafety(nullptr);
        P5CompilerLog::Info("REASON", "  States: " + states->Count + ", Safety rules: " + safetyRules->Count);

        P5CompilerLog::Info("SYNTH", "Step 4: Program Synthesis...");
        P4Requirement^ requirement = P4RequirementParser::Parse(problem, config);
        // #region debug-point B:requirement-result
        {
            String^ reqInfo = "Inputs=" + (requirement->Inputs != nullptr ? requirement->Inputs->Count.ToString() : "null")
                + ", Outputs=" + (requirement->Outputs != nullptr ? requirement->Outputs->Count.ToString() : "null")
                + ", Timers=" + (requirement->Timers != nullptr ? requirement->Timers->Count.ToString() : "null")
                + ", ControlType=" + (requirement->ControlType != nullptr ? requirement->ControlType : "null");
            P5CompilerLog::Info("DEBUG-B", "Requirement: " + reqInfo);
            if (requirement->Outputs != nullptr) {
                for each (P4SignalDecl^ s in requirement->Outputs) {
                    P5CompilerLog::Info("DEBUG-B", "  Output: " + s->Name + " (" + s->Comment + ")");
                }
            }
            if (requirement->Timers != nullptr) {
                for each (P4TimerRequirement^ t in requirement->Timers) {
                    P5CompilerLog::Info("DEBUG-B", "  Timer: " + t->Name + " PT=" + t->Preset);
                }
            }
        }
        // #endregion
        P4SemanticPlan^ semanticPlan = P4SemanticPlanner::Plan(problem, requirement, config);
        // #region debug-point B:semantic-result
        {
            String^ semInfo = "IsSequential=" + semanticPlan->IsSequential
                + ", IsCyclic=" + semanticPlan->IsCyclic
                + ", HasAutoRoundTrip=" + semanticPlan->HasAutoRoundTrip
                + ", HasInterlock=" + semanticPlan->HasInterlock
                + ", HasTimerDelay=" + semanticPlan->HasTimerDelay
                + ", States=" + (semanticPlan->States != nullptr ? semanticPlan->States->Count.ToString() : "null");
            P5CompilerLog::Info("DEBUG-B", "SemanticPlan: " + semInfo);
            if (semanticPlan->States != nullptr) {
                for (int i = 0; i < semanticPlan->States->Count; i++) {
                    P4State^ st = semanticPlan->States[i];
                    P5CompilerLog::Info("DEBUG-B", "  State[" + i + "]: " + st->Name + " Action=" + st->Action + " Trans=" + st->TransitionCondition);
                }
            }
        }
        // #endregion
        P4VariablePlan^ varPlan = P4VariablePlanner::Plan(requirement, semanticPlan);
        P3Dsl^ dsl = P4StateMachineBuilder::Build(requirement, semanticPlan, varPlan);
        P3VariableEngine::AutoGenerateVariables(dsl);
        P5CompilerLog::Info("SYNTH", "  Variables: " + dsl->Variables->Count + ", Networks: " + dsl->Networks->Count);

        P5CompilerLog::Info("RULE", "Step 5: Industrial Rule Engine...");
        List<P6SafetyRule^>^ rules = P6IndustrialRuleEngine::ApplyRules(dsl);
        for each (P6SafetyRule^ r in rules) {
            if (!r->Applied) P5CompilerLog::Warn("RULE", "  " + r->Description);
        }

        P5CompilerLog::Info("VAL", "Step 6: Validation...");
        P3ValidationResult^ validation = P5SemanticValidator::Validate(dsl);
        if (!validation->IsValid) {
            P5CompilerLog::Info("SELFREPAIR", "Step 6b: Self-Repair...");
            List<String^>^ repairs = P6SelfRepairEngine::SelfRepair(dsl);
            for each (String^ r in repairs) P5CompilerLog::Info("SELFREPAIR", "  " + r);
            validation = P5SemanticValidator::Validate(dsl);
        }

        P5CompilerLog::Info("CFG", "Step 7: CFG Analysis...");
        ControlFlowGraph^ cfg = P5CfgAnalyzer::BuildCfg(dsl);
        P5CfgAnalyzer::DetectCycles(cfg);
        P5CfgAnalyzer::CompressStates(dsl);

        P5CompilerLog::Info("IR", "Step 8: IR Conversion + Optimization...");
        P5IRProgram^ ir = P5DslToIrConverter::Convert(dsl);
        P5Optimizer::Optimize(ir);

        P5CompilerLog::Info("SIM", "Step 9: Simulation...");
        Dictionary<String^, bool>^ simInputs = gcnew Dictionary<String^, bool>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Scope == "input" && v->Name != nullptr) simInputs[v->Name] = false;
        }
        List<String^>^ simLog = P6SimulationEngine::RunSimulation(dsl, 3, simInputs);
        P5CompilerLog::Info("SIM", "  Simulation: " + simLog->Count + " log entries");

        P5CompilerLog::Info("BACKEND", "Step 10: Multi-Backend Generation...");
        LadDsl^ ladDsl = P3DslConverter::ToLadDsl(dsl);
        String^ xml = BuildLadXml(ladDsl, templateXmlPath);
        String^ sclCode = P5BackendDispatcher::Generate(ir, P5BackendType::SCL);
        String^ stlCode = P5BackendDispatcher::Generate(ir, P5BackendType::STL);

        P5CompilerLog::Info("HMI", "Step 11: HMI Generation...");
        List<P6HmiElement^>^ hmiElements = P6HmiGenerator::Generate(dsl);
        String^ hmiXml = P6HmiGenerator::GenerateXml(hmiElements);

        P5CompilerLog::Info("DOC", "Step 12: Document Generation...");
        List<P6DocumentSection^>^ docSections = P6DocumentGenerator::Generate(dsl);
        String^ fullDoc = P6DocumentGenerator::GenerateFullDocument(docSections);

        P5CompilerLog::Info("ALARM", "Step 13: Alarm Logic Generation...");
        List<String^>^ alarms = P6ProgramSynthesisEngine::GenerateAlarmLogic(dsl);
        List<String^>^ hmiVars = P6ProgramSynthesisEngine::GenerateHMIVariables(dsl);

        if (xml == nullptr || xml->Length == 0) {
            P3PipelineResult^ r = gcnew P3PipelineResult();
            r->Dsl = dsl;
            r->ErrorMessage = "Failed to compile LAD to XML";
            return r;
        }

        Console::WriteLine();
        Console::WriteLine("=== Phase 6 Agent Pipeline Complete ===");
        Console::WriteLine("  Tasks planned: " + tasks->Count);
        Console::WriteLine("  Control actions: " + actions->Count);
        Console::WriteLine("  Safety rules: " + rules->Count);
        Console::WriteLine("  LAD XML: " + xml->Length + " chars");
        Console::WriteLine("  SCL Code: " + sclCode->Length + " chars");
        Console::WriteLine("  STL Code: " + stlCode->Length + " chars");
        Console::WriteLine("  HMI Elements: " + hmiElements->Count);
        Console::WriteLine("  Document Sections: " + docSections->Count);
        Console::WriteLine("  Alarms: " + alarms->Count);
        Console::WriteLine("  Simulation: " + simLog->Count + " log entries");
        Console::WriteLine("  Validation: " + (validation->IsValid ? "PASSED" : "FAILED"));

        P3PipelineResult^ result = gcnew P3PipelineResult();
        result->Xml = xml;
        result->Dsl = dsl;
        result->TagTableXml = P3TagTableGenerator::GenerateTagTableXml(dsl);
        result->Success = true;
        return result;

        }
        catch (Exception^ ex)
        {
            Console::WriteLine();
            Console::WriteLine("=== Phase 6 Pipeline FAILED ===");
            Console::WriteLine("  Exception: " + ex->Message);
            if (ex->InnerException != nullptr)
                Console::WriteLine("  Inner: " + ex->InnerException->Message);
            Console::WriteLine("  Stack: " + ex->StackTrace);

            P3PipelineResult^ r = gcnew P3PipelineResult();
            r->ErrorMessage = "Phase6 pipeline error: " + ex->Message;
            if (ex->InnerException != nullptr)
                r->ErrorMessage += " -> " + ex->InnerException->Message;
            return r;
        }
    }
};
