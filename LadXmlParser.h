#pragma once
#include "DataTypes.h"

ref struct ParsedPartInfo {
    String^ Type;
    bool Negated;
    String^ InstanceName;
    String^ TimerVersion;
    String^ PtAccessUid;
    int Cardinality;
    String^ In1AccessUid;
    String^ In2AccessUid;
    String^ OutAccessUid;
    String^ DataType;
};

ref struct ParsedAccessInfo {
    String^ Scope;
    String^ Symbol;
    String^ ConstantValue;
    String^ ConstantType;
    int TargetPartUid;
};

ref struct ParsedWireInfo {
    String^ FromKind;
    String^ FromPin;
    int FromUid;
    String^ ToKind;
    String^ ToPin;
    int ToUid;
    List<CuWireTarget^>^ ExtraTargets;
};

ref struct ParseContext {
    Dictionary<String^, ParsedPartInfo^>^ PartInfo;
    Dictionary<String^, ParsedAccessInfo^>^ AccessInfo;
    Dictionary<String^, String^>^ OperandMap;
    Dictionary<String^, String^>^ BitMap;
    Dictionary<String^, String^>^ NMap;
    Dictionary<String^, String^>^ NextMap;
    Dictionary<String^, List<String^>^>^ MultiNextMap;
};

ref struct BranchResult {
    bool HasBranches;
    String^ NextUid;
    bool ShouldContinue;
};

inline BranchResult^ ProcessParallelBranches(String^ currentUid, ParseContext^ ctx, LadNetwork^ net) {
    BranchResult^ result = gcnew BranchResult();
    result->HasBranches = false;
    result->ShouldContinue = false;

    if (!ctx->MultiNextMap->ContainsKey(currentUid)) {
        return result;
    }

    result->HasBranches = true;
    List<String^>^ mBranches = ctx->MultiNextMap[currentUid];
    LadElement^ parallel = gcnew LadElement();
    parallel->Type = "parallel";
    parallel->Branches = gcnew List<List<LadElement^>^>();

    for (int bi = 0; bi < mBranches->Count; bi++) {
        List<LadElement^>^ branch = gcnew List<LadElement^>();
        String^ brUid = mBranches[bi];
        HashSet<String^>^ brVisited = gcnew HashSet<String^>();
        while (brUid->Length > 0 && ctx->PartInfo->ContainsKey(brUid) && !brVisited->Contains(brUid)) {
            brVisited->Add(brUid);
            ParsedPartInfo^ bpi = ctx->PartInfo[brUid];
            String^ bpt = bpi->Type;
            if (bpt == "Contact") {
                String^ boperUid = "";
                if (ctx->OperandMap->TryGetValue(brUid, boperUid) && ctx->AccessInfo->ContainsKey(boperUid)) {
                    ParsedAccessInfo^ bacc = ctx->AccessInfo[boperUid];
                    LadElement^ baccEl = gcnew LadElement();
                    baccEl->Type = "access";
                    baccEl->Tag = (bacc->Scope == "TypedConstant") ? bacc->ConstantValue : bacc->Symbol;
                    baccEl->NormallyOpen = true;
                    branch->Add(baccEl);
                }
                LadElement^ bcontact = gcnew LadElement();
                bcontact->Type = "contact";
                bcontact->Tag = "";
                bcontact->NormallyOpen = !bpi->Negated;
                branch->Add(bcontact);
            }
            else if (bpt == "Coil") {
                String^ boperUid = "";
                if (ctx->OperandMap->TryGetValue(brUid, boperUid) && ctx->AccessInfo->ContainsKey(boperUid)) {
                    ParsedAccessInfo^ bacc = ctx->AccessInfo[boperUid];
                    LadElement^ baccEl = gcnew LadElement();
                    baccEl->Type = "access";
                    baccEl->Tag = (bacc->Scope == "TypedConstant") ? bacc->ConstantValue : bacc->Symbol;
                    baccEl->NormallyOpen = true;
                    branch->Add(baccEl);
                }
                LadElement^ bcoil = gcnew LadElement();
                bcoil->Type = "coil";
                branch->Add(bcoil);
                break;
            }
            else if (bpt == "SCoil") {
                String^ boperUid = "";
                if (ctx->OperandMap->TryGetValue(brUid, boperUid) && ctx->AccessInfo->ContainsKey(boperUid)) {
                    ParsedAccessInfo^ bacc = ctx->AccessInfo[boperUid];
                    LadElement^ baccEl = gcnew LadElement();
                    baccEl->Type = "access";
                    baccEl->Tag = (bacc->Scope == "TypedConstant") ? bacc->ConstantValue : bacc->Symbol;
                    baccEl->NormallyOpen = true;
                    branch->Add(baccEl);
                }
                LadElement^ bscoil = gcnew LadElement();
                bscoil->Type = "setCoil";
                branch->Add(bscoil);
                if (ctx->NextMap->ContainsKey(brUid))
                    brUid = ctx->NextMap[brUid];
                else
                    break;
                continue;
            }
            else if (bpt == "RCoil") {
                String^ boperUid = "";
                if (ctx->OperandMap->TryGetValue(brUid, boperUid) && ctx->AccessInfo->ContainsKey(boperUid)) {
                    ParsedAccessInfo^ bacc = ctx->AccessInfo[boperUid];
                    LadElement^ baccEl = gcnew LadElement();
                    baccEl->Type = "access";
                    baccEl->Tag = (bacc->Scope == "TypedConstant") ? bacc->ConstantValue : bacc->Symbol;
                    baccEl->NormallyOpen = true;
                    branch->Add(baccEl);
                }
                LadElement^ brcoil = gcnew LadElement();
                brcoil->Type = "resetCoil";
                branch->Add(brcoil);
                if (ctx->NextMap->ContainsKey(brUid))
                    brUid = ctx->NextMap[brUid];
                else
                    break;
                continue;
            }
            else if (bpt == "TON") {
                LadElement^ btimer = gcnew LadElement();
                btimer->Type = "timerOnDelay";
                btimer->InstanceName = bpi->InstanceName;
                if (bpi->PtAccessUid->Length > 0 && ctx->AccessInfo->ContainsKey(bpi->PtAccessUid)) {
                    ParsedAccessInfo^ bptAcc = ctx->AccessInfo[bpi->PtAccessUid];
                    btimer->PresetTime = (bptAcc->Scope == "TypedConstant") ? bptAcc->ConstantValue : bptAcc->Symbol;
                }
                branch->Add(btimer);
            }
            else if (bpt == "TOF") {
                LadElement^ btimer = gcnew LadElement();
                btimer->Type = "timerOffDelay";
                btimer->InstanceName = bpi->InstanceName;
                if (bpi->PtAccessUid->Length > 0 && ctx->AccessInfo->ContainsKey(bpi->PtAccessUid)) {
                    ParsedAccessInfo^ bptAcc = ctx->AccessInfo[bpi->PtAccessUid];
                    btimer->PresetTime = (bptAcc->Scope == "TypedConstant") ? bptAcc->ConstantValue : bptAcc->Symbol;
                }
                branch->Add(btimer);
            }
            else if (bpt == "CTU") {
                LadElement^ bcounter = gcnew LadElement();
                bcounter->Type = "counterUp";
                bcounter->InstanceName = bpi->InstanceName;
                if (bpi->PtAccessUid->Length > 0 && ctx->AccessInfo->ContainsKey(bpi->PtAccessUid)) {
                    ParsedAccessInfo^ bptAcc = ctx->AccessInfo[bpi->PtAccessUid];
                    bcounter->PresetTime = (bptAcc->Scope == "TypedConstant") ? bptAcc->ConstantValue : bptAcc->Symbol;
                }
                branch->Add(bcounter);
            }
            else if (bpt == "CTD") {
                LadElement^ bcounter = gcnew LadElement();
                bcounter->Type = "counterDown";
                bcounter->InstanceName = bpi->InstanceName;
                if (bpi->PtAccessUid->Length > 0 && ctx->AccessInfo->ContainsKey(bpi->PtAccessUid)) {
                    ParsedAccessInfo^ bptAcc = ctx->AccessInfo[bpi->PtAccessUid];
                    bcounter->PresetTime = (bptAcc->Scope == "TypedConstant") ? bptAcc->ConstantValue : bptAcc->Symbol;
                }
                branch->Add(bcounter);
            }
            else if (bpt == "TP") {
                LadElement^ btimer = gcnew LadElement();
                btimer->Type = "timerPulse";
                btimer->InstanceName = bpi->InstanceName;
                if (bpi->PtAccessUid->Length > 0 && ctx->AccessInfo->ContainsKey(bpi->PtAccessUid)) {
                    ParsedAccessInfo^ bptAcc = ctx->AccessInfo[bpi->PtAccessUid];
                    btimer->PresetTime = (bptAcc->Scope == "TypedConstant") ? bptAcc->ConstantValue : bptAcc->Symbol;
                }
                branch->Add(btimer);
            }
            else if (bpt == "CTUD") {
                LadElement^ bcounter = gcnew LadElement();
                bcounter->Type = "counterUpDown";
                bcounter->InstanceName = bpi->InstanceName;
                if (bpi->PtAccessUid->Length > 0 && ctx->AccessInfo->ContainsKey(bpi->PtAccessUid)) {
                    ParsedAccessInfo^ bptAcc = ctx->AccessInfo[bpi->PtAccessUid];
                    bcounter->PresetTime = (bptAcc->Scope == "TypedConstant") ? bptAcc->ConstantValue : bptAcc->Symbol;
                }
                branch->Add(bcounter);
            }
            else if (bpt == "CmpEQ" || bpt == "CmpNE" || bpt == "CmpGT" || bpt == "CmpLT" || bpt == "CmpGE" || bpt == "CmpLE" ||
                     bpt == "Eq" || bpt == "Ne" || bpt == "Gt" || bpt == "Lt" || bpt == "Ge" || bpt == "Le") {
                LadElement^ bcmp = gcnew LadElement();
                if (bpt == "CmpEQ" || bpt == "Eq") bcmp->Type = "compareEQ";
                else if (bpt == "CmpNE" || bpt == "Ne") bcmp->Type = "compareNE";
                else if (bpt == "CmpGT" || bpt == "Gt") bcmp->Type = "compareGT";
                else if (bpt == "CmpLT" || bpt == "Lt") bcmp->Type = "compareLT";
                else if (bpt == "CmpGE" || bpt == "Ge") bcmp->Type = "compareGE";
                else if (bpt == "CmpLE" || bpt == "Le") bcmp->Type = "compareLE";
                if (bpi->In1AccessUid->Length > 0 && ctx->AccessInfo->ContainsKey(bpi->In1AccessUid)) {
                    ParsedAccessInfo^ bacc1 = ctx->AccessInfo[bpi->In1AccessUid];
                    bcmp->Tag = (bacc1->Scope == "TypedConstant") ? bacc1->ConstantValue : bacc1->Symbol;
                }
                if (bpi->In2AccessUid->Length > 0 && ctx->AccessInfo->ContainsKey(bpi->In2AccessUid)) {
                    ParsedAccessInfo^ bacc2 = ctx->AccessInfo[bpi->In2AccessUid];
                    bcmp->Tag2 = (bacc2->Scope == "TypedConstant") ? bacc2->ConstantValue : bacc2->Symbol;
                }
                if (bpi->DataType != nullptr && bpi->DataType->Length > 0) {
                    bcmp->DataType = bpi->DataType;
                }
                branch->Add(bcmp);
            }
            else if (bpt == "P") {
                LadElement^ bre = gcnew LadElement();
                bre->Type = "risingEdge";
                String^ boperUid = "";
                if (ctx->OperandMap->TryGetValue(brUid, boperUid) && ctx->AccessInfo->ContainsKey(boperUid)) {
                    ParsedAccessInfo^ bacc = ctx->AccessInfo[boperUid];
                    bre->Tag = (bacc->Scope == "TypedConstant") ? bacc->ConstantValue : bacc->Symbol;
                }
                branch->Add(bre);
            }
            else if (bpt == "N") {
                LadElement^ bfe = gcnew LadElement();
                bfe->Type = "fallingEdge";
                String^ boperUid = "";
                if (ctx->OperandMap->TryGetValue(brUid, boperUid) && ctx->AccessInfo->ContainsKey(boperUid)) {
                    ParsedAccessInfo^ bacc = ctx->AccessInfo[boperUid];
                    bfe->Tag = (bacc->Scope == "TypedConstant") ? bacc->ConstantValue : bacc->Symbol;
                }
                branch->Add(bfe);
            }
            else if (bpt == "O") {
                break;
            }
            else if (bpt == "Move") {
                LadElement^ bmove = gcnew LadElement();
                bmove->Type = "move";
                String^ boperUid = "";
                if (ctx->OperandMap->TryGetValue(brUid, boperUid) && ctx->AccessInfo->ContainsKey(boperUid)) {
                    ParsedAccessInfo^ bacc = ctx->AccessInfo[boperUid];
                    bmove->Tag = (bacc->Scope == "TypedConstant") ? bacc->ConstantValue : bacc->Symbol;
                }
                if (bpi->In1AccessUid->Length > 0 && ctx->AccessInfo->ContainsKey(bpi->In1AccessUid)) {
                    ParsedAccessInfo^ bacc1 = ctx->AccessInfo[bpi->In1AccessUid];
                    bmove->Tag = (bacc1->Scope == "TypedConstant") ? bacc1->ConstantValue : bacc1->Symbol;
                }
                if (bpi->OutAccessUid->Length > 0 && ctx->AccessInfo->ContainsKey(bpi->OutAccessUid)) {
                    ParsedAccessInfo^ bacc2 = ctx->AccessInfo[bpi->OutAccessUid];
                    bmove->Tag2 = (bacc2->Scope == "TypedConstant") ? bacc2->ConstantValue : bacc2->Symbol;
                }
                if (bpi->DataType != nullptr && bpi->DataType->Length > 0) {
                    bmove->DataType = bpi->DataType;
                }
                branch->Add(bmove);
            }
            else if (bpt == "ADD" || bpt == "SUB" || bpt == "MUL" || bpt == "DIV" || bpt == "MOD" ||
                     bpt == "Add" || bpt == "Sub" || bpt == "Mul" || bpt == "Div" || bpt == "Mod") {
                LadElement^ bmath = gcnew LadElement();
                if (bpt == "ADD" || bpt == "Add") bmath->Type = "add";
                else if (bpt == "SUB" || bpt == "Sub") bmath->Type = "sub";
                else if (bpt == "MUL" || bpt == "Mul") bmath->Type = "mul";
                else if (bpt == "DIV" || bpt == "Div") bmath->Type = "div";
                else if (bpt == "MOD" || bpt == "Mod") bmath->Type = "mod";
                if (bpi->In1AccessUid->Length > 0 && ctx->AccessInfo->ContainsKey(bpi->In1AccessUid)) {
                    ParsedAccessInfo^ bacc1 = ctx->AccessInfo[bpi->In1AccessUid];
                    bmath->Tag = (bacc1->Scope == "TypedConstant") ? bacc1->ConstantValue : bacc1->Symbol;
                }
                if (bpi->In2AccessUid->Length > 0 && ctx->AccessInfo->ContainsKey(bpi->In2AccessUid)) {
                    ParsedAccessInfo^ bacc2 = ctx->AccessInfo[bpi->In2AccessUid];
                    bmath->Tag2 = (bacc2->Scope == "TypedConstant") ? bacc2->ConstantValue : bacc2->Symbol;
                }
                if (bpi->OutAccessUid->Length > 0 && ctx->AccessInfo->ContainsKey(bpi->OutAccessUid)) {
                    ParsedAccessInfo^ bacc3 = ctx->AccessInfo[bpi->OutAccessUid];
                    bmath->Tag3 = (bacc3->Scope == "TypedConstant") ? bacc3->ConstantValue : bacc3->Symbol;
                }
                if (bpi->DataType != nullptr && bpi->DataType->Length > 0) {
                    bmath->DataType = bpi->DataType;
                }
                branch->Add(bmath);
            }
            else if (bpt == "JMP") {
                LadElement^ bjmp = gcnew LadElement();
                bjmp->Type = "jmp";
                String^ boperUid = "";
                if (ctx->OperandMap->TryGetValue(brUid, boperUid) && ctx->AccessInfo->ContainsKey(boperUid)) {
                    ParsedAccessInfo^ bacc = ctx->AccessInfo[boperUid];
                    bjmp->Tag = (bacc->Scope == "TypedConstant") ? bacc->ConstantValue : bacc->Symbol;
                }
                branch->Add(bjmp);
            }
            else if (bpt == "LABEL") {
                LadElement^ blabel = gcnew LadElement();
                blabel->Type = "label";
                String^ boperUid = "";
                if (ctx->OperandMap->TryGetValue(brUid, boperUid) && ctx->AccessInfo->ContainsKey(boperUid)) {
                    ParsedAccessInfo^ bacc = ctx->AccessInfo[boperUid];
                    blabel->Tag = (bacc->Scope == "TypedConstant") ? bacc->ConstantValue : bacc->Symbol;
                }
                branch->Add(blabel);
            }
            else if (bpt == "RET") {
                LadElement^ bret = gcnew LadElement();
                bret->Type = "ret";
                branch->Add(bret);
            }
            else if (bpt == "NOP") {
                LadElement^ bnop = gcnew LadElement();
                bnop->Type = "nop";
                branch->Add(bnop);
            }
            if (ctx->NextMap->ContainsKey(brUid))
                brUid = ctx->NextMap[brUid];
            else
                break;
        }
        parallel->Branches->Add(branch);
    }
    net->Elements->Add(parallel);

    String^ orUid = "";
    String^ checkUid = mBranches[0];
    if (ctx->NextMap->ContainsKey(checkUid)) {
        String^ nextAfterBranch = ctx->NextMap[checkUid];
        if (ctx->PartInfo->ContainsKey(nextAfterBranch) && ctx->PartInfo[nextAfterBranch]->Type == "O") {
            orUid = nextAfterBranch;
        }
    }
    if (orUid->Length > 0 && ctx->NextMap->ContainsKey(orUid)) {
        result->NextUid = ctx->NextMap[orUid];
        result->ShouldContinue = true;
    }

    return result;
}

inline List<LadNetwork^>^ ParseLadFlgNetsFromXml(String^ xmlPath) {
    List<LadNetwork^>^ networks = gcnew List<LadNetwork^>();
    try {
        XmlDocument^ doc = gcnew XmlDocument();
        doc->Load(xmlPath);

        for each (XmlNode^ node in doc->SelectNodes("//*")) {
            if (node->LocalName != "FlgNet") continue;

            XmlNode^ partsNode = nullptr;
            XmlNode^ wiresNode = nullptr;
            for each (XmlNode^ child in node->ChildNodes) {
                if (child->LocalName == "Parts") partsNode = child;
                if (child->LocalName == "Wires") wiresNode = child;
            }
            if (partsNode == nullptr) continue;

            Dictionary<String^, ParsedPartInfo^>^ partInfo = gcnew Dictionary<String^, ParsedPartInfo^>();
            Dictionary<String^, ParsedAccessInfo^>^ accessInfo = gcnew Dictionary<String^, ParsedAccessInfo^>();

            int nextLocalAccessUid = 1;

            for each (XmlNode^ part in partsNode->ChildNodes) {
                String^ localName = part->LocalName;

                if (localName == "Access") {
                    XmlAttribute^ uidAttr = part->Attributes["UId"];
                    String^ uid = (uidAttr != nullptr) ? uidAttr->Value : gcnew String("_acc_" + nextLocalAccessUid++);

                    XmlAttribute^ scopeAttr = part->Attributes["Scope"];
                    String^ scope = (scopeAttr != nullptr) ? scopeAttr->Value : gcnew String("GlobalVariable");

                    ParsedAccessInfo^ acc = gcnew ParsedAccessInfo();
                    acc->Scope = scope;
                    acc->Symbol = "";
                    acc->ConstantValue = "";
                    acc->TargetPartUid = -1;

                    if (scope == "TypedConstant") {
                        XmlNode^ constNode = part->SelectSingleNode("*[local-name()='Constant']/*[local-name()='ConstantValue']");
                        if (constNode != nullptr) {
                            acc->ConstantValue = constNode->InnerText;
                            XmlAttribute^ typeAttr = constNode->Attributes["Type"];
                            if (typeAttr != nullptr) acc->ConstantType = typeAttr->Value;
                        }
                    }
                    else {
                        XmlNode^ compNode = part->SelectSingleNode("*[local-name()='Symbol']/*[local-name()='Component']");
                        if (compNode != nullptr) {
                            XmlAttribute^ nameAttr = compNode->Attributes["Name"];
                            if (nameAttr != nullptr) acc->Symbol = nameAttr->Value;
                        }
                    }

                    accessInfo[uid] = acc;
                }
                else if (localName == "Part") {
                    XmlAttribute^ uidAttr = part->Attributes["UId"];
                    if (uidAttr == nullptr) continue;
                    String^ uid = uidAttr->Value;

                    XmlAttribute^ nameAttr = part->Attributes["Name"];
                    String^ partName = (nameAttr != nullptr) ? nameAttr->Value : gcnew String("");

                    ParsedPartInfo^ pi = gcnew ParsedPartInfo();
                    pi->Type = partName;
                    pi->Negated = false;
                    pi->InstanceName = "";
                    pi->TimerVersion = "";
                    pi->PtAccessUid = "";
                    pi->Cardinality = 0;
                    pi->In1AccessUid = "";
                    pi->In2AccessUid = "";
                    pi->OutAccessUid = "";

                    XmlNode^ negNode = part->SelectSingleNode("*[local-name()='NormallyClosed']");
                    if (negNode == nullptr)
                        negNode = part->SelectSingleNode("*[local-name()='Negated']");
                    if (negNode != nullptr) {
                        String^ negText = negNode->InnerText->Trim()->ToLower();
                        pi->Negated = (negText == "" || negText->StartsWith("true"));
                    }

                    if (partName == "TON" || partName == "TOF" || partName == "TP" || partName == "CTU" || partName == "CTD" || partName == "CTUD") {
                        XmlAttribute^ verAttr = part->Attributes["Version"];
                        if (verAttr != nullptr) pi->TimerVersion = verAttr->Value;

                        XmlNode^ instNode = part->SelectSingleNode("*[local-name()='Instance']/*[local-name()='Component']");
                        if (instNode != nullptr) {
                            XmlAttribute^ nameA = instNode->Attributes["Name"];
                            if (nameA != nullptr) pi->InstanceName = nameA->Value;
                        }
                    }

                    if (partName == "CmpEQ" || partName == "CmpNE" || partName == "CmpGT" ||
                        partName == "CmpLT" || partName == "CmpGE" || partName == "CmpLE" ||
                        partName == "Eq" || partName == "Ne" || partName == "Gt" ||
                        partName == "Lt" || partName == "Ge" || partName == "Le") {
                        pi->Negated = false;
                    }

                    if (partName == "P") {
                        pi->Type = "P";
                    }
                    if (partName == "N") {
                        pi->Type = "N";
                    }

                    if (partName == "O") {
                        XmlNode^ cardNode = part->SelectSingleNode("*[local-name()='TemplateValue'][@Name='Card']");
                        if (cardNode != nullptr) {
                            int card;
                            if (Int32::TryParse(cardNode->InnerText->Trim(), card)) {
                                pi->Cardinality = card;
                            }
                        }
                    }

                    XmlNode^ dtNode = part->SelectSingleNode("*[local-name()='TemplateValue'][@Name='data_type']");
                    if (dtNode != nullptr && dtNode->InnerText->Trim()->Length > 0) {
                        pi->DataType = dtNode->InnerText->Trim();
                    }
                    XmlNode^ stNode = part->SelectSingleNode("*[local-name()='TemplateValue'][@Name='SrcType']");
                    if (stNode != nullptr && stNode->InnerText->Trim()->Length > 0) {
                        pi->DataType = stNode->InnerText->Trim();
                    }

                    partInfo[uid] = pi;
                }
                else if (localName == "Call") {
                    XmlAttribute^ uidAttr = part->Attributes["UId"];
                    if (uidAttr == nullptr) continue;
                    String^ uid = uidAttr->Value;

                    XmlNode^ callInfoNode = part->SelectSingleNode("*[local-name()='CallInfo']");
                    if (callInfoNode == nullptr) continue;

                    XmlAttribute^ nameAttr = callInfoNode->Attributes["Name"];
                    String^ callName = (nameAttr != nullptr) ? nameAttr->Value : gcnew String("");

                    ParsedPartInfo^ pi = gcnew ParsedPartInfo();
                    pi->Type = callName;
                    pi->Negated = false;
                    pi->InstanceName = "";
                    pi->TimerVersion = "1.0";
                    pi->PtAccessUid = "";
                    pi->Cardinality = 0;
                    pi->In1AccessUid = "";
                    pi->In2AccessUid = "";
                    pi->OutAccessUid = "";

                    XmlNode^ instNode = callInfoNode->SelectSingleNode("*[local-name()='Instance']/*[local-name()='Component']");
                    if (instNode != nullptr) {
                        XmlAttribute^ nameA = instNode->Attributes["Name"];
                        if (nameA != nullptr) pi->InstanceName = nameA->Value;
                    }

                    partInfo[uid] = pi;
                }
            }

            if (partInfo->Count == 0) continue;

            Dictionary<String^, String^>^ operandMap = gcnew Dictionary<String^, String^>();
            Dictionary<String^, String^>^ bitMap = gcnew Dictionary<String^, String^>();
            Dictionary<String^, String^>^ nMap = gcnew Dictionary<String^, String^>();
            Dictionary<String^, String^>^ nextMap = gcnew Dictionary<String^, String^>();
            Dictionary<String^, List<String^>^>^ multiNextMap = gcnew Dictionary<String^, List<String^>^>();
            Dictionary<String^, String^>^ prevMap = gcnew Dictionary<String^, String^>();
            List<String^>^ powerEntryUids = gcnew List<String^>();

            ParseContext^ ctx = gcnew ParseContext();
            ctx->PartInfo = partInfo;
            ctx->AccessInfo = accessInfo;
            ctx->OperandMap = operandMap;
            ctx->BitMap = bitMap;
            ctx->NMap = nMap;
            ctx->NextMap = nextMap;
            ctx->MultiNextMap = multiNextMap;

            if (wiresNode != nullptr) {
                for each (XmlNode^ wire in wiresNode->ChildNodes) {
                    if (wire->LocalName != "Wire") continue;

                    bool hasPowerRail = false;
                    String^ identUid = "";
                    List<System::Tuple<String^, String^>^>^ nameCons = gcnew List<System::Tuple<String^, String^>^>();
                    bool hasOpenCon = false;

                    for each (XmlNode^ wc in wire->ChildNodes) {
                        String^ wl = wc->LocalName;
                        if (wl == "Powerrail") {
                            hasPowerRail = true;
                        }
                        else if (wl == "IdentCon") {
                            XmlAttribute^ ua = wc->Attributes["UId"];
                            if (ua != nullptr) identUid = ua->Value;
                        }
                        else if (wl == "NameCon") {
                            XmlAttribute^ ua = wc->Attributes["UId"];
                            XmlAttribute^ na = wc->Attributes["Name"];
                            if (ua != nullptr && na != nullptr)
                                nameCons->Add(System::Tuple::Create(ua->Value, na->Value));
                        }
                        else if (wl == "OpenCon") {
                            hasOpenCon = true;
                        }
                    }

                    if (hasPowerRail && nameCons->Count >= 1) {
                        for (int nc = 0; nc < nameCons->Count; nc++) {
                            if (nameCons[nc]->Item2 == "in" || nameCons[nc]->Item2 == "CU" || nameCons[nc]->Item2 == "CD" || nameCons[nc]->Item2 == "pre" || nameCons[nc]->Item2 == "en") {
                                powerEntryUids->Add(nameCons[nc]->Item1);
                            }
                        }
                        continue;
                    }

                    if (identUid->Length > 0 && nameCons->Count >= 1) {
                        for (int nc = 0; nc < nameCons->Count; nc++) {
                            String^ pin = nameCons[nc]->Item2;
                            if (pin == "operand") {
                                operandMap[nameCons[nc]->Item1] = identUid;
                            }
                            else if (pin == "bit") {
                                bitMap[nameCons[nc]->Item1] = identUid;
                            }
                            else if (pin == "n") {
                                nMap[nameCons[nc]->Item1] = identUid;
                            }
                            else if (pin == "PT" || pin == "PV") {
                                operandMap[nameCons[nc]->Item1] = identUid;
                                String^ targetUid = nameCons[nc]->Item1;
                                if (partInfo->ContainsKey(targetUid)) {
                                    partInfo[targetUid]->PtAccessUid = identUid;
                                }
                            }
                            else if (pin == "in1") {
                                String^ targetUid = nameCons[nc]->Item1;
                                if (partInfo->ContainsKey(targetUid)) {
                                    partInfo[targetUid]->In1AccessUid = identUid;
                                }
                            }
                            else if (pin == "in2") {
                                String^ targetUid = nameCons[nc]->Item1;
                                if (partInfo->ContainsKey(targetUid)) {
                                    partInfo[targetUid]->In2AccessUid = identUid;
                                }
                            }
                        }

                        for (int nc = 0; nc < nameCons->Count; nc++) {
                            String^ fromPin = nameCons[nc]->Item2;
                            if (fromPin == "out" || fromPin == "out1") {
                                String^ fromUid = nameCons[nc]->Item1;
                                if (partInfo->ContainsKey(fromUid)) {
                                    partInfo[fromUid]->OutAccessUid = identUid;
                                }
                            }
                        }
                        continue;
                    }

                    for (int nc = 0; nc < nameCons->Count; nc++) {
                        String^ fromUid = nameCons[nc]->Item1;
                        String^ fromPin = nameCons[nc]->Item2;

                        if (fromPin == "out" || fromPin == "out1" || fromPin == "Q" || fromPin == "eno") {
                            List<String^>^ targets = gcnew List<String^>();
                            for (int nc2 = 0; nc2 < nameCons->Count; nc2++) {
                                if (nc2 == nc) continue;
                                String^ toPin = nameCons[nc2]->Item2;
                                if (toPin == "in" || toPin == "IN" || toPin == "CU" || toPin == "CD" || toPin == "en" || toPin->StartsWith("in")) {
                                    targets->Add(nameCons[nc2]->Item1);
                                }
                            }
                            if (targets->Count == 1) {
                                nextMap[fromUid] = targets[0];
                            }
                            else if (targets->Count > 1) {
                                nextMap[fromUid] = targets[0];
                                multiNextMap[fromUid] = targets;
                            }
                            break;
                        }
                    }
                }
            }

            if (powerEntryUids->Count == 0) continue;

            for (int ri = 0; ri < powerEntryUids->Count; ri++) {
                String^ entryUid = powerEntryUids[ri];

                LadNetwork^ net = gcnew LadNetwork();
                net->Number = networks->Count + 1;
                net->Title = "";
                net->Elements = gcnew List<LadElement^>();

                HashSet<String^>^ visited = gcnew HashSet<String^>();
                String^ currentUid = entryUid;

                while (currentUid->Length > 0 && partInfo->ContainsKey(currentUid) && !visited->Contains(currentUid)) {
                    visited->Add(currentUid);
                    ParsedPartInfo^ pi = partInfo[currentUid];
                    String^ pt = pi->Type;

                    if (pt == "Contact") {
                        String^ operUid = "";
                        if (operandMap->TryGetValue(currentUid, operUid) && accessInfo->ContainsKey(operUid)) {
                            ParsedAccessInfo^ acc = accessInfo[operUid];
                            LadElement^ accEl = gcnew LadElement();
                            accEl->Type = "access";
                            accEl->Tag = (acc->Scope == "TypedConstant") ? acc->ConstantValue : acc->Symbol;
                            accEl->NormallyOpen = true;
                            net->Elements->Add(accEl);
                        }

                        LadElement^ contact = gcnew LadElement();
                        contact->Type = "contact";
                        contact->Tag = "";
                        contact->NormallyOpen = !pi->Negated;
                        net->Elements->Add(contact);

                        BranchResult^ br = ProcessParallelBranches(currentUid, ctx, net);
                        if (br->HasBranches) {
                            if (br->ShouldContinue) {
                                currentUid = br->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "Coil") {
                        String^ operUid = "";
                        if (operandMap->TryGetValue(currentUid, operUid) && accessInfo->ContainsKey(operUid)) {
                            ParsedAccessInfo^ acc = accessInfo[operUid];
                            LadElement^ accEl = gcnew LadElement();
                            accEl->Type = "access";
                            accEl->Tag = (acc->Scope == "TypedConstant") ? acc->ConstantValue : acc->Symbol;
                            accEl->NormallyOpen = true;
                            net->Elements->Add(accEl);
                        }

                        LadElement^ coil = gcnew LadElement();
                        coil->Type = "coil";
                        net->Elements->Add(coil);
                        break;
                    }
                    else if (pt == "SCoil") {
                        String^ operUid = "";
                        if (operandMap->TryGetValue(currentUid, operUid) && accessInfo->ContainsKey(operUid)) {
                            ParsedAccessInfo^ acc = accessInfo[operUid];
                            LadElement^ accEl = gcnew LadElement();
                            accEl->Type = "access";
                            accEl->Tag = (acc->Scope == "TypedConstant") ? acc->ConstantValue : acc->Symbol;
                            accEl->NormallyOpen = true;
                            net->Elements->Add(accEl);
                        }

                        LadElement^ scoil = gcnew LadElement();
                        scoil->Type = "setCoil";
                        net->Elements->Add(scoil);

                        BranchResult^ br2 = ProcessParallelBranches(currentUid, ctx, net);
                        if (br2->HasBranches) {
                            if (br2->ShouldContinue) {
                                currentUid = br2->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "RCoil") {
                        String^ operUid = "";
                        if (operandMap->TryGetValue(currentUid, operUid) && accessInfo->ContainsKey(operUid)) {
                            ParsedAccessInfo^ acc = accessInfo[operUid];
                            LadElement^ accEl = gcnew LadElement();
                            accEl->Type = "access";
                            accEl->Tag = (acc->Scope == "TypedConstant") ? acc->ConstantValue : acc->Symbol;
                            accEl->NormallyOpen = true;
                            net->Elements->Add(accEl);
                        }

                        LadElement^ rcoil = gcnew LadElement();
                        rcoil->Type = "resetCoil";
                        net->Elements->Add(rcoil);

                        BranchResult^ br3 = ProcessParallelBranches(currentUid, ctx, net);
                        if (br3->HasBranches) {
                            if (br3->ShouldContinue) {
                                currentUid = br3->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "PContact") {
                        LadElement^ edgeContact = gcnew LadElement();
                        edgeContact->Type = "risingEdgeContact";
                        edgeContact->Tag = "";
                        if (operandMap->ContainsKey(currentUid) && accessInfo->ContainsKey(operandMap[currentUid])) {
                            ParsedAccessInfo^ acc = accessInfo[operandMap[currentUid]];
                            edgeContact->Tag = (acc->Scope == "TypedConstant") ? acc->ConstantValue : acc->Symbol;
                        }
                        if (bitMap->ContainsKey(currentUid) && accessInfo->ContainsKey(bitMap[currentUid])) {
                            ParsedAccessInfo^ bitAcc = accessInfo[bitMap[currentUid]];
                            edgeContact->Tag2 = (bitAcc->Scope == "TypedConstant") ? bitAcc->ConstantValue : bitAcc->Symbol;
                        }
                        net->Elements->Add(edgeContact);

                        BranchResult^ brP = ProcessParallelBranches(currentUid, ctx, net);
                        if (brP->HasBranches) {
                            if (brP->ShouldContinue) {
                                currentUid = brP->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "NContact") {
                        LadElement^ edgeContact = gcnew LadElement();
                        edgeContact->Type = "fallingEdgeContact";
                        edgeContact->Tag = "";
                        if (operandMap->ContainsKey(currentUid) && accessInfo->ContainsKey(operandMap[currentUid])) {
                            ParsedAccessInfo^ acc = accessInfo[operandMap[currentUid]];
                            edgeContact->Tag = (acc->Scope == "TypedConstant") ? acc->ConstantValue : acc->Symbol;
                        }
                        if (bitMap->ContainsKey(currentUid) && accessInfo->ContainsKey(bitMap[currentUid])) {
                            ParsedAccessInfo^ bitAcc = accessInfo[bitMap[currentUid]];
                            edgeContact->Tag2 = (bitAcc->Scope == "TypedConstant") ? bitAcc->ConstantValue : bitAcc->Symbol;
                        }
                        net->Elements->Add(edgeContact);

                        BranchResult^ brN = ProcessParallelBranches(currentUid, ctx, net);
                        if (brN->HasBranches) {
                            if (brN->ShouldContinue) {
                                currentUid = brN->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "RBitfield") {
                        LadElement^ rbit = gcnew LadElement();
                        rbit->Type = "resetBitfield";
                        if (operandMap->ContainsKey(currentUid) && accessInfo->ContainsKey(operandMap[currentUid])) {
                            ParsedAccessInfo^ acc = accessInfo[operandMap[currentUid]];
                            rbit->Tag = (acc->Scope == "TypedConstant") ? acc->ConstantValue : acc->Symbol;
                        }
                        if (nMap->ContainsKey(currentUid) && accessInfo->ContainsKey(nMap[currentUid])) {
                            ParsedAccessInfo^ nAcc = accessInfo[nMap[currentUid]];
                            rbit->Tag2 = (nAcc->Scope == "TypedConstant" || nAcc->Scope == "LiteralConstant") ? nAcc->ConstantValue : nAcc->Symbol;
                        }
                        net->Elements->Add(rbit);
                        break;
                    }
                    else if (pt == "SBitfield") {
                        LadElement^ sbit = gcnew LadElement();
                        sbit->Type = "setBitfield";
                        if (operandMap->ContainsKey(currentUid) && accessInfo->ContainsKey(operandMap[currentUid])) {
                            ParsedAccessInfo^ acc = accessInfo[operandMap[currentUid]];
                            sbit->Tag = (acc->Scope == "TypedConstant") ? acc->ConstantValue : acc->Symbol;
                        }
                        if (nMap->ContainsKey(currentUid) && accessInfo->ContainsKey(nMap[currentUid])) {
                            ParsedAccessInfo^ nAcc = accessInfo[nMap[currentUid]];
                            sbit->Tag2 = (nAcc->Scope == "TypedConstant" || nAcc->Scope == "LiteralConstant") ? nAcc->ConstantValue : nAcc->Symbol;
                        }
                        net->Elements->Add(sbit);
                        break;
                    }
                    else if (pt == "TON") {
                        LadElement^ timer = gcnew LadElement();
                        timer->Type = "timerOnDelay";
                        timer->InstanceName = pi->InstanceName;

                        if (pi->PtAccessUid->Length > 0 && accessInfo->ContainsKey(pi->PtAccessUid)) {
                            ParsedAccessInfo^ ptAcc = accessInfo[pi->PtAccessUid];
                            timer->PresetTime = (ptAcc->Scope == "TypedConstant") ? ptAcc->ConstantValue : ptAcc->Symbol;
                        }

                        net->Elements->Add(timer);

                        BranchResult^ br4 = ProcessParallelBranches(currentUid, ctx, net);
                        if (br4->HasBranches) {
                            if (br4->ShouldContinue) {
                                currentUid = br4->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "TOF") {
                        LadElement^ timer = gcnew LadElement();
                        timer->Type = "timerOffDelay";
                        timer->InstanceName = pi->InstanceName;

                        if (pi->PtAccessUid->Length > 0 && accessInfo->ContainsKey(pi->PtAccessUid)) {
                            ParsedAccessInfo^ ptAcc = accessInfo[pi->PtAccessUid];
                            timer->PresetTime = (ptAcc->Scope == "TypedConstant") ? ptAcc->ConstantValue : ptAcc->Symbol;
                        }

                        net->Elements->Add(timer);

                        BranchResult^ br5 = ProcessParallelBranches(currentUid, ctx, net);
                        if (br5->HasBranches) {
                            if (br5->ShouldContinue) {
                                currentUid = br5->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "CTU") {
                        LadElement^ counter = gcnew LadElement();
                        counter->Type = "counterUp";
                        counter->InstanceName = pi->InstanceName;

                        if (pi->PtAccessUid->Length > 0 && accessInfo->ContainsKey(pi->PtAccessUid)) {
                            ParsedAccessInfo^ ptAcc = accessInfo[pi->PtAccessUid];
                            counter->PresetTime = (ptAcc->Scope == "TypedConstant") ? ptAcc->ConstantValue : ptAcc->Symbol;
                        }

                        net->Elements->Add(counter);

                        BranchResult^ br6 = ProcessParallelBranches(currentUid, ctx, net);
                        if (br6->HasBranches) {
                            if (br6->ShouldContinue) {
                                currentUid = br6->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "CTD") {
                        LadElement^ counter = gcnew LadElement();
                        counter->Type = "counterDown";
                        counter->InstanceName = pi->InstanceName;

                        if (pi->PtAccessUid->Length > 0 && accessInfo->ContainsKey(pi->PtAccessUid)) {
                            ParsedAccessInfo^ ptAcc = accessInfo[pi->PtAccessUid];
                            counter->PresetTime = (ptAcc->Scope == "TypedConstant") ? ptAcc->ConstantValue : ptAcc->Symbol;
                        }

                        net->Elements->Add(counter);

                        BranchResult^ br7 = ProcessParallelBranches(currentUid, ctx, net);
                        if (br7->HasBranches) {
                            if (br7->ShouldContinue) {
                                currentUid = br7->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "TP") {
                        LadElement^ timer = gcnew LadElement();
                        timer->Type = "timerPulse";
                        timer->InstanceName = pi->InstanceName;

                        if (pi->PtAccessUid->Length > 0 && accessInfo->ContainsKey(pi->PtAccessUid)) {
                            ParsedAccessInfo^ ptAcc = accessInfo[pi->PtAccessUid];
                            timer->PresetTime = (ptAcc->Scope == "TypedConstant") ? ptAcc->ConstantValue : ptAcc->Symbol;
                        }

                        net->Elements->Add(timer);

                        BranchResult^ br8 = ProcessParallelBranches(currentUid, ctx, net);
                        if (br8->HasBranches) {
                            if (br8->ShouldContinue) {
                                currentUid = br8->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "CTUD") {
                        LadElement^ counter = gcnew LadElement();
                        counter->Type = "counterUpDown";
                        counter->InstanceName = pi->InstanceName;

                        if (pi->PtAccessUid->Length > 0 && accessInfo->ContainsKey(pi->PtAccessUid)) {
                            ParsedAccessInfo^ ptAcc = accessInfo[pi->PtAccessUid];
                            counter->PresetTime = (ptAcc->Scope == "TypedConstant") ? ptAcc->ConstantValue : ptAcc->Symbol;
                        }

                        net->Elements->Add(counter);

                        BranchResult^ br9 = ProcessParallelBranches(currentUid, ctx, net);
                        if (br9->HasBranches) {
                            if (br9->ShouldContinue) {
                                currentUid = br9->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "CmpEQ" || pt == "CmpNE" || pt == "CmpGT" || pt == "CmpLT" || pt == "CmpGE" || pt == "CmpLE" ||
                             pt == "Eq" || pt == "Ne" || pt == "Gt" || pt == "Lt" || pt == "Ge" || pt == "Le") {
                        LadElement^ cmp = gcnew LadElement();
                        if (pt == "CmpEQ" || pt == "Eq") cmp->Type = "compareEQ";
                        else if (pt == "CmpNE" || pt == "Ne") cmp->Type = "compareNE";
                        else if (pt == "CmpGT" || pt == "Gt") cmp->Type = "compareGT";
                        else if (pt == "CmpLT" || pt == "Lt") cmp->Type = "compareLT";
                        else if (pt == "CmpGE" || pt == "Ge") cmp->Type = "compareGE";
                        else if (pt == "CmpLE" || pt == "Le") cmp->Type = "compareLE";

                        if (pi->In1AccessUid->Length > 0 && accessInfo->ContainsKey(pi->In1AccessUid)) {
                            ParsedAccessInfo^ acc1 = accessInfo[pi->In1AccessUid];
                            cmp->Tag = (acc1->Scope == "TypedConstant") ? acc1->ConstantValue : acc1->Symbol;
                        }
                        if (pi->In2AccessUid->Length > 0 && accessInfo->ContainsKey(pi->In2AccessUid)) {
                            ParsedAccessInfo^ acc2 = accessInfo[pi->In2AccessUid];
                            cmp->Tag2 = (acc2->Scope == "TypedConstant") ? acc2->ConstantValue : acc2->Symbol;
                        }
                        if (pi->DataType != nullptr && pi->DataType->Length > 0) {
                            cmp->DataType = pi->DataType;
                        }

                        net->Elements->Add(cmp);

                        BranchResult^ br10 = ProcessParallelBranches(currentUid, ctx, net);
                        if (br10->HasBranches) {
                            if (br10->ShouldContinue) {
                                currentUid = br10->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "P") {
                        String^ operUid = "";
                        if (operandMap->TryGetValue(currentUid, operUid) && accessInfo->ContainsKey(operUid)) {
                            ParsedAccessInfo^ acc = accessInfo[operUid];
                            LadElement^ accEl = gcnew LadElement();
                            accEl->Type = "access";
                            accEl->Tag = (acc->Scope == "TypedConstant") ? acc->ConstantValue : acc->Symbol;
                            accEl->NormallyOpen = true;
                            net->Elements->Add(accEl);
                        }

                        LadElement^ re = gcnew LadElement();
                        re->Type = "risingEdge";
                        net->Elements->Add(re);

                        BranchResult^ br11 = ProcessParallelBranches(currentUid, ctx, net);
                        if (br11->HasBranches) {
                            if (br11->ShouldContinue) {
                                currentUid = br11->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "N") {
                        String^ operUid = "";
                        if (operandMap->TryGetValue(currentUid, operUid) && accessInfo->ContainsKey(operUid)) {
                            ParsedAccessInfo^ acc = accessInfo[operUid];
                            LadElement^ accEl = gcnew LadElement();
                            accEl->Type = "access";
                            accEl->Tag = (acc->Scope == "TypedConstant") ? acc->ConstantValue : acc->Symbol;
                            accEl->NormallyOpen = true;
                            net->Elements->Add(accEl);
                        }

                        LadElement^ fe = gcnew LadElement();
                        fe->Type = "fallingEdge";
                        net->Elements->Add(fe);

                        BranchResult^ br12 = ProcessParallelBranches(currentUid, ctx, net);
                        if (br12->HasBranches) {
                            if (br12->ShouldContinue) {
                                currentUid = br12->NextUid;
                                continue;
                            }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }
                    else if (pt == "Move") {
                        LadElement^ moveEl = gcnew LadElement();
                        moveEl->Type = "move";
                        String^ operUid = "";
                        if (operandMap->TryGetValue(currentUid, operUid) && accessInfo->ContainsKey(operUid)) {
                            ParsedAccessInfo^ acc = accessInfo[operUid];
                            moveEl->Tag = (acc->Scope == "TypedConstant") ? acc->ConstantValue : acc->Symbol;
                        }
                        if (pi->In1AccessUid->Length > 0 && accessInfo->ContainsKey(pi->In1AccessUid)) {
                            ParsedAccessInfo^ acc1 = accessInfo[pi->In1AccessUid];
                            moveEl->Tag = (acc1->Scope == "TypedConstant") ? acc1->ConstantValue : acc1->Symbol;
                        }
                        if (pi->OutAccessUid->Length > 0 && accessInfo->ContainsKey(pi->OutAccessUid)) {
                            ParsedAccessInfo^ acc2 = accessInfo[pi->OutAccessUid];
                            moveEl->Tag2 = (acc2->Scope == "TypedConstant") ? acc2->ConstantValue : acc2->Symbol;
                        }
                        if (pi->DataType != nullptr && pi->DataType->Length > 0) {
                            moveEl->DataType = pi->DataType;
                        }
                        net->Elements->Add(moveEl);

                        BranchResult^ brMove = ProcessParallelBranches(currentUid, ctx, net);
                        if (brMove->HasBranches) {
                            if (brMove->ShouldContinue) { currentUid = brMove->NextUid; continue; }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid)) currentUid = nextMap[currentUid];
                        else break;
                    }
                    else if (pt == "ADD" || pt == "SUB" || pt == "MUL" || pt == "DIV" || pt == "MOD" ||
                             pt == "Add" || pt == "Sub" || pt == "Mul" || pt == "Div" || pt == "Mod") {
                        LadElement^ mathEl = gcnew LadElement();
                        if (pt == "ADD" || pt == "Add") mathEl->Type = "add";
                        else if (pt == "SUB" || pt == "Sub") mathEl->Type = "sub";
                        else if (pt == "MUL" || pt == "Mul") mathEl->Type = "mul";
                        else if (pt == "DIV" || pt == "Div") mathEl->Type = "div";
                        else if (pt == "MOD" || pt == "Mod") mathEl->Type = "mod";
                        if (pi->In1AccessUid->Length > 0 && accessInfo->ContainsKey(pi->In1AccessUid)) {
                            ParsedAccessInfo^ acc1 = accessInfo[pi->In1AccessUid];
                            mathEl->Tag = (acc1->Scope == "TypedConstant") ? acc1->ConstantValue : acc1->Symbol;
                        }
                        if (pi->In2AccessUid->Length > 0 && accessInfo->ContainsKey(pi->In2AccessUid)) {
                            ParsedAccessInfo^ acc2 = accessInfo[pi->In2AccessUid];
                            mathEl->Tag2 = (acc2->Scope == "TypedConstant") ? acc2->ConstantValue : acc2->Symbol;
                        }
                        if (pi->OutAccessUid->Length > 0 && accessInfo->ContainsKey(pi->OutAccessUid)) {
                            ParsedAccessInfo^ acc3 = accessInfo[pi->OutAccessUid];
                            mathEl->Tag3 = (acc3->Scope == "TypedConstant") ? acc3->ConstantValue : acc3->Symbol;
                        }
                        if (pi->DataType != nullptr && pi->DataType->Length > 0) {
                            mathEl->DataType = pi->DataType;
                        }
                        net->Elements->Add(mathEl);

                        BranchResult^ brMath = ProcessParallelBranches(currentUid, ctx, net);
                        if (brMath->HasBranches) {
                            if (brMath->ShouldContinue) { currentUid = brMath->NextUid; continue; }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid)) currentUid = nextMap[currentUid];
                        else break;
                    }
                    else if (pt == "JMP") {
                        LadElement^ jmpEl = gcnew LadElement();
                        jmpEl->Type = "jmp";
                        String^ operUid = "";
                        if (operandMap->TryGetValue(currentUid, operUid) && accessInfo->ContainsKey(operUid)) {
                            ParsedAccessInfo^ acc = accessInfo[operUid];
                            jmpEl->Tag = (acc->Scope == "TypedConstant") ? acc->ConstantValue : acc->Symbol;
                        }
                        net->Elements->Add(jmpEl);

                        BranchResult^ brJmp = ProcessParallelBranches(currentUid, ctx, net);
                        if (brJmp->HasBranches) {
                            if (brJmp->ShouldContinue) { currentUid = brJmp->NextUid; continue; }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid)) currentUid = nextMap[currentUid];
                        else break;
                    }
                    else if (pt == "LABEL") {
                        LadElement^ labelEl = gcnew LadElement();
                        labelEl->Type = "label";
                        String^ operUid = "";
                        if (operandMap->TryGetValue(currentUid, operUid) && accessInfo->ContainsKey(operUid)) {
                            ParsedAccessInfo^ acc = accessInfo[operUid];
                            labelEl->Tag = (acc->Scope == "TypedConstant") ? acc->ConstantValue : acc->Symbol;
                        }
                        net->Elements->Add(labelEl);

                        BranchResult^ brLabel = ProcessParallelBranches(currentUid, ctx, net);
                        if (brLabel->HasBranches) {
                            if (brLabel->ShouldContinue) { currentUid = brLabel->NextUid; continue; }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid)) currentUid = nextMap[currentUid];
                        else break;
                    }
                    else if (pt == "RET") {
                        LadElement^ retEl = gcnew LadElement();
                        retEl->Type = "ret";
                        net->Elements->Add(retEl);

                        BranchResult^ brRet = ProcessParallelBranches(currentUid, ctx, net);
                        if (brRet->HasBranches) {
                            if (brRet->ShouldContinue) { currentUid = brRet->NextUid; continue; }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid)) currentUid = nextMap[currentUid];
                        else break;
                    }
                    else if (pt == "NOP") {
                        LadElement^ nopEl = gcnew LadElement();
                        nopEl->Type = "nop";
                        net->Elements->Add(nopEl);

                        BranchResult^ brNop = ProcessParallelBranches(currentUid, ctx, net);
                        if (brNop->HasBranches) {
                            if (brNop->ShouldContinue) { currentUid = brNop->NextUid; continue; }
                            break;
                        }
                        else if (nextMap->ContainsKey(currentUid)) currentUid = nextMap[currentUid];
                        else break;
                    }
                    else {
                        if (nextMap->ContainsKey(currentUid))
                            currentUid = nextMap[currentUid];
                        else
                            break;
                    }

                    
                }

                if (net->Elements->Count > 0)
                    networks->Add(net);
            }
        }
    }
    catch (Exception^ e) {
        Console::WriteLine("      Parse error: " + e->Message);
    }
    return networks;
}