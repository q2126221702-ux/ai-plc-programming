#pragma once
#include "DataTypes.h"

inline String^ EscapeJson(String^ s) {
    if (s == nullptr) return "null";
    s = s->Replace("\\", "\\\\");
    s = s->Replace("\"", "\\\"");
    s = s->Replace("\n", "\\n");
    s = s->Replace("\r", "\\r");
    s = s->Replace("\t", "\\t");
    return "\"" + s + "\"";
}

inline String^ LadDslToJson(LadDsl^ dsl) {
    System::Text::StringBuilder^ sb = gcnew System::Text::StringBuilder();
    sb->Append("{\"networks\":[");
    for (int ni = 0; ni < dsl->Networks->Count; ni++) {
        if (ni > 0) sb->Append(",");
        LadNetwork^ net = dsl->Networks[ni];
        sb->Append("{\"number\":");
        sb->Append(Convert::ToString(net->Number));
        sb->Append(",\"title\":");
        sb->Append(EscapeJson(net->Title));
        sb->Append(",\"elements\":[");
        for (int ei = 0; ei < net->Elements->Count; ei++) {
            if (ei > 0) sb->Append(",");
            LadElement^ el = net->Elements[ei];
            sb->Append("{\"type\":");
            sb->Append(EscapeJson(el->Type));
            if (el->Type == "parallel" && el->Branches != nullptr) {
                sb->Append(",\"branches\":[");
                for (int bi = 0; bi < el->Branches->Count; bi++) {
                    if (bi > 0) sb->Append(",");
                    sb->Append("[");
                    List<LadElement^>^ branch = el->Branches[bi];
                    for (int bei = 0; bei < branch->Count; bei++) {
                        if (bei > 0) sb->Append(",");
                        LadElement^ bel = branch[bei];
                        sb->Append("{\"type\":");
                        sb->Append(EscapeJson(bel->Type));
                        if (bel->Tag != nullptr && bel->Tag->Length > 0) {
                            sb->Append(",\"tag\":");
                            sb->Append(EscapeJson(bel->Tag));
                        }
                        if (bel->Type == "contact") {
                            sb->Append(",\"normallyOpen\":");
                            sb->Append(bel->NormallyOpen ? gcnew String("true") : gcnew String("false"));
                        }
                        if (bel->Type == "timerOnDelay" || bel->Type == "timerOffDelay" || bel->Type == "timerPulse" || bel->Type == "counterUp" || bel->Type == "counterDown" || bel->Type == "counterUpDown") {
                            if (bel->InstanceName != nullptr && bel->InstanceName->Length > 0) {
                                sb->Append(",\"instance\":");
                                sb->Append(EscapeJson(bel->InstanceName));
                            }
                            if (bel->PresetTime != nullptr && bel->PresetTime->Length > 0) {
                                sb->Append(",\"pt\":");
                                sb->Append(EscapeJson(bel->PresetTime));
                            }
                        }
                        if (bel->Type == "compareEQ" || bel->Type == "compareNE" || bel->Type == "compareGT" ||
                            bel->Type == "compareLT" || bel->Type == "compareGE" || bel->Type == "compareLE" ||
                            bel->Type == "add" || bel->Type == "sub" || bel->Type == "mul" ||
                            bel->Type == "div" || bel->Type == "mod" || bel->Type == "move") {
                            if (bel->Tag2 != nullptr && bel->Tag2->Length > 0) {
                                sb->Append(",\"tag2\":");
                                sb->Append(EscapeJson(bel->Tag2));
                            }
                        }
                        if (bel->Type == "add" || bel->Type == "sub" || bel->Type == "mul" ||
                            bel->Type == "div" || bel->Type == "mod") {
                            if (bel->Tag3 != nullptr && bel->Tag3->Length > 0) {
                                sb->Append(",\"tag3\":");
                                sb->Append(EscapeJson(bel->Tag3));
                            }
                        }
                        if ((bel->Type == "add" || bel->Type == "sub" || bel->Type == "mul" ||
                             bel->Type == "div" || bel->Type == "mod" || bel->Type == "move") &&
                            bel->DataType != nullptr && bel->DataType->Length > 0) {
                            sb->Append(",\"dataType\":");
                            sb->Append(EscapeJson(bel->DataType));
                        }
                        sb->Append("}");
                    }
                    sb->Append("]");
                }
                sb->Append("]");
            }
            else {
                if (el->Tag != nullptr && el->Tag->Length > 0) {
                    sb->Append(",\"tag\":");
                    sb->Append(EscapeJson(el->Tag));
                }
                if (el->Type == "contact") {
                    sb->Append(",\"normallyOpen\":");
                    sb->Append(el->NormallyOpen ? gcnew String("true") : gcnew String("false"));
                }
                if (el->Type == "timerOnDelay" || el->Type == "timerOffDelay" || el->Type == "timerPulse" || el->Type == "counterUp" || el->Type == "counterDown" || el->Type == "counterUpDown") {
                    if (el->InstanceName != nullptr && el->InstanceName->Length > 0) {
                        sb->Append(",\"instance\":");
                        sb->Append(EscapeJson(el->InstanceName));
                    }
                    if (el->PresetTime != nullptr && el->PresetTime->Length > 0) {
                        sb->Append(",\"pt\":");
                        sb->Append(EscapeJson(el->PresetTime));
                    }
                }
                if (el->Type == "compareEQ" || el->Type == "compareNE" || el->Type == "compareGT" ||
                    el->Type == "compareLT" || el->Type == "compareGE" || el->Type == "compareLE" ||
                    el->Type == "add" || el->Type == "sub" || el->Type == "mul" ||
                    el->Type == "div" || el->Type == "mod" || el->Type == "move") {
                    if (el->Tag2 != nullptr && el->Tag2->Length > 0) {
                        sb->Append(",\"tag2\":");
                        sb->Append(EscapeJson(el->Tag2));
                    }
                }
                if (el->Type == "add" || el->Type == "sub" || el->Type == "mul" ||
                    el->Type == "div" || el->Type == "mod") {
                    if (el->Tag3 != nullptr && el->Tag3->Length > 0) {
                        sb->Append(",\"tag3\":");
                        sb->Append(EscapeJson(el->Tag3));
                    }
                }
                if ((el->Type == "add" || el->Type == "sub" || el->Type == "mul" ||
                     el->Type == "div" || el->Type == "mod" || el->Type == "move") &&
                    el->DataType != nullptr && el->DataType->Length > 0) {
                    sb->Append(",\"dataType\":");
                    sb->Append(EscapeJson(el->DataType));
                }
            }
            sb->Append("}");
        }
        sb->Append("]");
        sb->Append("}");
    }
    sb->Append("]}");
    return sb->ToString();
}

inline LadDsl^ JsonToLadDsl(String^ json) {
    LadDsl^ dsl = gcnew LadDsl();
    dsl->Networks = gcnew List<LadNetwork^>();

    String^ s = json->Trim();
    if (!s->StartsWith("{")) return dsl;

    s = s->Substring(1, s->Length - 2);
    int netStart = s->IndexOf("\"networks\"");
    if (netStart < 0) return dsl;
    int arrStart = s->IndexOf('[', netStart);
    if (arrStart < 0) return dsl;

    array<Char>^ sc = s->ToCharArray();

    int depth = 0;
    int netObjStart = -1;
    for (int i = arrStart + 1; i < sc->Length; i++) {
        if (sc[i] == '{') {
            if (depth == 0) netObjStart = i;
            depth++;
        }
        else if (sc[i] == '}') {
            depth--;
            if (depth == 0 && netObjStart >= 0) {
                String^ netJson = s->Substring(netObjStart, i - netObjStart + 1);
                LadNetwork^ net = gcnew LadNetwork();
                net->Number = 1;
                net->Title = gcnew String("");
                net->Elements = gcnew List<LadElement^>();

                int numIdx = netJson->IndexOf("\"number\"");
                if (numIdx >= 0) {
                    int colon = netJson->IndexOf(':', numIdx);
                    if (colon >= 0) {
                        int valEnd = netJson->IndexOfAny(gcnew array<Char>{',', '}'}, colon);
                        if (valEnd >= 0) {
                            String^ numStr = netJson->Substring(colon + 1, valEnd - colon - 1)->Trim();
                            int num;
                            if (Int32::TryParse(numStr, num)) net->Number = num;
                        }
                    }
                }

                int ttlIdx = netJson->IndexOf("\"title\"");
                if (ttlIdx >= 0) {
                    int colon = netJson->IndexOf(':', ttlIdx);
                    if (colon >= 0) {
                        int quoteStart = netJson->IndexOf('"', colon + 1);
                        if (quoteStart >= 0) {
                            int quoteEnd = netJson->IndexOf('"', quoteStart + 1);
                            if (quoteEnd >= 0) {
                                net->Title = netJson->Substring(quoteStart + 1, quoteEnd - quoteStart - 1);
                            }
                        }
                    }
                }

                int elemsIdx = netJson->IndexOf("\"elements\"");
                if (elemsIdx >= 0) {
                    int elemArr = netJson->IndexOf('[', elemsIdx);
                    if (elemArr >= 0) {
                        array<Char>^ njs = netJson->ToCharArray();
                        int ed = 0;
                        int eObjStart = -1;
                        for (int j = elemArr + 1; j < njs->Length; j++) {
                            if (njs[j] == '{') {
                                if (ed == 0) eObjStart = j;
                                ed++;
                            }
                            else if (njs[j] == '}') {
                                ed--;
                                if (ed == 0 && eObjStart >= 0) {
                                    String^ ej = netJson->Substring(eObjStart, j - eObjStart + 1);
                                    LadElement^ el = gcnew LadElement();
                                    el->Type = "";
                                    el->Tag = "";
                                    el->NormallyOpen = true;
                                    el->InstanceName = "";
                                    el->PresetTime = "";
                                    el->Tag2 = "";

                                    int tpIdx = ej->IndexOf("\"type\"");
                                    if (tpIdx >= 0) {
                                        int q1 = ej->IndexOf('"', tpIdx + 7);
                                        if (q1 >= 0) {
                                            int q2 = ej->IndexOf('"', q1 + 1);
                                            if (q2 >= 0)
                                                el->Type = ej->Substring(q1 + 1, q2 - q1 - 1);
                                        }
                                    }

                                    int tgIdx = ej->IndexOf("\"tag\"");
                                    if (tgIdx >= 0) {
                                        int q1 = ej->IndexOf('"', tgIdx + 6);
                                        if (q1 >= 0) {
                                            int q2 = ej->IndexOf('"', q1 + 1);
                                            if (q2 >= 0)
                                                el->Tag = ej->Substring(q1 + 1, q2 - q1 - 1);
                                        }
                                    }

                                    int noIdx = ej->IndexOf("\"normallyOpen\"");
                                    if (noIdx >= 0) {
                                        int colon = ej->IndexOf(':', noIdx);
                                        if (colon >= 0) {
                                            int endPos = colon + 1 + 5;
                                            if (endPos > ej->Length) endPos = ej->Length;
                                            String^ val = ej->Substring(colon + 1, endPos - colon - 1)->Trim()->ToLower();
                                            el->NormallyOpen = val->StartsWith("true");
                                        }
                                    }

                                    int insIdx = ej->IndexOf("\"instance\"");
                                    if (insIdx >= 0) {
                                        int q1 = ej->IndexOf('"', insIdx + 10);
                                        if (q1 >= 0) {
                                            int q2 = ej->IndexOf('"', q1 + 1);
                                            if (q2 >= 0)
                                                el->InstanceName = ej->Substring(q1 + 1, q2 - q1 - 1);
                                        }
                                    }

                                    int ptIdx = ej->IndexOf("\"pt\"");
                                    if (ptIdx >= 0) {
                                        int q1 = ej->IndexOf('"', ptIdx + 5);
                                        if (q1 >= 0) {
                                            int q2 = ej->IndexOf('"', q1 + 1);
                                            if (q2 >= 0)
                                                el->PresetTime = ej->Substring(q1 + 1, q2 - q1 - 1);
                                        }
                                    }

                                    int tg2Idx = ej->IndexOf("\"tag2\"");
                                    if (tg2Idx >= 0) {
                                        int q1 = ej->IndexOf('"', tg2Idx + 7);
                                        if (q1 >= 0) {
                                            int q2 = ej->IndexOf('"', q1 + 1);
                                            if (q2 >= 0)
                                                el->Tag2 = ej->Substring(q1 + 1, q2 - q1 - 1);
                                        }
                                    }

                                    int tg3Idx = ej->IndexOf("\"tag3\"");
                                    if (tg3Idx >= 0) {
                                        int q1 = ej->IndexOf('"', tg3Idx + 7);
                                        if (q1 >= 0) {
                                            int q2 = ej->IndexOf('"', q1 + 1);
                                            if (q2 >= 0)
                                                el->Tag3 = ej->Substring(q1 + 1, q2 - q1 - 1);
                                        }
                                    }

                                    int dtIdx = ej->IndexOf("\"dataType\"");
                                    if (dtIdx >= 0) {
                                        int q1 = ej->IndexOf('"', dtIdx + 11);
                                        if (q1 >= 0) {
                                            int q2 = ej->IndexOf('"', q1 + 1);
                                            if (q2 >= 0)
                                                el->DataType = ej->Substring(q1 + 1, q2 - q1 - 1);
                                        }
                                    }

                                    if (el->Type == "parallel") {
                                        int brIdx = ej->IndexOf("\"branches\"");
                                        if (brIdx >= 0) {
                                            int brArrStart = ej->IndexOf('[', brIdx);
                                            if (brArrStart >= 0) {
                                                el->Branches = gcnew List<List<LadElement^>^>();
                                                array<Char>^ brChars = ej->ToCharArray();
                                                int brDepth = 0;
                                                int bStart = -1;
                                                for (int bk = brArrStart + 1; bk < brChars->Length; bk++) {
                                                    if (brChars[bk] == '[') {
                                                        if (brDepth == 0) bStart = bk;
                                                        brDepth++;
                                                    }
                                                    else if (brChars[bk] == ']') {
                                                        brDepth--;
                                                        if (brDepth == 0 && bStart >= 0) {
                                                            String^ branchJson = ej->Substring(bStart, bk - bStart + 1);
                                                            List<LadElement^>^ branchEls = gcnew List<LadElement^>();
                                                            array<Char>^ bjs = branchJson->ToCharArray();
                                                            int bed = 0;
                                                            int beStart = -1;
                                                            for (int bc = 1; bc < bjs->Length; bc++) {
                                                                if (bjs[bc] == '{') {
                                                                    if (bed == 0) beStart = bc;
                                                                    bed++;
                                                                }
                                                                else if (bjs[bc] == '}') {
                                                                    bed--;
                                                                    if (bed == 0 && beStart >= 0) {
                                                                        String^ bej = branchJson->Substring(beStart, bc - beStart + 1);
                                                                        LadElement^ bel = gcnew LadElement();
                                                                        bel->Type = "";
                                                                        bel->Tag = "";
                                                                        bel->NormallyOpen = true;
                                                                        bel->InstanceName = "";
                                                                        bel->PresetTime = "";
                                                                        bel->Tag2 = "";
                                                                        int btpIdx = bej->IndexOf("\"type\"");
                                                                        if (btpIdx >= 0) {
                                                                            int bq1 = bej->IndexOf('"', btpIdx + 7);
                                                                            if (bq1 >= 0) {
                                                                                int bq2 = bej->IndexOf('"', bq1 + 1);
                                                                                if (bq2 >= 0) bel->Type = bej->Substring(bq1 + 1, bq2 - bq1 - 1);
                                                                            }
                                                                        }
                                                                        int btgIdx = bej->IndexOf("\"tag\"");
                                                                        if (btgIdx >= 0) {
                                                                            int bq1 = bej->IndexOf('"', btgIdx + 6);
                                                                            if (bq1 >= 0) {
                                                                                int bq2 = bej->IndexOf('"', bq1 + 1);
                                                                                if (bq2 >= 0) bel->Tag = bej->Substring(bq1 + 1, bq2 - bq1 - 1);
                                                                            }
                                                                        }
                                                                        int bnoIdx = bej->IndexOf("\"normallyOpen\"");
                                                                        if (bnoIdx >= 0) {
                                                                            int bcolon = bej->IndexOf(':', bnoIdx);
                                                                            if (bcolon >= 0) {
                                                                                String^ bval = bej->Substring(bcolon + 1)->Trim()->ToLower();
                                                                                if (bval->Length > 5) bval = bval->Substring(0, 5);
                                                                                bel->NormallyOpen = bval->StartsWith("true");
                                                                            }
                                                                        }
                                                                        int binsIdx = bej->IndexOf("\"instance\"");
                                                                        if (binsIdx >= 0) {
                                                                            int bq1 = bej->IndexOf('"', binsIdx + 10);
                                                                            if (bq1 >= 0) {
                                                                                int bq2 = bej->IndexOf('"', bq1 + 1);
                                                                                if (bq2 >= 0) bel->InstanceName = bej->Substring(bq1 + 1, bq2 - bq1 - 1);
                                                                            }
                                                                        }
                                                                        int bptIdx = bej->IndexOf("\"pt\"");
                                                                        if (bptIdx >= 0) {
                                                                            int bq1 = bej->IndexOf('"', bptIdx + 5);
                                                                            if (bq1 >= 0) {
                                                                                int bq2 = bej->IndexOf('"', bq1 + 1);
                                                                                if (bq2 >= 0) bel->PresetTime = bej->Substring(bq1 + 1, bq2 - bq1 - 1);
                                                                            }
                                                                        }
                                                                        int btg2Idx = bej->IndexOf("\"tag2\"");
                                                                        if (btg2Idx >= 0) {
                                                                            int bq1 = bej->IndexOf('"', btg2Idx + 7);
                                                                            if (bq1 >= 0) {
                                                                                int bq2 = bej->IndexOf('"', bq1 + 1);
                                                                                if (bq2 >= 0) bel->Tag2 = bej->Substring(bq1 + 1, bq2 - bq1 - 1);
                                                                            }
                                                                        }
                                                                        int btg3Idx = bej->IndexOf("\"tag3\"");
                                                                        if (btg3Idx >= 0) {
                                                                            int bq1 = bej->IndexOf('"', btg3Idx + 7);
                                                                            if (bq1 >= 0) {
                                                                                int bq2 = bej->IndexOf('"', bq1 + 1);
                                                                                if (bq2 >= 0) bel->Tag3 = bej->Substring(bq1 + 1, bq2 - bq1 - 1);
                                                                            }
                                                                        }
                                                                        int bdtIdx = bej->IndexOf("\"dataType\"");
                                                                        if (bdtIdx >= 0) {
                                                                            int bq1 = bej->IndexOf('"', bdtIdx + 11);
                                                                            if (bq1 >= 0) {
                                                                                int bq2 = bej->IndexOf('"', bq1 + 1);
                                                                                if (bq2 >= 0) bel->DataType = bej->Substring(bq1 + 1, bq2 - bq1 - 1);
                                                                            }
                                                                        }
                                                                        branchEls->Add(bel);
                                                                        beStart = -1;
                                                                    }
                                                                }
                                                            }
                                                            el->Branches->Add(branchEls);
                                                            bStart = -1;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    net->Elements->Add(el);
                                    eObjStart = -1;
                                }
                            }
                        }
                    }
                }

                dsl->Networks->Add(net);
                netObjStart = -1;
            }
        }
    }

    return dsl;
}