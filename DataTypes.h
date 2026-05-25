#pragma once

using namespace System;
using namespace System::Collections::Generic;

ref struct LadElement {
    String^ Type;
    String^ Tag;
    bool NormallyOpen;
    List<List<LadElement^>^>^ Branches;
    String^ InstanceName;
    String^ PresetTime;
    String^ AccessScope;
    String^ Tag2;
    String^ Tag3;
    String^ DataType;
};

ref struct LadNetwork {
    int Number;
    String^ Title;
    List<LadElement^>^ Elements;
};

ref struct LadDsl {
    List<LadNetwork^>^ Networks;
};

enum class CuPartType {
    Contact = 0,
    Coil = 1,
    SCoil = 2,
    RCoil = 3,
    TON = 4,
    TOF = 5,
    CTU = 6,
    CTD = 7,
    Compare = 8,
    Math = 9,
    Move = 10,
    O = 11,
    Junction = 12,
    TP = 13,
    CTUD = 14,
    CompareEQ = 15,
    CompareNE = 16,
    CompareGT = 17,
    CompareLT = 18,
    CompareGE = 19,
    CompareLE = 20,
    RisingEdge = 21,
    FallingEdge = 22,
    ADD = 23,
    SUB = 24,
    MUL = 25,
    DIV = 26,
    MOD = 27,
    JMP = 28,
    LABEL = 29,
    RET = 30,
    NOP = 31,
    RBitfield = 32,
    SBitfield = 33
};

ref struct LgNode {
    int Id;
    CuPartType NodeType;
    String^ Tag;
    bool NormallyOpen;
    String^ InstanceName;
    String^ PresetTime;
    bool IsParallelJoin;
    String^ Tag2;
    String^ Tag3;
    String^ DataType;
};

ref struct LgEdge {
    int FromId;
    int ToId;
};

ref struct LogicGraph {
    List<LgNode^>^ Nodes;
    List<LgEdge^>^ Edges;
};

#define PIN_DIR_IN  0
#define PIN_DIR_OUT 1

ref struct CuPin {
    String^ Name;
    int Dir;
    bool Required;
};

ref struct PinDef {
    String^ Name;
    int Dir;
    bool Required;
};

ref struct TemplateValueDef {
    String^ Name;
    String^ Type;
    String^ DefaultValue;
    bool IsAutomaticTyped;
};

ref struct PartSchema {
    CuPartType PartType;
    String^ FlgNetName;
    List<PinDef^>^ InputPins;
    List<PinDef^>^ OutputPins;
    bool RequiresInstanceDB;
    bool RequiresOperand;
    bool SupportsNegation;
    bool HasDynamicCardinality;
    String^ PresetPinName;
    String^ Version;
    List<TemplateValueDef^>^ TemplateValues;
    String^ InstanceType;
    bool UseCallStructure;
    bool DisabledENO;
    bool RequiresInstance;
};

ref struct InstructionDef {
    String^ Name;
    List<PinDef^>^ Pins;
};

ref struct CuPart {
    int Uid;
    CuPartType PartType;
    String^ Type;
    bool NormallyClosed;
    int Cardinality;
    String^ InstanceName;
    String^ TimerVersion;
    int InstanceUid;
    List<CuPin^>^ Pins;
    String^ DataType;
};

ref struct CuAccess {
    int Uid;
    String^ Symbol;
    int TargetPartUid;
    String^ Scope;
    String^ ConstantValue;
    String^ ConstantType;
};

ref struct CuWireTarget {
    int Uid;
    String^ Pin;
};

ref struct CuWire {
    int Uid;
    String^ FromKind;
    int FromUid;
    String^ FromPin;
    String^ ToKind;
    int ToUid;
    String^ ToPin;
    List<CuWireTarget^>^ ExtraTargets;
};

ref struct CompileUnitIR {
    List<CuPart^>^ Parts;
    List<CuAccess^>^ Accesses;
    List<CuWire^>^ Wires;
    String^ XmlNs;
};

enum class CfgNodeType {
    Entry,
    Exit,
    Serial,
    ParallelFork,
    ParallelJoin,
    OrFork,
    OrJoin,
    Instruction
};

ref struct CfgNode {
    int Id;
    CfgNodeType Type;
    CuPartType PartType;
    String^ Tag;
    bool NormallyOpen;
    String^ InstanceName;
    String^ PresetTime;
    List<int>^ Successors;
    List<int>^ Predecessors;
    int DominanceDepth;
    String^ Tag2;
    String^ Tag3;
    String^ DataType;
};

ref struct ControlFlowGraph {
    List<CfgNode^>^ Nodes;
    int EntryId;
    int ExitId;
    Dictionary<int, List<int>^>^ Adjacency;
};

ref struct OptimizationResult {
    bool Changed;
    String^ Description;
};

ref struct GraphCompilerPipeline {
    ControlFlowGraph^ Cfg;
    CompileUnitIR^ Ir;
    List<OptimizationResult^>^ OptimizationLog;
};

enum class IRNodeKind {
    Part = 0,
    Call = 1
};

ref struct IRParameter {
    String^ Name;
    String^ Section;
    String^ Datatype;
    int WireUid;
};

ref struct IRNode {
    int Uid;
    IRNodeKind Kind;
    String^ Name;
    bool Negated;
    int Cardinality;
    String^ InstanceDB;
    int InstanceUid;
    String^ DataType;
    List<IRParameter^>^ Parameters;
};

ref struct IRAccess {
    int Uid;
    String^ Symbol;
    String^ Scope;
    String^ ConstantValue;
    String^ ConstantType;
};

ref struct IRWireTarget {
    int Uid;
    String^ Pin;
};

ref struct IRWire {
    int Uid;
    String^ FromKind;
    int FromUid;
    String^ FromPin;
    String^ ToKind;
    int ToUid;
    String^ ToPin;
    List<IRWireTarget^>^ ExtraTargets;
};

ref struct IRProgram {
    List<IRNode^>^ Nodes;
    List<IRAccess^>^ Accesses;
    List<IRWire^>^ Wires;
    String^ XmlNs;
};

enum class P5CoilType {
    Normal = 0,
    Set = 1,
    Reset = 2,
    Negated = 3
};

ref struct P5IRInstruction {
    int UId;
};

ref struct P5IRContact : P5IRInstruction {
    String^ Tag;
    bool Negated;
};

ref struct P5IRCoil : P5IRInstruction {
    String^ Tag;
    P5CoilType CoilType;
};

ref struct P5IRCall : P5IRInstruction {
    String^ Name;
    Dictionary<String^, String^>^ Parameters;
};

ref struct P5IRBranch : P5IRInstruction {
    List<P5IRInstruction^>^ Children;
};

ref struct P5IRNetwork {
    String^ Title;
    List<P5IRInstruction^>^ Instructions;
};

ref struct P5IRProgram {
    String^ Name;
    List<P5IRNetwork^>^ Networks;
    List<String^>^ VariableNames;
};

enum class P5BackendType {
    LAD = 0,
    SCL = 1,
    FBD = 2,
    STL = 3,
    Visualization = 4,
    Simulation = 5
};

interface class P5IBackend {
    String^ Generate(P5IRProgram^ program);
};

enum class VariableScope {
    Global = 0,
    Temp = 1,
    Static = 2,
    Constant = 3,
    Input = 4,
    Output = 5
};

enum class VariableType {
    Bool = 0,
    Int = 1,
    Real = 2,
    Timer = 3,
    Counter = 4,
    String = 5,
    DInt = 6,
    Time = 7
};

ref struct VariableInfo {
    String^ Name;
    VariableScope Scope;
    VariableType Type;
    String^ InstanceDB;
    String^ DefaultValue;
};

ref struct InstanceDBInfo {
    String^ Name;
    String^ BlockType;
    VariableType DataType;
};

ref class VariableEngine {
public:
    static VariableType InferTypeFromPartType(CuPartType pt) {
        switch (pt) {
        case CuPartType::TON:
        case CuPartType::TOF:
        case CuPartType::TP:
            return VariableType::Timer;
        case CuPartType::CTU:
        case CuPartType::CTD:
        case CuPartType::CTUD:
            return VariableType::Counter;
        case CuPartType::ADD:
        case CuPartType::SUB:
        case CuPartType::MUL:
        case CuPartType::DIV:
        case CuPartType::MOD:
            return VariableType::Int;
        case CuPartType::Move:
            return VariableType::Int;
        case CuPartType::CompareEQ:
        case CuPartType::CompareNE:
        case CuPartType::CompareGT:
        case CuPartType::CompareLT:
        case CuPartType::CompareGE:
        case CuPartType::CompareLE:
            return VariableType::Bool;
        case CuPartType::Contact:
        case CuPartType::Coil:
        case CuPartType::SCoil:
        case CuPartType::RCoil:
        case CuPartType::RisingEdge:
        case CuPartType::FallingEdge:
            return VariableType::Bool;
        default:
            return VariableType::Bool;
        }
    }

    static String^ GenerateInstanceDBName(CuPartType pt, int index) {
        switch (pt) {
        case CuPartType::TON: return "IEC_TIMER_" + index.ToString();
        case CuPartType::TOF: return "IEC_TIMER_" + index.ToString();
        case CuPartType::TP: return "IEC_TIMER_" + index.ToString();
        case CuPartType::CTU: return "IEC_COUNTER_" + index.ToString();
        case CuPartType::CTD: return "IEC_COUNTER_" + index.ToString();
        case CuPartType::CTUD: return "IEC_COUNTER_" + index.ToString();
        default: return "DB_" + index.ToString();
        }
    }

    static String^ TypeToString(VariableType vt) {
        switch (vt) {
        case VariableType::Bool: return "Bool";
        case VariableType::Int: return "Int";
        case VariableType::Real: return "Real";
        case VariableType::Timer: return "Timer";
        case VariableType::Counter: return "Counter";
        case VariableType::String: return "String";
        case VariableType::DInt: return "DInt";
        case VariableType::Time: return "Time";
        default: return "Bool";
        }
    }

    static VariableType InferTypeFromSymbol(String^ symbol) {
        if (symbol == nullptr || symbol->Length == 0) return VariableType::Bool;
        if (symbol->StartsWith("M") || symbol->StartsWith("I") || symbol->StartsWith("Q"))
            return VariableType::Bool;
        if (symbol->Contains("DB_") || symbol->Contains("DBD"))
            return VariableType::Int;
        if (symbol->StartsWith("T#"))
            return VariableType::Time;
        return VariableType::Bool;
    }
};

ref class TypeInferenceEngine {
public:
    static String^ InferSrcType(String^ instrName, List<String^>^ operandTypes) {
        if (instrName == "Add" || instrName == "Sub" || instrName == "Mul" ||
            instrName == "Div" || instrName == "Mod") {
            return InferMathResultType(operandTypes);
        }
        if (instrName == "Eq" || instrName == "Ne" || instrName == "Gt" ||
            instrName == "Lt" || instrName == "Ge" || instrName == "Le") {
            return InferCompareSrcType(operandTypes);
        }
        if (instrName == "Move") {
            if (operandTypes != nullptr && operandTypes->Count > 0) {
                String^ first = operandTypes[0];
                if (first != nullptr && first->Length > 0 && first != "Bool") return first;
            }
            return "Int";
        }
        return "Int";
    }

    static String^ InferMathResultType(List<String^>^ operandTypes) {
        bool hasReal = false;
        bool hasDInt = false;
        bool hasInt = false;
        if (operandTypes != nullptr) {
            for each (String^ t in operandTypes) {
                if (t == "Real" || t == "LReal") hasReal = true;
                else if (t == "DInt" || t == "UDInt" || t == "DWord") hasDInt = true;
                else if (t == "Int" || t == "UInt" || t == "Word" || t == "SInt" || t == "USInt" || t == "Byte") hasInt = true;
            }
        }
        if (hasReal) return "Real";
        if (hasDInt) return "DInt";
        if (hasInt) return "Int";
        return "Int";
    }

    static String^ InferCompareSrcType(List<String^>^ operandTypes) {
        return InferMathResultType(operandTypes);
    }

    static String^ InferTypeFromConstant(String^ constValue) {
        if (constValue == nullptr || constValue->Length == 0) return "DInt";
        if (constValue->StartsWith("T#")) return "Time";
        if (constValue->StartsWith("16#")) return "DInt";
        if (constValue->Contains(".") && !constValue->StartsWith("DB")) return "Real";
        return "DInt";
    }

    static String^ InferTypeFromSymbol(String^ symbol) {
        if (symbol == nullptr || symbol->Length == 0) return "Bool";
        if (symbol->StartsWith("MW") || symbol->StartsWith("MD")) return "Int";
        if (symbol->StartsWith("DBD") || symbol->StartsWith("DBW")) return "Int";
        if (symbol->Contains("Real") || symbol->Contains("real")) return "Real";
        return "Bool";
    }
};

interface class IBackend {
    String^ Generate(IRProgram^ program);
};

enum class P6TaskType {
    AnalyzeDevices = 0,
    ExtractIO = 1,
    DeriveActions = 2,
    GenerateStateMachine = 3,
    GenerateProgram = 4,
    VerifyInterlock = 5,
    GenerateAlarm = 6,
    GenerateHMIVars = 7,
    ExportProject = 8
};

ref struct P6Task {
    int Id;
    P6TaskType Type;
    String^ Description;
    String^ Status;
};

ref struct P6ControlAction {
    String^ Type;
    String^ Target;
    String^ Condition;
    String^ Parameter;
};

ref struct P6SafetyRule {
    String^ RuleType;
    String^ Description;
    String^ TargetTag;
    bool Applied;
};

ref struct P6KnowledgeTemplate {
    String^ Name;
    String^ Category;
    String^ Description;
    List<String^>^ RequiredInputs;
    List<String^>^ RequiredOutputs;
    String^ DslTemplate;
};

ref struct P6SimulationState {
    Dictionary<String^, bool>^ BoolVars;
    Dictionary<String^, int>^ IntVars;
    Dictionary<String^, String^>^ TimerStates;
    Dictionary<String^, int>^ CounterValues;
    int CycleCount;
    List<String^>^ TraceLog;

    P6SimulationState() {
        BoolVars = gcnew Dictionary<String^, bool>();
        IntVars = gcnew Dictionary<String^, int>();
        TimerStates = gcnew Dictionary<String^, String^>();
        CounterValues = gcnew Dictionary<String^, int>();
        CycleCount = 0;
        TraceLog = gcnew List<String^>();
    }
};

ref struct P6HmiElement {
    String^ Type;
    String^ Tag;
    String^ Label;
    int X;
    int Y;
    int Width;
    int Height;
};

ref struct P6DocumentSection {
    String^ Title;
    String^ Content;
};