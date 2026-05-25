#pragma once
#include "Dsl.h"
#include "LadConversion.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::IO;
using namespace System::Net;
using namespace System::Text;
using namespace System::Text::RegularExpressions;

ref struct P3Config {
    String^ ApiUrl;
    String^ ApiKey;
    String^ Model;
    double Temperature;
    int MaxTokens;
    int MaxRetries;

    P3Config() {
        ApiUrl = "https://api.deepseek.com/chat/completions";
        ApiKey = "";
        Model = "deepseek-v4-flash";
        Temperature = 0.1;
        MaxTokens = 8192;
        MaxRetries = 3;
    }

    static P3Config^ Load(String^ path) {
        P3Config^ cfg = gcnew P3Config();
        if (!File::Exists(path)) return cfg;
        try {
            String^ json = File::ReadAllText(path, Encoding::UTF8);
            Match^ m;

            m = Regex::Match(json, "\"api_url\"\\s*:\\s*\"([^\"]*)\"");
            if (m->Success && m->Groups[1]->Value->Length > 0) cfg->ApiUrl = m->Groups[1]->Value;

            m = Regex::Match(json, "\"api_key\"\\s*:\\s*\"([^\"]*)\"");
            if (m->Success && m->Groups[1]->Value->Length > 0) cfg->ApiKey = m->Groups[1]->Value;

            m = Regex::Match(json, "\"model\"\\s*:\\s*\"([^\"]*)\"");
            if (m->Success && m->Groups[1]->Value->Length > 0) cfg->Model = m->Groups[1]->Value;

            m = Regex::Match(json, "\"temperature\"\\s*:\\s*([0-9.]+)");
            if (m->Success) {
                double t;
                if (Double::TryParse(m->Groups[1]->Value, t)) cfg->Temperature = t;
            }
        }
        catch (Exception^) {}
        return cfg;
    }

    void Save(String^ path) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("{");
        sb->AppendLine("  \"api_url\": " + EscapeJson(ApiUrl) + ",");
        sb->AppendLine("  \"api_key\": " + EscapeJson(ApiKey) + ",");
        sb->AppendLine("  \"model\": " + EscapeJson(Model) + ",");
        sb->AppendLine("  \"temperature\": " + Temperature.ToString() + ",");
        sb->AppendLine("  \"max_tokens\": " + MaxTokens.ToString() + ",");
        sb->AppendLine("  \"max_retries\": " + MaxRetries.ToString());
        sb->AppendLine("}");
        File::WriteAllText(path, sb->ToString(), gcnew UTF8Encoding(false));
    }
};

ref class P3PromptEngine {
private:
    static String^ SystemPrompt() {
        return "You are a PLC (Programmable Logic Controller) programming expert specializing in Ladder Diagram (LAD) programming for Siemens TIA Portal.\n"
            "\n"
            "Your task is to generate PLC programs in a standardized JSON DSL format.\n"
            "\n"
            "## DSL Format Specification\n"
            "\n"
            "The output must be a single JSON object with this structure:\n"
            "{\n"
            "  \"dsl_version\": \"1.0\",\n"
            "  \"variables\": [ ... ],\n"
            "  \"networks\": [ ... ],\n"
            "  \"steps\": [ ... ],\n"
            "  \"timers\": [ ... ],\n"
            "  \"counters\": [ ... ]\n"
            "}\n"
            "\n"
            "### Variable Declaration\n"
            "Each variable has: name, type, scope, comment, timer_type, counter_type, preset\n"
            "- type: \"Bool\", \"Int\", \"Real\", \"Timer\", \"Counter\", \"DInt\", \"Word\", \"Byte\"\n"
            "- scope: \"input\", \"output\", \"internal\", \"inout\"\n"
            "- timer_type: \"TON\", \"TOF\", \"TP\" (only when type=\"Timer\")\n"
            "- counter_type: \"CTU\", \"CTD\", \"CTUD\" (only when type=\"Counter\")\n"
            "- preset: timer preset (e.g. \"T#3S\") or counter preset (e.g. \"5\")\n"
            "\n"
            "### Steps (for sequential control)\n"
            "Optional: define steps for sequential processes\n"
            "Each step has: name, action, transition, next_step\n"
            "- name: step variable name (e.g. \"Step1\", \"Step2\")\n"
            "- action: what this step does (e.g. \"Fill tank\")\n"
            "- transition: condition to move to next step (e.g. \"High_Level reached\")\n"
            "- next_step: name of the next step\n"
            "\n"
            "### Timers (declarative)\n"
            "Optional: declare timers separately for clarity\n"
            "Each timer has: name, timer_type, preset, comment\n"
            "\n"
            "### Counters (declarative)\n"
            "Optional: declare counters separately for clarity\n"
            "Each counter has: name, counter_type, preset, comment\n"
            "\n"
            "### Network Structure\n"
            "Each network has: title, nodes[]\n"
            "Nodes are executed in order (left to right, top to bottom).\n"
            "\n"
            "### Node Types\n"
            "- {\"type\":\"contact\", \"tag\":\"VarName\", \"normallyOpen\":true} - Normally open contact\n"
            "- {\"type\":\"contact\", \"tag\":\"VarName\", \"normallyOpen\":false} - Normally closed contact\n"
            "- {\"type\":\"negated_contact\", \"tag\":\"VarName\"} - Negated contact\n"
            "- {\"type\":\"coil\", \"tag\":\"VarName\"} - Output coil\n"
            "- {\"type\":\"set\", \"tag\":\"VarName\"} - Set (latch) coil\n"
            "- {\"type\":\"reset\", \"tag\":\"VarName\"} - Reset (unlatch) coil\n"
            "- {\"type\":\"ton\", \"instance\":\"TimerDB\", \"pt\":\"T#3S\"} - On-delay timer\n"
            "- {\"type\":\"tof\", \"instance\":\"TimerDB\", \"pt\":\"T#5S\"} - Off-delay timer\n"
            "- {\"type\":\"tp\", \"instance\":\"TimerDB\", \"pt\":\"T#2S\"} - Pulse timer\n"
            "- {\"type\":\"ctu\", \"instance\":\"CounterDB\", \"pv\":\"5\"} - Count up counter\n"
            "- {\"type\":\"ctd\", \"instance\":\"CounterDB\", \"pv\":\"10\"} - Count down counter\n"
            "- {\"type\":\"ctud\", \"instance\":\"CounterDB\", \"pv\":\"5\"} - Up/down counter\n"
            "- {\"type\":\"compare_eq\", \"tag\":\"Var1\", \"tag2\":\"Var2\", \"data_type\":\"Int\"} - Equal compare\n"
            "- {\"type\":\"compare_ne\", \"tag\":\"Var1\", \"tag2\":\"Var2\"} - Not equal compare\n"
            "- {\"type\":\"compare_gt\", \"tag\":\"Var1\", \"tag2\":\"Var2\"} - Greater than compare\n"
            "- {\"type\":\"compare_lt\", \"tag\":\"Var1\", \"tag2\":\"Var2\"} - Less than compare\n"
            "- {\"type\":\"compare_ge\", \"tag\":\"Var1\", \"tag2\":\"Var2\"} - Greater or equal compare\n"
            "- {\"type\":\"compare_le\", \"tag\":\"Var1\", \"tag2\":\"Var2\"} - Less or equal compare\n"
            "- {\"type\":\"add\", \"tag\":\"Var1\", \"tag2\":\"Var2\", \"tag3\":\"Result\", \"data_type\":\"Int\"} - Addition\n"
            "- {\"type\":\"sub\", \"tag\":\"Var1\", \"tag2\":\"Var2\", \"tag3\":\"Result\"} - Subtraction\n"
            "- {\"type\":\"mul\", \"tag\":\"Var1\", \"tag2\":\"Var2\", \"tag3\":\"Result\"} - Multiplication\n"
            "- {\"type\":\"div\", \"tag\":\"Var1\", \"tag2\":\"Var2\", \"tag3\":\"Result\"} - Division\n"
            "- {\"type\":\"mod\", \"tag\":\"Var1\", \"tag2\":\"Var2\", \"tag3\":\"Result\"} - Modulo\n"
            "- {\"type\":\"move\", \"tag\":\"Source\", \"tag2\":\"Dest\"} - Move data\n"
            "- {\"type\":\"rising_edge\", \"tag\":\"VarName\"} - Rising edge detection\n"
            "- {\"type\":\"falling_edge\", \"tag\":\"VarName\"} - Falling edge detection\n"
            "- {\"type\":\"jump\", \"label\":\"LABEL1\"} - Jump to label\n"
            "- {\"type\":\"label\", \"label\":\"LABEL1\"} - Label definition\n"
            "- {\"type\":\"step\", \"tag\":\"Step1\"} - Step marker (marks step activation point)\n"
            "- {\"type\":\"transition\", \"tag\":\"Step1\", \"tag2\":\"Step2\"} - Transition from one step to next\n"
            "- {\"type\":\"ret\"} - Return\n"
            "- {\"type\":\"nop\"} - No operation\n"
            "\n"
            "### Parallel Branches\n"
            "Use parallel structure for OR/AND branches:\n"
            "{\"type\":\"parallel\", \"branches\":[\n"
            "  {\"nodes\":[ ... ]},\n"
            "  {\"nodes\":[ ... ]}\n"
            "]}\n"
            "\n"
            "## Critical PLC Rules\n"
            "1. Every network MUST have at least one output element (coil, set, reset, ton, tof, tp, ctu, ctd, ctud, move, add, sub, mul, div, mod, jump)\n"
            "2. Timer instances must be declared in variables with type=\"Timer\" and timer_type\n"
            "3. Counter instances must be declared in variables with type=\"Counter\" and counter_type\n"
            "4. Timer preset format: \"T#<value><unit>\" (e.g. \"T#3S\", \"T#500MS\", \"T#1M30S\")\n"
            "5. Counter preset format: integer string (e.g. \"5\", \"10\", \"100\")\n"
            "6. Self-holding circuits: use contact + coil with same tag, parallel with start button\n"
            "7. Avoid double coil: same output tag should not appear in multiple networks unless intentional\n"
            "8. Compare/Math instructions need data_type (\"Int\" or \"Real\") for proper compilation\n"
            "9. Move instruction: tag=source, tag2=destination\n"
            "10. Math instructions: tag=in1, tag2=in2, tag3=result\n"
            "11. Compare instructions: tag=in1, tag2=in2\n"
            "\n"
            "## Sequential Control (Step Logic) Rules\n"
            "12. For sequential processes (multi-step operations), use Step variables (Step1, Step2, ...) with SET/RESET pattern\n"
            "13. Each step should: SET the next step AND RESET the current step in the transition network\n"
            "14. Step activation: use SET coil to activate next step, RESET coil to deactivate current step\n"
            "15. Step output: each step's output actions should be in a separate network with step contact as condition\n"
            "16. Initial step: the first step should be activated by a start condition (SET Step1)\n"
            "17. Cycle completion: the last step should either RESET all steps or loop back to Step1\n"
            "18. Interlocking: for mutually exclusive outputs (e.g. forward/reverse), use NC contacts of the opposite output\n"
            "19. For cyclic operations, use a counter (CTU) to count cycles and stop after reaching the target count\n"
            "20. For timed transitions between steps, use TON timer with step contact as condition\n"
            "\n"
            "## Output Rules\n"
            "- Output ONLY the JSON DSL, no other text\n"
            "- No markdown code blocks, no explanations\n"
            "- Ensure valid JSON syntax\n"
            "- Use meaningful variable names in English\n";
    }

    static String^ PlanningPrompt(String^ problem) {
        return "Analyze this PLC control problem and plan the solution:\n\n"
            + problem + "\n\n"
            "Think step by step:\n"
            "1. What are the inputs and outputs?\n"
            "2. What is the control sequence/flow?\n"
            "3. Is this a SEQUENTIAL process? (multiple steps that execute in order)\n"
            "   - If YES: identify each step, its action, and its transition condition\n"
            "   - Use Step variables (Step1, Step2, ...) with SET/RESET pattern\n"
            "4. Is this a CYCLIC process? (repeats N times)\n"
            "   - If YES: add a counter (CTU) and cycle completion logic\n"
            "5. Is there INTERLOCKING needed? (mutually exclusive outputs like forward/reverse)\n"
            "   - If YES: add NC contacts for mutual exclusion\n"
            "6. What timers or counters are needed?\n"
            "7. What internal variables are needed?\n"
            "8. How should the networks be organized?\n"
            "   - Each step's output actions = separate network\n"
            "   - Each step transition = separate network\n"
            "9. Are there any self-holding, interlocking, or sequential control requirements?\n\n"
            "Provide a brief analysis, then generate the DSL JSON.";
    }

    static String^ DslGenerationPrompt(String^ problem, String^ planning) {
        return "Based on the following PLC problem and analysis, generate the complete DSL JSON:\n\n"
            "Problem: " + problem + "\n\n"
            + (planning != nullptr && planning->Length > 0 ? "Analysis: " + planning + "\n\n" : "") +
            "Generate the complete PLC DSL JSON now. Output ONLY the JSON, no other text.";
    }

    static String^ RepairPrompt(String^ dslJson, String^ errors) {
        return "The following PLC DSL JSON has validation errors. Fix them and output the corrected JSON.\n\n"
            "Original DSL:\n" + dslJson + "\n\n"
            "Errors:\n" + errors + "\n\n"
            "Output the corrected DSL JSON. Output ONLY the JSON, no other text.";
    }

    static String^ FewShotExample1() {
        return "Example: Motor start/stop with self-holding\n"
            "Problem: Press Start button, motor runs. Press Stop button, motor stops. Motor keeps running after Start is released.\n\n"
            "Solution:\n"
            "{\n"
            "  \"dsl_version\": \"1.0\",\n"
            "  \"variables\": [\n"
            "    {\"name\": \"Start_Btn\", \"type\": \"Bool\", \"scope\": \"input\", \"comment\": \"Start button\"},\n"
            "    {\"name\": \"Stop_Btn\", \"type\": \"Bool\", \"scope\": \"input\", \"comment\": \"Stop button\"},\n"
            "    {\"name\": \"Motor_Run\", \"type\": \"Bool\", \"scope\": \"output\", \"comment\": \"Motor running\"}\n"
            "  ],\n"
            "  \"networks\": [\n"
            "    {\n"
            "      \"title\": \"Motor Start/Stop with Self-holding\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Stop_Btn\", \"normallyOpen\": true},\n"
            "        {\"type\": \"parallel\", \"branches\": [\n"
            "          {\"nodes\": [{\"type\": \"contact\", \"tag\": \"Start_Btn\", \"normallyOpen\": true}]},\n"
            "          {\"nodes\": [{\"type\": \"contact\", \"tag\": \"Motor_Run\", \"normallyOpen\": true}]}\n"
            "        ]},\n"
            "        {\"type\": \"coil\", \"tag\": \"Motor_Run\"}\n"
            "      ]\n"
            "    }\n"
            "  ]\n"
            "}";
    }

    static String^ FewShotExample2() {
        return "Example: Timer delay control\n"
            "Problem: After motor starts, wait 3 seconds then activate valve. When motor stops, valve deactivates immediately.\n\n"
            "Solution:\n"
            "{\n"
            "  \"dsl_version\": \"1.0\",\n"
            "  \"variables\": [\n"
            "    {\"name\": \"Motor_Run\", \"type\": \"Bool\", \"scope\": \"input\", \"comment\": \"Motor running\"},\n"
            "    {\"name\": \"Valve_Open\", \"type\": \"Bool\", \"scope\": \"output\", \"comment\": \"Valve open\"},\n"
            "    {\"name\": \"Delay_Timer\", \"type\": \"Timer\", \"scope\": \"internal\", \"timer_type\": \"TON\", \"preset\": \"T#3S\", \"comment\": \"3s delay\"}\n"
            "  ],\n"
            "  \"networks\": [\n"
            "    {\n"
            "      \"title\": \"Timer Delay\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Motor_Run\", \"normallyOpen\": true},\n"
            "        {\"type\": \"ton\", \"instance\": \"Delay_Timer\", \"pt\": \"T#3S\"}\n"
            "      ]\n"
            "    },\n"
            "    {\n"
            "      \"title\": \"Valve Control\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Delay_Timer.Q\", \"normallyOpen\": true},\n"
            "        {\"type\": \"coil\", \"tag\": \"Valve_Open\"}\n"
            "      ]\n"
            "    }\n"
            "  ]\n"
            "}";
    }

    static String^ FewShotExample3() {
        return "Example: Counter-based cycle control\n"
            "Problem: Motor runs forward, reaches limit switch B, stops for 3 seconds, then reverses. After 5 cycles, stop completely.\n\n"
            "Solution:\n"
            "{\n"
            "  \"dsl_version\": \"1.0\",\n"
            "  \"variables\": [\n"
            "    {\"name\": \"Start_Btn\", \"type\": \"Bool\", \"scope\": \"input\", \"comment\": \"Start button\"},\n"
            "    {\"name\": \"Limit_B\", \"type\": \"Bool\", \"scope\": \"input\", \"comment\": \"Limit switch B\"},\n"
            "    {\"name\": \"Limit_A\", \"type\": \"Bool\", \"scope\": \"input\", \"comment\": \"Limit switch A\"},\n"
            "    {\"name\": \"Motor_Fwd\", \"type\": \"Bool\", \"scope\": \"output\", \"comment\": \"Motor forward\"},\n"
            "    {\"name\": \"Motor_Rev\", \"type\": \"Bool\", \"scope\": \"output\", \"comment\": \"Motor reverse\"},\n"
            "    {\"name\": \"Cycle_Done\", \"type\": \"Bool\", \"scope\": \"internal\", \"comment\": \"All cycles done\"},\n"
            "    {\"name\": \"At_B\", \"type\": \"Bool\", \"scope\": \"internal\", \"comment\": \"At position B\"},\n"
            "    {\"name\": \"Delay_Timer\", \"type\": \"Timer\", \"scope\": \"internal\", \"timer_type\": \"TON\", \"preset\": \"T#3S\", \"comment\": \"3s delay at B\"},\n"
            "    {\"name\": \"Cycle_Counter\", \"type\": \"Counter\", \"scope\": \"internal\", \"counter_type\": \"CTU\", \"preset\": \"5\", \"comment\": \"Cycle counter\"}\n"
            "  ],\n"
            "  \"networks\": [\n"
            "    {\n"
            "      \"title\": \"Forward Run\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Start_Btn\", \"normallyOpen\": true},\n"
            "        {\"type\": \"contact\", \"tag\": \"Cycle_Done\", \"normallyOpen\": false},\n"
            "        {\"type\": \"contact\", \"tag\": \"At_B\", \"normallyOpen\": false},\n"
            "        {\"type\": \"parallel\", \"branches\": [\n"
            "          {\"nodes\": [{\"type\": \"contact\", \"tag\": \"Start_Btn\", \"normallyOpen\": true}]},\n"
            "          {\"nodes\": [{\"type\": \"contact\", \"tag\": \"Motor_Fwd\", \"normallyOpen\": true}]}\n"
            "        ]},\n"
            "        {\"type\": \"coil\", \"tag\": \"Motor_Fwd\"}\n"
            "      ]\n"
            "    },\n"
            "    {\n"
            "      \"title\": \"Reached B - Start Delay\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Limit_B\", \"normallyOpen\": true},\n"
            "        {\"type\": \"set\", \"tag\": \"At_B\"}\n"
            "      ]\n"
            "    },\n"
            "    {\n"
            "      \"title\": \"Delay at B\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"At_B\", \"normallyOpen\": true},\n"
            "        {\"type\": \"ton\", \"instance\": \"Delay_Timer\", \"pt\": \"T#3S\"}\n"
            "      ]\n"
            "    },\n"
            "    {\n"
            "      \"title\": \"Reverse after Delay\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Delay_Timer.Q\", \"normallyOpen\": true},\n"
            "        {\"type\": \"contact\", \"tag\": \"Cycle_Done\", \"normallyOpen\": false},\n"
            "        {\"type\": \"coil\", \"tag\": \"Motor_Rev\"}\n"
            "      ]\n"
            "    },\n"
            "    {\n"
            "      \"title\": \"Back at A - Count Cycle\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Limit_A\", \"normallyOpen\": true},\n"
            "        {\"type\": \"contact\", \"tag\": \"Motor_Rev\", \"normallyOpen\": true},\n"
            "        {\"type\": \"ctu\", \"instance\": \"Cycle_Counter\", \"pv\": \"5\"},\n"
            "        {\"type\": \"reset\", \"tag\": \"At_B\"}\n"
            "      ]\n"
            "    },\n"
            "    {\n"
            "      \"title\": \"Cycle Complete\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Cycle_Counter.Q\", \"normallyOpen\": true},\n"
            "        {\"type\": \"set\", \"tag\": \"Cycle_Done\"}\n"
            "      ]\n"
            "    }\n"
            "  ]\n"
            "}";
    }

    static String^ FewShotExample4() {
        return "Example: Sequential control with steps\n"
            "Problem: Step1: Fill tank until high level. Step2: Heat for 10 seconds. Step3: Drain until low level. Then return to Step1.\n\n"
            "Solution:\n"
            "{\n"
            "  \"dsl_version\": \"1.0\",\n"
            "  \"variables\": [\n"
            "    {\"name\": \"High_Level\", \"type\": \"Bool\", \"scope\": \"input\", \"comment\": \"High level sensor\"},\n"
            "    {\"name\": \"Low_Level\", \"type\": \"Bool\", \"scope\": \"input\", \"comment\": \"Low level sensor\"},\n"
            "    {\"name\": \"Fill_Valve\", \"type\": \"Bool\", \"scope\": \"output\", \"comment\": \"Fill valve\"},\n"
            "    {\"name\": \"Heater\", \"type\": \"Bool\", \"scope\": \"output\", \"comment\": \"Heater\"},\n"
            "    {\"name\": \"Drain_Valve\", \"type\": \"Bool\", \"scope\": \"output\", \"comment\": \"Drain valve\"},\n"
            "    {\"name\": \"Step1\", \"type\": \"Bool\", \"scope\": \"internal\", \"comment\": \"Fill step\"},\n"
            "    {\"name\": \"Step2\", \"type\": \"Bool\", \"scope\": \"internal\", \"comment\": \"Heat step\"},\n"
            "    {\"name\": \"Step3\", \"type\": \"Bool\", \"scope\": \"internal\", \"comment\": \"Drain step\"},\n"
            "    {\"name\": \"Heat_Timer\", \"type\": \"Timer\", \"scope\": \"internal\", \"timer_type\": \"TON\", \"preset\": \"T#10S\", \"comment\": \"Heat timer\"}\n"
            "  ],\n"
            "  \"networks\": [\n"
            "    {\n"
            "      \"title\": \"Step1 - Fill\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Step1\", \"normallyOpen\": true},\n"
            "        {\"type\": \"coil\", \"tag\": \"Fill_Valve\"}\n"
            "      ]\n"
            "    },\n"
            "    {\n"
            "      \"title\": \"Step1->Step2 Transition\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Step1\", \"normallyOpen\": true},\n"
            "        {\"type\": \"contact\", \"tag\": \"High_Level\", \"normallyOpen\": true},\n"
            "        {\"type\": \"set\", \"tag\": \"Step2\"},\n"
            "        {\"type\": \"reset\", \"tag\": \"Step1\"}\n"
            "      ]\n"
            "    },\n"
            "    {\n"
            "      \"title\": \"Step2 - Heat\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Step2\", \"normallyOpen\": true},\n"
            "        {\"type\": \"coil\", \"tag\": \"Heater\"},\n"
            "        {\"type\": \"ton\", \"instance\": \"Heat_Timer\", \"pt\": \"T#10S\"}\n"
            "      ]\n"
            "    },\n"
            "    {\n"
            "      \"title\": \"Step2->Step3 Transition\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Step2\", \"normallyOpen\": true},\n"
            "        {\"type\": \"contact\", \"tag\": \"Heat_Timer.Q\", \"normallyOpen\": true},\n"
            "        {\"type\": \"set\", \"tag\": \"Step3\"},\n"
            "        {\"type\": \"reset\", \"tag\": \"Step2\"}\n"
            "      ]\n"
            "    },\n"
            "    {\n"
            "      \"title\": \"Step3 - Drain\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Step3\", \"normallyOpen\": true},\n"
            "        {\"type\": \"coil\", \"tag\": \"Drain_Valve\"}\n"
            "      ]\n"
            "    },\n"
            "    {\n"
            "      \"title\": \"Step3->Step1 Transition\",\n"
            "      \"nodes\": [\n"
            "        {\"type\": \"contact\", \"tag\": \"Step3\", \"normallyOpen\": true},\n"
            "        {\"type\": \"contact\", \"tag\": \"Low_Level\", \"normallyOpen\": true},\n"
            "        {\"type\": \"set\", \"tag\": \"Step1\"},\n"
            "        {\"type\": \"reset\", \"tag\": \"Step3\"}\n"
            "      ]\n"
            "    }\n"
            "  ]\n"
            "}";
    }

public:
    static String^ BuildSystemPrompt() {
        return SystemPrompt();
    }

    static String^ BuildUserPrompt(String^ problem) {
        String^ examples = LoadRelevantExamples(problem);
        return examples +
            FewShotExample1() + "\n\n" +
            FewShotExample2() + "\n\n" +
            FewShotExample3() + "\n\n" +
            FewShotExample4() + "\n\n" +
            PlanningPrompt(problem) + "\n\n" +
            DslGenerationPrompt(problem, "");
    }

    static String^ BuildRepairPrompt(String^ dslJson, String^ errors) {
        return RepairPrompt(dslJson, errors);
    }

    static String^ LoadRelevantExamples(String^ problem) {
        String^ exeDir = Path::GetDirectoryName(
            System::Reflection::Assembly::GetExecutingAssembly()->Location);
        String^ examplesPath = Path::Combine(exeDir, "phase3_examples.json");
        if (!File::Exists(examplesPath)) return "";

        try {
            String^ json = File::ReadAllText(examplesPath, Encoding::UTF8);
            String^ relevantExamples = "";
            array<String^>^ keywords = gcnew array<String^>{"sequential", "step", "cycle", "counter", "timer",
                "interlock", "forward", "reverse", "conveyor", "tank", "fill", "drain", "heat"};

            String^ problemLower = problem->ToLower();
            bool needAdvanced = problemLower->Contains("step") || problemLower->Contains("cycle") ||
                problemLower->Contains("sequential") || problemLower->Contains("state machine") ||
                problemLower->Contains("conveyor") || problemLower->Contains("station") ||
                problemLower->Contains("loop");
            bool needIntermediate = problemLower->Contains("count") || problemLower->Contains("counter") ||
                problemLower->Contains("delay") || problemLower->Contains("timer") ||
                problemLower->Contains("belt");

            if (needAdvanced || needIntermediate) {
                String^ categoryKey = needAdvanced ? "\"advanced\"" : "\"intermediate\"";
                int catIdx = json->IndexOf(categoryKey);
                if (catIdx >= 0) {
                    int exampleStart = json->IndexOf("\"examples\"", catIdx);
                    if (exampleStart >= 0) {
                        int arrStart = json->IndexOf('[', exampleStart);
                        int arrEnd = arrStart;
                        int depth = 0;
                        for (int i = arrStart; i < json->Length; i++) {
                            if (json[i] == '[') depth++;
                            else if (json[i] == ']') {
                                depth--;
                                if (depth == 0) { arrEnd = i; break; }
                            }
                        }
                        if (arrEnd > arrStart) {
                            String^ examplesArr = json->Substring(arrStart, arrEnd - arrStart + 1);
                            List<String^>^ items = P3DslParser::SplitJsonArray(examplesArr);
                            for each (String^ item in items) {
                                String^ title = P3DslParser::ExtractString(item, "title");
                                String^ prob = P3DslParser::ExtractString(item, "problem");
                                if (title != nullptr && prob != nullptr) {
                                    relevantExamples += "Reference Example: " + title + "\n" +
                                        "Problem: " + prob + "\n";
                                    String^ dslArr = P3DslParser::ExtractArray(item, "dsl");
                                    if (dslArr != nullptr && dslArr->Length > 2) {
                                        relevantExamples += "Solution:\n" + dslArr + "\n\n";
                                    }
                                    else {
                                        relevantExamples += "\n\n";
                                    }
                                }
                            }
                        }
                    }
                }
            }

            return relevantExamples;
        }
        catch (...) {
            return "";
        }
    }
};

ref class P3LlmClient {
public:
    static Action<String^>^ UiLogCallback;

    static String^ CallLlm(P3Config^ config, String^ systemPrompt, String^ userPrompt) {
        if (config->ApiKey == nullptr || config->ApiKey->Length == 0) {
            throw gcnew Exception("API key not configured. Please set api_key in phase3_config.json");
        }

        bool isAnthropic = config->ApiUrl->Contains("anthropic.com");
        bool isOpenAiCompat = !isAnthropic;

        String^ requestBody = isOpenAiCompat
            ? BuildOpenAiRequestBody(config, systemPrompt, userPrompt)
            : BuildAnthropicRequestBody(config, systemPrompt, userPrompt);

        Console::WriteLine("[LLM] Sending request to " + config->ApiUrl + " ...");
        Console::WriteLine("[LLM] Model: " + config->Model);
        if (UiLogCallback != nullptr) UiLogCallback("[INFO] 正在调用AI模型 " + config->Model + "...");

        for (int attempt = 0; attempt <= config->MaxRetries; attempt++) {
            try {
                String^ response;
                if (isAnthropic) {
                    response = HttpPostAnthropic(config->ApiUrl, config->ApiKey, requestBody);
                } else {
                    response = HttpPost(config->ApiUrl, config->ApiKey, requestBody);
                }

                String^ content = isAnthropic
                    ? ExtractAnthropicContent(response)
                    : ExtractContent(response);

                if (content != nullptr && content->Length > 0) {
                    Console::WriteLine("[LLM] Response received (" + content->Length + " chars)");
                    if (UiLogCallback != nullptr) UiLogCallback("[INFO] AI响应已收到 (" + content->Length + " 字符)");
                    return content;
                }
                Console::WriteLine("[LLM] Empty response, attempt " + (attempt + 1) + "/" + (config->MaxRetries + 1));
                if (UiLogCallback != nullptr) UiLogCallback("[WARNING] AI返回空响应，重试 " + (attempt + 1) + "/" + (config->MaxRetries + 1));
            }
            catch (Exception^ e) {
                Console::WriteLine("[LLM] Error on attempt " + (attempt + 1) + ": " + e->Message);
                if (UiLogCallback != nullptr) UiLogCallback("[WARNING] AI调用失败: " + e->Message + "，重试 " + (attempt + 1));
                if (attempt == config->MaxRetries) throw;
            }
        }
        return "";
    }

    static String^ CallLlm(P3Config^ config, String^ systemPrompt, String^ userPrompt, double temperature) {
        double origTemp = config->Temperature;
        config->Temperature = temperature;
        try {
            return CallLlm(config, systemPrompt, userPrompt);
        }
        finally {
            config->Temperature = origTemp;
        }
    }

private:
    static String^ BuildOpenAiRequestBody(P3Config^ config, String^ systemPrompt, String^ userPrompt) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->Append("{");
        sb->Append("\"model\":");
        sb->Append(EscapeJson(config->Model));
        sb->Append(",\"messages\":[");
        sb->Append("{\"role\":\"system\",\"content\":");
        sb->Append(EscapeJson(systemPrompt));
        sb->Append("},{\"role\":\"user\",\"content\":");
        sb->Append(EscapeJson(userPrompt));
        sb->Append("}]");
        sb->Append(",\"temperature\":");
        sb->Append(config->Temperature.ToString());
        sb->Append(",\"max_tokens\":");
        sb->Append(config->MaxTokens.ToString());
        sb->Append("}");
        return sb->ToString();
    }

    static String^ BuildAnthropicRequestBody(P3Config^ config, String^ systemPrompt, String^ userPrompt) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->Append("{");
        sb->Append("\"model\":");
        sb->Append(EscapeJson(config->Model));
        sb->Append(",\"system\":");
        sb->Append(EscapeJson(systemPrompt));
        sb->Append(",\"messages\":[");
        sb->Append("{\"role\":\"user\",\"content\":");
        sb->Append(EscapeJson(userPrompt));
        sb->Append("}]");
        sb->Append(",\"max_tokens\":");
        sb->Append(config->MaxTokens.ToString());
        sb->Append("}");
        return sb->ToString();
    }

    static String^ HttpPost(String^ url, String^ apiKey, String^ body) {
        System::Net::ServicePointManager::SecurityProtocol = System::Net::SecurityProtocolType::Tls12;

        HttpWebRequest^ request = (HttpWebRequest^)WebRequest::Create(url);
        request->Method = "POST";
        request->ContentType = "application/json";
        request->Headers->Add("Authorization", "Bearer " + apiKey);
        request->Timeout = 120000;
        request->ReadWriteTimeout = 180000;

        array<Byte>^ bodyBytes = Encoding::UTF8->GetBytes(body);
        request->ContentLength = bodyBytes->Length;
        Stream^ stream = request->GetRequestStream();
        stream->Write(bodyBytes, 0, bodyBytes->Length);
        stream->Close();

        HttpWebResponse^ response = (HttpWebResponse^)request->GetResponse();
        Stream^ respStream = response->GetResponseStream();
        StreamReader^ reader = gcnew StreamReader(respStream, Encoding::UTF8);
        String^ result = reader->ReadToEnd();
        reader->Close();
        response->Close();
        return result;
    }

    static String^ HttpPostAnthropic(String^ url, String^ apiKey, String^ body) {
        System::Net::ServicePointManager::SecurityProtocol = System::Net::SecurityProtocolType::Tls12;

        HttpWebRequest^ request = (HttpWebRequest^)WebRequest::Create(url);
        request->Method = "POST";
        request->ContentType = "application/json";
        request->Headers->Add("x-api-key", apiKey);
        request->Headers->Add("anthropic-version", "2023-06-01");
        request->Timeout = 120000;
        request->ReadWriteTimeout = 180000;

        array<Byte>^ bodyBytes = Encoding::UTF8->GetBytes(body);
        request->ContentLength = bodyBytes->Length;
        Stream^ stream = request->GetRequestStream();
        stream->Write(bodyBytes, 0, bodyBytes->Length);
        stream->Close();

        HttpWebResponse^ response = (HttpWebResponse^)request->GetResponse();
        Stream^ respStream = response->GetResponseStream();
        StreamReader^ reader = gcnew StreamReader(respStream, Encoding::UTF8);
        String^ result = reader->ReadToEnd();
        reader->Close();
        response->Close();
        return result;
    }

    static String^ ExtractContent(String^ responseJson) {
        String^ content = ExtractStringField(responseJson, "content");
        if (content != nullptr && content->Length > 0) return content;

        String^ reasoning = ExtractStringField(responseJson, "reasoning_content");
        if (reasoning != nullptr && reasoning->Length > 0) return reasoning;

        return content;
    }

    static String^ ExtractStringField(String^ json, String^ fieldName) {
        String^ pattern = "\"" + Regex::Escape(fieldName) + "\"\\s*:\\s*\"((?:[^\"\\\\]|\\\\.)*)\"";
        Match^ m = Regex::Match(json, pattern);
        if (!m->Success) {
            if (json->Contains("\"" + fieldName + "\"")) return "";
            return nullptr;
        }
        String^ val = m->Groups[1]->Value;
        val = val->Replace("\\\"", "\"")->Replace("\\\\", "\\")->Replace("\\n", "\n")->Replace("\\r", "\r")->Replace("\\t", "\t");
        return val;
    }

    static String^ ExtractAnthropicContent(String^ responseJson) {
        String^ text = ExtractStringField(responseJson, "text");
        if (text != nullptr && text->Length > 0) return text;
        return ExtractContent(responseJson);
    }
};

ref class P4RequirementParser {
public:
    static P4Requirement^ Parse(String^ problem, P3Config^ config) {
        Console::WriteLine("[RequirementParser] Analyzing problem requirements...");

        String^ systemPrompt = "You are a PLC requirements analyst. Extract structured control requirements from the problem description.\n"
            "Output ONLY a JSON object with this exact structure:\n"
            "{\n"
            "  \"inputs\": [{\"name\": \"Signal_Name\", \"comment\": \"description\"}],\n"
            "  \"outputs\": [{\"name\": \"Signal_Name\", \"comment\": \"description\"}],\n"
            "  \"timers\": [{\"name\": \"Timer_Name\", \"preset\": \"T#3S\"}],\n"
            "  \"counters\": [{\"name\": \"Counter_Name\", \"preset\": \"5\"}],\n"
            "  \"control_type\": \"sequential|combinational|cyclic|mixed\",\n"
            "  \"control_patterns\": [\"self_hold\", \"interlock\", \"sequential\", \"star_delta\", \"limit\", \"auto_round_trip\", \"timer_delay\", \"counter\", \"single_button\", \"flash\", \"multi_location\", \"mode_select\"],\n"
            "  \"interlock_pairs\": [[\"Output1\", \"Output2\"]],\n"
            "  \"sequential_order\": [\"Output1\", \"Output2\", \"Output3\"],\n"
            "  \"description\": \"Brief description of the control logic\"\n"
            "}\n\n"
            "Rules:\n"
            "- Use English variable names (e.g. Start_Btn, Motor_Fwd, Limit_E, SA)\n"
            "- Keep the original letter designations in variable names (e.g. Limit_E, Limit_F, Limit_G, Limit_H)\n"
            "- Identify ALL input signals (buttons, sensors, switches, limit switches)\n"
            "- Identify ALL output signals (motors, valves, indicators, doors)\n"
            "- For manual/auto mode: name the switch SA, jog buttons as Jog_Fwd/Jog_Rev\n"
            "- For motors: use Motor_Fwd (forward/正转) and Motor_Rev (reverse/反转)\n"
            "- For limit switches: use Limit_X format (e.g. Limit_E, Limit_F, Limit_G, Limit_H, Limit_R, Limit_L)\n"
            "- For star-delta: use KM_Main (main contactor), KM_Star (star contactor), KM_Delta (delta contactor)\n"
            "- For stop buttons: always use Stop_Btn, Stop_Btn_1, Stop_Btn_2 etc. (these are NC contacts physically)\n"
            "- Identify timers with correct preset format (T#<value><S/MS/M>)\n"
            "- Identify counters with integer preset values\n"
            "- control_type: sequential=multi-step process, combinational=simple logic, cyclic=repeating, mixed=combination\n"
            "- control_patterns: identify ALL applicable patterns from the list above\n"
            "- interlock_pairs: list pairs of outputs that must NOT be ON simultaneously (e.g. [[\"Motor_Fwd\",\"Motor_Rev\"],[\"KM_Star\",\"KM_Delta\"]])\n"
            "- sequential_order: list outputs in the order they must start (e.g. [\"Motor1\",\"Motor2\",\"Motor3\"])\n"
            "- Output ONLY the JSON, no other text\n";

        String^ userPrompt = "Analyze this PLC control problem and extract the requirements:\n\n" + problem;

        String^ llmResponse = P3LlmClient::CallLlm(config, systemPrompt, userPrompt);
        if (llmResponse == nullptr || llmResponse->Length == 0) {
            Console::WriteLine("  [RequirementParser] LLM returned empty, using fallback");
            return FallbackParse(problem);
        }

        String^ json = P3DslParser::ExtractJsonFromLlmResponse(llmResponse);
        if (json == nullptr || json->Length == 0) {
            Console::WriteLine("  [RequirementParser] Could not extract JSON, using fallback");
            Console::WriteLine("  [RequirementParser] Raw LLM response (first 300 chars): " + llmResponse->Substring(0, Math::Min(300, llmResponse->Length)));
            return FallbackParse(problem);
        }

        Console::WriteLine("  [RequirementParser] Extracted JSON (first 200 chars): " + json->Substring(0, Math::Min(200, json->Length)));

        P4Requirement^ req = gcnew P4Requirement();

        String^ inputsArr = P3DslParser::ExtractArray(json, "inputs");
        if (inputsArr != nullptr) {
            for each (String^ s in P3DslParser::SplitJsonArray(inputsArr)) {
                P4SignalDecl^ sig = gcnew P4SignalDecl();
                sig->Name = P3DslParser::ExtractString(s, "name");
                sig->Comment = P3DslParser::ExtractString(s, "comment");
                if (sig->Name != nullptr && sig->Name->Length > 0) req->Inputs->Add(sig);
            }
        }

        String^ outputsArr = P3DslParser::ExtractArray(json, "outputs");
        if (outputsArr != nullptr) {
            for each (String^ s in P3DslParser::SplitJsonArray(outputsArr)) {
                P4SignalDecl^ sig = gcnew P4SignalDecl();
                sig->Name = P3DslParser::ExtractString(s, "name");
                sig->Comment = P3DslParser::ExtractString(s, "comment");
                if (sig->Name != nullptr && sig->Name->Length > 0) req->Outputs->Add(sig);
            }
        }

        String^ timersArr = P3DslParser::ExtractArray(json, "timers");
        if (timersArr != nullptr) {
            for each (String^ s in P3DslParser::SplitJsonArray(timersArr)) {
                P4TimerRequirement^ t = gcnew P4TimerRequirement();
                t->Name = P3DslParser::ExtractString(s, "name");
                t->Preset = P3DslParser::ExtractString(s, "preset");
                if (t->Name != nullptr && t->Name->Length > 0) req->Timers->Add(t);
            }
        }

        String^ countersArr = P3DslParser::ExtractArray(json, "counters");
        if (countersArr != nullptr) {
            for each (String^ s in P3DslParser::SplitJsonArray(countersArr)) {
                P4CounterRequirement^ c = gcnew P4CounterRequirement();
                c->Name = P3DslParser::ExtractString(s, "name");
                c->Preset = P3DslParser::ExtractString(s, "preset");
                if (c->Name != nullptr && c->Name->Length > 0) req->Counters->Add(c);
            }
        }

        req->ControlType = P3DslParser::ExtractString(json, "control_type");
        req->Description = P3DslParser::ExtractString(json, "description");
        if (req->ControlType == nullptr) req->ControlType = "mixed";

        String^ patternsArr = P3DslParser::ExtractArray(json, "control_patterns");
        if (patternsArr != nullptr) {
            for each (String^ p in P3DslParser::SplitJsonArray(patternsArr)) {
                String^ clean = p->Trim()->Trim('"');
                if (clean->Length > 0) req->ControlPatterns->Add(clean);
            }
        }

        String^ interlockArr = P3DslParser::ExtractArray(json, "interlock_pairs");
        if (interlockArr != nullptr) {
            for each (String^ pairStr in P3DslParser::SplitJsonArray(interlockArr)) {
                String^ pairTrimmed = pairStr->Trim();
                if (pairTrimmed->StartsWith("[")) {
                    String^ innerArr = P3DslParser::ExtractArray("{\"d\":" + pairTrimmed + "}", "d");
                    if (innerArr != nullptr) {
                        List<String^>^ items = P3DslParser::SplitJsonArray(innerArr);
                        if (items->Count >= 2) {
                            String^ a = items[0]->Trim()->Trim('"');
                            String^ b = items[1]->Trim()->Trim('"');
                            if (a->Length > 0 && b->Length > 0) req->InterlockPairs->Add(a + "," + b);
                        }
                    }
                }
                else {
                    String^ clean = pairTrimmed->Trim('"');
                    if (clean->Length > 0 && clean->Contains(",")) req->InterlockPairs->Add(clean);
                }
            }
        }

        String^ seqArr = P3DslParser::ExtractArray(json, "sequential_order");
        if (seqArr != nullptr) {
            for each (String^ s in P3DslParser::SplitJsonArray(seqArr)) {
                String^ clean = s->Trim()->Trim('"');
                if (clean->Length > 0) req->SequentialOrder->Add(clean);
            }
        }

        if (req->ControlPatterns->Count == 0) {
            InferControlPatterns(req);
        }

        AutoExtractTimers(req, problem);

        Console::WriteLine("  Inputs: " + req->Inputs->Count + ", Outputs: " + req->Outputs->Count +
            ", Timers: " + req->Timers->Count + ", Counters: " + req->Counters->Count +
            ", Type: " + req->ControlType +
            ", Patterns: " + String::Join(",", req->ControlPatterns) +
            ", Interlocks: " + req->InterlockPairs->Count);
        return req;
    }

private:
    static P4Requirement^ FallbackParse(String^ problem) {
        P4Requirement^ req = gcnew P4Requirement();
        req->ControlType = "mixed";
        req->Description = problem;
        InferControlPatterns(req);
        return req;
    }

    static void InferControlPatterns(P4Requirement^ req) {
        if (req->ControlPatterns->Count > 0) return;

        String^ desc = (req->Description != nullptr ? req->Description : "")->ToLower();
        for each (P4SignalDecl^ s in req->Inputs) {
            if (s->Comment != nullptr) desc += " " + s->Comment->ToLower();
        }
        for each (P4SignalDecl^ s in req->Outputs) {
            if (s->Comment != nullptr) desc += " " + s->Comment->ToLower();
        }

        if (desc->Contains(L"互锁") || desc->Contains("interlock") || desc->Contains(L"不能同时"))
            if (!req->ControlPatterns->Contains("interlock")) req->ControlPatterns->Add("interlock");

        if (desc->Contains(L"顺序") || desc->Contains("sequential") || desc->Contains(L"依次"))
            if (!req->ControlPatterns->Contains("sequential")) req->ControlPatterns->Add("sequential");

        if (desc->Contains(L"星三角") || desc->Contains("star") || desc->Contains("delta") || desc->Contains(L"降压启动"))
            if (!req->ControlPatterns->Contains("star_delta")) req->ControlPatterns->Add("star_delta");

        if (desc->Contains(L"限位") || desc->Contains("limit"))
            if (!req->ControlPatterns->Contains("limit")) req->ControlPatterns->Add("limit");

        if (desc->Contains(L"往返") || desc->Contains("round") || desc->Contains(L"自动换向") || desc->Contains(L"自动往返"))
            if (!req->ControlPatterns->Contains("auto_round_trip")) req->ControlPatterns->Add("auto_round_trip");

        if (desc->Contains(L"延时") || desc->Contains("delay") || desc->Contains("timer") || desc->Contains(L"定时"))
            if (!req->ControlPatterns->Contains("timer_delay")) req->ControlPatterns->Add("timer_delay");

        if (desc->Contains(L"计数") || desc->Contains("counter") || desc->Contains(L"次数"))
            if (!req->ControlPatterns->Contains("counter")) req->ControlPatterns->Add("counter");

        if (desc->Contains(L"单按钮") || desc->Contains("single button") || desc->Contains(L"启停"))
            if (!req->ControlPatterns->Contains("single_button")) req->ControlPatterns->Add("single_button");

        if (desc->Contains(L"闪烁") || desc->Contains("flash") || desc->Contains(L"交替"))
            if (!req->ControlPatterns->Contains("flash")) req->ControlPatterns->Add("flash");

        if (desc->Contains(L"多地") || desc->Contains("multi-location"))
            if (!req->ControlPatterns->Contains("multi_location")) req->ControlPatterns->Add("multi_location");

        if (desc->Contains(L"手动") || desc->Contains(L"自动") || desc->Contains("manual") || desc->Contains("auto") || desc->Contains("mode"))
            if (!req->ControlPatterns->Contains("mode_select")) req->ControlPatterns->Add("mode_select");

        if (desc->Contains(L"正转") || desc->Contains(L"反转") || desc->Contains("forward") || desc->Contains("reverse") || desc->Contains("fwd") || desc->Contains("rev")) {
            if (!req->ControlPatterns->Contains("interlock")) req->ControlPatterns->Add("interlock");
        }

        if (req->Outputs->Count >= 2 && req->ControlPatterns->Count == 0) {
            req->ControlPatterns->Add("self_hold");
        }

        if (req->Outputs->Count == 1 && req->Inputs->Count >= 2) {
            req->ControlPatterns->Add("self_hold");
        }

        if (req->InterlockPairs->Count > 0 && !req->ControlPatterns->Contains("interlock")) {
            req->ControlPatterns->Add("interlock");
        }

        if (req->SequentialOrder->Count > 0 && !req->ControlPatterns->Contains("sequential")) {
            req->ControlPatterns->Add("sequential");
        }
    }

    static void AutoExtractTimers(P4Requirement^ req, String^ problem) {
        if (req->Timers != nullptr && req->Timers->Count > 0) return;
        if (problem == nullptr || problem->Length == 0) return;

        String^ desc = problem->ToLower();
        bool hasTimerHint = desc->Contains(L"定时") || desc->Contains(L"延时") || desc->Contains(L"等待") ||
            desc->Contains(L"计时") || desc->Contains(L"秒") || desc->Contains(L"分钟") ||
            desc->Contains("timer") || desc->Contains("delay") || desc->Contains("second") ||
            System::Text::RegularExpressions::Regex::IsMatch(desc, "\\d+\\s*[sms秒]");

        if (!hasTimerHint) return;

        array<String^, 2>^ actionPatterns = gcnew array<String^, 2> {
            {L"装料", "Timer_Load"}, {L"装", "Timer_Load"}, {"load", "Timer_Load"}, {"fill", "Timer_Load"},
            {L"卸料", "Timer_Unload"}, {L"卸", "Timer_Unload"}, {"unload", "Timer_Unload"}, {"drain", "Timer_Unload"},
            {L"清洗", "Timer_Clean"}, {L"清", "Timer_Clean"}, {"clean", "Timer_Clean"}, {"wash", "Timer_Clean"},
            {L"搅拌", "Timer_Mix"}, {L"搅", "Timer_Mix"}, {"mix", "Timer_Mix"}, {"stir", "Timer_Mix"},
            {L"加热", "Timer_Heat"}, {L"加", "Timer_Heat"}, {"heat", "Timer_Heat"},
            {L"冷却", "Timer_Cool"}, {L"冷", "Timer_Cool"}, {"cool", "Timer_Cool"},
            {L"运行", "Timer_Run"}, {"run", "Timer_Run"},
            {L"前进", "Timer_Fwd"}, {"forward", "Timer_Fwd"}, {"fwd", "Timer_Fwd"},
            {L"后退", "Timer_Rev"}, {"backward", "Timer_Rev"}, {"reverse", "Timer_Rev"}, {"rev", "Timer_Rev"},
            {L"开门", "Timer_DoorOpen"}, {L"关门", "Timer_DoorClose"},
            {L"注水", "Timer_Fill"}, {L"排水", "Timer_Drain"}
        };

        System::Text::RegularExpressions::Regex^ timePattern =
            gcnew System::Text::RegularExpressions::Regex(
                L"([\\u4e00-\\u9fff\\w]+?)\\s*[，,]?\\s*(\\d+)\\s*(秒|s|S|分钟|min|MIN|ms|MS)\\s*[后以]?",
                System::Text::RegularExpressions::RegexOptions::None);

        System::Text::RegularExpressions::MatchCollection^ matches = timePattern->Matches(problem);
        for each (System::Text::RegularExpressions::Match^ m in matches) {
            if (m->Groups->Count >= 3) {
                String^ action = m->Groups[1]->Value;
                String^ numStr = m->Groups[2]->Value;
                String^ unit = m->Groups[3]->Value;
                int numVal;
                if (!Int32::TryParse(numStr, numVal)) continue;

                String^ preset = "T#" + numStr;
                if (unit->Contains(L"分钟") || unit->ToLower()->Contains("min")) preset += "M";
                else if (unit->ToLower()->Contains("ms")) preset += "MS";
                else preset += "S";

                String^ timerName = nullptr;
                for (int i = 0; i < actionPatterns->GetLength(0); i++) {
                    if (action->Contains(actionPatterns[i, 0])) {
                        timerName = actionPatterns[i, 1];
                        break;
                    }
                }

                if (timerName == nullptr) {
                    bool foundExisting = false;
                    for each (P4TimerRequirement^ et in req->Timers) {
                        if (et->Preset != nullptr && et->Preset == preset) { foundExisting = true; break; }
                    }
                    if (!foundExisting) {
                        timerName = "Timer_" + (req->Timers->Count + 1);
                    }
                }

                if (timerName != nullptr) {
                    bool exists = false;
                    for each (P4TimerRequirement^ et in req->Timers) {
                        if (et->Name == timerName) { exists = true; break; }
                    }
                    if (!exists) {
                        P4TimerRequirement^ t = gcnew P4TimerRequirement();
                        t->Name = timerName;
                        t->Preset = preset;
                        req->Timers->Add(t);
                        Console::WriteLine("  [AutoExtractTimers] Found timer: " + timerName + " (" + preset + ") from action: " + action);
                    }
                }
            }
        }

        if (req->Timers->Count > 0 && !req->ControlPatterns->Contains("timer_delay")) {
            req->ControlPatterns->Add("timer_delay");
        }
    }
};

ref class P4SemanticPlanner {
public:
    static P4SemanticPlan^ Plan(String^ problem, P4Requirement^ requirement, P3Config^ config) {
        Console::WriteLine("[SemanticPlanner] Planning control flow (two-stage)...");

        String^ reqSummary = "Inputs: ";
        for each (P4SignalDecl^ s in requirement->Inputs) reqSummary += s->Name + "(" + s->Comment + ") ";
        reqSummary += "\nOutputs: ";
        for each (P4SignalDecl^ s in requirement->Outputs) reqSummary += s->Name + "(" + s->Comment + ") ";
        reqSummary += "\nTimers: ";
        for each (P4TimerRequirement^ t in requirement->Timers) reqSummary += t->Name + "(" + t->Preset + ") ";
        reqSummary += "\nCounters: ";
        for each (P4CounterRequirement^ c in requirement->Counters) reqSummary += c->Name + "(" + c->Preset + ") ";
        reqSummary += "\nControl Type: " + requirement->ControlType;

        String^ thinkingPrompt = "You are a PLC control flow architect. Analyze the following PLC problem and design a state machine.\n\n"
            "Please output your analysis in this format:\n\n"
            "1. State list (name, action, transition condition, next state)\n"
            "2. Initial state\n"
            "3. Is this a sequential process? (yes/no)\n"
            "4. Is this a cyclic process? (yes/no)\n"
            "5. Interlock pairs (mutually exclusive outputs)\n"
            "6. Which outputs are limited by which limit switches?\n"
            "7. Does it have auto round-trip? (yes/no)\n"
            "8. Does it have timer delays? (yes/no)\n"
            "9. Flow description (arrow notation: IDLE -> STATE1 -> STATE2 -> ...)\n\n"
            "Rules:\n"
            "- State names should be UPPERCASE (e.g. IDLE, FORWARD_TO_G, LOADING, BACKWARD_TO_F)\n"
            "- In action: use actual output variable names (e.g. Motor_Rev on, Load_Valve on)\n"
            "- In transition condition: use actual input variable names (e.g. Limit_G, Timer_Load.Q)\n"
            "- For timed transitions: reference timer names (e.g. Timer_Load.Q)\n"
            "- Include IDLE state as initial state\n"
            "- For cyclic processes, the last state should transition back to the first working state\n"
            "- For manual mode: include MANUAL_FWD and MANUAL_REV states\n"
            "- For mode selection: include conditions based on SA switch\n"
            "- Identify ALL interlock pairs\n"
            "- Do NOT output JSON. Output natural language analysis only.\n";

        String^ thinkingUserPrompt = "Analyze this PLC problem:\n\n" + problem + "\n\nRequirements extracted:\n" + reqSummary;

        String^ thinkingResult = P3LlmClient::CallLlm(config, thinkingPrompt, thinkingUserPrompt, 0.7);
        if (thinkingResult == nullptr || thinkingResult->Length == 0) {
            Console::WriteLine("  [SemanticPlanner] Thinking stage returned empty, using fallback");
            return FallbackPlan(requirement);
        }

        try {
            System::IO::File::WriteAllText("debug_thinking_result.txt", thinkingResult, System::Text::Encoding::UTF8);
        } catch (...) {}

        String^ jsonPrompt = "You are a JSON generator for PLC state machines. You MUST output ONLY a valid JSON object.\n"
            "STRICT RULES:\n"
            "- Output ONLY valid JSON. No thinking, no explanation, no markdown.\n"
            "- Do NOT wrap in ```json code fences.\n"
            "- Start your response with { and end with }.\n"
            "- No text before or after the JSON.\n\n"
            "JSON structure:\n"
            "{\n"
            "  \"states\": [\n"
            "    {\"name\": \"STATE_NAME\", \"action\": \"description\", \"transition_condition\": \"condition\", \"next_state\": \"NEXT_STATE\", \"comment\": \"note\"}\n"
            "  ],\n"
            "  \"initial_state\": \"IDLE\",\n"
            "  \"is_sequential\": true,\n"
            "  \"is_cyclic\": true,\n"
            "  \"has_interlock\": true,\n"
            "  \"interlock_pairs\": [\"Motor_Fwd,Motor_Rev\"],\n"
            "  \"has_limit_switch\": true,\n"
            "  \"limit_outputs\": [{\"output\": \"Motor_Fwd\", \"limit\": \"Limit_R\"}],\n"
            "  \"has_auto_round_trip\": true,\n"
            "  \"has_timer_delay\": true,\n"
            "  \"has_counter\": false,\n"
            "  \"flow_description\": \"IDLE -> STATE1 -> STATE2 -> ...\"\n"
            "}\n";

        String^ jsonUserPrompt = "Based on this analysis, generate the JSON state machine.\n\n"
            "Problem: " + problem + "\n\n"
            "Requirements:\n" + reqSummary + "\n\n"
            "Analysis result:\n" + thinkingResult + "\n\n"
            "Now output ONLY the JSON object. No explanation.";

        String^ jsonResult = P3LlmClient::CallLlm(config, jsonPrompt, jsonUserPrompt, 0.1);
        if (jsonResult == nullptr || jsonResult->Length == 0) {
            Console::WriteLine("  [SemanticPlanner] JSON stage returned empty, using fallback");
            return FallbackPlan(requirement);
        }

        try {
            System::IO::File::WriteAllText("debug_json_result.txt", jsonResult, System::Text::Encoding::UTF8);
        } catch (...) {}

        String^ json = P3DslParser::ExtractJsonFromLlmResponse(jsonResult);
        if (json == nullptr || json->Length == 0) {
            Console::WriteLine("[SEMANTIC] Could not extract JSON, attempting repair...");
            json = RepairJsonViaLlm(config, jsonResult, problem, reqSummary);
            if (json == nullptr || json->Length == 0) {
                Console::WriteLine("[SEMANTIC] Repair failed, using fallback");
                return FallbackPlan(requirement);
            }
        }

        try {
            System::IO::File::WriteAllText("debug_extracted_json.txt", json, System::Text::Encoding::UTF8);
        } catch (...) {}

        P4SemanticPlan^ plan = ParseSemanticJson(json, requirement);

        if (plan->States->Count == 0) {
            Console::WriteLine("[SEMANTIC] Parsed 0 states, attempting repair...");
            String^ repaired = RepairJsonViaLlm(config, jsonResult, problem, reqSummary);
            if (repaired != nullptr && repaired->Length > 0) {
                plan = ParseSemanticJson(repaired, requirement);
            }
        }

        if (plan->States->Count == 0) {
            Console::WriteLine("[SEMANTIC] Still 0 states after repair, using fallback");
            return FallbackPlan(requirement);
        }

        Console::WriteLine("  States: " + plan->States->Count + ", Sequential: " + plan->IsSequential +
            ", Cyclic: " + plan->IsCyclic + ", Interlock: " + plan->HasInterlock +
            ", InterlockPairs: " + plan->InterlockPairs->Count +
            ", SelfHold: " + plan->HasSelfHold + ", StarDelta: " + plan->HasStarDelta +
            ", Limit: " + plan->HasLimitSwitch + ", RoundTrip: " + plan->HasAutoRoundTrip);
        Console::WriteLine("  Flow: " + plan->FlowDescription);
        return plan;
    }

private:
    static P4SemanticPlan^ ParseSemanticJson(String^ json, P4Requirement^ requirement) {
        P4SemanticPlan^ plan = gcnew P4SemanticPlan();

        String^ statesArr = P3DslParser::ExtractArray(json, "states");
        if (statesArr != nullptr) {
            for each (String^ s in P3DslParser::SplitJsonArray(statesArr)) {
                P4State^ state = gcnew P4State();
                state->Name = P3DslParser::ExtractString(s, "name");
                state->Action = P3DslParser::ExtractString(s, "action");
                state->TransitionCondition = P3DslParser::ExtractString(s, "transition_condition");
                state->NextState = P3DslParser::ExtractString(s, "next_state");
                state->Comment = P3DslParser::ExtractString(s, "comment");
                if (state->Name != nullptr && state->Name->Length > 0) plan->States->Add(state);
            }
        }

        MergeDuplicateStates(plan);

        plan->InitialState = P3DslParser::ExtractString(json, "initial_state");
        String^ isSeq = P3DslParser::ExtractString(json, "is_sequential");
        plan->IsSequential = (isSeq != nullptr && (isSeq->ToLower() == "true" || isSeq == "1"));
        String^ isCyc = P3DslParser::ExtractString(json, "is_cyclic");
        plan->IsCyclic = (isCyc != nullptr && (isCyc->ToLower() == "true" || isCyc == "1"));
        String^ cycCnt = P3DslParser::ExtractString(json, "cycle_count");
        if (cycCnt != nullptr) {
            int v;
            if (Int32::TryParse(cycCnt, v)) plan->CycleCount = v;
        }
        String^ hasInt = P3DslParser::ExtractString(json, "has_interlock");
        plan->HasInterlock = (hasInt != nullptr && (hasInt->ToLower() == "true" || hasInt == "1"));

        String^ interlockArr = P3DslParser::ExtractArray(json, "interlock_pairs");
        if (interlockArr != nullptr) {
            for each (String^ pair in P3DslParser::SplitJsonArray(interlockArr)) {
                String^ cleanPair = pair->Trim()->Trim('"');
                if (cleanPair->Length > 0) plan->InterlockPairs->Add(cleanPair);
            }
        }
        else {
            String^ interlockStr = P3DslParser::ExtractString(json, "interlock_pairs");
            if (interlockStr != nullptr && interlockStr->Length > 0) {
                array<String^>^ pairs = interlockStr->Split(gcnew array<Char>{',', ';'});
                for each (String^ p in pairs) {
                    String^ clean = p->Trim()->Trim('"');
                    if (clean->Length > 0) plan->InterlockPairs->Add(clean);
                }
            }
        }

        plan->FlowDescription = P3DslParser::ExtractString(json, "flow_description");

        String^ hasSH = P3DslParser::ExtractString(json, "has_self_hold");
        plan->HasSelfHold = (hasSH != nullptr && (hasSH->ToLower() == "true" || hasSH == "1"));

        String^ shArr = P3DslParser::ExtractArray(json, "self_hold_outputs");
        if (shArr != nullptr) {
            for each (String^ s in P3DslParser::SplitJsonArray(shArr)) {
                String^ clean = s->Trim()->Trim('"');
                if (clean->Length > 0) plan->SelfHoldOutputs->Add(clean);
            }
        }

        String^ hasSD = P3DslParser::ExtractString(json, "has_star_delta");
        plan->HasStarDelta = (hasSD != nullptr && (hasSD->ToLower() == "true" || hasSD == "1"));

        String^ hasLS = P3DslParser::ExtractString(json, "has_limit_switch");
        plan->HasLimitSwitch = (hasLS != nullptr && (hasLS->ToLower() == "true" || hasLS == "1"));

        String^ limArr = P3DslParser::ExtractArray(json, "limit_outputs");
        if (limArr != nullptr) {
            for each (String^ s in P3DslParser::SplitJsonArray(limArr)) {
                String^ output = P3DslParser::ExtractString(s, "output");
                String^ limit = P3DslParser::ExtractString(s, "limit");
                if (output != nullptr && limit != nullptr && output->Length > 0 && limit->Length > 0) {
                    plan->LimitOutputs->Add(output + "," + limit);
                }
            }
        }

        String^ hasART = P3DslParser::ExtractString(json, "has_auto_round_trip");
        plan->HasAutoRoundTrip = (hasART != nullptr && (hasART->ToLower() == "true" || hasART == "1"));

        String^ hasTD = P3DslParser::ExtractString(json, "has_timer_delay");
        plan->HasTimerDelay = (hasTD != nullptr && (hasTD->ToLower() == "true" || hasTD == "1"));

        String^ hasC = P3DslParser::ExtractString(json, "has_counter");
        plan->HasCounter = (hasC != nullptr && (hasC->ToLower() == "true" || hasC == "1"));

        if (!plan->HasInterlock && requirement != nullptr && requirement->ControlPatterns->Contains("interlock")) {
            plan->HasInterlock = true;
        }
        if (!plan->HasSelfHold && requirement != nullptr && requirement->ControlPatterns->Contains("self_hold")) {
            plan->HasSelfHold = true;
        }
        if (!plan->HasStarDelta && requirement != nullptr && requirement->ControlPatterns->Contains("star_delta")) {
            plan->HasStarDelta = true;
        }
        if (!plan->HasLimitSwitch && requirement != nullptr && requirement->ControlPatterns->Contains("limit")) {
            plan->HasLimitSwitch = true;
        }
        if (!plan->HasAutoRoundTrip && requirement != nullptr && requirement->ControlPatterns->Contains("auto_round_trip")) {
            plan->HasAutoRoundTrip = true;
        }

        return plan;
    }

    static String^ RepairJsonViaLlm(P3Config^ config, String^ invalidResponse, String^ problem, String^ reqSummary) {
        Console::WriteLine("[SEMANTIC] Repairing JSON via LLM...");
        String^ repairPrompt = "You are a JSON repair tool. The following AI response should contain a valid JSON state machine but it is malformed or contains extra text.\n"
            "STRICT RULES:\n"
            "- Output ONLY valid JSON. No thinking, no explanation, no markdown.\n"
            "- Do NOT wrap in ```json code fences.\n"
            "- Start with { and end with }.\n"
            "- The JSON must have a \"states\" array with state objects.\n\n"
            "Malformed response:\n" + invalidResponse + "\n\n"
            "Output the corrected JSON:";

        String^ repairResult = P3LlmClient::CallLlm(config, repairPrompt, "Fix the JSON above. Output ONLY valid JSON.", 0.1);
        if (repairResult == nullptr || repairResult->Length == 0) return nullptr;

        String^ repaired = P3DslParser::ExtractJsonFromLlmResponse(repairResult);
        if (repaired != nullptr && repaired->Contains("\"states\"")) return repaired;
        return nullptr;
    }

    static P4SemanticPlan^ FallbackPlan(P4Requirement^ req) {
        P4SemanticPlan^ plan = gcnew P4SemanticPlan();
        plan->IsSequential = req->ControlType->ToLower()->Contains("seq");
        plan->IsCyclic = req->ControlType->ToLower()->Contains("cyc");
        plan->FlowDescription = "Fallback: direct DSL generation";
        return plan;
    }

    static void MergeDuplicateStates(P4SemanticPlan^ plan) {
        if (plan->States == nullptr || plan->States->Count <= 1) return;

        Dictionary<String^, P4State^>^ merged = gcnew Dictionary<String^, P4State^>(StringComparer::OrdinalIgnoreCase);
        List<String^>^ order = gcnew List<String^>();

        for each (P4State^ state in plan->States) {
            String^ key = state->Name->Trim();
            if (!merged->ContainsKey(key)) {
                merged[key] = state;
                order->Add(key);
            }
            else {
                P4State^ existing = merged[key];
                if (state->TransitionCondition != nullptr && state->TransitionCondition->Length > 0) {
                    if (existing->TransitionCondition != nullptr && existing->TransitionCondition->Length > 0) {
                        existing->TransitionCondition = existing->TransitionCondition + "||" + state->TransitionCondition;
                    }
                    else {
                        existing->TransitionCondition = state->TransitionCondition;
                    }
                }
                if (state->NextState != nullptr && state->NextState->Length > 0) {
                    if (existing->NextState != nullptr && existing->NextState->Length > 0) {
                        existing->NextState = existing->NextState + "|" + state->NextState;
                    }
                    else {
                        existing->NextState = state->NextState;
                    }
                }
                if (state->Action != nullptr && state->Action->Length > 0 &&
                    (existing->Action == nullptr || existing->Action->Length == 0 || existing->Action->Trim()->ToLower() == "none")) {
                    existing->Action = state->Action;
                }
            }
        }

        int before = plan->States->Count;
        plan->States->Clear();
        for each (String^ key in order) {
            plan->States->Add(merged[key]);
        }
        Console::WriteLine("[SEMANTIC] Merged duplicate states: " + before + " -> " + plan->States->Count);
    }
};

ref class P4VariablePlanner {
public:
    static P4VariablePlan^ Plan(P4Requirement^ requirement, P4SemanticPlan^ semanticPlan) {
        Console::WriteLine("[VariablePlanner] Generating variable plan with address allocation...");

        P4VariablePlan^ varPlan = gcnew P4VariablePlan();
        int inputAddr = 0;
        int outputAddr = 0;
        int internalAddr = 0;
        int timerIdx = 1;
        int counterIdx = 1;

        for each (P4SignalDecl^ sig in requirement->Inputs) {
            P3Variable^ v = gcnew P3Variable();
            String^ varName = sig->Name;
            v->Name = varName;
            v->Type = "Bool";
            v->Scope = "input";
            if (sig->Comment != nullptr && sig->Comment->Length > 0)
                v->Comment = sig->Name + " " + sig->Comment;
            else
                v->Comment = sig->Name;
            v->Preset = "";
            varPlan->Variables->Add(v);
            String^ addr = "I0." + inputAddr;
            varPlan->AddressMap[varName] = addr;
            inputAddr++;
        }

        for each (P4SignalDecl^ sig in requirement->Outputs) {
            P3Variable^ v = gcnew P3Variable();
            String^ varName = sig->Name;
            v->Name = varName;
            v->Type = "Bool";
            v->Scope = "output";
            if (sig->Comment != nullptr && sig->Comment->Length > 0)
                v->Comment = sig->Name + " " + sig->Comment;
            else
                v->Comment = sig->Name;
            v->Preset = "";
            varPlan->Variables->Add(v);
            String^ addr = "Q0." + outputAddr;
            varPlan->AddressMap[varName] = addr;
            outputAddr++;
        }

        if (semanticPlan->IsSequential) {
            for (int i = 0; i < semanticPlan->States->Count; i++) {
                P4State^ state = semanticPlan->States[i];
                String^ stepName = "Step" + (i + 1);
                P3Variable^ v = gcnew P3Variable();
                v->Name = stepName;
                v->Type = "Bool";
                v->Scope = "internal";
                v->Comment = state->Name + " - " + state->Action;
                v->Preset = "";
                varPlan->Variables->Add(v);
                String^ addr = "M0." + internalAddr;
                varPlan->AddressMap[stepName] = addr;
                internalAddr++;
            }
        }

        if (semanticPlan->IsCyclic && semanticPlan->CycleCount > 0) {
            P3Variable^ doneVar = gcnew P3Variable();
            doneVar->Name = "Cycle_Done";
            doneVar->Type = "Bool";
            doneVar->Scope = "internal";
            doneVar->Comment = "All cycles completed";
            doneVar->Preset = "";
            varPlan->Variables->Add(doneVar);
            varPlan->AddressMap["Cycle_Done"] = "M0." + internalAddr;
            internalAddr++;
        }

        for each (P4TimerRequirement^ t in requirement->Timers) {
            P3Variable^ v = gcnew P3Variable();
            v->Name = t->Name;
            v->Type = "Timer";
            v->Scope = "internal";
            v->TimerType = "TON";
            v->Preset = t->Preset != nullptr && t->Preset->Length > 0 ? t->Preset : "T#1S";
            v->Comment = "Timer " + t->Name;
            varPlan->Variables->Add(v);
        }

        for each (P4CounterRequirement^ c in requirement->Counters) {
            P3Variable^ v = gcnew P3Variable();
            v->Name = c->Name;
            v->Type = "Counter";
            v->Scope = "internal";
            v->CounterType = "CTU";
            v->Preset = c->Preset != nullptr && c->Preset->Length > 0 ? c->Preset : "1";
            v->Comment = "Counter " + c->Name;
            varPlan->Variables->Add(v);
        }

        Console::WriteLine("  Variables: " + varPlan->Variables->Count +
            ", Addresses allocated: " + varPlan->AddressMap->Count);
        return varPlan;
    }
};

ref class P4AiEnhancedGenerator {
public:
    static String^ BuildRequirementContext(P4Requirement^ req, P4SemanticPlan^ plan, P4VariablePlan^ varPlan) {
        StringBuilder^ sb = gcnew StringBuilder();

        sb->AppendLine("## Already Parsed Requirements (MUST follow these exactly)");
        sb->AppendLine();

        sb->AppendLine("### Inputs:");
        for each (P4SignalDecl^ sig in req->Inputs) {
            sb->AppendLine("  - " + sig->Name + (sig->Comment != nullptr && sig->Comment->Length > 0 ? " (" + sig->Comment + ")" : ""));
        }

        sb->AppendLine();
        sb->AppendLine("### Outputs:");
        for each (P4SignalDecl^ sig in req->Outputs) {
            sb->AppendLine("  - " + sig->Name + (sig->Comment != nullptr && sig->Comment->Length > 0 ? " (" + sig->Comment + ")" : ""));
        }

        if (req->Timers != nullptr && req->Timers->Count > 0) {
            sb->AppendLine();
            sb->AppendLine("### Timers:");
            for each (P4TimerRequirement^ t in req->Timers) {
                sb->AppendLine("  - " + t->Name + " (PT=" + t->Preset + ")");
            }
        }

        if (req->Counters != nullptr && req->Counters->Count > 0) {
            sb->AppendLine();
            sb->AppendLine("### Counters:");
            for each (P4CounterRequirement^ c in req->Counters) {
                sb->AppendLine("  - " + c->Name + " (PV=" + c->Preset + ")");
            }
        }

        if (req->ControlPatterns != nullptr && req->ControlPatterns->Count > 0) {
            sb->AppendLine();
            sb->AppendLine("### Control Patterns Detected:");
            for each (String^ p in req->ControlPatterns) {
                sb->AppendLine("  - " + p);
            }
        }

        if (req->InterlockPairs != nullptr && req->InterlockPairs->Count > 0) {
            sb->AppendLine();
            sb->AppendLine("### Interlock Pairs (these outputs must NOT be on simultaneously):");
            for each (String^ pair in req->InterlockPairs) {
                sb->AppendLine("  - " + pair);
            }
        }

        if (plan != nullptr) {
            sb->AppendLine();
            sb->AppendLine("### Semantic Plan:");
            sb->AppendLine("  - IsSequential: " + plan->IsSequential);
            sb->AppendLine("  - IsCyclic: " + plan->IsCyclic + (plan->IsCyclic ? " (count=" + plan->CycleCount + ")" : ""));
            sb->AppendLine("  - HasInterlock: " + plan->HasInterlock);
            sb->AppendLine("  - HasSelfHold: " + plan->HasSelfHold);
            sb->AppendLine("  - HasStarDelta: " + plan->HasStarDelta);
            sb->AppendLine("  - HasLimitSwitch: " + plan->HasLimitSwitch);
            sb->AppendLine("  - HasAutoRoundTrip: " + plan->HasAutoRoundTrip);
            sb->AppendLine("  - HasTimerDelay: " + plan->HasTimerDelay);
            sb->AppendLine("  - HasCounter: " + plan->HasCounter);
            if (plan->FlowDescription != nullptr && plan->FlowDescription->Length > 0) {
                sb->AppendLine("  - Flow: " + plan->FlowDescription);
            }
            if (plan->States != nullptr && plan->States->Count > 0) {
                sb->AppendLine("  - States:");
                for each (P4State^ st in plan->States) {
                    sb->AppendLine("    * " + st->Name + ": " + st->Action +
                        " -> transition: " + st->TransitionCondition + " -> next: " + st->NextState);
                }
            }
        }

        if (varPlan != nullptr && varPlan->Variables != nullptr && varPlan->Variables->Count > 0) {
            sb->AppendLine();
            sb->AppendLine("### Pre-allocated Variables (use these names exactly):");
            for each (P3Variable^ v in varPlan->Variables) {
                sb->Append("  - " + v->Name + " (" + v->Type + ", " + v->Scope);
                if (v->Comment != nullptr && v->Comment->Length > 0) sb->Append(", " + v->Comment);
                sb->AppendLine(")");
            }
        }

        return sb->ToString();
    }

    static String^ BuildEnhancedSystemPrompt() {
        return "You are a PLC (Programmable Logic Controller) programming expert specializing in Ladder Diagram (LAD) programming for Siemens TIA Portal.\n"
            "\n"
            "Your task is to generate PLC programs in a standardized JSON DSL format.\n"
            "\n"
            "## DSL Format Specification\n"
            "\n"
            "The output must be a single JSON object with this structure:\n"
            "{\n"
            "  \"dsl_version\": \"1.0\",\n"
            "  \"variables\": [ ... ],\n"
            "  \"networks\": [ ... ],\n"
            "  \"steps\": [ ... ],\n"
            "  \"timers\": [ ... ],\n"
            "  \"counters\": [ ... ]\n"
            "}\n"
            "\n"
            "### Variable Declaration\n"
            "Each variable has: name, type, scope, comment, timer_type, counter_type, preset\n"
            "- type: \"Bool\", \"Int\", \"Real\", \"Timer\", \"Counter\", \"DInt\", \"Word\", \"Byte\"\n"
            "- scope: \"input\", \"output\", \"internal\", \"inout\"\n"
            "- timer_type: \"TON\", \"TOF\", \"TP\" (only when type=\"Timer\")\n"
            "- counter_type: \"CTU\", \"CTD\", \"CTUD\" (only when type=\"Counter\")\n"
            "- preset: timer preset (e.g. \"T#3S\") or counter preset (e.g. \"5\")\n"
            "\n"
            "### Network Structure\n"
            "Each network has: title, nodes[]\n"
            "\n"
            "### Node Types\n"
            "- {\"type\":\"contact\", \"tag\":\"VarName\", \"normallyOpen\":true} - NO contact\n"
            "- {\"type\":\"contact\", \"tag\":\"VarName\", \"normallyOpen\":false} - NC contact\n"
            "- {\"type\":\"coil\", \"tag\":\"VarName\"} - Output coil\n"
            "- {\"type\":\"set\", \"tag\":\"VarName\"} - Set (latch) coil\n"
            "- {\"type\":\"reset\", \"tag\":\"VarName\"} - Reset (unlatch) coil\n"
            "- {\"type\":\"ton\", \"instance\":\"TimerDB\", \"pt\":\"T#3S\"} - On-delay timer\n"
            "- {\"type\":\"tof\", \"instance\":\"TimerDB\", \"pt\":\"T#5S\"} - Off-delay timer\n"
            "- {\"type\":\"ctu\", \"instance\":\"CounterDB\", \"pv\":\"5\"} - Count up counter\n"
            "- {\"type\":\"ctud\", \"instance\":\"CounterDB\", \"pv\":\"5\"} - Up/down counter\n"
            "- {\"type\":\"rising_edge\", \"tag\":\"VarName\"} - Rising edge detection\n"
            "- {\"type\":\"falling_edge\", \"tag\":\"VarName\"} - Falling edge detection\n"
            "- {\"type\":\"compare_eq\", \"tag\":\"Var1\", \"tag2\":\"Var2\", \"data_type\":\"Int\"}\n"
            "- {\"type\":\"compare_gt\", \"tag\":\"Var1\", \"tag2\":\"Var2\", \"data_type\":\"Int\"}\n"
            "- {\"type\":\"compare_lt\", \"tag\":\"Var1\", \"tag2\":\"Var2\", \"data_type\":\"Int\"}\n"
            "- {\"type\":\"move\", \"tag\":\"Source\", \"tag2\":\"Dest\"} - Move data\n"
            "- {\"type\":\"add\", \"tag\":\"Var1\", \"tag2\":\"Var2\", \"tag3\":\"Result\", \"data_type\":\"Int\"}\n"
            "- {\"type\":\"sub\", \"tag\":\"Var1\", \"tag2\":\"Var2\", \"tag3\":\"Result\", \"data_type\":\"Int\"}\n"
            "\n"
            "### Parallel Branches (for OR logic)\n"
            "{\"type\":\"parallel\", \"branches\":[{\"nodes\":[...]},{\"nodes\":[...]}]}\n"
            "\n"
            "## CRITICAL RULES\n"
            "1. Every network MUST have at least one output element (coil, set, reset, ton, tof, ctu, move, etc.)\n"
            "2. Use the EXACT variable names from the pre-allocated variables list\n"
            "3. Self-holding: use parallel branch with start button NO contact + self coil NO contact\n"
            "4. Interlocking: add NC contact of the opposing output in series\n"
            "5. Sequential control: use Step variables with SET/RESET pattern\n"
            "6. Timer: TON with step contact as input, .Q as transition condition\n"
            "7. Counter: CTU with count event as CU input, .Q as done condition\n"
            "8. Limit switch protection: add limit NC contact in series before coil\n"
            "9. Auto round-trip: limit NO contact triggers reverse, limit NC contact stops forward\n"
            "10. Star-delta: main contactor self-holds, star contactor on during timer, delta after timer.Q\n"
            "11. Single button start/stop: use rising_edge + internal toggle flip-flop\n"
            "12. Flash circuit: two TON timers cross-connected (T1.Q starts T2, T2.Q resets T1)\n"
            "\n"
            "## Output Rules\n"
            "- Output ONLY the JSON DSL, no other text\n"
            "- No markdown code blocks, no explanations\n"
            "- Ensure valid JSON syntax\n";
    }

    static P3Dsl^ GenerateFromContext(String^ problem, P4Requirement^ requirement, P4SemanticPlan^ semanticPlan, P4VariablePlan^ varPlan, P3Config^ config) {
        Console::WriteLine("  [AiEnhanced] Building enhanced prompt with requirement context...");

        String^ contextInfo = BuildRequirementContext(requirement, semanticPlan, varPlan);

        String^ userPrompt = "You are generating a PLC program for a control problem that the specialized template generators could NOT handle.\n"
            "This means the control pattern is complex or novel. You must generate the COMPLETE DSL JSON.\n\n"
            "## Original Problem\n" + problem + "\n\n"
            + contextInfo + "\n\n"
            "## Instructions\n"
            "1. Use the EXACT variable names from the pre-allocated variables above\n"
            "2. Follow the control patterns and interlock requirements exactly\n"
            "3. Generate COMPLETE networks for ALL outputs listed above\n"
            "4. Apply proper PLC design patterns (self-holding, interlocking, etc.) as indicated\n"
            "5. If the problem involves a pattern not in the standard list, implement it correctly based on PLC best practices\n"
            "6. Make sure every output has at least one network driving it\n\n"
            "Generate the complete PLC DSL JSON now. Output ONLY the JSON, no other text.";

        String^ systemPrompt = BuildEnhancedSystemPrompt();

        Console::WriteLine("  [AiEnhanced] Calling LLM with enhanced context (" + userPrompt->Length + " chars)...");
        String^ llmResponse = P3LlmClient::CallLlm(config, systemPrompt, userPrompt);

        if (llmResponse == nullptr || llmResponse->Length == 0) {
            Console::WriteLine("  [AiEnhanced] LLM returned empty response");
            return nullptr;
        }

        Console::WriteLine("  [AiEnhanced] LLM response received (" + llmResponse->Length + " chars)");
        String^ dslJson = P3DslParser::ExtractJsonFromLlmResponse(llmResponse);
        if (dslJson == nullptr || dslJson->Length == 0) {
            Console::WriteLine("  [AiEnhanced] Could not extract JSON from LLM response");
            Console::WriteLine("  Raw response preview: " + llmResponse->Substring(0, Math::Min(300, llmResponse->Length)));
            return nullptr;
        }

        Console::WriteLine("  [AiEnhanced] DSL JSON extracted (" + dslJson->Length + " chars)");
        P3Dsl^ dsl = P3DslParser::Parse(dslJson);
        if (dsl == nullptr) {
            Console::WriteLine("  [AiEnhanced] Failed to parse DSL JSON");
            return nullptr;
        }

        Console::WriteLine("  [AiEnhanced] Parsed: " + dsl->Variables->Count + " variables, " + dsl->Networks->Count + " networks");
        return dsl;
    }
};

ref class P4LogicGraphGenerator {
public:
    static Action<String^>^ LogCallback;

    static void Log(String^ msg) {
        Console::WriteLine(msg);
        if (LogCallback != nullptr) try { LogCallback("[INFO] " + msg); } catch (...) {}
    }

    static P3Dsl^ Generate(P4Requirement^ requirement, P4SemanticPlan^ semanticPlan, P4VariablePlan^ varPlan) {
        Console::WriteLine("[LogicGraphGenerator] Converting state machine to LogicGraph...");

        // #region debug-point A:branch-decision
        {
            String^ dbgInfo = String::Format(
                "HasStarDelta={0}, HasAutoRoundTrip={1}, IsSequential={2}, States={3}, HasInterlock={4}, HasSelfHold={5}, Outputs={6}",
                semanticPlan->HasStarDelta,
                semanticPlan->HasAutoRoundTrip,
                semanticPlan->IsSequential,
                semanticPlan->States != nullptr ? semanticPlan->States->Count.ToString() : "null",
                semanticPlan->HasInterlock,
                semanticPlan->HasSelfHold,
                requirement != nullptr && requirement->Outputs != nullptr ? requirement->Outputs->Count.ToString() : "null");
            Console::WriteLine("[DEBUG-A] " + dbgInfo);
            try {
                auto wc = gcnew System::Net::WebClient();
                wc->Headers->Add("Content-Type", "application/json");
                String^ json = "{\"sessionId\":\"plc-two-networks\",\"runId\":\"pre\",\"hypothesisId\":\"A\","
                    + "\"location\":\"LogicGraphGenerator\",\"msg\":\"[DEBUG] " + dbgInfo->Replace("\"", "\\\"") + "\"}";
                wc->UploadStringAsync(gcnew Uri("http://127.0.0.1:7777/event"), "POST", json);
            } catch (...) {}
        }
        // #endregion

        P3Dsl^ dsl = gcnew P3Dsl();
        dsl->DslVersion = "1.0";
        dsl->Variables = varPlan->Variables;
        dsl->Networks = gcnew List<P3Network^>();
        dsl->Steps = gcnew List<P3Step^>();
        dsl->Timers = gcnew List<P3TimerDecl^>();
        dsl->Counters = gcnew List<P3CounterDecl^>();

        GenerateStepDeclarations(dsl, semanticPlan);
        GenerateTimerDeclarations(dsl, requirement);
        GenerateCounterDeclarations(dsl, requirement);

        bool handledBySpecialized = false;
        bool hasRichStateMachine = semanticPlan->IsSequential && semanticPlan->States != nullptr && semanticPlan->States->Count > 2;

        Console::WriteLine("[LOGICGRAPH] Branch decision: StarDelta=" + semanticPlan->HasStarDelta + " AutoRoundTrip=" + semanticPlan->HasAutoRoundTrip + " IsSequential=" + semanticPlan->IsSequential + " States=" + (semanticPlan->States != nullptr ? semanticPlan->States->Count.ToString() : "null") + " RichStateMachine=" + hasRichStateMachine);
        Log("[LOGICGRAPH] Branch decision: StarDelta=" + semanticPlan->HasStarDelta + " AutoRoundTrip=" + semanticPlan->HasAutoRoundTrip + " IsSequential=" + semanticPlan->IsSequential + " States=" + (semanticPlan->States != nullptr ? semanticPlan->States->Count.ToString() : "null") + " RichStateMachine=" + hasRichStateMachine);

        if (hasRichStateMachine) {
            Console::WriteLine("[DEBUG-A] BRANCH: Sequential (priority over StarDelta/AutoRoundTrip)");
            Log("[LOGICGRAPH] BRANCH: Sequential (priority)");
            GenerateModeSelectionNetworks(dsl, requirement, varPlan);
            GenerateSequentialNetworks(dsl, semanticPlan, varPlan, requirement);
            GenerateManualModeNetworks(dsl, requirement, varPlan);
            handledBySpecialized = true;
        }

        if (!handledBySpecialized && semanticPlan->HasStarDelta) {
            Console::WriteLine("[DEBUG-A] BRANCH: StarDelta");
            Log("[LOGICGRAPH] BRANCH: StarDelta");
            GenerateStarDeltaNetworks(dsl, requirement, semanticPlan, varPlan);
            handledBySpecialized = true;
        }

        if (!handledBySpecialized && semanticPlan->HasAutoRoundTrip) {
            Console::WriteLine("[DEBUG-A] BRANCH: AutoRoundTrip");
            Log("[LOGICGRAPH] BRANCH: AutoRoundTrip");
            GenerateAutoRoundTripNetworks(dsl, requirement, semanticPlan, varPlan);
            handledBySpecialized = true;
        }

        if (!handledBySpecialized && semanticPlan->States != nullptr && semanticPlan->States->Count > 0) {
            if (semanticPlan->IsSequential) {
                Console::WriteLine("[DEBUG-A] BRANCH: Sequential (fallback)");
                Log("[LOGICGRAPH] BRANCH: Sequential (fallback)");
                GenerateModeSelectionNetworks(dsl, requirement, varPlan);
                GenerateSequentialNetworks(dsl, semanticPlan, varPlan, requirement);
                GenerateManualModeNetworks(dsl, requirement, varPlan);
            }
            else {
                Console::WriteLine("[DEBUG-A] BRANCH: Combinational");
                Log("[LOGICGRAPH] BRANCH: Combinational");
                GenerateCombinationalNetworks(dsl, semanticPlan, varPlan, requirement);
            }
        }
        else if (!handledBySpecialized && requirement != nullptr && requirement->Outputs != nullptr && requirement->Outputs->Count > 0) {
            if (semanticPlan->HasInterlock && requirement->Outputs->Count >= 2) {
                GenerateInterlockNetworks(dsl, requirement, semanticPlan, varPlan);
                handledBySpecialized = true;
            }
            else if (semanticPlan->HasSelfHold || requirement->Outputs->Count == 1) {
                for each (P4SignalDecl^ sig in requirement->Outputs) {
                    String^ varName = MapSignalNameToVarName(sig->Name, varPlan);
                    GenerateSingleMotorNetwork(dsl, sig->Name, varName, requirement, varPlan);
                }
                handledBySpecialized = true;
            }
            else {
                GenerateDirectOutputNetworks(dsl, requirement, varPlan);
            }
        }

        if (semanticPlan->HasInterlock && !handledBySpecialized) {
            GenerateInterlockNetworks(dsl, requirement, semanticPlan, varPlan);
        }
        else if (semanticPlan->HasInterlock && handledBySpecialized && dsl->Networks->Count > 0) {
            AddInterlockToExistingNetworks(dsl, requirement, semanticPlan, varPlan);
        }

        if (semanticPlan->HasLimitSwitch && !semanticPlan->HasAutoRoundTrip) {
            AddLimitSwitchProtection(dsl, requirement, semanticPlan, varPlan);
        }

        if (semanticPlan->IsCyclic && semanticPlan->CycleCount > 0) {
            GenerateCycleNetworks(dsl, semanticPlan, requirement);
        }

        EnsureSelfHoldingOnAllMotorOutputs(dsl, requirement, semanticPlan, varPlan);

        // #region debug-point A:network-count
        Console::WriteLine("[DEBUG-A] Networks generated: " + dsl->Networks->Count + ", handledBySpecialized=" + handledBySpecialized);
        // #endregion

        bool needsAiFallback = false;
        if (dsl->Networks->Count == 0) {
            needsAiFallback = true;
            Console::WriteLine("  [Fallback] No networks generated by specialized generators, will use AI-enhanced generation");
        }
        else if (requirement != nullptr && requirement->Outputs != nullptr && requirement->Outputs->Count > 0) {
            int outputCoilsFound = 0;
            for each (P3Network^ net in dsl->Networks) {
                for each (Object^ item in net->Items) {
                    P3Node^ n = dynamic_cast<P3Node^>(item);
                    if (n != nullptr && n->NodeType->ToLower() == "coil") outputCoilsFound++;
                }
            }
            if (outputCoilsFound < requirement->Outputs->Count) {
                needsAiFallback = true;
                Console::WriteLine("  [Fallback] Only " + outputCoilsFound + "/" + requirement->Outputs->Count + " outputs have coils, will use AI-enhanced generation");
            }
        }

        if (needsAiFallback) {
            dsl->AiFallbackNeeded = true;
        }

        Console::WriteLine("  LogicGraph: " + dsl->Networks->Count + " networks, " + dsl->Steps->Count + " steps" +
            (needsAiFallback ? " (AI fallback needed)" : ""));
        return dsl;
    }

private:
    static String^ MapSignalNameToVarName(String^ signalName, P4VariablePlan^ varPlan) {
        if (signalName == nullptr || signalName->Length == 0) return signalName;
        for each (P3Variable^ v in varPlan->Variables) {
            if (v->Name == signalName) return v->Name;
            if (v->Comment == signalName) return v->Name;
        }
        return signalName;
    }

    static List<String^>^ MapSignalNamesToVarNames(List<String^>^ signalNames, P4VariablePlan^ varPlan) {
        List<String^>^ result = gcnew List<String^>();
        for each (String^ name in signalNames) {
            result->Add(MapSignalNameToVarName(name, varPlan));
        }
        return result;
    }

    static void GenerateCombinationalNetworks(P3Dsl^ dsl, P4SemanticPlan^ plan, P4VariablePlan^ varPlan, P4Requirement^ requirement) {
        Console::WriteLine("  [Combinational] Generating networks from " + plan->States->Count + " states...");

        for (int i = 0; i < plan->States->Count; i++) {
            P4State^ state = plan->States[i];

            List<String^>^ outputTags = DetermineOutputActions(state, plan, requirement);
            outputTags = MapSignalNamesToVarNames(outputTags, varPlan);

            if (outputTags->Count == 0) {
                outputTags = InferOutputsFromStateName(state->Name, requirement);
                outputTags = MapSignalNamesToVarNames(outputTags, varPlan);
            }

            if (outputTags->Count == 0) continue;

            P3Network^ net = gcnew P3Network();
            net->Title = state->Name;
            net->Items = gcnew List<Object^>();

            List<String^>^ condInputs = gcnew List<String^>();
            if (state->TransitionCondition != nullptr && state->TransitionCondition->Length > 0) {
                condInputs = ParseTransitionCondition(state->TransitionCondition, varPlan, state, requirement);
            }

            if (condInputs->Count == 0) {
                String^ triggerName = FindInputName(varPlan, state->Name);
                if (triggerName != nullptr && triggerName->Length > 0) {
                    condInputs->Add(triggerName);
                }
            }

            for each (String^ condTag in condInputs) {
                P3Node^ contact = gcnew P3Node();
                contact->NodeType = "contact";
                contact->Tag = condTag;
                contact->NormallyOpen = true;
                net->Items->Add(contact);
            }

            for each (String^ outputTag in outputTags) {
                P3Node^ coil = gcnew P3Node();
                coil->NodeType = "coil";
                coil->Tag = outputTag;
                net->Items->Add(coil);
            }

            if (net->Items->Count > 0) {
                dsl->Networks->Add(net);
                Console::WriteLine("    Network: " + net->Title + " (" + net->Items->Count + " items)");
            }
        }

        if (dsl->Networks->Count == 0 && requirement != nullptr && requirement->Outputs != nullptr) {
            GenerateDirectOutputNetworks(dsl, requirement, varPlan);
        }
    }

    static void GenerateDirectOutputNetworks(P3Dsl^ dsl, P4Requirement^ requirement, P4VariablePlan^ varPlan) {
        Console::WriteLine("  [DirectOutput] Generating networks from " + requirement->Outputs->Count + " outputs...");

        for each (P4SignalDecl^ sig in requirement->Outputs) {
            String^ outputVarName = MapSignalNameToVarName(sig->Name, varPlan);
            P3Network^ net = gcnew P3Network();
            net->Title = sig->Name;
            net->Items = gcnew List<Object^>();

            List<String^>^ relatedInputs = FindRelatedInputs(sig->Name, requirement, varPlan);
            for each (String^ inputTag in relatedInputs) {
                P3Node^ contact = gcnew P3Node();
                contact->NodeType = "contact";
                contact->Tag = MapSignalNameToVarName(inputTag, varPlan);
                contact->NormallyOpen = true;
                net->Items->Add(contact);
            }

            P3Node^ coil = gcnew P3Node();
            coil->NodeType = "coil";
            coil->Tag = outputVarName;
            net->Items->Add(coil);

            dsl->Networks->Add(net);
            Console::WriteLine("    Network: " + net->Title + " (" + net->Items->Count + " items)");
        }
    }

    static List<String^>^ InferOutputsFromStateName(String^ stateName, P4Requirement^ requirement) {
        List<String^>^ outputs = gcnew List<String^>();
        if (stateName == nullptr || requirement == nullptr || requirement->Outputs == nullptr) return outputs;

        String^ nl = stateName->ToLower();

        for each (P4SignalDecl^ sig in requirement->Outputs) {
            String^ ol = sig->Name->ToLower();
            String^ ocl = (sig->Comment != nullptr) ? sig->Comment->ToLower() : "";

            if (nl->Contains("motor1") && (ol->Contains("motor1") || ol->Contains("fwd") || ol->Contains("forward") || ocl->Contains(L"正转") || ocl->Contains(L"电机1"))) {
                outputs->Add(sig->Name);
            }
            else if (nl->Contains("motor2") && (ol->Contains("motor2") || ol->Contains("rev") || ol->Contains("reverse") || ocl->Contains(L"反转") || ocl->Contains(L"电机2"))) {
                outputs->Add(sig->Name);
            }
            else if (nl->Contains("run") && ol->Contains("run")) {
                outputs->Add(sig->Name);
            }
            else if (nl->Contains("fill") && (ol->Contains("fill") || ol->Contains("valve") || ocl->Contains(L"装料") || ocl->Contains(L"阀门"))) {
                outputs->Add(sig->Name);
            }
            else if (nl->Contains("drain") && (ol->Contains("drain") || ol->Contains("unload") || ocl->Contains(L"卸料"))) {
                outputs->Add(sig->Name);
            }
            else if (nl->Contains("heat") && (ol->Contains("heat") || ocl->Contains(L"加热"))) {
                outputs->Add(sig->Name);
            }
            else if (nl->Contains("clean") && (ol->Contains("clean") || ol->Contains("wash") || ocl->Contains(L"清洗"))) {
                outputs->Add(sig->Name);
            }
        }

        if (outputs->Count == 0 && requirement->Outputs->Count > 0) {
            int idx = 0;
            for each (P4SignalDecl^ sig in requirement->Outputs) {
                if (nl->Contains((idx + 1).ToString())) {
                    outputs->Add(sig->Name);
                    break;
                }
                idx++;
            }
        }

        return outputs;
    }

    static List<String^>^ FindRelatedInputs(String^ outputName, P4Requirement^ requirement, P4VariablePlan^ varPlan) {
        List<String^>^ inputs = gcnew List<String^>();
        if (outputName == nullptr || requirement == nullptr || requirement->Inputs == nullptr) return inputs;

        String^ ol = outputName->ToLower();

        for each (P4SignalDecl^ sig in requirement->Inputs) {
            String^ il = sig->Name->ToLower();
            String^ icl = (sig->Comment != nullptr) ? sig->Comment->ToLower() : "";

            if (ol->Contains("fwd") && (il->Contains("start1") || il->Contains("jog_fwd") || icl->Contains(L"正转启动") || icl->Contains(L"点动正转"))) {
                inputs->Add(sig->Name);
            }
            else if (ol->Contains("rev") && (il->Contains("start2") || il->Contains("jog_rev") || icl->Contains(L"反转启动") || icl->Contains(L"点动反转"))) {
                inputs->Add(sig->Name);
            }
            else if (ol->Contains("motor1") && (il->Contains("start1") || il->Contains("start_1") || icl->Contains(L"启动1"))) {
                inputs->Add(sig->Name);
            }
            else if (ol->Contains("motor2") && (il->Contains("start2") || il->Contains("start_2") || icl->Contains(L"启动2"))) {
                inputs->Add(sig->Name);
            }
            else if (il->Contains("start") && ol->Contains("motor")) {
                inputs->Add(sig->Name);
            }
        }

        if (inputs->Count == 0 && requirement->Inputs->Count > 0) {
            String^ startInput = FindInputName(varPlan, "start");
            if (startInput != nullptr && startInput->Length > 0) {
                inputs->Add(startInput);
            }
        }

        return inputs;
    }

    static void GenerateModeSelectionNetworks(P3Dsl^ dsl, P4Requirement^ requirement, P4VariablePlan^ varPlan) {
        String^ modeSwitch = nullptr;
        for each (P3Variable^ v in varPlan->Variables) {
            if (v->Name == nullptr) continue;
            String^ nl = v->Name->ToLower();
            String^ cl = (v->Comment != nullptr) ? v->Comment->ToLower() : "";
            if (cl->Contains("sa") || cl->Contains("mode") || cl->Contains("auto_switch") ||
                nl->Contains(L"手动") || nl->Contains(L"自动") || nl->Contains(L"选择开关") || nl->Contains("mode")) {
                modeSwitch = v->Name;
                break;
            }
        }
        if (modeSwitch == nullptr) return;

        P3Network^ autoNet = gcnew P3Network();
        autoNet->Title = L"自动模式使能";
        autoNet->Items = gcnew List<Object^>();

        P3Node^ saNo = gcnew P3Node();
        saNo->NodeType = "contact";
        saNo->Tag = modeSwitch;
        saNo->NormallyOpen = true;
        autoNet->Items->Add(saNo);

        P3Node^ autoCoil = gcnew P3Node();
        autoCoil->NodeType = "coil";
        autoCoil->Tag = "Auto_Mode";
        autoCoil->NormallyOpen = true;
        autoNet->Items->Add(autoCoil);

        dsl->Networks->Add(autoNet);

        P3Network^ manualNet = gcnew P3Network();
        manualNet->Title = L"手动模式使能";
        manualNet->Items = gcnew List<Object^>();

        P3Node^ saNc = gcnew P3Node();
        saNc->NodeType = "contact";
        saNc->Tag = modeSwitch;
        saNc->NormallyOpen = false;
        manualNet->Items->Add(saNc);

        P3Node^ manualCoil = gcnew P3Node();
        manualCoil->NodeType = "coil";
        manualCoil->Tag = "Manual_Mode";
        manualCoil->NormallyOpen = true;
        manualNet->Items->Add(manualCoil);

        dsl->Networks->Add(manualNet);
    }

    static void GenerateManualModeNetworks(P3Dsl^ dsl, P4Requirement^ requirement, P4VariablePlan^ varPlan) {
        String^ jogFwd = nullptr;
        String^ jogRev = nullptr;
        String^ limitRight = nullptr;
        String^ limitLeft = nullptr;
        String^ motorFwd = nullptr;
        String^ motorRev = nullptr;

        for each (P3Variable^ v in varPlan->Variables) {
            if (v->Name == nullptr) continue;
            String^ nl = v->Name->ToLower();
            String^ cl = (v->Comment != nullptr) ? v->Comment->ToLower() : "";

            if (v->Scope == "input") {
                if (cl->Contains("jog_fwd") || cl->Contains("jog_forward") || cl->Contains("jog_right") ||
                    nl->Contains(L"点动正转") || nl->Contains(L"正转按钮") || nl->Contains(L"点动前进"))
                    jogFwd = v->Name;
                if (cl->Contains("jog_rev") || cl->Contains("jog_reverse") || cl->Contains("jog_left") || cl->Contains("jog_backward") ||
                    nl->Contains(L"点动反转") || nl->Contains(L"反转按钮") || nl->Contains(L"点动后退"))
                    jogRev = v->Name;
                if (cl->Contains("limit_r") || cl->Contains("right_limit") || cl->Contains("limit_right") ||
                    nl->Contains(L"右限位") || nl->Contains(L"右限"))
                    limitRight = v->Name;
                if (cl->Contains("limit_l") || cl->Contains("left_limit") || cl->Contains("limit_left") ||
                    nl->Contains(L"左限位") || nl->Contains(L"左限"))
                    limitLeft = v->Name;
            }
            if (v->Scope == "output") {
                if (cl->Contains("motor_fwd") || cl->Contains("motor_forward") || cl->Contains("move_right") || cl->Contains("move_fwd") ||
                    nl->Contains(L"正转") || nl->Contains(L"前进"))
                    motorFwd = v->Name;
                if (cl->Contains("motor_rev") || cl->Contains("motor_reverse") || cl->Contains("move_left") || cl->Contains("move_rev") ||
                    nl->Contains(L"反转") || nl->Contains(L"后退"))
                    motorRev = v->Name;
            }
        }

        if (jogFwd != nullptr && motorFwd != nullptr) {
            P3Network^ fwdNet = gcnew P3Network();
            fwdNet->Title = L"手动正转（前进）";
            fwdNet->Items = gcnew List<Object^>();

            P3Node^ manualContact = gcnew P3Node();
            manualContact->NodeType = "contact";
            manualContact->Tag = "Manual_Mode";
            manualContact->NormallyOpen = true;
            fwdNet->Items->Add(manualContact);

            P3Node^ jogFwdContact = gcnew P3Node();
            jogFwdContact->NodeType = "contact";
            jogFwdContact->Tag = jogFwd;
            jogFwdContact->NormallyOpen = true;
            fwdNet->Items->Add(jogFwdContact);

            if (limitRight != nullptr) {
                P3Node^ limitNc = gcnew P3Node();
                limitNc->NodeType = "contact";
                limitNc->Tag = limitRight;
                limitNc->NormallyOpen = false;
                fwdNet->Items->Add(limitNc);
            }

            if (motorRev != nullptr) {
                P3Node^ interlockNc = gcnew P3Node();
                interlockNc->NodeType = "contact";
                interlockNc->Tag = motorRev;
                interlockNc->NormallyOpen = false;
                fwdNet->Items->Add(interlockNc);
            }

            P3Node^ fwdCoil = gcnew P3Node();
            fwdCoil->NodeType = "coil";
            fwdCoil->Tag = motorFwd;
            fwdCoil->NormallyOpen = true;
            fwdNet->Items->Add(fwdCoil);

            dsl->Networks->Add(fwdNet);
        }

        if (jogRev != nullptr && motorRev != nullptr) {
            P3Network^ revNet = gcnew P3Network();
            revNet->Title = L"手动反转（后退）";
            revNet->Items = gcnew List<Object^>();

            P3Node^ manualContact = gcnew P3Node();
            manualContact->NodeType = "contact";
            manualContact->Tag = "Manual_Mode";
            manualContact->NormallyOpen = true;
            revNet->Items->Add(manualContact);

            P3Node^ jogRevContact = gcnew P3Node();
            jogRevContact->NodeType = "contact";
            jogRevContact->Tag = jogRev;
            jogRevContact->NormallyOpen = true;
            revNet->Items->Add(jogRevContact);

            if (limitLeft != nullptr) {
                P3Node^ limitNc = gcnew P3Node();
                limitNc->NodeType = "contact";
                limitNc->Tag = limitLeft;
                limitNc->NormallyOpen = false;
                revNet->Items->Add(limitNc);
            }

            if (motorFwd != nullptr) {
                P3Node^ interlockNc = gcnew P3Node();
                interlockNc->NodeType = "contact";
                interlockNc->Tag = motorFwd;
                interlockNc->NormallyOpen = false;
                revNet->Items->Add(interlockNc);
            }

            P3Node^ revCoil = gcnew P3Node();
            revCoil->NodeType = "coil";
            revCoil->Tag = motorRev;
            revCoil->NormallyOpen = true;
            revNet->Items->Add(revCoil);

            dsl->Networks->Add(revNet);
        }
    }

    static void GenerateStepDeclarations(P3Dsl^ dsl, P4SemanticPlan^ plan) {
        for each (P4State^ state in plan->States) {
            P3Step^ step = gcnew P3Step();
            int idx = plan->States->IndexOf(state) + 1;
            step->Name = "Step" + idx;
            step->Action = state->Action;
            step->Transition = state->TransitionCondition;
            step->NextStep = state->NextState;
            dsl->Steps->Add(step);
        }
    }

    static void GenerateTimerDeclarations(P3Dsl^ dsl, P4Requirement^ requirement) {
        for each (P4TimerRequirement^ t in requirement->Timers) {
            P3TimerDecl^ td = gcnew P3TimerDecl();
            td->Name = t->Name;
            td->TimerType = "TON";
            td->Preset = t->Preset;
            td->Comment = "Timer " + t->Name;
            dsl->Timers->Add(td);
        }
    }

    static void GenerateCounterDeclarations(P3Dsl^ dsl, P4Requirement^ requirement) {
        for each (P4CounterRequirement^ c in requirement->Counters) {
            P3CounterDecl^ cd = gcnew P3CounterDecl();
            cd->Name = c->Name;
            cd->CounterType = "CTU";
            cd->Preset = c->Preset;
            cd->Comment = "Counter " + c->Name;
            dsl->Counters->Add(cd);
        }
    }

    static void GenerateSequentialNetworks(P3Dsl^ dsl, P4SemanticPlan^ plan, P4VariablePlan^ varPlan, P4Requirement^ requirement) {
        String^ seqLog = "[SEQGEN] GenerateSequentialNetworks: States=" + plan->States->Count + ", existing networks=" + dsl->Networks->Count;
        Log(seqLog);

        bool hasAutoMode = false;
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ n = dynamic_cast<P3Node^>(item);
                if (n != nullptr && n->Tag == "Auto_Mode") { hasAutoMode = true; break; }
            }
            if (hasAutoMode) break;
        }

        P3Network^ initNet = gcnew P3Network();
        initNet->Title = L"自动-启动初始化";
        initNet->Items = gcnew List<Object^>();

        if (hasAutoMode) {
            P3Node^ autoContact = gcnew P3Node();
            autoContact->NodeType = "contact";
            autoContact->Tag = "Auto_Mode";
            autoContact->NormallyOpen = true;
            initNet->Items->Add(autoContact);
        }

        P3Node^ startContact = gcnew P3Node();
        startContact->NodeType = "contact";
        startContact->Tag = FindInputName(varPlan, "start");
        startContact->NormallyOpen = true;
        initNet->Items->Add(startContact);

        if (plan->IsCyclic) {
            P3Node^ doneNc = gcnew P3Node();
            doneNc->NodeType = "contact";
            doneNc->Tag = "Cycle_Done";
            doneNc->NormallyOpen = false;
            initNet->Items->Add(doneNc);
        }

        P3Node^ setStep1 = gcnew P3Node();
        setStep1->NodeType = "set";
        setStep1->Tag = "Step1";
        initNet->Items->Add(setStep1);

        dsl->Networks->Add(initNet);

        for (int i = 0; i < plan->States->Count; i++) {
            P4State^ state = plan->States[i];
            String^ stepName = "Step" + (i + 1);

            String^ stateLog = "[SEQGEN]   State[" + i + "]: " + state->Name + " Action=" + state->Action + " Trans=" + (state->TransitionCondition != nullptr ? state->TransitionCondition : "null") + " Next=" + (state->NextState != nullptr ? state->NextState : "null");
            Log(stateLog);

            P3Network^ actionNet = gcnew P3Network();
            actionNet->Title = stepName + " - " + state->Name;
            actionNet->Items = gcnew List<Object^>();

            P3Node^ stepContact = gcnew P3Node();
            stepContact->NodeType = "contact";
            stepContact->Tag = stepName;
            stepContact->NormallyOpen = true;
            actionNet->Items->Add(stepContact);

            List<String^>^ outputActions = DetermineOutputActions(state, plan, requirement);
            outputActions = MapSignalNamesToVarNames(outputActions, varPlan);
            Log("[SEQGEN]     outputActions=" + outputActions->Count + (outputActions->Count > 0 ? " [" + String::Join(",", outputActions) + "]" : ""));
            for each (String^ outputTag in outputActions) {
                P3Node^ coil = gcnew P3Node();
                coil->NodeType = "coil";
                coil->Tag = outputTag;
                actionNet->Items->Add(coil);
            }

            if (state->TransitionCondition != nullptr && state->TransitionCondition->Length > 0) {
                List<String^>^ timerRefs = FindTimerReferencesForStep(state, requirement, varPlan);
                for each (String^ timerRef in timerRefs) {
                    P3Node^ tonNode = gcnew P3Node();
                    tonNode->NodeType = "ton";
                    tonNode->Instance = timerRef;
                    tonNode->Pt = FindTimerPreset(varPlan, timerRef);
                    actionNet->Items->Add(tonNode);
                }
            }

            if (actionNet->Items->Count > 1) {
                dsl->Networks->Add(actionNet);
                Log("[SEQGEN]     Added actionNet: " + actionNet->Title + " items=" + actionNet->Items->Count);
            }
            else if (state->TransitionCondition != nullptr && state->TransitionCondition->Length > 0) {
                P3Node^ placeholderCoil = gcnew P3Node();
                placeholderCoil->NodeType = "coil";
                placeholderCoil->Tag = stepName + L"_Active";
                placeholderCoil->NormallyOpen = true;
                actionNet->Items->Add(placeholderCoil);
                dsl->Networks->Add(actionNet);
                Log("[SEQGEN]     Added actionNet (placeholder): " + actionNet->Title + " items=" + actionNet->Items->Count);
            }
            else {
                Log("[SEQGEN]     SKIPPED actionNet: " + actionNet->Title + " items=" + actionNet->Items->Count);
            }

            if (state->TransitionCondition != nullptr && state->TransitionCondition->Length > 0) {
                array<String^>^ branches = state->TransitionCondition->Split(gcnew array<String^>{"||"}, StringSplitOptions::RemoveEmptyEntries);
                array<String^>^ nextStates = (state->NextState != nullptr && state->NextState->Length > 0)
                    ? state->NextState->Split(gcnew array<Char>{'|'}) : gcnew array<String^>{""};

                for (int b = 0; b < branches->Length; b++) {
                    String^ branchCond = branches[b]->Trim();
                    String^ branchNext = (b < nextStates->Length) ? nextStates[b]->Trim() : "";

                    P3Network^ transNet = gcnew P3Network();
                    transNet->Title = branches->Length > 1
                        ? stepName + " Trans" + (b + 1) + " - " + branchCond
                        : stepName + " Transition";
                    transNet->Items = gcnew List<Object^>();

                    P3Node^ tStepContact = gcnew P3Node();
                    tStepContact->NodeType = "contact";
                    tStepContact->Tag = stepName;
                    tStepContact->NormallyOpen = true;
                    transNet->Items->Add(tStepContact);

                    List<String^>^ condContacts = ParseTransitionCondition(branchCond, varPlan, state, requirement);
                    for each (String^ condTag in condContacts) {
                        P3Node^ condContact = gcnew P3Node();
                        condContact->NodeType = "contact";
                        condContact->Tag = condTag;
                        condContact->NormallyOpen = !IsNegatedCondition(branchCond, condTag);
                        transNet->Items->Add(condContact);
                    }

                    String^ nextStepName = ResolveNextStepName(branchNext, plan);
                    if (nextStepName == nullptr || nextStepName->Length == 0) {
                        nextStepName = DetermineNextStep(state, plan);
                    }
                    if (nextStepName != nullptr && nextStepName->Length > 0) {
                        P3Node^ setNext = gcnew P3Node();
                        setNext->NodeType = "set";
                        setNext->Tag = nextStepName;
                        transNet->Items->Add(setNext);

                        P3Node^ resetCur = gcnew P3Node();
                        resetCur->NodeType = "reset";
                        resetCur->Tag = stepName;
                        transNet->Items->Add(resetCur);
                    }

                    dsl->Networks->Add(transNet);
                }
            }
        }

        Log("[SEQGEN] GenerateSequentialNetworks done: total networks=" + dsl->Networks->Count);
    }

    static void GenerateInterlockNetworks(P3Dsl^ dsl, P4Requirement^ requirement, P4SemanticPlan^ plan, P4VariablePlan^ varPlan) {
        bool hasExistingNetworks = dsl->Networks->Count > 0;

        if (hasExistingNetworks) {
            AddInterlockToExistingNetworks(dsl, requirement, plan, varPlan);
        }
        else {
            if (plan->InterlockPairs->Count > 0) {
                for each (String^ pair in plan->InterlockPairs) {
                    array<String^>^ parts = pair->Split(gcnew array<Char>{',', ';'});
                    if (parts->Length >= 2) {
                        String^ sigA = parts[0]->Trim();
                        String^ sigB = parts[1]->Trim();
                        String^ varA = MapSignalNameToVarName(sigA, varPlan);
                        String^ varB = MapSignalNameToVarName(sigB, varPlan);
                        if (varA->Length > 0 && varB->Length > 0) {
                            GenerateInterlockControlNetwork(dsl, sigA, sigB, varA, varB, requirement, varPlan);
                        }
                    }
                }
            }
            else {
                for (int i = 0; i < requirement->Outputs->Count; i++) {
                    for (int j = i + 1; j < requirement->Outputs->Count; j++) {
                        String^ sigA = requirement->Outputs[i]->Name;
                        String^ sigB = requirement->Outputs[j]->Name;
                        String^ varA = MapSignalNameToVarName(sigA, varPlan);
                        String^ varB = MapSignalNameToVarName(sigB, varPlan);
                        GenerateInterlockControlNetwork(dsl, sigA, sigB, varA, varB, requirement, varPlan);
                    }
                }
            }

            for each (P4SignalDecl^ sig in requirement->Outputs) {
                String^ varName = MapSignalNameToVarName(sig->Name, varPlan);
                bool alreadyHandled = false;
                for each (P3Network^ net in dsl->Networks) {
                    for each (Object^ item in net->Items) {
                        P3Node^ n = dynamic_cast<P3Node^>(item);
                        if (n != nullptr && n->Tag == varName && (n->NodeType->ToLower() == "coil" || n->NodeType->ToLower() == "set")) {
                            alreadyHandled = true;
                            break;
                        }
                    }
                    if (alreadyHandled) break;
                }
                if (!alreadyHandled) {
                    GenerateSingleMotorNetwork(dsl, sig->Name, varName, requirement, varPlan);
                }
            }
        }
    }

    static void AddInterlockToExistingNetworks(P3Dsl^ dsl, P4Requirement^ requirement, P4SemanticPlan^ plan, P4VariablePlan^ varPlan) {
        List<String^>^ interlockPairList = gcnew List<String^>();
        if (plan->InterlockPairs->Count > 0) {
            interlockPairList = plan->InterlockPairs;
        }
        else {
            for (int i = 0; i < requirement->Outputs->Count; i++) {
                for (int j = i + 1; j < requirement->Outputs->Count; j++) {
                    interlockPairList->Add(requirement->Outputs[i]->Name + "," + requirement->Outputs[j]->Name);
                }
            }
        }

        for each (String^ pair in interlockPairList) {
            array<String^>^ parts = pair->Split(gcnew array<Char>{',', ';'});
            if (parts->Length < 2) continue;
            String^ sigA = parts[0]->Trim();
            String^ sigB = parts[1]->Trim();
            String^ varA = MapSignalNameToVarName(sigA, varPlan);
            String^ varB = MapSignalNameToVarName(sigB, varPlan);

            for each (P3Network^ net in dsl->Networks) {
                for (int idx = net->Items->Count - 1; idx >= 0; idx--) {
                    P3Node^ n = dynamic_cast<P3Node^>(net->Items[idx]);
                    if (n != nullptr && n->NodeType->ToLower() == "coil") {
                        if (n->Tag == varA) {
                            P3Node^ interlockNC = gcnew P3Node();
                            interlockNC->NodeType = "contact";
                            interlockNC->Tag = varB;
                            interlockNC->NormallyOpen = false;
                            net->Items->Insert(idx, interlockNC);
                            Console::WriteLine("    Added interlock " + varB + "(NC) before coil " + varA + " in network " + net->Title);
                        }
                        else if (n->Tag == varB) {
                            P3Node^ interlockNC = gcnew P3Node();
                            interlockNC->NodeType = "contact";
                            interlockNC->Tag = varA;
                            interlockNC->NormallyOpen = false;
                            net->Items->Insert(idx, interlockNC);
                            Console::WriteLine("    Added interlock " + varA + "(NC) before coil " + varB + " in network " + net->Title);
                        }
                    }
                }
            }
        }
    }

    static void GenerateInterlockControlNetwork(P3Dsl^ dsl, String^ sigA, String^ sigB, String^ varA, String^ varB, P4Requirement^ requirement, P4VariablePlan^ varPlan) {
        String^ startA = FindStartButton(sigA, requirement, varPlan);
        String^ stopA = FindStopButton(sigA, requirement, varPlan);
        String^ startB = FindStartButton(sigB, requirement, varPlan);
        String^ stopB = FindStopButton(sigB, requirement, varPlan);

        P3Network^ netA = gcnew P3Network();
        netA->Title = sigA + L" 控制（互锁）";
        netA->Items = gcnew List<Object^>();

        if (stopA != nullptr) {
            P3Node^ stopContact = gcnew P3Node();
            stopContact->NodeType = "contact";
            stopContact->Tag = stopA;
            stopContact->NormallyOpen = false;
            netA->Items->Add(stopContact);
        }

        P3Parallel^ startOrHoldA = gcnew P3Parallel();
        startOrHoldA->Branches = gcnew List<P3Branch^>();

        P3Branch^ startBranchA = gcnew P3Branch();
        startBranchA->Nodes = gcnew List<P3Node^>();
        if (startA != nullptr) {
            P3Node^ startContact = gcnew P3Node();
            startContact->NodeType = "contact";
            startContact->Tag = startA;
            startContact->NormallyOpen = true;
            startBranchA->Nodes->Add(startContact);
        }
        startOrHoldA->Branches->Add(startBranchA);

        P3Branch^ holdBranchA = gcnew P3Branch();
        holdBranchA->Nodes = gcnew List<P3Node^>();
        P3Node^ selfHoldA = gcnew P3Node();
        selfHoldA->NodeType = "contact";
        selfHoldA->Tag = varA;
        selfHoldA->NormallyOpen = true;
        holdBranchA->Nodes->Add(selfHoldA);
        startOrHoldA->Branches->Add(holdBranchA);

        netA->Items->Add(startOrHoldA);

        P3Node^ interlockB = gcnew P3Node();
        interlockB->NodeType = "contact";
        interlockB->Tag = varB;
        interlockB->NormallyOpen = false;
        netA->Items->Add(interlockB);

        P3Node^ coilA = gcnew P3Node();
        coilA->NodeType = "coil";
        coilA->Tag = varA;
        netA->Items->Add(coilA);

        dsl->Networks->Add(netA);

        P3Network^ netB = gcnew P3Network();
        netB->Title = sigB + L" 控制（互锁）";
        netB->Items = gcnew List<Object^>();

        if (stopB != nullptr) {
            P3Node^ stopContact = gcnew P3Node();
            stopContact->NodeType = "contact";
            stopContact->Tag = stopB;
            stopContact->NormallyOpen = false;
            netB->Items->Add(stopContact);
        }

        P3Parallel^ startOrHoldB = gcnew P3Parallel();
        startOrHoldB->Branches = gcnew List<P3Branch^>();

        P3Branch^ startBranchB = gcnew P3Branch();
        startBranchB->Nodes = gcnew List<P3Node^>();
        if (startB != nullptr) {
            P3Node^ startContactB = gcnew P3Node();
            startContactB->NodeType = "contact";
            startContactB->Tag = startB;
            startContactB->NormallyOpen = true;
            startBranchB->Nodes->Add(startContactB);
        }
        startOrHoldB->Branches->Add(startBranchB);

        P3Branch^ holdBranchB = gcnew P3Branch();
        holdBranchB->Nodes = gcnew List<P3Node^>();
        P3Node^ selfHoldB = gcnew P3Node();
        selfHoldB->NodeType = "contact";
        selfHoldB->Tag = varB;
        selfHoldB->NormallyOpen = true;
        holdBranchB->Nodes->Add(selfHoldB);
        startOrHoldB->Branches->Add(holdBranchB);

        netB->Items->Add(startOrHoldB);

        P3Node^ interlockA = gcnew P3Node();
        interlockA->NodeType = "contact";
        interlockA->Tag = varA;
        interlockA->NormallyOpen = false;
        netB->Items->Add(interlockA);

        P3Node^ coilB = gcnew P3Node();
        coilB->NodeType = "coil";
        coilB->Tag = varB;
        netB->Items->Add(coilB);

        dsl->Networks->Add(netB);
    }

    static void GenerateSingleMotorNetwork(P3Dsl^ dsl, String^ sigName, String^ varName, P4Requirement^ requirement, P4VariablePlan^ varPlan) {
        String^ startBtn = FindStartButton(sigName, requirement, varPlan);
        String^ stopBtn = FindStopButton(sigName, requirement, varPlan);

        P3Network^ net = gcnew P3Network();
        net->Title = sigName + L" 控制";
        net->Items = gcnew List<Object^>();

        if (stopBtn != nullptr) {
            P3Node^ stopContact = gcnew P3Node();
            stopContact->NodeType = "contact";
            stopContact->Tag = stopBtn;
            stopContact->NormallyOpen = false;
            net->Items->Add(stopContact);
        }

        P3Parallel^ startOrHold = gcnew P3Parallel();
        startOrHold->Branches = gcnew List<P3Branch^>();

        P3Branch^ startBranch = gcnew P3Branch();
        startBranch->Nodes = gcnew List<P3Node^>();
        if (startBtn != nullptr) {
            P3Node^ startContact = gcnew P3Node();
            startContact->NodeType = "contact";
            startContact->Tag = startBtn;
            startContact->NormallyOpen = true;
            startBranch->Nodes->Add(startContact);
        }
        startOrHold->Branches->Add(startBranch);

        P3Branch^ holdBranch = gcnew P3Branch();
        holdBranch->Nodes = gcnew List<P3Node^>();
        P3Node^ selfHold = gcnew P3Node();
        selfHold->NodeType = "contact";
        selfHold->Tag = varName;
        selfHold->NormallyOpen = true;
        holdBranch->Nodes->Add(selfHold);
        startOrHold->Branches->Add(holdBranch);

        net->Items->Add(startOrHold);

        P3Node^ coil = gcnew P3Node();
        coil->NodeType = "coil";
        coil->Tag = varName;
        net->Items->Add(coil);

        dsl->Networks->Add(net);
    }

    static void GenerateStarDeltaNetworks(P3Dsl^ dsl, P4Requirement^ requirement, P4SemanticPlan^ plan, P4VariablePlan^ varPlan) {
        Console::WriteLine("  [StarDelta] Generating star-delta start networks...");

        String^ startBtn = FindInputByKeywords(varPlan, requirement, gcnew array<String^>{"start", L"启动"});
        String^ stopBtn = FindInputByKeywords(varPlan, requirement, gcnew array<String^>{"stop", L"停止"});
        String^ kmMain = FindOutputByKeywords(varPlan, requirement, gcnew array<String^>{"km_main", "main", L"主接触器", L"主"});
        String^ kmStar = FindOutputByKeywords(varPlan, requirement, gcnew array<String^>{"km_star", "star", L"星型", L"星"});
        String^ kmDelta = FindOutputByKeywords(varPlan, requirement, gcnew array<String^>{"km_delta", "delta", L"三角", L"角"});
        String^ starTimer = FindTimerByKeywords(varPlan, requirement, gcnew array<String^>{"star", "delta", "switch", L"切换", L"转换"});

        if (kmMain == nullptr && requirement->Outputs->Count > 0) kmMain = MapSignalNameToVarName(requirement->Outputs[0]->Name, varPlan);
        if (kmStar == nullptr && requirement->Outputs->Count > 1) kmStar = MapSignalNameToVarName(requirement->Outputs[1]->Name, varPlan);
        if (kmDelta == nullptr && requirement->Outputs->Count > 2) kmDelta = MapSignalNameToVarName(requirement->Outputs[2]->Name, varPlan);
        if (starTimer == nullptr) starTimer = "Star_Delta_Timer";

        P3Network^ mainNet = gcnew P3Network();
        mainNet->Title = L"主接触器控制（星三角启动）";
        mainNet->Items = gcnew List<Object^>();

        if (stopBtn != nullptr) {
            P3Node^ stopNC = gcnew P3Node();
            stopNC->NodeType = "contact";
            stopNC->Tag = stopBtn;
            stopNC->NormallyOpen = false;
            mainNet->Items->Add(stopNC);
        }

        P3Parallel^ startOrHold = gcnew P3Parallel();
        startOrHold->Branches = gcnew List<P3Branch^>();
        P3Branch^ startBranch = gcnew P3Branch();
        startBranch->Nodes = gcnew List<P3Node^>();
        if (startBtn != nullptr) {
            P3Node^ startNO = gcnew P3Node();
            startNO->NodeType = "contact";
            startNO->Tag = startBtn;
            startNO->NormallyOpen = true;
            startBranch->Nodes->Add(startNO);
        }
        startOrHold->Branches->Add(startBranch);
        P3Branch^ holdBranch = gcnew P3Branch();
        holdBranch->Nodes = gcnew List<P3Node^>();
        if (kmMain != nullptr) {
            P3Node^ selfHold = gcnew P3Node();
            selfHold->NodeType = "contact";
            selfHold->Tag = kmMain;
            selfHold->NormallyOpen = true;
            holdBranch->Nodes->Add(selfHold);
        }
        startOrHold->Branches->Add(holdBranch);
        mainNet->Items->Add(startOrHold);

        if (kmMain != nullptr) {
            P3Node^ mainCoil = gcnew P3Node();
            mainCoil->NodeType = "coil";
            mainCoil->Tag = kmMain;
            mainNet->Items->Add(mainCoil);
        }
        dsl->Networks->Add(mainNet);

        P3Network^ starNet = gcnew P3Network();
        starNet->Title = L"星型接触器控制";
        starNet->Items = gcnew List<Object^>();

        if (kmMain != nullptr) {
            P3Node^ mainNO = gcnew P3Node();
            mainNO->NodeType = "contact";
            mainNO->Tag = kmMain;
            mainNO->NormallyOpen = true;
            starNet->Items->Add(mainNO);
        }

        if (kmDelta != nullptr) {
            P3Node^ deltaNC = gcnew P3Node();
            deltaNC->NodeType = "contact";
            deltaNC->Tag = kmDelta;
            deltaNC->NormallyOpen = false;
            starNet->Items->Add(deltaNC);
        }

        if (starTimer != nullptr) {
            P3Node^ timerNC = gcnew P3Node();
            timerNC->NodeType = "contact";
            timerNC->Tag = starTimer + ".Q";
            timerNC->NormallyOpen = false;
            starNet->Items->Add(timerNC);
        }

        if (kmStar != nullptr) {
            P3Node^ starCoil = gcnew P3Node();
            starCoil->NodeType = "coil";
            starCoil->Tag = kmStar;
            starNet->Items->Add(starCoil);
        }
        dsl->Networks->Add(starNet);

        P3Network^ timerNet = gcnew P3Network();
        timerNet->Title = L"星三角切换定时器";
        timerNet->Items = gcnew List<Object^>();

        if (kmStar != nullptr) {
            P3Node^ starNO = gcnew P3Node();
            starNO->NodeType = "contact";
            starNO->Tag = kmStar;
            starNO->NormallyOpen = true;
            timerNet->Items->Add(starNO);
        }

        P3Node^ tonNode = gcnew P3Node();
        tonNode->NodeType = "ton";
        tonNode->Instance = starTimer;
        tonNode->Pt = FindTimerPreset(varPlan, starTimer);
        if (tonNode->Pt == "T#1S") tonNode->Pt = "T#5S";
        timerNet->Items->Add(tonNode);
        dsl->Networks->Add(timerNet);

        P3Network^ deltaNet = gcnew P3Network();
        deltaNet->Title = L"三角型接触器控制";
        deltaNet->Items = gcnew List<Object^>();

        if (kmMain != nullptr) {
            P3Node^ mainNO2 = gcnew P3Node();
            mainNO2->NodeType = "contact";
            mainNO2->Tag = kmMain;
            mainNO2->NormallyOpen = true;
            deltaNet->Items->Add(mainNO2);
        }

        if (kmStar != nullptr) {
            P3Node^ starNC = gcnew P3Node();
            starNC->NodeType = "contact";
            starNC->Tag = kmStar;
            starNC->NormallyOpen = false;
            deltaNet->Items->Add(starNC);
        }

        if (starTimer != nullptr) {
            P3Node^ timerNO = gcnew P3Node();
            timerNO->NodeType = "contact";
            timerNO->Tag = starTimer + ".Q";
            timerNO->NormallyOpen = true;
            deltaNet->Items->Add(timerNO);
        }

        if (kmDelta != nullptr) {
            P3Node^ deltaCoil = gcnew P3Node();
            deltaCoil->NodeType = "coil";
            deltaCoil->Tag = kmDelta;
            deltaNet->Items->Add(deltaCoil);
        }
        dsl->Networks->Add(deltaNet);
    }

    static void GenerateAutoRoundTripNetworks(P3Dsl^ dsl, P4Requirement^ requirement, P4SemanticPlan^ plan, P4VariablePlan^ varPlan) {
        Console::WriteLine("  [AutoRoundTrip] Generating auto round-trip networks...");

        String^ startBtn = FindInputByKeywords(varPlan, requirement, gcnew array<String^>{"start", L"启动"});
        String^ stopBtn = FindInputByKeywords(varPlan, requirement, gcnew array<String^>{"stop", L"停止"});
        String^ motorFwd = FindOutputByKeywords(varPlan, requirement, gcnew array<String^>{"fwd", "forward", "right", L"正转", L"前进", L"右行"});
        String^ motorRev = FindOutputByKeywords(varPlan, requirement, gcnew array<String^>{"rev", "reverse", "left", L"反转", L"后退", L"左行"});
        String^ limitRight = FindInputByKeywords(varPlan, requirement, gcnew array<String^>{"limit_r", "right_limit", "limit_right", L"右限位", L"右限"});
        String^ limitLeft = FindInputByKeywords(varPlan, requirement, gcnew array<String^>{"limit_l", "left_limit", "limit_left", L"左限位", L"左限"});

        if (motorFwd == nullptr && requirement->Outputs->Count > 0) motorFwd = MapSignalNameToVarName(requirement->Outputs[0]->Name, varPlan);
        if (motorRev == nullptr && requirement->Outputs->Count > 1) motorRev = MapSignalNameToVarName(requirement->Outputs[1]->Name, varPlan);

        P3Network^ fwdNet = gcnew P3Network();
        fwdNet->Title = L"正转（前进）控制（自动往返）";
        fwdNet->Items = gcnew List<Object^>();

        if (stopBtn != nullptr) {
            P3Node^ stopNC = gcnew P3Node();
            stopNC->NodeType = "contact";
            stopNC->Tag = stopBtn;
            stopNC->NormallyOpen = false;
            fwdNet->Items->Add(stopNC);
        }

        P3Parallel^ fwdStartOrHold = gcnew P3Parallel();
        fwdStartOrHold->Branches = gcnew List<P3Branch^>();

        P3Branch^ fwdStartBranch = gcnew P3Branch();
        fwdStartBranch->Nodes = gcnew List<P3Node^>();
        P3Parallel^ fwdStartParallel = gcnew P3Parallel();
        fwdStartParallel->Branches = gcnew List<P3Branch^>();
        P3Branch^ fwdStartBtnBranch = gcnew P3Branch();
        fwdStartBtnBranch->Nodes = gcnew List<P3Node^>();
        if (startBtn != nullptr) {
            P3Node^ startNO = gcnew P3Node();
            startNO->NodeType = "contact";
            startNO->Tag = startBtn;
            startNO->NormallyOpen = true;
            fwdStartBtnBranch->Nodes->Add(startNO);
        }
        fwdStartParallel->Branches->Add(fwdStartBtnBranch);
        if (limitRight != nullptr) {
            P3Branch^ limitBranch = gcnew P3Branch();
            limitBranch->Nodes = gcnew List<P3Node^>();
            P3Node^ limitNO = gcnew P3Node();
            limitNO->NodeType = "contact";
            limitNO->Tag = limitRight;
            limitNO->NormallyOpen = true;
            limitBranch->Nodes->Add(limitNO);
            fwdStartParallel->Branches->Add(limitBranch);
        }
        if (fwdStartParallel->Branches->Count == 1) {
            for each (P3Node^ n in fwdStartParallel->Branches[0]->Nodes) fwdStartBranch->Nodes->Add(n);
        }
        else {
            for each (P3Branch^ b in fwdStartParallel->Branches) {
                P3Branch^ wrapperBranch = gcnew P3Branch();
                wrapperBranch->Nodes = gcnew List<P3Node^>();
                for each (P3Node^ n in b->Nodes) wrapperBranch->Nodes->Add(n);
                fwdStartOrHold->Branches->Add(wrapperBranch);
            }
            fwdStartBranch = nullptr;
        }

        if (fwdStartBranch != nullptr) fwdStartOrHold->Branches->Add(fwdStartBranch);

        P3Branch^ fwdHoldBranch = gcnew P3Branch();
        fwdHoldBranch->Nodes = gcnew List<P3Node^>();
        if (motorFwd != nullptr) {
            P3Node^ selfHold = gcnew P3Node();
            selfHold->NodeType = "contact";
            selfHold->Tag = motorFwd;
            selfHold->NormallyOpen = true;
            fwdHoldBranch->Nodes->Add(selfHold);
        }
        fwdStartOrHold->Branches->Add(fwdHoldBranch);
        fwdNet->Items->Add(fwdStartOrHold);

        if (limitRight != nullptr) {
            P3Node^ limitNC = gcnew P3Node();
            limitNC->NodeType = "contact";
            limitNC->Tag = limitRight;
            limitNC->NormallyOpen = false;
            fwdNet->Items->Add(limitNC);
        }

        if (motorRev != nullptr) {
            P3Node^ interlockNC = gcnew P3Node();
            interlockNC->NodeType = "contact";
            interlockNC->Tag = motorRev;
            interlockNC->NormallyOpen = false;
            fwdNet->Items->Add(interlockNC);
        }

        if (motorFwd != nullptr) {
            P3Node^ fwdCoil = gcnew P3Node();
            fwdCoil->NodeType = "coil";
            fwdCoil->Tag = motorFwd;
            fwdNet->Items->Add(fwdCoil);
        }
        dsl->Networks->Add(fwdNet);

        P3Network^ revNet = gcnew P3Network();
        revNet->Title = L"反转（后退）控制（自动往返）";
        revNet->Items = gcnew List<Object^>();

        if (stopBtn != nullptr) {
            P3Node^ stopNC2 = gcnew P3Node();
            stopNC2->NodeType = "contact";
            stopNC2->Tag = stopBtn;
            stopNC2->NormallyOpen = false;
            revNet->Items->Add(stopNC2);
        }

        P3Parallel^ revStartOrHold = gcnew P3Parallel();
        revStartOrHold->Branches = gcnew List<P3Branch^>();

        if (limitLeft != nullptr) {
            P3Branch^ revLimitBranch = gcnew P3Branch();
            revLimitBranch->Nodes = gcnew List<P3Node^>();
            P3Node^ limitNO2 = gcnew P3Node();
            limitNO2->NodeType = "contact";
            limitNO2->Tag = limitLeft;
            limitNO2->NormallyOpen = true;
            revLimitBranch->Nodes->Add(limitNO2);
            revStartOrHold->Branches->Add(revLimitBranch);
        }

        P3Branch^ revHoldBranch = gcnew P3Branch();
        revHoldBranch->Nodes = gcnew List<P3Node^>();
        if (motorRev != nullptr) {
            P3Node^ selfHold2 = gcnew P3Node();
            selfHold2->NodeType = "contact";
            selfHold2->Tag = motorRev;
            selfHold2->NormallyOpen = true;
            revHoldBranch->Nodes->Add(selfHold2);
        }
        revStartOrHold->Branches->Add(revHoldBranch);
        revNet->Items->Add(revStartOrHold);

        if (limitLeft != nullptr) {
            P3Node^ limitNC2 = gcnew P3Node();
            limitNC2->NodeType = "contact";
            limitNC2->Tag = limitLeft;
            limitNC2->NormallyOpen = false;
            revNet->Items->Add(limitNC2);
        }

        if (motorFwd != nullptr) {
            P3Node^ interlockNC2 = gcnew P3Node();
            interlockNC2->NodeType = "contact";
            interlockNC2->Tag = motorFwd;
            interlockNC2->NormallyOpen = false;
            revNet->Items->Add(interlockNC2);
        }

        if (motorRev != nullptr) {
            P3Node^ revCoil = gcnew P3Node();
            revCoil->NodeType = "coil";
            revCoil->Tag = motorRev;
            revNet->Items->Add(revCoil);
        }
        dsl->Networks->Add(revNet);
    }

    static void AddLimitSwitchProtection(P3Dsl^ dsl, P4Requirement^ requirement, P4SemanticPlan^ plan, P4VariablePlan^ varPlan) {
        Console::WriteLine("  [LimitSwitch] Adding limit switch protection...");

        for each (String^ limPair in plan->LimitOutputs) {
            array<String^>^ parts = limPair->Split(gcnew array<Char>{','});
            if (parts->Length < 2) continue;
            String^ outputVar = MapSignalNameToVarName(parts[0]->Trim(), varPlan);
            String^ limitVar = MapSignalNameToVarName(parts[1]->Trim(), varPlan);
            if (outputVar == nullptr || limitVar == nullptr || outputVar->Length == 0 || limitVar->Length == 0) continue;

            for each (P3Network^ net in dsl->Networks) {
                bool hasOutputCoil = false;
                for each (Object^ item in net->Items) {
                    P3Node^ n = dynamic_cast<P3Node^>(item);
                    if (n != nullptr && n->NodeType->ToLower() == "coil" && n->Tag == outputVar) {
                        hasOutputCoil = true;
                        break;
                    }
                }
                if (!hasOutputCoil) continue;

                bool alreadyHasLimit = false;
                for each (Object^ item in net->Items) {
                    P3Node^ n = dynamic_cast<P3Node^>(item);
                    if (n != nullptr && n->Tag == limitVar) {
                        alreadyHasLimit = true;
                        break;
                    }
                }
                if (alreadyHasLimit) continue;

                for (int idx = net->Items->Count - 1; idx >= 0; idx--) {
                    P3Node^ n = dynamic_cast<P3Node^>(net->Items[idx]);
                    if (n != nullptr && n->NodeType->ToLower() == "coil" && n->Tag == outputVar) {
                        P3Node^ limitNC = gcnew P3Node();
                        limitNC->NodeType = "contact";
                        limitNC->Tag = limitVar;
                        limitNC->NormallyOpen = false;
                        net->Items->Insert(idx, limitNC);
                        Console::WriteLine("    Added limit " + limitVar + "(NC) before coil " + outputVar + " in " + net->Title);
                        break;
                    }
                }
            }
        }
    }

    static void EnsureSelfHoldingOnAllMotorOutputs(P3Dsl^ dsl, P4Requirement^ requirement, P4SemanticPlan^ plan, P4VariablePlan^ varPlan) {
        for each (P4SignalDecl^ sig in requirement->Outputs) {
            String^ varName = MapSignalNameToVarName(sig->Name, varPlan);
            bool hasSelfHold = false;
            bool hasCoil = false;

            for each (P3Network^ net in dsl->Networks) {
                for each (Object^ item in net->Items) {
                    P3Node^ n = dynamic_cast<P3Node^>(item);
                    if (n != nullptr && n->Tag == varName) {
                        if (n->NodeType->ToLower() == "coil") hasCoil = true;
                        if (n->NodeType->ToLower() == "contact" && n->NormallyOpen) hasSelfHold = true;
                    }
                    P3Parallel^ par = dynamic_cast<P3Parallel^>(item);
                    if (par != nullptr) {
                        for each (P3Branch^ br in par->Branches) {
                            for each (P3Node^ bn in br->Nodes) {
                                if (bn->Tag == varName && bn->NormallyOpen) hasSelfHold = true;
                            }
                        }
                    }
                }
            }

            if (hasCoil && !hasSelfHold && plan->HasSelfHold) {
                Console::WriteLine("    [SelfHold] Adding self-holding for " + varName);

                for each (P3Network^ net in dsl->Networks) {
                    for (int idx = 0; idx < net->Items->Count; idx++) {
                        P3Node^ n = dynamic_cast<P3Node^>(net->Items[idx]);
                        if (n != nullptr && n->NodeType->ToLower() == "coil" && n->Tag == varName) {
                            P3Node^ prevContact = nullptr;
                            if (idx > 0) {
                                P3Node^ p = dynamic_cast<P3Node^>(net->Items[idx - 1]);
                                if (p != nullptr && p->NodeType->ToLower() == "contact") prevContact = p;
                            }

                            if (prevContact != nullptr) {
                                net->Items->RemoveAt(idx - 1);
                                P3Parallel^ par = gcnew P3Parallel();
                                par->Branches = gcnew List<P3Branch^>();
                                P3Branch^ br1 = gcnew P3Branch();
                                br1->Nodes = gcnew List<P3Node^>();
                                br1->Nodes->Add(prevContact);
                                par->Branches->Add(br1);
                                P3Branch^ br2 = gcnew P3Branch();
                                br2->Nodes = gcnew List<P3Node^>();
                                P3Node^ holdContact = gcnew P3Node();
                                holdContact->NodeType = "contact";
                                holdContact->Tag = varName;
                                holdContact->NormallyOpen = true;
                                br2->Nodes->Add(holdContact);
                                par->Branches->Add(br2);
                                net->Items->Insert(idx - 1, par);
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    static String^ FindInputByKeywords(P4VariablePlan^ varPlan, P4Requirement^ requirement, array<String^>^ keywords) {
        for each (P4SignalDecl^ sig in requirement->Inputs) {
            String^ il = sig->Name->ToLower();
            String^ icl = (sig->Comment != nullptr) ? sig->Comment->ToLower() : "";
            for each (String^ kw in keywords) {
                if (il->Contains(kw->ToLower()) || icl->Contains(kw->ToLower()))
                    return MapSignalNameToVarName(sig->Name, varPlan);
            }
        }
        for each (P3Variable^ v in varPlan->Variables) {
            if (v->Scope != "input") continue;
            String^ nl = v->Name->ToLower();
            String^ cl = (v->Comment != nullptr) ? v->Comment->ToLower() : "";
            for each (String^ kw in keywords) {
                if (nl->Contains(kw->ToLower()) || cl->Contains(kw->ToLower()))
                    return v->Name;
            }
        }
        return nullptr;
    }

    static String^ FindOutputByKeywords(P4VariablePlan^ varPlan, P4Requirement^ requirement, array<String^>^ keywords) {
        for each (P4SignalDecl^ sig in requirement->Outputs) {
            String^ ol = sig->Name->ToLower();
            String^ ocl = (sig->Comment != nullptr) ? sig->Comment->ToLower() : "";
            for each (String^ kw in keywords) {
                if (ol->Contains(kw->ToLower()) || ocl->Contains(kw->ToLower()))
                    return MapSignalNameToVarName(sig->Name, varPlan);
            }
        }
        for each (P3Variable^ v in varPlan->Variables) {
            if (v->Scope != "output") continue;
            String^ nl = v->Name->ToLower();
            String^ cl = (v->Comment != nullptr) ? v->Comment->ToLower() : "";
            for each (String^ kw in keywords) {
                if (nl->Contains(kw->ToLower()) || cl->Contains(kw->ToLower()))
                    return v->Name;
            }
        }
        return nullptr;
    }

    static String^ FindTimerByKeywords(P4VariablePlan^ varPlan, P4Requirement^ requirement, array<String^>^ keywords) {
        for each (P4TimerRequirement^ t in requirement->Timers) {
            String^ tl = t->Name->ToLower();
            for each (String^ kw in keywords) {
                if (tl->Contains(kw->ToLower())) return t->Name;
            }
        }
        for each (P3Variable^ v in varPlan->Variables) {
            if (v->Type != "Timer") continue;
            String^ nl = v->Name->ToLower();
            String^ cl = (v->Comment != nullptr) ? v->Comment->ToLower() : "";
            for each (String^ kw in keywords) {
                if (nl->Contains(kw->ToLower()) || cl->Contains(kw->ToLower()))
                    return v->Name;
            }
        }
        return nullptr;
    }

    static String^ FindStartButton(String^ outputName, P4Requirement^ requirement, P4VariablePlan^ varPlan) {
        String^ ol = outputName->ToLower();
        for each (P4SignalDecl^ sig in requirement->Inputs) {
            String^ il = sig->Name->ToLower();
            String^ icl = (sig->Comment != nullptr) ? sig->Comment->ToLower() : "";
            if (il->Contains("start") || icl->Contains(L"启动")) {
                if ((ol->Contains("motor1") || ol->Contains("m1")) && (il->Contains("1") || il->Contains("_1") || icl->Contains(L"1") || icl->Contains(L"电机1") || icl->Contains(L"1号")))
                    return MapSignalNameToVarName(sig->Name, varPlan);
                if ((ol->Contains("motor2") || ol->Contains("m2")) && (il->Contains("2") || il->Contains("_2") || icl->Contains(L"2") || icl->Contains(L"电机2") || icl->Contains(L"2号")))
                    return MapSignalNameToVarName(sig->Name, varPlan);
                if (ol->Contains("fwd") && (il->Contains("fwd") || il->Contains("forward") || icl->Contains(L"正转") || icl->Contains(L"前进")))
                    return MapSignalNameToVarName(sig->Name, varPlan);
                if (ol->Contains("rev") && (il->Contains("rev") || il->Contains("reverse") || icl->Contains(L"反转") || icl->Contains(L"后退")))
                    return MapSignalNameToVarName(sig->Name, varPlan);
            }
        }
        for each (P4SignalDecl^ sig in requirement->Inputs) {
            String^ il = sig->Name->ToLower();
            String^ icl = (sig->Comment != nullptr) ? sig->Comment->ToLower() : "";
            if (il->Contains("start") || icl->Contains(L"启动")) {
                if (ol->Contains("1") && (il->Contains("1") || icl->Contains(L"1")))
                    return MapSignalNameToVarName(sig->Name, varPlan);
                if (ol->Contains("2") && (il->Contains("2") || icl->Contains(L"2")))
                    return MapSignalNameToVarName(sig->Name, varPlan);
            }
        }
        for each (P3Variable^ v in varPlan->Variables) {
            if (v->Scope != "input") continue;
            String^ nl = v->Name->ToLower();
            String^ cl = (v->Comment != nullptr) ? v->Comment->ToLower() : "";
            if (cl->Contains("start") && cl->Contains(outputName->ToLower()))
                return v->Name;
            if (nl->Contains(L"启动") && ol->Contains("1") && nl->Contains(L"1"))
                return v->Name;
            if (nl->Contains(L"启动") && ol->Contains("2") && nl->Contains(L"2"))
                return v->Name;
        }
        return nullptr;
    }

    static String^ FindStopButton(String^ outputName, P4Requirement^ requirement, P4VariablePlan^ varPlan) {
        String^ ol = outputName->ToLower();
        for each (P4SignalDecl^ sig in requirement->Inputs) {
            String^ il = sig->Name->ToLower();
            String^ icl = (sig->Comment != nullptr) ? sig->Comment->ToLower() : "";
            if (il->Contains("stop") || icl->Contains(L"停止")) {
                if ((ol->Contains("motor1") || ol->Contains("m1")) && (il->Contains("1") || il->Contains("_1") || icl->Contains(L"1") || icl->Contains(L"电机1") || icl->Contains(L"1号")))
                    return MapSignalNameToVarName(sig->Name, varPlan);
                if ((ol->Contains("motor2") || ol->Contains("m2")) && (il->Contains("2") || il->Contains("_2") || icl->Contains(L"2") || icl->Contains(L"电机2") || icl->Contains(L"2号")))
                    return MapSignalNameToVarName(sig->Name, varPlan);
            }
        }
        for each (P4SignalDecl^ sig in requirement->Inputs) {
            String^ il = sig->Name->ToLower();
            String^ icl = (sig->Comment != nullptr) ? sig->Comment->ToLower() : "";
            if (il->Contains("stop") || icl->Contains(L"停止")) {
                if (ol->Contains("1") && (il->Contains("1") || icl->Contains(L"1")))
                    return MapSignalNameToVarName(sig->Name, varPlan);
                if (ol->Contains("2") && (il->Contains("2") || icl->Contains(L"2")))
                    return MapSignalNameToVarName(sig->Name, varPlan);
            }
        }
        for each (P3Variable^ v in varPlan->Variables) {
            if (v->Scope != "input") continue;
            String^ cl = (v->Comment != nullptr) ? v->Comment->ToLower() : "";
            String^ nl = v->Name->ToLower();
            if (cl->Contains("stop") && cl->Contains(outputName->ToLower()))
                return v->Name;
            if (nl->Contains(L"停止") && ol->Contains("1") && nl->Contains(L"1"))
                return v->Name;
            if (nl->Contains(L"停止") && ol->Contains("2") && nl->Contains(L"2"))
                return v->Name;
        }
        return nullptr;
    }

    static void GenerateCycleNetworks(P3Dsl^ dsl, P4SemanticPlan^ plan, P4Requirement^ requirement) {
        for each (P4CounterRequirement^ cr in requirement->Counters) {
            P3Network^ countNet = gcnew P3Network();
            countNet->Title = "Cycle Count - " + cr->Name;
            countNet->Items = gcnew List<Object^>();

            if (plan->States->Count > 0) {
                String^ lastStep = "Step" + plan->States->Count;
                P3Node^ lastStepContact = gcnew P3Node();
                lastStepContact->NodeType = "contact";
                lastStepContact->Tag = lastStep;
                lastStepContact->NormallyOpen = true;
                countNet->Items->Add(lastStepContact);
            }

            P3Node^ ctuNode = gcnew P3Node();
            ctuNode->NodeType = "ctu";
            ctuNode->Instance = cr->Name;
            ctuNode->Pv = cr->Preset != nullptr ? cr->Preset : "1";
            countNet->Items->Add(ctuNode);

            dsl->Networks->Add(countNet);
        }

        P3Network^ doneNet = gcnew P3Network();
        doneNet->Title = "Cycle Complete";
        doneNet->Items = gcnew List<Object^>();

        for each (P4CounterRequirement^ cr in requirement->Counters) {
            P3Node^ qContact = gcnew P3Node();
            qContact->NodeType = "contact";
            qContact->Tag = cr->Name + ".Q";
            qContact->NormallyOpen = true;
            doneNet->Items->Add(qContact);
        }

        P3Node^ doneCoil = gcnew P3Node();
        doneCoil->NodeType = "set";
        doneCoil->Tag = "Cycle_Done";
        doneNet->Items->Add(doneCoil);

        dsl->Networks->Add(doneNet);
    }

    static String^ FindInputName(P4VariablePlan^ varPlan, String^ keyword) {
        for each (P3Variable^ v in varPlan->Variables) {
            if (v->Scope != "input" || v->Name == nullptr) continue;
            String^ nl = v->Name->ToLower();
            String^ cl = (v->Comment != nullptr) ? v->Comment->ToLower() : "";
            if (nl->Contains(keyword) || cl->Contains(keyword))
                return v->Name;
        }
        for each (P3Variable^ v in varPlan->Variables) {
            if (v->Scope == "input") return v->Name;
        }
        return "Start_Btn";
    }

    static List<String^>^ DetermineOutputActions(P4State^ state, P4SemanticPlan^ plan, P4Requirement^ requirement) {
        List<String^>^ outputs = gcnew List<String^>();
        String^ action = state->Action != nullptr ? state->Action : "";
        String^ stateName = state->Name != nullptr ? state->Name : "";
        String^ actionLower = action->ToLower();
        String^ stateNameLower = stateName->ToLower();

        array<String^>^ actionTokens = action->Split(gcnew array<Char>{' ', ',', ';', '(', ')', '=', '<', '>', '!', '&', '|', '/', '\n', '\r', '\t'}, StringSplitOptions::RemoveEmptyEntries);
        HashSet<String^>^ actionTokenSet = gcnew HashSet<String^>(StringComparer::OrdinalIgnoreCase);
        for each (String^ t in actionTokens) actionTokenSet->Add(t->Trim());

        for each (P4SignalDecl^ sig in requirement->Outputs) {
            if (sig->Name == nullptr || sig->Name->Length == 0) continue;
            String^ sigLower = sig->Name->ToLower();
            String^ commentLower = (sig->Comment != nullptr) ? sig->Comment->ToLower() : "";

            if (actionTokenSet->Contains(sig->Name)) {
                if (!outputs->Contains(sig->Name)) outputs->Add(sig->Name);
                continue;
            }

            bool keywordMatch = false;
            array<String^>^ sigParts = sig->Name->Split(gcnew array<Char>{'_'});
            for each (String^ part in sigParts) {
                if (part->Length > 1 && actionTokenSet->Contains(part)) {
                    keywordMatch = true;
                    break;
                }
            }
            if (!keywordMatch && commentLower->Length > 0) {
                array<String^>^ commentParts = commentLower->Split(gcnew array<Char>{' ', '_', ','}, StringSplitOptions::RemoveEmptyEntries);
                for each (String^ cp in commentParts) {
                    if (cp->Length > 1 && actionTokenSet->Contains(cp)) {
                        keywordMatch = true;
                        break;
                    }
                }
            }
            if (keywordMatch) {
                if (!outputs->Contains(sig->Name)) outputs->Add(sig->Name);
                continue;
            }

            if (stateNameLower->Contains(sigLower) || (commentLower->Length > 0 && stateNameLower->Contains(commentLower))) {
                if (!outputs->Contains(sig->Name)) outputs->Add(sig->Name);
                continue;
            }
        }

        if (outputs->Count == 0) {
            if (actionLower->Contains("forward") || actionLower->Contains("fwd") || stateNameLower->Contains("forward") ||
                actionLower->Contains(L"前进") || stateNameLower->Contains(L"前进") || actionLower->Contains(L"右行"))
                outputs->Add("Motor_Fwd");
            if (actionLower->Contains("reverse") || actionLower->Contains("rev") || actionLower->Contains("backward") || stateNameLower->Contains("backward") ||
                actionLower->Contains(L"后退") || stateNameLower->Contains(L"后退") || actionLower->Contains(L"左行") || actionLower->Contains(L"返回"))
                outputs->Add("Motor_Rev");
            if (actionLower->Contains("fill") || stateNameLower->Contains("fill") ||
                actionLower->Contains(L"装料") || stateNameLower->Contains(L"装料"))
                outputs->Add("Fill_Valve");
            if (actionLower->Contains("heat") || stateNameLower->Contains("heat") ||
                actionLower->Contains(L"加热") || stateNameLower->Contains(L"加热"))
                outputs->Add("Heater");
            if (actionLower->Contains("drain") || stateNameLower->Contains("drain") ||
                actionLower->Contains(L"卸料") || stateNameLower->Contains(L"卸料"))
                outputs->Add("Drain_Valve");
            if (actionLower->Contains("open") || stateNameLower->Contains("open") ||
                actionLower->Contains(L"开门") || stateNameLower->Contains(L"开门"))
                outputs->Add("Valve_Open");
            if (actionLower->Contains("run") && !actionLower->Contains("stop"))
                outputs->Add("Motor_Run");
            if (actionLower->Contains("clean") || stateNameLower->Contains("clean") || stateNameLower->Contains("wash") ||
                actionLower->Contains(L"清洗") || stateNameLower->Contains(L"清洗"))
                outputs->Add("Clean_Valve");
            if (actionLower->Contains("unload") || stateNameLower->Contains("unload") ||
                actionLower->Contains(L"卸料") || stateNameLower->Contains(L"卸料"))
                outputs->Add("Unload_Door");
        }

        for each (P4SignalDecl^ sig in requirement->Outputs) {
            if (sig->Name == nullptr || sig->Name->Length == 0) continue;
            String^ sigLower = sig->Name->ToLower();
            if (sigLower->Contains("motor") || sigLower->Contains("move") || sigLower->Contains("valve") || sigLower->Contains("door")) {
                if (actionLower->Contains(sigLower) || stateNameLower->Contains(sigLower)) {
                    if (!outputs->Contains(sig->Name)) outputs->Add(sig->Name);
                }
            }
        }
        return outputs;
    }

    static List<String^>^ ExtractTimerReferences(String^ condition) {
        List<String^>^ refs = gcnew List<String^>();
        if (condition == nullptr) return refs;
        String^ c = condition->ToLower();
        bool hasTimerHint = c->Contains("timer") || c->Contains("delay") || c->Contains("wait") || c->Contains("ton") ||
            c->Contains(L"定时") || c->Contains(L"延时") || c->Contains(L"等待") || c->Contains(L"计时") ||
            c->Contains(L"秒") || c->Contains(L"分钟") || c->Contains(L"后") ||
            System::Text::RegularExpressions::Regex::IsMatch(c, "\\d+\\s*[sms秒]");

        if (hasTimerHint) {
            for each (String^ word in condition->Split(gcnew array<Char>{' ', ',', '(', ')', '.'})) {
                String^ wl = word->ToLower();
                if (wl->Contains("timer") || wl->Contains("delay") || wl->Contains("ton")) {
                    if (word->Length > 0 && !refs->Contains(word)) refs->Add(word);
                }
            }
        }
        return refs;
    }

    static List<String^>^ FindTimerReferencesForStep(P4State^ state, P4Requirement^ requirement, P4VariablePlan^ varPlan) {
        List<String^>^ refs = gcnew List<String^>();

        List<String^>^ extracted = ExtractTimerReferences(state->TransitionCondition);
        for each (String^ r in extracted) {
            if (!refs->Contains(r)) refs->Add(r);
        }

        if (refs->Count > 0) return refs;

        if (requirement != nullptr && requirement->Timers != nullptr && requirement->Timers->Count > 0) {
            String^ actionLower = (state->Action != nullptr ? state->Action : "")->ToLower();
            String^ transLower = (state->TransitionCondition != nullptr ? state->TransitionCondition : "")->ToLower();
            String^ nameLower = (state->Name != nullptr ? state->Name : "")->ToLower();
            String^ commentLower = (state->Comment != nullptr ? state->Comment : "")->ToLower();
            String^ combined = actionLower + " " + transLower + " " + nameLower + " " + commentLower;

            for each (P4TimerRequirement^ t in requirement->Timers) {
                String^ timerNameLower = t->Name->ToLower();
                String^ timerPresetLower = (t->Preset != nullptr ? t->Preset : "")->ToLower();

                if (combined->Contains(timerNameLower)) {
                    if (!refs->Contains(t->Name)) refs->Add(t->Name);
                    continue;
                }

                String^ timerKeyword = nullptr;
                if (timerNameLower->Contains("load") || timerNameLower->Contains("fill") || timerNameLower->Contains("装")) timerKeyword = L"装";
                else if (timerNameLower->Contains("unload") || timerNameLower->Contains("drain") || timerNameLower->Contains("卸")) timerKeyword = L"卸";
                else if (timerNameLower->Contains("clean") || timerNameLower->Contains("wash") || timerNameLower->Contains("清")) timerKeyword = L"清";
                else if (timerNameLower->Contains("mix") || timerNameLower->Contains("搅")) timerKeyword = L"搅";
                else if (timerNameLower->Contains("heat") || timerNameLower->Contains("加")) timerKeyword = L"加";
                else if (timerNameLower->Contains("cool") || timerNameLower->Contains("冷")) timerKeyword = L"冷";

                if (timerKeyword != nullptr && combined->Contains(timerKeyword)) {
                    if (!refs->Contains(t->Name)) refs->Add(t->Name);
                    continue;
                }

                if (timerPresetLower->Length > 0) {
                    String^ presetNum = System::Text::RegularExpressions::Regex::Match(t->Preset, "\\d+")->Value;
                    if (presetNum->Length > 0 && combined->Contains(presetNum)) {
                        if (!refs->Contains(t->Name)) refs->Add(t->Name);
                        continue;
                    }
                }
            }
        }

        if (refs->Count == 0 && varPlan != nullptr) {
            String^ transLower = (state->TransitionCondition != nullptr ? state->TransitionCondition : "")->ToLower();
            for each (P3Variable^ v in varPlan->Variables) {
                if (v->Type == "Timer" && v->Name != nullptr) {
                    String^ qTag = v->Name + ".Q";
                    if (transLower->Contains(v->Name->ToLower()) || transLower->Contains(qTag->ToLower())) {
                        if (!refs->Contains(v->Name)) refs->Add(v->Name);
                    }
                }
            }
        }

        return refs;
    }

    static String^ FindTimerPreset(P4VariablePlan^ varPlan, String^ timerName) {
        for each (P3Variable^ v in varPlan->Variables) {
            if (v->Type == "Timer" && v->Name != nullptr &&
                (v->Name == timerName || timerName->Contains(v->Name) || v->Name->Contains(timerName))) {
                return v->Preset != nullptr && v->Preset->Length > 0 ? v->Preset : "T#1S";
            }
        }
        return "T#1S";
    }

    static List<String^>^ ParseTransitionCondition(String^ condition, P4VariablePlan^ varPlan) {
        return ParseTransitionCondition(condition, varPlan, nullptr, nullptr);
    }

    static List<String^>^ ParseTransitionCondition(String^ condition, P4VariablePlan^ varPlan, P4State^ state, P4Requirement^ requirement) {
        List<String^>^ contacts = gcnew List<String^>();
        if (condition == nullptr || condition->Length == 0) return contacts;

        array<String^>^ condTokens = condition->Split(gcnew array<Char>{' ', ',', '(', ')', '=', '<', '>', '!', '&', '|', '/', '\n', '\r', '\t'}, StringSplitOptions::RemoveEmptyEntries);
        HashSet<String^>^ condTokenSet = gcnew HashSet<String^>();
        for each (String^ t in condTokens) {
            condTokenSet->Add(t->Trim());
            condTokenSet->Add(t->Trim()->ToLower());
        }

        for each (P3Variable^ v in varPlan->Variables) {
            if (v->Name == nullptr || v->Name->Length == 0) continue;
            if (v->Scope != "input" && v->Scope != "internal" && v->Type != "Timer") continue;
            if (v->Name->StartsWith("Step")) continue;
            if (contacts->Contains(v->Name)) continue;

            bool matched = false;
            if (condTokenSet->Contains(v->Name)) {
                matched = true;
            }
            else if (condTokenSet->Contains(v->Name->ToLower())) {
                matched = true;
            }
            else if (v->Comment != nullptr && v->Comment->Length > 0 && condTokenSet->Contains(v->Comment)) {
                matched = true;
            }
            else if (v->Comment != nullptr && v->Comment->Length > 0 && condTokenSet->Contains(v->Comment->ToLower())) {
                matched = true;
            }
            else {
                for each (String^ token in condTokens) {
                    String^ trimmed = token->Trim();
                    if (trimmed->Length > 0 && (trimmed->Equals(v->Name, StringComparison::OrdinalIgnoreCase) ||
                        (v->Comment != nullptr && v->Comment->Length > 0 && trimmed->Equals(v->Comment, StringComparison::OrdinalIgnoreCase)))) {
                        matched = true;
                        break;
                    }
                }
            }

            if (matched) {
                if (v->Type == "Timer") {
                    String^ qTag = v->Name + ".Q";
                    if (!contacts->Contains(qTag)) contacts->Add(qTag);
                }
                else {
                    contacts->Add(v->Name);
                }
            }
        }

        if (state != nullptr && requirement != nullptr) {
            List<String^>^ timerRefs = FindTimerReferencesForStep(state, requirement, varPlan);
            for each (String^ timerRef in timerRefs) {
                String^ qTag = timerRef + ".Q";
                bool alreadyHas = false;
                for each (String^ c in contacts) {
                    if (c->Equals(qTag, StringComparison::OrdinalIgnoreCase) || c->Equals(timerRef, StringComparison::OrdinalIgnoreCase)) {
                        alreadyHas = true;
                        break;
                    }
                }
                if (!alreadyHas) {
                    contacts->Add(qTag);
                }
            }
        }

        return contacts;
    }

    static String^ DetermineNextStep(P4State^ state, P4SemanticPlan^ plan) {
        if (state->NextState != nullptr && state->NextState->Length > 0) {
            String^ firstNext = state->NextState;
            if (firstNext->Contains("|")) firstNext = firstNext->Split(gcnew array<Char>{'|'})[0]->Trim();
            for (int i = 0; i < plan->States->Count; i++) {
                if (plan->States[i]->Name == firstNext) {
                    return "Step" + (i + 1);
                }
            }
        }
        int curIdx = plan->States->IndexOf(state);
        if (curIdx >= 0 && curIdx < plan->States->Count - 1) {
            return "Step" + (curIdx + 2);
        }
        if (plan->IsCyclic && plan->States->Count > 0) {
            return "Step1";
        }
        return "";
    }

    static String^ ResolveNextStepName(String^ nextStateName, P4SemanticPlan^ plan) {
        if (nextStateName == nullptr || nextStateName->Length == 0) return "";
        for (int i = 0; i < plan->States->Count; i++) {
            if (plan->States[i]->Name == nextStateName) {
                return "Step" + (i + 1);
            }
        }
        return "";
    }

    static bool IsNegatedCondition(String^ condition, String^ tag) {
        if (condition == nullptr || tag == nullptr) return false;
        String^ condLower = condition->ToLower();
        String^ tagLower = tag->ToLower();
        int pos = condLower->IndexOf(tagLower);
        if (pos < 0) return false;
        if (pos >= 3) {
            String^ prefix = condLower->Substring(pos - 3, 3);
            if (prefix == "not" || prefix == "not") return true;
        }
        if (pos >= 1) {
            String^ prefix = condLower->Substring(pos - 1, 1);
            if (prefix == "!") return true;
        }
        return false;
    }
};

ref class P4StateMachineBuilder {
public:
    static P3Dsl^ Build(P4Requirement^ requirement, P4SemanticPlan^ semanticPlan, P4VariablePlan^ varPlan) {
        Console::WriteLine("[StateMachineBuilder] Delegating to LogicGraphGenerator...");
        return P4LogicGraphGenerator::Generate(requirement, semanticPlan, varPlan);
    }
};

ref struct P3ValidationResult {
    bool IsValid;
    List<String^>^ Errors;
    List<String^>^ Warnings;
    List<String^>^ AutoFixes;

    P3ValidationResult() {
        IsValid = true;
        Errors = gcnew List<String^>();
        Warnings = gcnew List<String^>();
        AutoFixes = gcnew List<String^>();
    }
};

ref class P3VariableEngine {
public:
    static void AutoGenerateVariables(P3Dsl^ dsl) {
        Dictionary<String^, P3Variable^>^ varMap = gcnew Dictionary<String^, P3Variable^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr && v->Name->Length > 0) {
                varMap[v->Name] = v;
            }
        }

        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                P3Parallel^ par = dynamic_cast<P3Parallel^>(item);
                if (node != nullptr) {
                    EnsureVariableForNode(dsl, varMap, node);
                }
                else if (par != nullptr) {
                    for each (P3Branch^ branch in par->Branches) {
                        for each (P3Node^ bn in branch->Nodes) {
                            EnsureVariableForNode(dsl, varMap, bn);
                        }
                    }
                }
            }
        }
    }

private:
    static void EnsureVariableForNode(P3Dsl^ dsl, Dictionary<String^, P3Variable^>^ varMap, P3Node^ node) {
        String^ t = node->NodeType->ToLower();

        if (t == "ton" || t == "tof" || t == "tp") {
            String^ inst = node->Instance;
            if (inst != nullptr && inst->Length > 0 && !varMap->ContainsKey(inst)) {
                P3Variable^ v = gcnew P3Variable();
                v->Name = inst;
                v->Type = "Timer";
                v->Scope = "internal";
                v->TimerType = t->ToUpper();
                v->Preset = (t == "ton" || t == "tof" || t == "tp") ? node->Pt : node->Pv;
                if (v->Preset == nullptr || v->Preset->Length == 0) {
                    v->Preset = (t == "ton" || t == "tof" || t == "tp") ? "T#1S" : "1";
                }
                v->Comment = "Auto-generated timer instance";
                dsl->Variables->Add(v);
                varMap[inst] = v;
                EnsureInstanceDb(dsl, varMap, inst, "Timer");
            }
            else if (inst != nullptr && inst->Length > 0) {
                EnsureInstanceDb(dsl, varMap, inst, "Timer");
            }
        }
        else if (t == "ctu" || t == "ctd" || t == "ctud") {
            String^ inst = node->Instance;
            if (inst != nullptr && inst->Length > 0 && !varMap->ContainsKey(inst)) {
                P3Variable^ v = gcnew P3Variable();
                v->Name = inst;
                v->Type = "Counter";
                v->Scope = "internal";
                v->CounterType = t->ToUpper();
                v->Preset = node->Pv;
                if (v->Preset == nullptr || v->Preset->Length == 0) v->Preset = "1";
                v->Comment = "Auto-generated counter instance";
                dsl->Variables->Add(v);
                varMap[inst] = v;
                EnsureInstanceDb(dsl, varMap, inst, "Counter");
            }
            else if (inst != nullptr && inst->Length > 0) {
                EnsureInstanceDb(dsl, varMap, inst, "Counter");
            }
        }
        else if (t == "contact" || t == "negated_contact" || t == "coil" || t == "set" || t == "reset") {
            EnsureTagVariable(dsl, varMap, node->Tag, "Bool");
        }
        else if (t == "compare_eq" || t == "compare_ne" || t == "compare_gt" || t == "compare_lt" || t == "compare_ge" || t == "compare_le") {
            String^ dt = (node->DataType != nullptr && node->DataType->Length > 0) ? node->DataType : "Int";
            EnsureTagVariable(dsl, varMap, node->Tag, dt);
            EnsureTagVariable(dsl, varMap, node->Tag2, dt);
        }
        else if (t == "add" || t == "sub" || t == "mul" || t == "div" || t == "mod") {
            String^ dt = (node->DataType != nullptr && node->DataType->Length > 0) ? node->DataType : "Int";
            EnsureTagVariable(dsl, varMap, node->Tag, dt);
            EnsureTagVariable(dsl, varMap, node->Tag2, dt);
            EnsureTagVariable(dsl, varMap, node->Tag3, dt);
        }
        else if (t == "move") {
            String^ dt = (node->DataType != nullptr && node->DataType->Length > 0) ? node->DataType : "Int";
            EnsureTagVariable(dsl, varMap, node->Tag, dt);
            EnsureTagVariable(dsl, varMap, node->Tag2, dt);
        }
        else if (t == "rising_edge" || t == "falling_edge") {
            EnsureTagVariable(dsl, varMap, node->Tag, "Bool");
        }
        else if (t == "step") {
            EnsureTagVariable(dsl, varMap, node->Tag, "Bool");
        }
        else if (t == "transition") {
            EnsureTagVariable(dsl, varMap, node->Tag, "Bool");
            EnsureTagVariable(dsl, varMap, node->Tag2, "Bool");
        }
    }

    static void EnsureInstanceDb(P3Dsl^ dsl, Dictionary<String^, P3Variable^>^ varMap, String^ instanceName, String^ iecType) {
        if (instanceName == nullptr || instanceName->Length == 0) return;
        String^ dbPrefix = (iecType == "Timer") ? "IEC_Timer_" : "IEC_Counter_";
        String^ dbName = dbPrefix + instanceName;
        if (varMap->ContainsKey(dbName)) return;

        P3Variable^ dbVar = gcnew P3Variable();
        dbVar->Name = dbName;
        dbVar->Type = iecType;
        dbVar->Scope = "internal";
        dbVar->Comment = "Auto-generated IEC " + iecType + " DB instance for " + instanceName;
        if (iecType == "Timer") {
            dbVar->TimerType = "TON";
        }
        else if (iecType == "Counter") {
            dbVar->CounterType = "CTU";
        }
        dsl->Variables->Add(dbVar);
        varMap[dbName] = dbVar;
    }

    static void EnsureTagVariable(P3Dsl^ dsl, Dictionary<String^, P3Variable^>^ varMap, String^ tag, String^ type) {
        if (tag == nullptr || tag->Length == 0) return;
        if (tag->Contains(".")) return;
        if (varMap->ContainsKey(tag)) return;

        bool isConst = true;
        for (int i = 0; i < tag->Length; i++) {
            if (Char::IsLetter(tag[i]) || tag[i] == '_') { isConst = false; break; }
        }
        if (isConst) return;

        P3Variable^ v = gcnew P3Variable();
        v->Name = tag;
        v->Type = type;
        v->Scope = "internal";
        v->Comment = "Auto-generated";
        dsl->Variables->Add(v);
        varMap[tag] = v;
    }
};

ref class P3SemanticValidator {
public:
    static P3ValidationResult^ Validate(P3Dsl^ dsl) {
        P3ValidationResult^ result = gcnew P3ValidationResult();

        ValidateVariables(dsl, result);
        ValidateNetworks(dsl, result);
        CheckDoubleCoil(dsl, result);
        CheckTimerUsage(dsl, result);
        CheckCounterUsage(dsl, result);
        CheckSequentialLogic(dsl, result);
        CheckInterlockErrors(dsl, result);
        CheckJumpLoop(dsl, result);
        CheckUnconnectedNodes(dsl, result);
        CheckMissingStopLogic(dsl, result);
        CheckMissingReset(dsl, result);
        CheckUndefinedVariables(dsl, result);

        result->IsValid = (result->Errors->Count == 0);
        return result;
    }

    static String^ AutoFix(P3Dsl^ dsl) {
        List<String^>^ fixes = gcnew List<String^>();

        for each (P3Network^ net in dsl->Networks) {
            bool hasOutput = false;
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr && IsOutputType(node->NodeType)) hasOutput = true;
                P3Parallel^ par = dynamic_cast<P3Parallel^>(item);
                if (par != nullptr) {
                    for each (P3Branch^ branch in par->Branches) {
                        for each (P3Node^ bn in branch->Nodes) {
                            if (IsOutputType(bn->NodeType)) hasOutput = true;
                        }
                    }
                }
            }
            if (!hasOutput) {
                P3Node^ coil = gcnew P3Node();
                coil->NodeType = "nop";
                net->Items->Add(coil);
                fixes->Add("Network '" + net->Title + "': added NOP (no output element)");
            }
        }

        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) FixNode(node, fixes);
                P3Parallel^ par = dynamic_cast<P3Parallel^>(item);
                if (par != nullptr) {
                    for each (P3Branch^ branch in par->Branches) {
                        for each (P3Node^ bn in branch->Nodes) {
                            FixNode(bn, fixes);
                        }
                    }
                }
            }
        }

        P3VariableEngine::AutoGenerateVariables(dsl);

        if (fixes->Count > 0) {
            return "Auto-fixed: " + String::Join("; ", fixes);
        }
        return "";
    }

private:
    static bool IsOutputType(String^ nodeType) {
        String^ t = nodeType->ToLower();
        return t == "coil" || t == "set" || t == "reset" || t == "scoil" || t == "rcoil" ||
            t == "ton" || t == "tof" || t == "tp" || t == "ctu" || t == "ctd" || t == "ctud" ||
            t == "move" || t == "add" || t == "sub" || t == "mul" || t == "div" || t == "mod" ||
            t == "jump" || t == "jmp" || t == "ret" || t == "nop" ||
            t == "rbitfield" || t == "sbitfield" || t == "resetbitfield" || t == "setbitfield";
    }

    static void FixNode(P3Node^ node, List<String^>^ fixes) {
        String^ t = node->NodeType->ToLower();
        if (t == "ton" || t == "tof" || t == "tp") {
            if (node->Pt == nullptr || node->Pt->Length == 0) {
                node->Pt = "T#1S";
                fixes->Add("Timer '" + node->Instance + "': set default PT=T#1S");
            }
        }
        if (t == "ctu" || t == "ctd" || t == "ctud") {
            if (node->Pv == nullptr || node->Pv->Length == 0) {
                node->Pv = "1";
                fixes->Add("Counter '" + node->Instance + "': set default PV=1");
            }
        }
        if (t == "add" || t == "sub" || t == "mul" || t == "div" || t == "mod") {
            if (node->DataType == nullptr || node->DataType->Length == 0) {
                node->DataType = "Int";
                fixes->Add("Math instruction: set default data_type=Int");
            }
        }
        if (t == "compare_eq" || t == "compare_ne" || t == "compare_gt" || t == "compare_lt" || t == "compare_ge" || t == "compare_le") {
            if (node->DataType == nullptr || node->DataType->Length == 0) {
                node->DataType = "Int";
                fixes->Add("Compare instruction: set default data_type=Int");
            }
        }
    }

    static void ValidateVariables(P3Dsl^ dsl, P3ValidationResult^ result) {
        HashSet<String^>^ names = gcnew HashSet<String^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name == nullptr || v->Name->Length == 0) {
                result->Errors->Add("Variable has empty name");
                continue;
            }
            if (names->Contains(v->Name)) {
                result->Errors->Add("Duplicate variable name: " + v->Name);
            }
            names->Add(v->Name);

            if (v->Type == "Timer" && (v->TimerType == nullptr || v->TimerType->Length == 0)) {
                result->Warnings->Add("Timer variable '" + v->Name + "' missing timer_type");
            }
            if (v->Type == "Counter" && (v->CounterType == nullptr || v->CounterType->Length == 0)) {
                result->Warnings->Add("Counter variable '" + v->Name + "' missing counter_type");
            }
        }
    }

    static void ValidateNetworks(P3Dsl^ dsl, P3ValidationResult^ result) {
        if (dsl->Networks->Count == 0) {
            result->Errors->Add("No networks defined");
            return;
        }

        for (int i = 0; i < dsl->Networks->Count; i++) {
            P3Network^ net = dsl->Networks[i];
            if (net->Items->Count == 0) {
                result->Errors->Add("Network " + (i + 1) + " '" + net->Title + "' has no nodes");
                continue;
            }

            bool hasOutput = false;
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr && IsOutputType(node->NodeType)) hasOutput = true;
                P3Parallel^ par = dynamic_cast<P3Parallel^>(item);
                if (par != nullptr) {
                    for each (P3Branch^ branch in par->Branches) {
                        for each (P3Node^ bn in branch->Nodes) {
                            if (IsOutputType(bn->NodeType)) hasOutput = true;
                        }
                    }
                }
            }
            if (!hasOutput) {
                result->Errors->Add("Network " + (i + 1) + " '" + net->Title + "' has no output element (needs coil/timer/counter/move/math/jump)");
            }
        }
    }

    static void CheckDoubleCoil(P3Dsl^ dsl, P3ValidationResult^ result) {
        Dictionary<String^, List<String^>^>^ coilNetworks = gcnew Dictionary<String^, List<String^>^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "coil" || t == "set" || t == "reset") && node->Tag != nullptr && node->Tag->Length > 0) {
                        if (!coilNetworks->ContainsKey(node->Tag)) {
                            coilNetworks[node->Tag] = gcnew List<String^>();
                        }
                        coilNetworks[node->Tag]->Add(net->Title);
                    }
                }
            }
        }
        for each (KeyValuePair<String^, List<String^>^>^ kv in coilNetworks) {
            if (kv->Value->Count > 1) {
                result->Warnings->Add("Double coil warning: '" + kv->Key + "' written in multiple networks: " + String::Join(", ", kv->Value));
            }
        }
    }

    static void CheckTimerUsage(P3Dsl^ dsl, P3ValidationResult^ result) {
        HashSet<String^>^ timerInstances = gcnew HashSet<String^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Type == "Timer" && v->Name != nullptr) {
                timerInstances->Add(v->Name);
            }
        }
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "ton" || t == "tof" || t == "tp") && node->Instance != nullptr && node->Instance->Length > 0) {
                        if (!timerInstances->Contains(node->Instance)) {
                            result->Warnings->Add("Timer instance '" + node->Instance + "' not declared in variables");
                        }
                        if (node->Pt == nullptr || node->Pt->Length == 0) {
                            result->Errors->Add("Timer '" + node->Instance + "' missing PT (preset time) in network '" + net->Title + "'");
                        }
                    }
                }
            }
        }
    }

    static void CheckCounterUsage(P3Dsl^ dsl, P3ValidationResult^ result) {
        HashSet<String^>^ counterInstances = gcnew HashSet<String^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Type == "Counter" && v->Name != nullptr) {
                counterInstances->Add(v->Name);
            }
        }
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "ctu" || t == "ctd" || t == "ctud") && node->Instance != nullptr && node->Instance->Length > 0) {
                        if (!counterInstances->Contains(node->Instance)) {
                            result->Warnings->Add("Counter instance '" + node->Instance + "' not declared in variables");
                        }
                        if (node->Pv == nullptr || node->Pv->Length == 0) {
                            result->Errors->Add("Counter '" + node->Instance + "' missing PV (preset value) in network '" + net->Title + "'");
                        }
                    }
                }
            }
        }
    }

    static void CheckSequentialLogic(P3Dsl^ dsl, P3ValidationResult^ result) {
        List<String^>^ stepVars = gcnew List<String^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr && (v->Name->StartsWith("Step") || v->Name->Contains("_Step") || v->Name->Contains("step"))) {
                stepVars->Add(v->Name);
            }
        }
        if (stepVars->Count == 0) return;

        HashSet<String^>^ setSteps = gcnew HashSet<String^>();
        HashSet<String^>^ resetSteps = gcnew HashSet<String^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "set" || t == "scoil" || t == "setcoil") && node->Tag != nullptr) {
                        setSteps->Add(node->Tag);
                    }
                    if ((t == "reset" || t == "rcoil" || t == "resetcoil") && node->Tag != nullptr) {
                        resetSteps->Add(node->Tag);
                    }
                }
            }
        }

        for each (String^ step in stepVars) {
            if (!setSteps->Contains(step)) {
                result->Warnings->Add("Step variable '" + step + "' is never SET (no entry condition)");
            }
            if (!resetSteps->Contains(step)) {
                result->Warnings->Add("Step variable '" + step + "' is never RESET (no exit condition - potential infinite step)");
            }
        }

        for each (String^ step in stepVars) {
            bool hasOutput = false;
            for each (P3Network^ net in dsl->Networks) {
                for each (Object^ item in net->Items) {
                    P3Node^ node = dynamic_cast<P3Node^>(item);
                    if (node != nullptr && node->Tag == step && node->NodeType->ToLower() == "contact") {
                        for each (Object^ item2 in net->Items) {
                            P3Node^ n2 = dynamic_cast<P3Node^>(item2);
                            if (n2 != nullptr && n2 != node) {
                                String^ nt = n2->NodeType->ToLower();
                                if (nt == "coil" || nt == "set" || nt == "reset" || nt == "ton" || nt == "tof" || nt == "tp" ||
                                    nt == "ctu" || nt == "ctd" || nt == "ctud" || nt == "move" || nt == "add" || nt == "sub") {
                                    hasOutput = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if (!hasOutput) {
                result->Warnings->Add("Step variable '" + step + "' has no associated output action (dead step)");
            }
        }
    }

    static void CheckInterlockErrors(P3Dsl^ dsl, P3ValidationResult^ result) {
        Dictionary<String^, List<String^>^>^ coilConditions = gcnew Dictionary<String^, List<String^>^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "coil" || t == "set") && node->Tag != nullptr && node->Tag->Length > 0) {
                        if (!coilConditions->ContainsKey(node->Tag)) {
                            coilConditions[node->Tag] = gcnew List<String^>();
                        }
                    }
                }
            }
        }

        List<String^>^ outputVars = gcnew List<String^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Scope == "output" && v->Type == "Bool") {
                outputVars->Add(v->Name);
            }
        }

        for (int i = 0; i < outputVars->Count; i++) {
            for (int j = i + 1; j < outputVars->Count; j++) {
                String^ a = outputVars[i];
                String^ b = outputVars[j];
                bool aHasBnc = false;
                bool bHasAnc = false;

                for each (P3Network^ net in dsl->Networks) {
                    bool hasA = false, hasB = false, hasAnc = false, hasBnc = false;
                    for each (Object^ item in net->Items) {
                        P3Node^ node = dynamic_cast<P3Node^>(item);
                        if (node == nullptr) continue;
                        String^ t = node->NodeType->ToLower();
                        if (t == "contact" && node->Tag == a) hasA = true;
                        if (t == "contact" && node->Tag == b) hasB = true;
                        if (t == "contact" && node->Tag == b && !node->NormallyOpen) hasBnc = true;
                        if (t == "contact" && node->Tag == a && !node->NormallyOpen) hasAnc = true;
                    }
                    if (hasA && hasBnc) aHasBnc = true;
                    if (hasB && hasAnc) bHasAnc = true;
                }

                if (!aHasBnc && !bHasAnc) {
                    String^ aLower = a->ToLower();
                    String^ bLower = b->ToLower();
                    if ((aLower->Contains("fwd") && bLower->Contains("rev")) ||
                        (aLower->Contains("forward") && bLower->Contains("reverse")) ||
                        (aLower->Contains("open") && bLower->Contains("close")) ||
                        (aLower->Contains("up") && bLower->Contains("down")) ||
                        (aLower->Contains("run") && bLower->Contains("stop"))) {
                        result->Warnings->Add("Potential interlock missing: '" + a + "' and '" + b + "' should have mutual exclusion (NC contact)");
                    }
                }
            }
        }
    }

    static void CheckJumpLoop(P3Dsl^ dsl, P3ValidationResult^ result) {
        Dictionary<String^, int>^ labelNetworkIndex = gcnew Dictionary<String^, int>();
        List<KeyValuePair<int, String^>>^ jumpList = gcnew List<KeyValuePair<int, String^>>();

        for (int i = 0; i < dsl->Networks->Count; i++) {
            for each (Object^ item in dsl->Networks[i]->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "label" && node->Label != nullptr) {
                        labelNetworkIndex[node->Label] = i;
                    }
                    if (t == "jump" || t == "jmp") {
                        String^ target = (node->Label != nullptr && node->Label->Length > 0) ? node->Label : node->Tag;
                        if (target != nullptr) {
                            jumpList->Add(KeyValuePair<int, String^>(i, target));
                        }
                    }
                }
            }
        }

        for each (KeyValuePair<int, String^>^ kv in jumpList) {
            int jumpNetIdx = kv->Key;
            String^ target = kv->Value;
            if (labelNetworkIndex->ContainsKey(target)) {
                int labelNetIdx = labelNetworkIndex[target];
                if (labelNetIdx <= jumpNetIdx) {
                    bool hasExitCondition = false;
                    for each (P3Network^ net in dsl->Networks) {
                        for each (Object^ item in net->Items) {
                            P3Node^ node = dynamic_cast<P3Node^>(item);
                            if (node != nullptr) {
                                String^ t = node->NodeType->ToLower();
                                if ((t == "ctu" || t == "ctd" || t == "ctud") && node->Instance != nullptr) {
                                    hasExitCondition = true;
                                }
                                if (t == "compare_eq" || t == "compare_gt" || t == "compare_ge") {
                                    hasExitCondition = true;
                                }
                            }
                        }
                    }
                    if (!hasExitCondition) {
                        result->Warnings->Add("Potential infinite loop: JMP to '" + target + "' (backward jump) without counter or compare exit condition");
                    }
                }
            }
        }
    }

    static void CheckUnconnectedNodes(P3Dsl^ dsl, P3ValidationResult^ result) {
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "contact" || t == "coil" || t == "set" || t == "reset") {
                        if (node->Tag == nullptr || node->Tag->Length == 0) {
                            result->Errors->Add("Network '" + net->Title + "': " + t + " node has empty tag (unconnected)");
                        }
                    }
                    if (t == "ton" || t == "tof" || t == "tp") {
                        if (node->Instance == nullptr || node->Instance->Length == 0) {
                            result->Errors->Add("Network '" + net->Title + "': timer node missing instance name");
                        }
                    }
                    if (t == "ctu" || t == "ctd" || t == "ctud") {
                        if (node->Instance == nullptr || node->Instance->Length == 0) {
                            result->Errors->Add("Network '" + net->Title + "': counter node missing instance name");
                        }
                    }
                }
            }
        }
    }

    static void CheckMissingStopLogic(P3Dsl^ dsl, P3ValidationResult^ result) {
        bool hasStopBtn = false;
        bool hasStopCondition = false;
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr && v->Scope == "input") {
                String^ n = v->Name->ToLower();
                if (n->Contains("stop") || n->Contains("halt") || n->Contains("abort")) {
                    hasStopBtn = true;
                }
            }
        }
        if (hasStopBtn) {
            for each (P3Network^ net in dsl->Networks) {
                for each (Object^ item in net->Items) {
                    P3Node^ node = dynamic_cast<P3Node^>(item);
                    if (node != nullptr && node->Tag != nullptr) {
                        String^ tag = node->Tag->ToLower();
                        if ((tag->Contains("stop") || tag->Contains("halt")) &&
                            node->NodeType->ToLower() == "contact" && !node->NormallyOpen) {
                            hasStopCondition = true;
                        }
                    }
                }
            }
            if (!hasStopCondition) {
                result->Warnings->Add("Stop button declared but no NC stop contact found in networks (missing stop logic)");
            }
        }

        bool hasOutput = false;
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Scope == "output") hasOutput = true;
        }
        if (hasOutput && !hasStopBtn) {
            bool hasAnyNc = false;
            for each (P3Network^ net in dsl->Networks) {
                for each (Object^ item in net->Items) {
                    P3Node^ node = dynamic_cast<P3Node^>(item);
                    if (node != nullptr && node->NodeType->ToLower() == "contact" && !node->NormallyOpen) {
                        hasAnyNc = true;
                    }
                }
            }
            if (!hasAnyNc) {
                result->Warnings->Add("No stop/shutdown logic detected (no NC contacts in any network)");
            }
        }
    }

    static void CheckMissingReset(P3Dsl^ dsl, P3ValidationResult^ result) {
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
                result->Warnings->Add("Variable '" + s + "' is SET but never RESET (potential latch without release)");
            }
        }
    }

    static void CheckUndefinedVariables(P3Dsl^ dsl, P3ValidationResult^ result) {
        HashSet<String^>^ definedVars = gcnew HashSet<String^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr) definedVars->Add(v->Name);
        }
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr && node->Tag != nullptr && node->Tag->Length > 0) {
                    String^ baseTag = node->Tag;
                    int dotIdx = baseTag->IndexOf('.');
                    if (dotIdx > 0) baseTag = baseTag->Substring(0, dotIdx);
                    if (!definedVars->Contains(baseTag) && baseTag->Length > 0) {
                        String^ t = node->NodeType->ToLower();
                        if (t != "jump" && t != "label" && t != "nop" && t != "ret") {
                            result->Warnings->Add("Tag '" + baseTag + "' used in network '" + net->Title + "' but not declared in variables");
                        }
                    }
                }
            }
        }
    }
};

ref class P3RepairEngine {
public:
    static String^ AnalyzeTiaError(String^ tiaErrorLog) {
        StringBuilder^ analysis = gcnew StringBuilder();
        array<String^>^ lines = tiaErrorLog->Split(gcnew array<Char>{'\n', '\r'}, StringSplitOptions::RemoveEmptyEntries);

        for each (String^ line in lines) {
            String^ l = line->Trim()->ToLower();
            if (l->Contains("coil") || l->Contains("required") || l->Contains("need")) {
                analysis->AppendLine("Missing output element: " + line->Trim());
            }
            else if (l->Contains("timer") || l->Contains("ton") || l->Contains("tof") || l->Contains("tp")) {
                analysis->AppendLine("Timer issue: " + line->Trim());
            }
            else if (l->Contains("counter") || l->Contains("ctu") || l->Contains("ctd")) {
                analysis->AppendLine("Counter issue: " + line->Trim());
            }
            else if (l->Contains("uid") || l->Contains("duplicate")) {
                analysis->AppendLine("UID/duplicate issue: " + line->Trim());
            }
            else if (l->Contains("error") || l->Contains("fail")) {
                analysis->AppendLine("General error: " + line->Trim());
            }
        }
        return analysis->ToString();
    }

    static P3Dsl^ RepairFromErrors(P3Dsl^ originalDsl, String^ errors, P3Config^ config) {
        String^ dslJson = P3DslSerializer::Serialize(originalDsl);
        String^ repairPrompt = P3PromptEngine::BuildRepairPrompt(dslJson, errors);

        try {
            String^ llmResponse = P3LlmClient::CallLlm(config, P3PromptEngine::BuildSystemPrompt(), repairPrompt);
            String^ extractedJson = P3DslParser::ExtractJsonFromLlmResponse(llmResponse);
            if (extractedJson != nullptr && extractedJson->Length > 0) {
                P3Dsl^ repaired = P3DslParser::Parse(extractedJson);
                if (repaired->Networks->Count > 0) {
                    return repaired;
                }
            }
        }
        catch (Exception^ e) {
            Console::WriteLine("[Repair] LLM repair failed: " + e->Message);
        }

        P3SemanticValidator::AutoFix(originalDsl);
        RepairUndefinedVariables(originalDsl);
        return originalDsl;
    }

    static void RepairUndefinedVariables(P3Dsl^ dsl) {
        HashSet<String^>^ definedVars = gcnew HashSet<String^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr) definedVars->Add(v->Name);
        }

        List<P3Variable^>^ newVars = gcnew List<P3Variable^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr && node->Tag != nullptr && node->Tag->Length > 0) {
                    String^ baseTag = node->Tag;
                    int dotIdx = baseTag->IndexOf('.');
                    if (dotIdx > 0) baseTag = baseTag->Substring(0, dotIdx);
                    if (!definedVars->Contains(baseTag) && baseTag->Length > 0) {
                        String^ t = node->NodeType->ToLower();
                        if (t != "jump" && t != "label" && t != "nop" && t != "ret") {
                            P3Variable^ newVar = gcnew P3Variable();
                            newVar->Name = baseTag;
                            newVar->Type = "Bool";
                            newVar->Scope = "internal";
                            newVar->Comment = "Auto-generated for undefined tag";
                            newVars->Add(newVar);
                            definedVars->Add(baseTag);
                        }
                    }
                }
            }
        }
        for each (P3Variable^ v in newVars) {
            dsl->Variables->Add(v);
        }
    }
};

ref class P3TagTableGenerator {
public:
    static String^ GenerateTagTableXml(P3Dsl^ dsl) {
        Dictionary<String^, P3Variable^>^ varMap = gcnew Dictionary<String^, P3Variable^>();

        if (dsl->Variables != nullptr) {
            for each (P3Variable^ v in dsl->Variables) {
                if (v->Name == nullptr || v->Name->Length == 0) continue;
                if (!varMap->ContainsKey(v->Name)) varMap[v->Name] = v;
            }
        }

        if (dsl->Networks != nullptr) {
            for each (P3Network^ net in dsl->Networks) {
                CollectTagsFromNetwork(net, varMap);
            }
        }

        if (varMap->Count == 0) return "";

        List<String^>^ sortedNames = gcnew List<String^>(varMap->Keys);
        sortedNames->Sort();

        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        sb->AppendLine("<Document>");
        sb->AppendLine("  <SW.Tags.PlcTagTable ID=\"0\">");
        sb->AppendLine("    <AttributeList>");
        sb->AppendLine("      <Name>DefaultTagTable</Name>");
        sb->AppendLine("    </AttributeList>");
        sb->AppendLine("    <ObjectList>");

        int tagId = 1;
        int commentId = 100;
        int itemId = 200;
        int inputAddr = 0;
        int outputAddr = 0;
        int markerAddr = 0;

        for each (String^ name in sortedNames) {
            P3Variable^ v = varMap[name];

            String^ dataType = GetDataType(v);
            String^ address = GetAddress(v, inputAddr, outputAddr, markerAddr);

            String^ comment = v->Comment;
            if (comment == nullptr || comment->Length == 0) {
                comment = TranslateVariableName(v);
            }
            else {
                bool nameIsChinese = false;
                for each (Char ch in v->Name) {
                    if (ch > 0x4E00) { nameIsChinese = true; break; }
                }
                if (nameIsChinese && comment->Length > 0) {
                    bool commentIsEnglish = true;
                    for each (Char ch in comment) {
                        if (ch > 0x4E00) { commentIsEnglish = false; break; }
                    }
                    if (commentIsEnglish) {
                        comment = v->Name + "(" + comment + ")";
                    }
                }
            }

            sb->AppendLine("      <SW.Tags.PlcTag ID=\"" + tagId + "\" CompositionName=\"Tags\">");
            sb->AppendLine("        <AttributeList>");
            sb->AppendLine("          <DataTypeName>" + dataType + "</DataTypeName>");
            sb->AppendLine("          <ExternalAccessible>true</ExternalAccessible>");
            sb->AppendLine("          <ExternalVisible>true</ExternalVisible>");
            sb->AppendLine("          <ExternalWritable>true</ExternalWritable>");
            if (address->Length > 0) {
                sb->AppendLine("          <LogicalAddress>%" + address + "</LogicalAddress>");
            }
            sb->AppendLine("          <Name>" + EscapeXml(v->Name) + "</Name>");
            sb->AppendLine("        </AttributeList>");
            sb->AppendLine("        <ObjectList>");
            sb->AppendLine("          <MultilingualText ID=\"" + commentId + "\" CompositionName=\"Comment\">");
            sb->AppendLine("            <ObjectList>");
            sb->AppendLine("              <MultilingualTextItem ID=\"" + itemId + "\" CompositionName=\"Items\">");
            sb->AppendLine("                <AttributeList>");
            sb->AppendLine("                  <Culture>zh-CN</Culture>");
            sb->AppendLine("                  <Text>" + EscapeXml(comment) + "</Text>");
            sb->AppendLine("                </AttributeList>");
            sb->AppendLine("              </MultilingualTextItem>");
            sb->AppendLine("            </ObjectList>");
            sb->AppendLine("          </MultilingualText>");
            sb->AppendLine("        </ObjectList>");
            sb->AppendLine("      </SW.Tags.PlcTag>");
            tagId++;
            commentId++;
            itemId++;
        }

        sb->AppendLine("    </ObjectList>");
        sb->AppendLine("  </SW.Tags.PlcTagTable>");
        sb->AppendLine("</Document>");

        return sb->ToString();
    }

private:
    static void CollectTagsFromNetwork(P3Network^ net, Dictionary<String^, P3Variable^>^ varMap) {
        if (net == nullptr || net->Items == nullptr) return;
        for each (Object^ item in net->Items) {
            P3Node^ node = dynamic_cast<P3Node^>(item);
            P3Parallel^ par = dynamic_cast<P3Parallel^>(item);
            if (node != nullptr) {
                CollectTagsFromNode(node, varMap);
            }
            else if (par != nullptr) {
                if (par->Branches != nullptr) {
                    for each (P3Branch^ branch in par->Branches) {
                        if (branch->Nodes != nullptr) {
                            for each (P3Node^ bn in branch->Nodes) {
                                CollectTagsFromNode(bn, varMap);
                            }
                        }
                    }
                }
            }
        }
    }

    static void CollectTagsFromNode(P3Node^ n, Dictionary<String^, P3Variable^>^ varMap) {
        AddTagToMap(n->Tag, n, varMap);
        if (n->Tag2 != nullptr && n->Tag2->Length > 0) AddTagToMap(n->Tag2, n, varMap);
        if (n->Tag3 != nullptr && n->Tag3->Length > 0) AddTagToMap(n->Tag3, n, varMap);
        if (n->Instance != nullptr && n->Instance->Length > 0) AddTagToMap(n->Instance, n, varMap);
    }

    static void AddTagToMap(String^ tag, P3Node^ node, Dictionary<String^, P3Variable^>^ varMap) {
        if (tag == nullptr || tag->Length == 0) return;
        if (varMap->ContainsKey(tag)) return;

        if (tag->Length > 0 && !Char::IsLetter(tag[0]) && tag[0] != '_' && tag[0] < 0x4E00) return;

        for each (Char c in tag) {
            if (!Char::IsLetterOrDigit(c) && c != '_' && c < 0x4E00) return;
        }

        if (tag == "on" || tag == "off" || tag == "both" || tag == "all" ||
            tag == "and" || tag == "or" || tag == "not" || tag == "the" ||
            tag == "to" || tag == "from" || tag == "with" || tag == "for") return;

        P3Variable^ v = gcnew P3Variable();
        v->Name = tag;
        v->Type = "Bool";
        v->Scope = "internal";
        v->Comment = "";

        String^ nt = (node->NodeType != nullptr) ? node->NodeType->ToLower() : "";
        if (nt == "coil" || nt == "set" || nt == "reset" || nt == "setcoil" || nt == "resetcoil" ||
            nt == "s_coil" || nt == "r_coil" || nt == "scoil" || nt == "rcoil") {
            v->Scope = "output";
        }
        else if (nt == "contact" || nt == "negated_contact" || nt == "risingedge" || nt == "fallingedge") {
            v->Scope = "input";
        }
        else if (nt == "ton" || nt == "tof" || nt == "tp") {
            v->Type = "Timer";
            v->Scope = "internal";
        }
        else if (nt == "ctu" || nt == "ctd" || nt == "ctud") {
            v->Type = "Counter";
            v->Scope = "internal";
        }
        else if (nt == "move") {
            v->Type = (node->DataType != nullptr && node->DataType->Length > 0) ? node->DataType : "Int";
            v->Scope = "internal";
        }
        else if (nt->Contains("compare") || nt->Contains("add") || nt->Contains("sub") ||
                 nt->Contains("mul") || nt->Contains("div") || nt->Contains("mod")) {
            v->Type = (node->DataType != nullptr && node->DataType->Length > 0) ? node->DataType : "Int";
        }

        varMap[tag] = v;
    }
    static String^ GetDataType(P3Variable^ v) {
        if (v->Type == nullptr) return "Bool";
        String^ t = v->Type->ToLower();
        if (t == "timer" || t == "ton" || t == "tof" || t == "tp") return "Timer";
        if (t == "counter" || t == "ctu" || t == "ctd" || t == "ctud") return "Counter";
        if (t == "int" || t == "integer") return "Int";
        if (t == "dint") return "DInt";
        if (t == "real") return "Real";
        if (t == "word") return "Word";
        if (t == "dword") return "DWord";
        if (t == "byte") return "Byte";
        if (t == "bool" || t == "boolean") return "Bool";
        return "Bool";
    }

    static String^ GetAddress(P3Variable^ v, int& inputAddr, int& outputAddr, int& markerAddr) {
        if (v->Scope == nullptr) return "";
        String^ scope = v->Scope->ToLower();

        if (scope == "input") {
            String^ addr = "I" + inputAddr + ".0";
            inputAddr++;
            return addr;
        }
        if (scope == "output") {
            String^ addr = "Q" + outputAddr + ".0";
            outputAddr++;
            return addr;
        }
        if (scope == "internal" || scope == "memory" || scope == "marker") {
            String^ dataType = (v->Type != nullptr) ? v->Type->ToLower() : "bool";
            if (dataType == "timer") return "";
            if (dataType == "counter") return "";
            String^ addr = "M" + markerAddr + ".0";
            markerAddr++;
            return addr;
        }
        return "";
    }

    static String^ TranslateVariableName(P3Variable^ v) {
        if (v->Name == nullptr) return "";
        String^ n = v->Name;
        String^ nl = n->ToLower();

        if (nl->Contains("start_btn") || nl->Contains("startbtn")) return L"启动按钮";
        if (nl->Contains("stop_btn") || nl->Contains("stopbtn")) return L"停止按钮";
        if (nl == "sa") return L"手动/自动选择开关";
        if (nl->Contains("sa_") || nl->Contains("mode")) return L"模式选择开关";
        if (nl->Contains("jog_fwd") || nl->Contains("jog_forward")) return L"点动正转按钮";
        if (nl->Contains("jog_rev") || nl->Contains("jog_reverse")) return L"点动反转按钮";
        if (nl->Contains("motor_fwd") || nl->Contains("motor_forward") || nl->Contains("move_fwd") || nl->Contains("move_right")) return L"电动机正转";
        if (nl->Contains("motor_rev") || nl->Contains("motor_reverse") || nl->Contains("move_rev") || nl->Contains("move_left")) return L"电动机反转";
        if (nl->Contains("limit_e")) return L"原位限位开关E";
        if (nl->Contains("limit_f")) return L"卸料处限位开关F";
        if (nl->Contains("limit_g")) return L"装料处限位开关G";
        if (nl->Contains("limit_h")) return L"清洗处限位开关H";
        if (nl->Contains("limit_r") || nl->Contains("right_limit")) return L"右限位开关";
        if (nl->Contains("limit_l") || nl->Contains("left_limit")) return L"左限位开关";
        if (nl->Contains("limit_")) return L"限位开关";
        if (nl->Contains("fill") || nl->Contains("load")) return L"装料";
        if (nl->Contains("drain") || nl->Contains("unload")) return L"卸料";
        if (nl->Contains("clean") || nl->Contains("wash")) return L"清洗";
        if (nl->Contains("heat")) return L"加热";
        if (nl->Contains("valve")) return L"阀门";
        if (nl->Contains("door")) return L"门";
        if (nl->Contains("auto_mode")) return L"自动模式";
        if (nl->Contains("manual_mode")) return L"手动模式";
        if (nl->Contains("cycle_done")) return L"循环完成";
        if (nl->StartsWith("step")) return L"步骤" + n->Substring(4);
        if (nl->Contains("timer") || nl->Contains("delay")) return L"定时器";
        if (nl->Contains("counter") || nl->Contains("count")) return L"计数器";

        return n;
    }

    static String^ EscapeXml(String^ s) {
        if (s == nullptr) return "";
        return s->Replace("&", "&amp;")->Replace("<", "&lt;")->Replace(">", "&gt;")->Replace("\"", "&quot;");
    }
};

ref struct P3PipelineResult {
    String^ Xml;
    String^ TagTableXml;
    P3Dsl^ Dsl;
    bool Success;
    String^ ErrorMessage;

    P3PipelineResult() {
        Success = false;
        ErrorMessage = "";
        TagTableXml = "";
    }
};

ref class P3Pipeline {
public:
    static P3PipelineResult^ RunPhase4(String^ problem, P3Config^ config, String^ templateXmlPath) {
        return RunPhase4(problem, config, templateXmlPath, nullptr);
    }

    static P3PipelineResult^ RunPhase4(String^ problem, P3Config^ config, String^ templateXmlPath, Action<String^>^ uiLog) {
        Console::WriteLine("=== Phase 4 AI PLC Pipeline (Multi-Step Reasoning) ===");
        Console::WriteLine();
        Console::WriteLine("  Problem: " + problem);
        Console::WriteLine();

        if (uiLog != nullptr) uiLog("[INFO] Phase4 Step1: 需求解析中...");
        Console::WriteLine("[Step 1] Requirement Parsing...");
        P4Requirement^ requirement = P4RequirementParser::Parse(problem, config);
        if (requirement->Inputs->Count == 0 && requirement->Outputs->Count == 0) {
            Console::WriteLine("  [Step 1] Requirement parsing failed, falling back to direct DSL generation");
            if (uiLog != nullptr) uiLog("[INFO] 需求解析失败，回退到直接DSL生成");
            return Run(problem, config, templateXmlPath);
        }
        if (uiLog != nullptr) uiLog("[INFO] 需求解析: 输入" + requirement->Inputs->Count + "个, 输出" + requirement->Outputs->Count + "个, 类型=" + requirement->ControlType);

        Console::WriteLine();
        Console::WriteLine("[Step 2] Semantic Planning...");
        if (uiLog != nullptr) uiLog("[INFO] Phase4 Step2: 语义规划中...");
        P4SemanticPlan^ semanticPlan = P4SemanticPlanner::Plan(problem, requirement, config);
        if (semanticPlan->States->Count == 0) {
            Console::WriteLine("  [Step 2] Semantic planning returned no states, falling back to direct DSL");
            if (uiLog != nullptr) uiLog("[INFO] 语义规划无状态，回退到直接DSL生成");
            return Run(problem, config, templateXmlPath);
        }
        if (uiLog != nullptr) uiLog("[INFO] 语义规划: " + semanticPlan->States->Count + "个状态, 顺序=" + semanticPlan->IsSequential + ", 流程=" + semanticPlan->FlowDescription);

        Console::WriteLine();
        Console::WriteLine("[Step 3] Variable Planning...");
        if (uiLog != nullptr) uiLog("[INFO] Phase4 Step3: 变量规划中...");
        P4VariablePlan^ varPlan = P4VariablePlanner::Plan(requirement, semanticPlan);

        Console::WriteLine();
        Console::WriteLine("[Step 4] State Machine Building + LogicGraph Generation...");
        Console::WriteLine("  [Architecture] AI Semantic -> LogicGraph -> Compiler -> XML");
        if (uiLog != nullptr) uiLog("[INFO] Phase4 Step4: 状态机构建中...");
        P3Dsl^ dsl = P4StateMachineBuilder::Build(requirement, semanticPlan, varPlan);
        Console::WriteLine("  Variables: " + dsl->Variables->Count + ", Networks: " + dsl->Networks->Count);
        if (uiLog != nullptr) uiLog("[INFO] DSL生成: " + dsl->Variables->Count + "个变量, " + dsl->Networks->Count + "个网络");

        if (dsl->AiFallbackNeeded) {
            Console::WriteLine();
            Console::WriteLine("[Step 4b] AI-Enhanced Fallback Generation...");
            Console::WriteLine("  Specialized generators could not fully handle this control pattern");
            Console::WriteLine("  Falling back to AI-enhanced DSL generation with requirement context...");
            if (uiLog != nullptr) uiLog("[INFO] 专用生成器无法完全处理，回退到AI增强生成...");

            P3Dsl^ aiDsl = P4AiEnhancedGenerator::GenerateFromContext(problem, requirement, semanticPlan, varPlan, config);
            if (aiDsl != nullptr && aiDsl->Networks->Count > 0) {
                Console::WriteLine("  AI generated " + aiDsl->Networks->Count + " networks, merging with existing...");
                if (uiLog != nullptr) uiLog("[INFO] AI生成了" + aiDsl->Networks->Count + "个网络，合并中...");

                for each (P3Variable^ v in aiDsl->Variables) {
                    bool exists = false;
                    for each (P3Variable^ ev in dsl->Variables) {
                        if (ev->Name == v->Name) { exists = true; break; }
                    }
                    if (!exists) dsl->Variables->Add(v);
                }

                if (dsl->Networks->Count == 0) {
                    dsl->Networks = aiDsl->Networks;
                }
                else {
                    HashSet<String^>^ existingCoils = gcnew HashSet<String^>();
                    for each (P3Network^ net in dsl->Networks) {
                        for each (Object^ item in net->Items) {
                            P3Node^ n = dynamic_cast<P3Node^>(item);
                            if (n != nullptr && n->NodeType->ToLower() == "coil" && n->Tag != nullptr) {
                                existingCoils->Add(n->Tag->ToLower());
                            }
                        }
                    }
                    for each (P3Network^ aiNet in aiDsl->Networks) {
                        bool hasNewCoil = false;
                        for each (Object^ item in aiNet->Items) {
                            P3Node^ n = dynamic_cast<P3Node^>(item);
                            if (n != nullptr && n->NodeType->ToLower() == "coil" && n->Tag != nullptr) {
                                if (!existingCoils->Contains(n->Tag->ToLower())) {
                                    hasNewCoil = true;
                                    break;
                                }
                            }
                        }
                        if (hasNewCoil) {
                            dsl->Networks->Add(aiNet);
                        }
                    }
                }

                for each (P3Step^ s in aiDsl->Steps) {
                    bool exists = false;
                    for each (P3Step^ es in dsl->Steps) {
                        if (es->Name == s->Name) { exists = true; break; }
                    }
                    if (!exists) dsl->Steps->Add(s);
                }
                for each (P3TimerDecl^ t in aiDsl->Timers) {
                    bool exists = false;
                    for each (P3TimerDecl^ et in dsl->Timers) {
                        if (et->Name == t->Name) { exists = true; break; }
                    }
                    if (!exists) dsl->Timers->Add(t);
                }
                for each (P3CounterDecl^ c in aiDsl->Counters) {
                    bool exists = false;
                    for each (P3CounterDecl^ ec in dsl->Counters) {
                        if (ec->Name == c->Name) { exists = true; break; }
                    }
                    if (!exists) dsl->Counters->Add(c);
                }

                Console::WriteLine("  After merge: " + dsl->Variables->Count + " variables, " + dsl->Networks->Count + " networks");
                if (uiLog != nullptr) uiLog("[INFO] 合并后: " + dsl->Variables->Count + "个变量, " + dsl->Networks->Count + "个网络");
            }
            else {
                Console::WriteLine("  AI-enhanced generation also failed, proceeding with best-effort...");
                if (uiLog != nullptr) uiLog("[WARN] AI增强生成也失败，尽力继续...");
            }
        }

        Console::WriteLine();
        Console::WriteLine("[Step 5] Auto-generating missing variables...");
        P3VariableEngine::AutoGenerateVariables(dsl);
        Console::WriteLine("  Variables after auto-gen: " + dsl->Variables->Count);

        Console::WriteLine();
        Console::WriteLine("[Step 6] Validating DSL...");
        if (uiLog != nullptr) uiLog("[INFO] Phase4 Step6: DSL验证中...");
        P3ValidationResult^ validation = P3SemanticValidator::Validate(dsl);
        if (validation->Warnings->Count > 0) {
            Console::WriteLine("  Warnings:");
            for each (String^ w in validation->Warnings) Console::WriteLine("    - " + w);
        }
        if (!validation->IsValid) {
            Console::WriteLine("  Errors:");
            for each (String^ e in validation->Errors) Console::WriteLine("    - " + e);

            Console::WriteLine();
            Console::WriteLine("[Step 6b] Attempting auto-fix...");
            String^ fixResult = P3SemanticValidator::AutoFix(dsl);
            if (fixResult->Length > 0) Console::WriteLine("  " + fixResult);
            P3RepairEngine::RepairUndefinedVariables(dsl);

            validation = P3SemanticValidator::Validate(dsl);
            if (!validation->IsValid) {
                Console::WriteLine();
                Console::WriteLine("[Step 6c] Attempting LLM repair...");
                String^ errorStr = String::Join("\n", validation->Errors);
                dsl = P3RepairEngine::RepairFromErrors(dsl, errorStr, config);
                validation = P3SemanticValidator::Validate(dsl);
            }
        }
        if (validation->IsValid) {
            Console::WriteLine("  Validation: PASSED");
        }
        else {
            Console::WriteLine("  Validation: FAILED (proceeding with best-effort)");
        }

        Console::WriteLine();
        Console::WriteLine("[Step 7] Converting DSL to LAD format...");
        LadDsl^ ladDsl = P3DslConverter::ToLadDsl(dsl);
        Console::WriteLine("  LAD Networks: " + ladDsl->Networks->Count);

        Console::WriteLine();
        Console::WriteLine("[Step 8] Compiling LAD to TIA XML...");
        String^ xml = BuildLadXml(ladDsl, templateXmlPath);
        if (xml == nullptr || xml->Length == 0) {
            P3PipelineResult^ r = gcnew P3PipelineResult();
            r->Dsl = dsl;
            r->ErrorMessage = "Failed to compile LAD to XML";
            return r;
        }
        Console::WriteLine("  XML generated (" + xml->Length + " chars)");

        Console::WriteLine();
        Console::WriteLine("=== Phase 4 Pipeline Complete ===");
        P3PipelineResult^ result = gcnew P3PipelineResult();
        result->Xml = xml;
        result->Dsl = dsl;
        result->TagTableXml = P3TagTableGenerator::GenerateTagTableXml(dsl);
        result->Success = true;
        return result;
    }

    static P3PipelineResult^ Run(String^ problem, P3Config^ config, String^ templateXmlPath) {
        Console::WriteLine("=== Phase 3 AI PLC Pipeline ===");
        Console::WriteLine();
        Console::WriteLine("[Step 1] Sending problem to LLM...");
        Console::WriteLine("  Problem: " + problem);
        Console::WriteLine();

        String^ systemPrompt = P3PromptEngine::BuildSystemPrompt();
        String^ userPrompt = P3PromptEngine::BuildUserPrompt(problem);

        String^ llmResponse = P3LlmClient::CallLlm(config, systemPrompt, userPrompt);
        if (llmResponse == nullptr || llmResponse->Length == 0) {
            P3PipelineResult^ r = gcnew P3PipelineResult();
            r->ErrorMessage = "LLM returned empty response";
            return r;
        }

        Console::WriteLine();
        Console::WriteLine("[Step 2] Extracting DSL JSON from LLM response...");
        String^ dslJson = P3DslParser::ExtractJsonFromLlmResponse(llmResponse);
        if (dslJson == nullptr || dslJson->Length == 0) {
            Console::WriteLine("  Raw LLM response:");
            Console::WriteLine(llmResponse->Substring(0, Math::Min(500, llmResponse->Length)));
            P3PipelineResult^ r = gcnew P3PipelineResult();
            r->ErrorMessage = "Could not extract JSON from LLM response";
            return r;
        }
        Console::WriteLine("  DSL JSON extracted (" + dslJson->Length + " chars)");

        Console::WriteLine();
        Console::WriteLine("[Step 3] Parsing DSL...");
        P3Dsl^ dsl = P3DslParser::Parse(dslJson);
        Console::WriteLine("  Variables: " + dsl->Variables->Count);
        Console::WriteLine("  Networks: " + dsl->Networks->Count);

        Console::WriteLine();
        Console::WriteLine("[Step 4] Auto-generating missing variables...");
        P3VariableEngine::AutoGenerateVariables(dsl);
        Console::WriteLine("  Variables after auto-gen: " + dsl->Variables->Count);

        Console::WriteLine();
        Console::WriteLine("[Step 5] Validating DSL...");
        P3ValidationResult^ validation = P3SemanticValidator::Validate(dsl);
        if (validation->Warnings->Count > 0) {
            Console::WriteLine("  Warnings:");
            for each (String^ w in validation->Warnings) {
                Console::WriteLine("    - " + w);
            }
        }

        if (!validation->IsValid) {
            Console::WriteLine("  Errors:");
            for each (String^ e in validation->Errors) {
                Console::WriteLine("    - " + e);
            }

            Console::WriteLine();
            Console::WriteLine("[Step 5b] Attempting auto-fix...");
            String^ fixResult = P3SemanticValidator::AutoFix(dsl);
            if (fixResult->Length > 0) Console::WriteLine("  " + fixResult);

            validation = P3SemanticValidator::Validate(dsl);
            if (!validation->IsValid) {
                Console::WriteLine();
                Console::WriteLine("[Step 5c] Attempting LLM repair...");
                String^ errorStr = String::Join("\n", validation->Errors);
                dsl = P3RepairEngine::RepairFromErrors(dsl, errorStr, config);
                validation = P3SemanticValidator::Validate(dsl);
            }
        }

        if (validation->IsValid) {
            Console::WriteLine("  Validation: PASSED");
        }
        else {
            Console::WriteLine("  Validation: FAILED (proceeding with best-effort)");
            for each (String^ e in validation->Errors) {
                Console::WriteLine("    - " + e);
            }
        }

        Console::WriteLine();
        Console::WriteLine("[Step 6] Converting DSL to LAD format...");
        LadDsl^ ladDsl = P3DslConverter::ToLadDsl(dsl);
        Console::WriteLine("  LAD Networks: " + ladDsl->Networks->Count);

        Console::WriteLine();
        Console::WriteLine("[Step 7] Compiling LAD to TIA XML...");
        String^ xml = BuildLadXml(ladDsl, templateXmlPath);
        if (xml == nullptr || xml->Length == 0) {
            P3PipelineResult^ r = gcnew P3PipelineResult();
            r->Dsl = dsl;
            r->ErrorMessage = "Failed to compile LAD to XML";
            return r;
        }
        Console::WriteLine("  XML generated (" + xml->Length + " chars)");

        Console::WriteLine();
        Console::WriteLine("=== Pipeline Complete ===");
        P3PipelineResult^ result = gcnew P3PipelineResult();
        result->Xml = xml;
        result->Dsl = dsl;
        result->TagTableXml = P3TagTableGenerator::GenerateTagTableXml(dsl);
        result->Success = true;
        return result;
    }

    static P3PipelineResult^ RunFromDslFile(String^ dslJsonPath, String^ templateXmlPath) {
        Console::WriteLine("=== Phase 3 DSL File Pipeline ===");
        Console::WriteLine();

        if (!File::Exists(dslJsonPath)) {
            P3PipelineResult^ r = gcnew P3PipelineResult();
            r->ErrorMessage = "DSL file not found: " + dslJsonPath;
            return r;
        }

        Console::WriteLine("[Step 1] Reading DSL file...");
        String^ dslJson = File::ReadAllText(dslJsonPath, Encoding::UTF8);

        Console::WriteLine("[Step 2] Parsing DSL...");
        P3Dsl^ dsl = P3DslParser::Parse(dslJson);
        Console::WriteLine("  Variables: " + dsl->Variables->Count);
        Console::WriteLine("  Networks: " + dsl->Networks->Count);

        Console::WriteLine("[Step 3] Auto-generating missing variables...");
        P3VariableEngine::AutoGenerateVariables(dsl);

        Console::WriteLine("[Step 4] Validating DSL...");
        P3ValidationResult^ validation = P3SemanticValidator::Validate(dsl);
        if (validation->Warnings->Count > 0) {
            for each (String^ w in validation->Warnings) Console::WriteLine("  Warning: " + w);
        }
        if (!validation->IsValid) {
            for each (String^ e in validation->Errors) Console::WriteLine("  Error: " + e);
            String^ fixResult = P3SemanticValidator::AutoFix(dsl);
            if (fixResult->Length > 0) Console::WriteLine("  " + fixResult);
        }

        Console::WriteLine("[Step 5] Converting DSL to LAD format...");
        LadDsl^ ladDsl = P3DslConverter::ToLadDsl(dsl);

        Console::WriteLine("[Step 6] Compiling LAD to TIA XML...");
        String^ xml = BuildLadXml(ladDsl, templateXmlPath);
        if (xml == nullptr || xml->Length == 0) {
            P3PipelineResult^ r = gcnew P3PipelineResult();
            r->Dsl = dsl;
            r->ErrorMessage = "Failed to compile LAD to XML";
            return r;
        }
        Console::WriteLine("  XML generated (" + xml->Length + " chars)");
        Console::WriteLine("=== Pipeline Complete ===");
        P3PipelineResult^ result = gcnew P3PipelineResult();
        result->Xml = xml;
        result->Dsl = dsl;
        result->TagTableXml = P3TagTableGenerator::GenerateTagTableXml(dsl);
        result->Success = true;
        return result;
    }

    static void SaveDslToFile(P3Dsl^ dsl, String^ path) {
        String^ json = P3DslSerializer::Serialize(dsl);
        File::WriteAllText(path, json, gcnew UTF8Encoding(false));
        Console::WriteLine("DSL saved to: " + path);
    }

    static P3Config^ EnsureConfig(String^ exeDir) {
        String^ configPath = Path::Combine(exeDir, "phase3_config.json");
        P3Config^ config = P3Config::Load(configPath);
        if (config->ApiKey == nullptr || config->ApiKey->Length == 0) {
            Console::WriteLine("Phase 3 configuration needed!");
            Console::WriteLine("Config file: " + configPath);
            Console::WriteLine();
            Console::Write("  Enter API URL (default: " + config->ApiUrl + "): ");
            String^ url = Console::ReadLine()->Trim();
            if (url->Length > 0) config->ApiUrl = url;

            Console::Write("  Enter API Key: ");
            String^ key = Console::ReadLine()->Trim();
            if (key->Length > 0) config->ApiKey = key;

            Console::Write("  Enter Model (default: " + config->Model + "): ");
            String^ model = Console::ReadLine()->Trim();
            if (model->Length > 0) config->Model = model;

            config->Save(configPath);
            Console::WriteLine("Config saved to: " + configPath);
            Console::WriteLine();
        }
        return config;
    }
};

ref class P5CompilerLog {
public:
    static Action<String^>^ UiLogCallback;

    static void Info(String^ stage, String^ msg) {
        String^ line = "[" + stage + "] " + msg;
        Console::WriteLine(line);
        if (UiLogCallback != nullptr) {
            try { UiLogCallback("[INFO] " + line); } catch (...) {}
        }
    }
    static void Warn(String^ stage, String^ msg) {
        String^ line = "[" + stage + " WARN] " + msg;
        Console::WriteLine(line);
        if (UiLogCallback != nullptr) {
            try { UiLogCallback("[WARN] " + line); } catch (...) {}
        }
    }
    static void Error(String^ stage, String^ msg) {
        String^ line = "[" + stage + " ERROR] " + msg;
        Console::WriteLine(line);
        if (UiLogCallback != nullptr) {
            try { UiLogCallback("[ERROR] " + line); } catch (...) {}
        }
    }
};

ref class P5DslToIrConverter {
public:
    static P5IRProgram^ Convert(P3Dsl^ dsl) {
        P5CompilerLog::Info("IR", "Converting DSL to P5 IR...");
        P5IRProgram^ program = gcnew P5IRProgram();
        program->Name = "Main";
        program->Networks = gcnew List<P5IRNetwork^>();
        program->VariableNames = gcnew List<String^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr) program->VariableNames->Add(v->Name);
        }

        int uid = 1;
        for each (P3Network^ net in dsl->Networks) {
            P5IRNetwork^ irNet = gcnew P5IRNetwork();
            irNet->Title = net->Title;
            irNet->Instructions = gcnew List<P5IRInstruction^>();

            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node == nullptr) continue;
                String^ t = node->NodeType->ToLower();

                if (t == "contact") {
                    P5IRContact^ c = gcnew P5IRContact();
                    c->UId = uid++;
                    c->Tag = node->Tag;
                    c->Negated = !node->NormallyOpen;
                    irNet->Instructions->Add(c);
                }
                else if (t == "coil") {
                    P5IRCoil^ c = gcnew P5IRCoil();
                    c->UId = uid++;
                    c->Tag = node->Tag;
                    c->CoilType = P5CoilType::Normal;
                    irNet->Instructions->Add(c);
                }
                else if (t == "set" || t == "scoil") {
                    P5IRCoil^ c = gcnew P5IRCoil();
                    c->UId = uid++;
                    c->Tag = node->Tag;
                    c->CoilType = P5CoilType::Set;
                    irNet->Instructions->Add(c);
                }
                else if (t == "reset" || t == "rcoil") {
                    P5IRCoil^ c = gcnew P5IRCoil();
                    c->UId = uid++;
                    c->Tag = node->Tag;
                    c->CoilType = P5CoilType::Reset;
                    irNet->Instructions->Add(c);
                }
                else if (t == "ton" || t == "tof" || t == "tp" || t == "ctu" || t == "ctd" || t == "ctud" ||
                         t == "move" || t == "add" || t == "sub" || t == "mul" || t == "div" ||
                         t == "compare_eq" || t == "compare_ne" || t == "compare_gt" || t == "compare_lt" || t == "compare_ge" || t == "compare_le") {
                    P5IRCall^ call = gcnew P5IRCall();
                    call->UId = uid++;
                    call->Name = node->NodeType;
                    call->Parameters = gcnew Dictionary<String^, String^>();
                    if (node->Instance != nullptr && node->Instance->Length > 0)
                        call->Parameters["instance"] = node->Instance;
                    if (node->Pt != nullptr && node->Pt->Length > 0)
                        call->Parameters["PT"] = node->Pt;
                    if (node->Pv != nullptr && node->Pv->Length > 0)
                        call->Parameters["PV"] = node->Pv;
                    if (node->DataType != nullptr && node->DataType->Length > 0)
                        call->Parameters["dataType"] = node->DataType;
                    irNet->Instructions->Add(call);
                }
            }
            program->Networks->Add(irNet);
        }

        P5CompilerLog::Info("IR", "Converted " + program->Networks->Count + " networks, " + uid + " instructions");
        return program;
    }
};

ref class P5WireRouter {
public:
    static List<P5IRInstruction^>^ RouteWires(P5IRNetwork^ net) {
        List<P5IRInstruction^>^ ordered = gcnew List<P5IRInstruction^>();
        List<P5IRContact^>^ contacts = gcnew List<P5IRContact^>();
        List<P5IRInstruction^>^ outputs = gcnew List<P5IRInstruction^>();
        List<P5IRBranch^>^ branches = gcnew List<P5IRBranch^>();

        for each (P5IRInstruction^ instr in net->Instructions) {
            P5IRContact^ c = dynamic_cast<P5IRContact^>(instr);
            if (c != nullptr) { contacts->Add(c); continue; }
            P5IRBranch^ b = dynamic_cast<P5IRBranch^>(instr);
            if (b != nullptr) { branches->Add(b); continue; }
            outputs->Add(instr);
        }

        for each (P5IRContact^ c in contacts) ordered->Add(c);
        for each (P5IRBranch^ b in branches) ordered->Add(b);
        for each (P5IRInstruction^ o in outputs) ordered->Add(o);
        return ordered;
    }
};

ref class P5BranchBuilder {
public:
    static P5IRBranch^ BuildOrBranch(List<P5IRContact^>^ parallelContacts) {
        P5IRBranch^ branch = gcnew P5IRBranch();
        branch->UId = 0;
        branch->Children = gcnew List<P5IRInstruction^>();
        for each (P5IRContact^ c in parallelContacts) branch->Children->Add(c);
        return branch;
    }
};

ref class P5BlockLayoutEngine {
public:
    static int CalculateWidth(P5IRInstruction^ instr) {
        P5IRContact^ c = dynamic_cast<P5IRContact^>(instr);
        if (c != nullptr) return 1;
        P5IRCoil^ coil = dynamic_cast<P5IRCoil^>(instr);
        if (coil != nullptr) return 1;
        P5IRCall^ call = dynamic_cast<P5IRCall^>(instr);
        if (call != nullptr) {
            String^ n = call->Name->ToUpper();
            if (n == "TON" || n == "TOF" || n == "TP" || n == "CTU" || n == "CTD" || n == "CTUD") return 2;
            return 2;
        }
        P5IRBranch^ b = dynamic_cast<P5IRBranch^>(instr);
        if (b != nullptr) return 1;
        return 1;
    }

    static int CalculateNetworkWidth(P5IRNetwork^ net) {
        int w = 0;
        for each (P5IRInstruction^ instr in net->Instructions) w += CalculateWidth(instr);
        return w;
    }
};

ref class P5PowerrailGenerator {
public:
    static String^ GeneratePowerrail(int networkCount) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("  <Powerrail>");
        for (int i = 0; i < networkCount; i++) {
            sb->AppendLine("    <Con i0=\"" + i + "\"/>");
        }
        sb->AppendLine("  </Powerrail>");
        return sb->ToString();
    }
};

ref class P5ConnectionResolver {
public:
    static Dictionary<int, List<int>^>^ ResolveConnections(P5IRNetwork^ net) {
        Dictionary<int, List<int>^>^ connections = gcnew Dictionary<int, List<int>^>();
        List<P5IRInstruction^>^ ordered = P5WireRouter::RouteWires(net);
        for (int i = 0; i < ordered->Count - 1; i++) {
            connections[ordered[i]->UId] = gcnew List<int>();
            connections[ordered[i]->UId]->Add(ordered[i + 1]->UId);
        }
        return connections;
    }
};

ref class P5LadBackend : P5IBackend {
public:
    virtual String^ Generate(P5IRProgram^ program) {
        P5CompilerLog::Info("LAD", "Generating LAD XML from P5 IR...");
        StringBuilder^ sb = gcnew StringBuilder();

        sb->AppendLine(P5PowerrailGenerator::GeneratePowerrail(program->Networks->Count));

        for each (P5IRNetwork^ net in program->Networks) {
            sb->AppendLine("Network: " + net->Title);
            int width = P5BlockLayoutEngine::CalculateNetworkWidth(net);
            sb->AppendLine("  Layout width: " + width);

            List<P5IRInstruction^>^ routed = P5WireRouter::RouteWires(net);
            Dictionary<int, List<int>^>^ connections = P5ConnectionResolver::ResolveConnections(net);

            for each (P5IRInstruction^ instr in routed) {
                P5IRContact^ contact = dynamic_cast<P5IRContact^>(instr);
                if (contact != nullptr) {
                    sb->AppendLine("  " + (contact->Negated ? "NC " : "NO ") + "Contact: " + contact->Tag);
                    continue;
                }
                P5IRCoil^ coil = dynamic_cast<P5IRCoil^>(instr);
                if (coil != nullptr) {
                    String^ coilType = "";
                    if (coil->CoilType == P5CoilType::Set) coilType = "(S) ";
                    else if (coil->CoilType == P5CoilType::Reset) coilType = "(R) ";
                    else if (coil->CoilType == P5CoilType::Negated) coilType = "(/) ";
                    sb->AppendLine("  Coil" + coilType + ": " + coil->Tag);
                    continue;
                }
                P5IRCall^ call = dynamic_cast<P5IRCall^>(instr);
                if (call != nullptr) {
                    int bw = P5BlockLayoutEngine::CalculateWidth(call);
                    sb->Append("  Call[" + bw + "]: " + call->Name + "(");
                    bool first = true;
                    for each (KeyValuePair<String^, String^>^ kv in call->Parameters) {
                        if (!first) sb->Append(", ");
                        sb->Append(kv->Key + "=" + kv->Value);
                        first = false;
                    }
                    sb->AppendLine(")");
                    continue;
                }
                P5IRBranch^ branch = dynamic_cast<P5IRBranch^>(instr);
                if (branch != nullptr) {
                    sb->AppendLine("  Branch: " + branch->Children->Count + " paths");
                }
            }
        }
        return sb->ToString();
    }
};

ref class P5SclBackend : P5IBackend {
public:
    virtual String^ Generate(P5IRProgram^ program) {
        P5CompilerLog::Info("SCL", "Generating SCL code from P5 IR...");
        StringBuilder^ sb = gcnew StringBuilder();

        sb->AppendLine("ORGANIZATION_BLOCK \"Main\"");
        sb->AppendLine("BEGIN");

        for each (P5IRNetwork^ net in program->Networks) {
            sb->AppendLine();
            sb->AppendLine("// " + net->Title);

            List<P5IRContact^>^ conditions = gcnew List<P5IRContact^>();
            List<P5IRInstruction^>^ actions = gcnew List<P5IRInstruction^>();

            for each (P5IRInstruction^ instr in net->Instructions) {
                P5IRContact^ c = dynamic_cast<P5IRContact^>(instr);
                if (c != nullptr) {
                    conditions->Add(c);
                    continue;
                }
                P5IRBranch^ branch = dynamic_cast<P5IRBranch^>(instr);
                if (branch != nullptr) {
                    for each (P5IRInstruction^ child in branch->Children) {
                        P5IRContact^ bc = dynamic_cast<P5IRContact^>(child);
                        if (bc != nullptr) conditions->Add(bc);
                        else actions->Add(child);
                    }
                    continue;
                }
                actions->Add(instr);
            }

            if (conditions->Count > 0 && actions->Count > 0) {
                sb->Append("IF ");
                for (int i = 0; i < conditions->Count; i++) {
                    if (i > 0) sb->Append(" AND ");
                    P5IRContact^ c = conditions[i];
                    if (c->Negated) sb->Append("NOT ");
                    sb->Append("\"" + c->Tag + "\"");
                }
                sb->AppendLine(" THEN");

                for each (P5IRInstruction^ act in actions) {
                    P5IRCoil^ coil = dynamic_cast<P5IRCoil^>(act);
                    if (coil != nullptr) {
                        if (coil->CoilType == P5CoilType::Set)
                            sb->AppendLine("    \"" + coil->Tag + "\" := TRUE;");
                        else if (coil->CoilType == P5CoilType::Reset)
                            sb->AppendLine("    \"" + coil->Tag + "\" := FALSE;");
                        else
                            sb->AppendLine("    \"" + coil->Tag + "\" := TRUE;");
                        continue;
                    }
                    P5IRCall^ call = dynamic_cast<P5IRCall^>(act);
                    if (call != nullptr) {
                        String^ sclCall = ConvertCallToScl(call);
                        sb->AppendLine("    " + sclCall);
                    }
                }
                sb->AppendLine("END_IF;");
            }
            else if (actions->Count > 0) {
                for each (P5IRInstruction^ act in actions) {
                    P5IRCoil^ coil = dynamic_cast<P5IRCoil^>(act);
                    if (coil != nullptr) {
                        sb->AppendLine("    \"" + coil->Tag + "\" := " + (coil->CoilType == P5CoilType::Reset ? "FALSE" : "TRUE") + ";");
                        continue;
                    }
                    P5IRCall^ call = dynamic_cast<P5IRCall^>(act);
                    if (call != nullptr) {
                        sb->AppendLine("    " + ConvertCallToScl(call));
                    }
                }
            }
        }

        sb->AppendLine();
        sb->AppendLine("END_ORGANIZATION_BLOCK");
        return sb->ToString();
    }

private:
    static String^ ConvertCallToScl(P5IRCall^ call) {
        String^ name = call->Name->ToUpper();
        if (name == "TON" || name == "TOF" || name == "TP") {
            String^ instance = GetParam(call, "instance", "T1");
            String^ pt = GetParam(call, "PT", "T#1S");
            String^ inVar = "TRUE";
            return "\"" + instance + "\"(IN := " + inVar + ", PT := " + pt + ");";
        }
        if (name == "CTU" || name == "CTD" || name == "CTUD") {
            String^ instance = GetParam(call, "instance", "C1");
            String^ pv = GetParam(call, "PV", "1");
            String^ cuVar = "TRUE";
            return "\"" + instance + "\"(CU := " + cuVar + ", PV := " + pv + ");";
        }
        if (name == "MOVE") {
            String^ instance = GetParam(call, "instance", "");
            return "MOVE(IN := " + (instance->Length > 0 ? instance : "0") + ");";
        }
        StringBuilder^ ps = gcnew StringBuilder();
        ps->Append(call->Name + "(");
        bool first = true;
        for each (KeyValuePair<String^, String^>^ kv in call->Parameters) {
            if (!first) ps->Append(", ");
            ps->Append(kv->Key + " := " + kv->Value);
            first = false;
        }
        ps->Append(");");
        return ps->ToString();
    }

    static String^ GetParam(P5IRCall^ call, String^ key, String^ defaultVal) {
        if (call->Parameters != nullptr && call->Parameters->ContainsKey(key))
            return call->Parameters[key];
        return defaultVal;
    }
};

ref class P5FbdBackend : P5IBackend {
public:
    virtual String^ Generate(P5IRProgram^ program) {
        P5CompilerLog::Info("FBD", "Generating FBD representation from P5 IR...");
        StringBuilder^ sb = gcnew StringBuilder();

        for each (P5IRNetwork^ net in program->Networks) {
            sb->AppendLine("Network: " + net->Title);
            for each (P5IRInstruction^ instr in net->Instructions) {
                P5IRContact^ contact = dynamic_cast<P5IRContact^>(instr);
                if (contact != nullptr) {
                    sb->AppendLine("  [AND] " + (contact->Negated ? "NOT " : "") + contact->Tag);
                    continue;
                }
                P5IRCoil^ coil = dynamic_cast<P5IRCoil^>(instr);
                if (coil != nullptr) {
                    String^ ct = "=";
                    if (coil->CoilType == P5CoilType::Set) ct = "S";
                    else if (coil->CoilType == P5CoilType::Reset) ct = "R";
                    sb->AppendLine("  [" + ct + "] " + coil->Tag);
                    continue;
                }
                P5IRCall^ call = dynamic_cast<P5IRCall^>(instr);
                if (call != nullptr) {
                    sb->AppendLine("  [FN] " + call->Name);
                }
            }
        }
        return sb->ToString();
    }
};

ref class P5StlBackend : P5IBackend {
public:
    virtual String^ Generate(P5IRProgram^ program) {
        P5CompilerLog::Info("STL", "Generating STL code from P5 IR...");
        StringBuilder^ sb = gcnew StringBuilder();

        for each (P5IRNetwork^ net in program->Networks) {
            sb->AppendLine("// " + net->Title);

            for each (P5IRInstruction^ instr in net->Instructions) {
                P5IRContact^ contact = dynamic_cast<P5IRContact^>(instr);
                if (contact != nullptr) {
                    if (contact->Negated)
                        sb->AppendLine("      AN    \"" + contact->Tag + "\"");
                    else
                        sb->AppendLine("      A     \"" + contact->Tag + "\"");
                    continue;
                }
                P5IRCoil^ coil = dynamic_cast<P5IRCoil^>(instr);
                if (coil != nullptr) {
                    if (coil->CoilType == P5CoilType::Set)
                        sb->AppendLine("      S     \"" + coil->Tag + "\"");
                    else if (coil->CoilType == P5CoilType::Reset)
                        sb->AppendLine("      R     \"" + coil->Tag + "\"");
                    else
                        sb->AppendLine("      =     \"" + coil->Tag + "\"");
                    continue;
                }
                P5IRCall^ call = dynamic_cast<P5IRCall^>(instr);
                if (call != nullptr) {
                    String^ name = call->Name->ToUpper();
                    if (name == "TON" || name == "TOF" || name == "TP") {
                        String^ instance = GetParam(call, "instance", "T1");
                        String^ pt = GetParam(call, "PT", "S5T#1S");
                        sb->AppendLine("      CALL  \"" + instance + "\"");
                        sb->AppendLine("      IN   := TRUE");
                        sb->AppendLine("      PT   := " + pt);
                    }
                    else if (name == "CTU" || name == "CTD" || name == "CTUD") {
                        String^ instance = GetParam(call, "instance", "C1");
                        String^ pv = GetParam(call, "PV", "1");
                        sb->AppendLine("      CALL  \"" + instance + "\"");
                        sb->AppendLine("      CU   := TRUE");
                        sb->AppendLine("      PV   := " + pv);
                    }
                    else {
                        sb->AppendLine("      CALL  " + call->Name);
                        for each (KeyValuePair<String^, String^>^ kv in call->Parameters) {
                            sb->AppendLine("      " + kv->Key + " := " + kv->Value);
                        }
                    }
                    continue;
                }
                P5IRBranch^ branch = dynamic_cast<P5IRBranch^>(instr);
                if (branch != nullptr) {
                    sb->AppendLine("      O(");
                    for each (P5IRInstruction^ child in branch->Children) {
                        P5IRContact^ bc = dynamic_cast<P5IRContact^>(child);
                        if (bc != nullptr) {
                            if (bc->Negated)
                                sb->AppendLine("        AN    \"" + bc->Tag + "\"");
                            else
                                sb->AppendLine("        A     \"" + bc->Tag + "\"");
                        }
                    }
                    sb->AppendLine("      )");
                }
            }
            sb->AppendLine();
        }
        return sb->ToString();
    }

private:
    static String^ GetParam(P5IRCall^ call, String^ key, String^ defaultVal) {
        if (call->Parameters != nullptr && call->Parameters->ContainsKey(key))
            return call->Parameters[key];
        return defaultVal;
    }
};

ref class P5Optimizer {
public:
    static P5IRProgram^ Optimize(P5IRProgram^ program) {
        P5CompilerLog::Info("OPT", "Running optimizations...");
        int totalChanges = 0;

        totalChanges += RemoveRedundantContacts(program);
        totalChanges += ConstantFold(program);
        totalChanges += RemoveEmptyBranches(program);
        totalChanges += RemoveEmptyNetworks(program);
        totalChanges += RemoveDeadVariables(program);
        totalChanges += MergeDuplicateTimers(program);
        totalChanges += CompressNetworks(program);

        P5CompilerLog::Info("OPT", "Optimization complete: " + totalChanges + " changes");
        return program;
    }

private:
    static int RemoveRedundantContacts(P5IRProgram^ program) {
        int changes = 0;
        for each (P5IRNetwork^ net in program->Networks) {
            Dictionary<String^, int>^ contactCount = gcnew Dictionary<String^, int>();
            for each (P5IRInstruction^ instr in net->Instructions) {
                P5IRContact^ c = dynamic_cast<P5IRContact^>(instr);
                if (c != nullptr && !c->Negated) {
                    String^ key = c->Tag;
                    if (!contactCount->ContainsKey(key)) contactCount[key] = 0;
                    contactCount[key]++;
                }
            }
            List<P5IRInstruction^>^ toRemove = gcnew List<P5IRInstruction^>();
            for each (P5IRInstruction^ instr in net->Instructions) {
                P5IRContact^ c = dynamic_cast<P5IRContact^>(instr);
                if (c != nullptr && !c->Negated && contactCount->ContainsKey(c->Tag) && contactCount[c->Tag] > 1) {
                    toRemove->Add(c);
                    contactCount[c->Tag]--;
                    changes++;
                }
            }
            for each (P5IRInstruction^ instr in toRemove) {
                net->Instructions->Remove(instr);
            }
        }
        if (changes > 0) P5CompilerLog::Info("OPT", "Removed " + changes + " redundant contacts");
        return changes;
    }

    static int ConstantFold(P5IRProgram^ program) {
        int changes = 0;
        for each (P5IRNetwork^ net in program->Networks) {
            List<P5IRInstruction^>^ toRemove = gcnew List<P5IRInstruction^>();
            for each (P5IRInstruction^ instr in net->Instructions) {
                P5IRContact^ c = dynamic_cast<P5IRContact^>(instr);
                if (c != nullptr && c->Tag != nullptr) {
                    String^ tagUpper = c->Tag->ToUpper();
                    if (!c->Negated && (tagUpper == "TRUE" || tagUpper == "1")) {
                        toRemove->Add(c);
                        changes++;
                    }
                    else if (c->Negated && (tagUpper == "FALSE" || tagUpper == "0")) {
                        toRemove->Add(c);
                        changes++;
                    }
                }
            }
            for each (P5IRInstruction^ instr in toRemove) {
                net->Instructions->Remove(instr);
            }
        }
        if (changes > 0) P5CompilerLog::Info("OPT", "Constant folded " + changes + " instructions");
        return changes;
    }

    static int RemoveEmptyBranches(P5IRProgram^ program) {
        int changes = 0;
        for each (P5IRNetwork^ net in program->Networks) {
            List<P5IRInstruction^>^ toRemove = gcnew List<P5IRInstruction^>();
            for each (P5IRInstruction^ instr in net->Instructions) {
                P5IRBranch^ branch = dynamic_cast<P5IRBranch^>(instr);
                if (branch != nullptr && (branch->Children == nullptr || branch->Children->Count == 0)) {
                    toRemove->Add(branch);
                    changes++;
                }
            }
            for each (P5IRInstruction^ instr in toRemove) {
                net->Instructions->Remove(instr);
            }
        }
        if (changes > 0) P5CompilerLog::Info("OPT", "Removed " + changes + " empty branches");
        return changes;
    }

    static int RemoveEmptyNetworks(P5IRProgram^ program) {
        int removed = 0;
        for (int i = program->Networks->Count - 1; i >= 0; i--) {
            if (program->Networks[i]->Instructions->Count == 0) {
                program->Networks->RemoveAt(i);
                removed++;
            }
        }
        if (removed > 0) P5CompilerLog::Info("OPT", "Removed " + removed + " empty networks");
        return removed;
    }

    static int RemoveDeadVariables(P5IRProgram^ program) {
        if (program->VariableNames == nullptr) return 0;
        HashSet<String^>^ usedTags = gcnew HashSet<String^>();
        for each (P5IRNetwork^ net in program->Networks) {
            for each (P5IRInstruction^ instr in net->Instructions) {
                P5IRContact^ c = dynamic_cast<P5IRContact^>(instr);
                if (c != nullptr && c->Tag != nullptr) usedTags->Add(c->Tag);
                P5IRCoil^ coil = dynamic_cast<P5IRCoil^>(instr);
                if (coil != nullptr && coil->Tag != nullptr) usedTags->Add(coil->Tag);
                P5IRCall^ call = dynamic_cast<P5IRCall^>(instr);
                if (call != nullptr && call->Parameters != nullptr && call->Parameters->ContainsKey("instance"))
                    usedTags->Add(call->Parameters["instance"]);
            }
        }
        int removed = 0;
        for (int i = program->VariableNames->Count - 1; i >= 0; i--) {
            if (!usedTags->Contains(program->VariableNames[i])) {
                program->VariableNames->RemoveAt(i);
                removed++;
            }
        }
        if (removed > 0) P5CompilerLog::Info("OPT", "Removed " + removed + " dead variables");
        return removed;
    }

    static int MergeDuplicateTimers(P5IRProgram^ program) {
        int changes = 0;
        for each (P5IRNetwork^ net in program->Networks) {
            Dictionary<String^, P5IRCall^>^ timerSigs = gcnew Dictionary<String^, P5IRCall^>();
            List<P5IRCall^>^ toRemove = gcnew List<P5IRCall^>();
            for each (P5IRInstruction^ instr in net->Instructions) {
                P5IRCall^ call = dynamic_cast<P5IRCall^>(instr);
                if (call != nullptr && call->Name != nullptr) {
                    String^ n = call->Name->ToUpper();
                    if (n == "TON" || n == "TOF" || n == "TP") {
                        String^ instance = "";
                        if (call->Parameters != nullptr && call->Parameters->ContainsKey("instance"))
                            instance = call->Parameters["instance"];
                        if (instance->Length > 0) {
                            if (timerSigs->ContainsKey(instance)) {
                                toRemove->Add(call);
                                changes++;
                            }
                            else {
                                timerSigs[instance] = call;
                            }
                        }
                    }
                }
            }
            for each (P5IRCall^ call in toRemove) {
                net->Instructions->Remove(call);
            }
        }
        if (changes > 0) P5CompilerLog::Info("OPT", "Merged " + changes + " duplicate timers");
        return changes;
    }

    static int CompressNetworks(P5IRProgram^ program) {
        int changes = 0;
        for (int i = program->Networks->Count - 1; i > 0; i--) {
            P5IRNetwork^ current = program->Networks[i];
            P5IRNetwork^ prev = program->Networks[i - 1];
            if (current->Instructions->Count <= 2 && prev->Instructions->Count <= 2) {
                bool currentHasCoil = false;
                bool prevHasCoil = false;
                for each (P5IRInstruction^ instr in current->Instructions) {
                    if (dynamic_cast<P5IRCoil^>(instr) != nullptr) currentHasCoil = true;
                }
                for each (P5IRInstruction^ instr in prev->Instructions) {
                    if (dynamic_cast<P5IRCoil^>(instr) != nullptr) prevHasCoil = true;
                }
                if (!currentHasCoil && !prevHasCoil) {
                    for each (P5IRInstruction^ instr in current->Instructions) {
                        prev->Instructions->Add(instr);
                    }
                    prev->Title = prev->Title + " + " + current->Title;
                    program->Networks->RemoveAt(i);
                    changes++;
                }
            }
        }
        if (changes > 0) P5CompilerLog::Info("OPT", "Compressed " + changes + " networks");
        return changes;
    }
};

ref class P5CfgAnalyzer {
public:
    static ControlFlowGraph^ BuildCfg(P3Dsl^ dsl) {
        P5CompilerLog::Info("CFG", "Building Control Flow Graph...");
        ControlFlowGraph^ cfg = gcnew ControlFlowGraph();
        cfg->Nodes = gcnew List<CfgNode^>();
        cfg->Adjacency = gcnew Dictionary<int, List<int>^>();

        for (int i = 0; i < dsl->Networks->Count; i++) {
            CfgNode^ node = gcnew CfgNode();
            node->Id = i;
            node->Tag = dsl->Networks[i]->Title;
            node->DominanceDepth = 0;
            cfg->Nodes->Add(node);
        }

        for (int i = 0; i < dsl->Networks->Count - 1; i++) {
            if (!cfg->Adjacency->ContainsKey(i))
                cfg->Adjacency[i] = gcnew List<int>();
            cfg->Adjacency[i]->Add(i + 1);

            for each (Object^ item in dsl->Networks[i]->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "jump" || t == "jmp") {
                        String^ target = (node->Label != nullptr && node->Label->Length > 0) ? node->Label : node->Tag;
                        if (target != nullptr) {
                            for (int j = 0; j < dsl->Networks->Count; j++) {
                                if (j != i + 1) {
                                    for each (Object^ item2 in dsl->Networks[j]->Items) {
                                        P3Node^ n2 = dynamic_cast<P3Node^>(item2);
                                        if (n2 != nullptr && n2->NodeType->ToLower() == "label" && n2->Label == target) {
                                            cfg->Adjacency[i]->Add(j);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if (dsl->Networks->Count > 0) {
            cfg->EntryId = 0;
            cfg->ExitId = dsl->Networks->Count - 1;
        }

        P5CompilerLog::Info("CFG", "CFG built: " + cfg->Nodes->Count + " nodes");
        return cfg;
    }

    static List<String^>^ DetectCycles(ControlFlowGraph^ cfg) {
        List<String^>^ cycles = gcnew List<String^>();
        HashSet<int>^ visited = gcnew HashSet<int>();
        HashSet<int>^ inStack = gcnew HashSet<int>();

        for each (CfgNode^ node in cfg->Nodes) {
            if (!visited->Contains(node->Id)) {
                DfsCycle(node->Id, cfg, visited, inStack, cycles);
            }
        }
        if (cycles->Count > 0) P5CompilerLog::Warn("CFG", "Detected " + cycles->Count + " cycles");
        return cycles;
    }

    static List<String^>^ DetectDeadStates(P3Dsl^ dsl) {
        List<String^>^ deadStates = gcnew List<String^>();
        HashSet<String^>^ setSteps = gcnew HashSet<String^>();
        HashSet<String^>^ stepVars = gcnew HashSet<String^>();

        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr && (v->Name->StartsWith("Step") || v->Name->Contains("step")))
                stepVars->Add(v->Name);
        }
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr && node->Tag != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "set" || t == "scoil") && stepVars->Contains(node->Tag))
                        setSteps->Add(node->Tag);
                }
            }
        }
        for each (String^ step in stepVars) {
            if (!setSteps->Contains(step)) {
                deadStates->Add(step);
            }
        }
        if (deadStates->Count > 0) P5CompilerLog::Warn("CFG", "Found " + deadStates->Count + " dead states");
        return deadStates;
    }

    static List<String^>^ AnalyzeReachability(ControlFlowGraph^ cfg) {
        List<String^>^ unreachable = gcnew List<String^>();
        if (cfg->Nodes->Count == 0) return unreachable;

        HashSet<int>^ reachable = gcnew HashSet<int>();
        Queue<int>^ queue = gcnew Queue<int>();
        queue->Enqueue(cfg->EntryId);
        reachable->Add(cfg->EntryId);

        while (queue->Count > 0) {
            int current = queue->Dequeue();
            if (cfg->Adjacency->ContainsKey(current)) {
                for each (int next in cfg->Adjacency[current]) {
                    if (!reachable->Contains(next)) {
                        reachable->Add(next);
                        queue->Enqueue(next);
                    }
                }
            }
        }

        for each (CfgNode^ node in cfg->Nodes) {
            if (!reachable->Contains(node->Id)) {
                unreachable->Add("Network '" + node->Tag + "' is unreachable from entry");
            }
        }
        if (unreachable->Count > 0) P5CompilerLog::Warn("CFG", "Found " + unreachable->Count + " unreachable networks");
        return unreachable;
    }

    static List<String^>^ AnalyzeBranches(ControlFlowGraph^ cfg) {
        List<String^>^ branchInfo = gcnew List<String^>();
        for each (CfgNode^ node in cfg->Nodes) {
            if (cfg->Adjacency->ContainsKey(node->Id) && cfg->Adjacency[node->Id]->Count > 1) {
                branchInfo->Add("Network '" + node->Tag + "' has " + cfg->Adjacency[node->Id]->Count + " outgoing branches");
            }
        }
        return branchInfo;
    }

    static List<String^>^ CompressStates(P3Dsl^ dsl) {
        List<String^>^ compressed = gcnew List<String^>();
        List<P3Network^>^ toRemove = gcnew List<P3Network^>();

        for (int i = 0; i < dsl->Networks->Count; i++) {
            bool onlyStepContact = true;
            String^ stepTag = nullptr;
            for each (Object^ item in dsl->Networks[i]->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    if (node->NodeType->ToLower() == "contact" && node->Tag != nullptr && node->Tag->StartsWith("Step")) {
                        stepTag = node->Tag;
                    }
                    else {
                        onlyStepContact = false;
                    }
                }
            }
            if (onlyStepContact && stepTag != nullptr) {
                toRemove->Add(dsl->Networks[i]);
                compressed->Add("Removed empty step network: " + dsl->Networks[i]->Title);
            }
        }

        for each (P3Network^ net in toRemove) {
            dsl->Networks->Remove(net);
        }
        if (compressed->Count > 0) P5CompilerLog::Info("CFG", "Compressed " + compressed->Count + " states");
        return compressed;
    }

private:
    static void DfsCycle(int node, ControlFlowGraph^ cfg, HashSet<int>^ visited, HashSet<int>^ inStack, List<String^>^ cycles) {
        visited->Add(node);
        inStack->Add(node);

        if (cfg->Adjacency->ContainsKey(node)) {
            for each (int next in cfg->Adjacency[node]) {
                if (inStack->Contains(next)) {
                    cycles->Add("Cycle detected: Network " + node + " -> Network " + next);
                }
                else if (!visited->Contains(next)) {
                    DfsCycle(next, cfg, visited, inStack, cycles);
                }
            }
        }
        inStack->Remove(node);
    }
};

ref class P5SemanticValidator {
public:
    static P3ValidationResult^ Validate(P3Dsl^ dsl) {
        P3ValidationResult^ result = P3SemanticValidator::Validate(dsl);

        CheckOutputRace(dsl, result);
        CheckOpenBranches(dsl, result);
        CheckTimerDeadlock(dsl, result);
        CheckCounterDeadlock(dsl, result);
        CheckUnresetCounter(dsl, result);
        CheckInvalidConnections(dsl, result);
        CheckDanglingNodes(dsl, result);

        result->IsValid = (result->Errors->Count == 0);
        P5CompilerLog::Info("VAL", "Phase 5 validation: " + result->Errors->Count + " errors, " + result->Warnings->Count + " warnings");
        return result;
    }

private:
    static void CheckOutputRace(P3Dsl^ dsl, P3ValidationResult^ result) {
        Dictionary<String^, List<String^>^>^ outputNetworks = gcnew Dictionary<String^, List<String^>^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr && node->Tag != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "coil" && node->Tag->Length > 0) {
                        if (!outputNetworks->ContainsKey(node->Tag))
                            outputNetworks[node->Tag] = gcnew List<String^>();
                        outputNetworks[node->Tag]->Add(net->Title);
                    }
                }
            }
        }
        for each (KeyValuePair<String^, List<String^>^>^ kv in outputNetworks) {
            if (kv->Value->Count > 1) {
                result->Warnings->Add("Output race: '" + kv->Key + "' driven by multiple coils in: " + String::Join(", ", kv->Value));
            }
        }
    }

    static void CheckOpenBranches(P3Dsl^ dsl, P3ValidationResult^ result) {
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Parallel^ par = dynamic_cast<P3Parallel^>(item);
                if (par != nullptr && par->Branches != nullptr) {
                    for each (P3Branch^ branch in par->Branches) {
                        if (branch->Nodes == nullptr || branch->Nodes->Count == 0) {
                            result->Errors->Add("Network '" + net->Title + "': empty parallel branch (open branch)");
                        }
                    }
                }
            }
        }
    }

    static void CheckTimerDeadlock(P3Dsl^ dsl, P3ValidationResult^ result) {
        Dictionary<String^, bool>^ timerHasInput = gcnew Dictionary<String^, bool>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "ton" || t == "tof" || t == "tp") && node->Instance != nullptr) {
                        bool hasInput = false;
                        for each (Object^ item2 in net->Items) {
                            P3Node^ n2 = dynamic_cast<P3Node^>(item2);
                            if (n2 != nullptr && n2 != node && n2->NodeType->ToLower() == "contact") {
                                hasInput = true;
                            }
                        }
                        if (!timerHasInput->ContainsKey(node->Instance))
                            timerHasInput[node->Instance] = hasInput;
                    }
                }
            }
        }
        for each (KeyValuePair<String^, bool>^ kv in timerHasInput) {
            if (!kv->Value) {
                result->Warnings->Add("Timer '" + kv->Key + "' has no input condition (potential deadlock: always running or never triggered)");
            }
        }
    }

    static void CheckCounterDeadlock(P3Dsl^ dsl, P3ValidationResult^ result) {
        Dictionary<String^, bool>^ counterHasInput = gcnew Dictionary<String^, bool>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "ctu" || t == "ctd" || t == "ctud") && node->Instance != nullptr) {
                        bool hasInput = false;
                        for each (Object^ item2 in net->Items) {
                            P3Node^ n2 = dynamic_cast<P3Node^>(item2);
                            if (n2 != nullptr && n2 != node && n2->NodeType->ToLower() == "contact") {
                                hasInput = true;
                            }
                        }
                        if (!counterHasInput->ContainsKey(node->Instance))
                            counterHasInput[node->Instance] = hasInput;
                    }
                }
            }
        }
        for each (KeyValuePair<String^, bool>^ kv in counterHasInput) {
            if (!kv->Value) {
                result->Warnings->Add("Counter '" + kv->Key + "' has no input condition (potential deadlock)");
            }
        }
    }

    static void CheckUnresetCounter(P3Dsl^ dsl, P3ValidationResult^ result) {
        HashSet<String^>^ counters = gcnew HashSet<String^>();
        HashSet<String^>^ resetCounters = gcnew HashSet<String^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "ctu" || t == "ctd" || t == "ctud") && node->Instance != nullptr)
                        counters->Add(node->Instance);
                    if (t == "reset" && node->Tag != nullptr && counters->Contains(node->Tag))
                        resetCounters->Add(node->Tag);
                }
            }
        }
        for each (String^ c in counters) {
            if (!resetCounters->Contains(c)) {
                result->Warnings->Add("Counter '" + c + "' is never RESET (will accumulate indefinitely)");
            }
        }
    }

    static void CheckInvalidConnections(P3Dsl^ dsl, P3ValidationResult^ result) {
        for each (P3Network^ net in dsl->Networks) {
            bool foundOutput = false;
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "contact" && foundOutput) {
                        result->Errors->Add("Network '" + net->Title + "': contact after output element (invalid connection)");
                    }
                    if (IsOutputType(t)) foundOutput = true;
                }
            }
        }
    }

    static void CheckDanglingNodes(P3Dsl^ dsl, P3ValidationResult^ result) {
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "set" || t == "reset" || t == "coil") && (node->Tag == nullptr || node->Tag->Length == 0)) {
                        result->Errors->Add("Network '" + net->Title + "': " + t + " node with empty tag (dangling)");
                    }
                }
            }
        }
    }

    static bool IsOutputType(String^ nodeType) {
        String^ t = nodeType->ToLower();
        return t == "coil" || t == "set" || t == "reset" || t == "scoil" || t == "rcoil" ||
            t == "ton" || t == "tof" || t == "tp" || t == "ctu" || t == "ctd" || t == "ctud" ||
            t == "move" || t == "add" || t == "sub" || t == "mul" || t == "div" || t == "mod" ||
            t == "jump" || t == "jmp" || t == "ret" || t == "nop";
    }
};

ref class P5RepairEngine {
public:
    static String^ AutoRepair(P3Dsl^ dsl) {
        P5CompilerLog::Info("REPAIR", "Running Phase 5 auto-repair...");
        List<String^>^ fixes = gcnew List<String^>();

        fixes->Add(RemoveDuplicateVariables(dsl));
        fixes->Add(FixDoubleCoil(dsl));
        fixes->Add(FixDanglingNodes(dsl));
        fixes->Add(FixInvalidConnections(dsl));
        fixes->Add(DeclareMissingTimerCounterInstances(dsl));
        fixes->Add(FixTimerDeadlock(dsl));
        fixes->Add(FixCounterDeadlock(dsl));
        fixes->Add(FixUnresetCounter(dsl));
        fixes->Add(FixJumpLoop(dsl));
        fixes->Add(AutoAddReset(dsl));
        fixes->Add(AutoAddInterlock(dsl));
        fixes->Add(AutoAddStopCircuit(dsl));
        fixes->Add(AutoFixParallelStructure(dsl));
        fixes->Add(AutoAddPt(dsl));
        fixes->Add(AutoAddPv(dsl));

        P3SemanticValidator::AutoFix(dsl);
        P3RepairEngine::RepairUndefinedVariables(dsl);

        List<String^>^ validFixes = gcnew List<String^>();
        for each (String^ s in fixes) {
            if (s != nullptr && s->Length > 0) validFixes->Add(s);
        }
        String^ result = String::Join(", ", validFixes);
        if (result->Length > 0) P5CompilerLog::Info("REPAIR", result);
        return result;
    }

private:
    static String^ AutoAddReset(P3Dsl^ dsl) {
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
        List<String^>^ added = gcnew List<String^>();
        for each (String^ s in setVars) {
            if (!resetVars->Contains(s)) {
                P3Network^ resetNet = gcnew P3Network();
                resetNet->Title = "Auto-Reset " + s;
                resetNet->Items = gcnew List<Object^>();

                P3Node^ stopContact = gcnew P3Node();
                stopContact->NodeType = "contact";
                stopContact->Tag = FindStopInput(dsl);
                stopContact->NormallyOpen = false;
                resetNet->Items->Add(stopContact);

                P3Node^ resetCoil = gcnew P3Node();
                resetCoil->NodeType = "reset";
                resetCoil->Tag = s;
                resetNet->Items->Add(resetCoil);

                dsl->Networks->Add(resetNet);
                added->Add(s);
            }
        }
        if (added->Count > 0) return "Auto-added RESET for: " + String::Join(", ", added);
        return "";
    }

    static String^ AutoAddInterlock(P3Dsl^ dsl) {
        List<String^>^ interlockPairs = gcnew List<String^>();
        List<String^>^ outputVars = gcnew List<String^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Scope == "output" && v->Type == "Bool") outputVars->Add(v->Name);
        }

        for (int i = 0; i < outputVars->Count; i++) {
            for (int j = i + 1; j < outputVars->Count; j++) {
                String^ a = outputVars[i]->ToLower();
                String^ b = outputVars[j]->ToLower();
                if ((a->Contains("fwd") && b->Contains("rev")) ||
                    (a->Contains("forward") && b->Contains("reverse")) ||
                    (a->Contains("open") && b->Contains("close")) ||
                    (a->Contains("up") && b->Contains("down"))) {
                    interlockPairs->Add(outputVars[i] + "," + outputVars[j]);
                }
            }
        }

        int added = 0;
        for each (String^ pair in interlockPairs) {
            array<String^>^ parts = pair->Split(gcnew array<Char>{','});
            if (parts->Length >= 2) {
                String^ a = parts[0]->Trim();
                String^ b = parts[1]->Trim();
                bool aHasBnc = false, bHasAnc = false;
                for each (P3Network^ net in dsl->Networks) {
                    for each (Object^ item in net->Items) {
                        P3Node^ node = dynamic_cast<P3Node^>(item);
                        if (node != nullptr && node->NodeType->ToLower() == "contact" && !node->NormallyOpen) {
                            if (node->Tag == b) aHasBnc = true;
                            if (node->Tag == a) bHasAnc = true;
                        }
                    }
                }
                if (!aHasBnc || !bHasAnc) {
                    for each (P3Network^ net in dsl->Networks) {
                        bool hasA = false, hasB = false;
                        for each (Object^ item in net->Items) {
                            P3Node^ node = dynamic_cast<P3Node^>(item);
                            if (node != nullptr) {
                                if (node->Tag == a && (node->NodeType->ToLower() == "coil" || node->NodeType->ToLower() == "set")) hasA = true;
                                if (node->Tag == b && (node->NodeType->ToLower() == "coil" || node->NodeType->ToLower() == "set")) hasB = true;
                            }
                        }
                        if (hasA && !aHasBnc) {
                            P3Node^ ncB = gcnew P3Node();
                            ncB->NodeType = "contact";
                            ncB->Tag = b;
                            ncB->NormallyOpen = false;
                            net->Items->Insert(net->Items->Count - 1, ncB);
                            added++;
                        }
                        if (hasB && !bHasAnc) {
                            P3Node^ ncA = gcnew P3Node();
                            ncA->NodeType = "contact";
                            ncA->Tag = a;
                            ncA->NormallyOpen = false;
                            net->Items->Insert(net->Items->Count - 1, ncA);
                            added++;
                        }
                    }
                }
            }
        }
        if (added > 0) return "Auto-added " + added + " interlock NC contacts";
        return "";
    }

    static String^ AutoAddStopCircuit(P3Dsl^ dsl) {
        bool hasStopInput = false;
        String^ stopName = "";
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr && v->Scope == "input") {
                String^ n = v->Name->ToLower();
                if (n->Contains("stop") || n->Contains("halt")) {
                    hasStopInput = true;
                    stopName = v->Name;
                }
            }
        }

        if (!hasStopInput) {
            P3Variable^ stopVar = gcnew P3Variable();
            stopVar->Name = "Stop_Btn";
            stopVar->Type = "Bool";
            stopVar->Scope = "input";
            stopVar->Comment = "Auto-added stop button";
            dsl->Variables->Add(stopVar);
            stopName = "Stop_Btn";
        }

        bool hasStopNc = false;
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr && node->NodeType->ToLower() == "contact" && !node->NormallyOpen &&
                    node->Tag != nullptr && node->Tag == stopName) {
                    hasStopNc = true;
                }
            }
        }

        if (!hasStopNc) {
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
                    P3Node^ stopNc = gcnew P3Node();
                    stopNc->NodeType = "contact";
                    stopNc->Tag = stopName;
                    stopNc->NormallyOpen = false;
                    net->Items->Insert(0, stopNc);
                }
            }
            return "Auto-added stop NC contacts using " + stopName;
        }
        return "";
    }

    static String^ AutoFixParallelStructure(P3Dsl^ dsl) {
        int fixed_ = 0;
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Parallel^ par = dynamic_cast<P3Parallel^>(item);
                if (par != nullptr && par->Branches != nullptr) {
                    for (int i = par->Branches->Count - 1; i >= 0; i--) {
                        if (par->Branches[i]->Nodes == nullptr || par->Branches[i]->Nodes->Count == 0) {
                            par->Branches->RemoveAt(i);
                            fixed_++;
                        }
                    }
                }
            }
        }
        if (fixed_ > 0) return "Fixed " + fixed_ + " empty parallel branches";
        return "";
    }

    static String^ AutoAddPt(P3Dsl^ dsl) {
        int added = 0;
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "ton" || t == "tof" || t == "tp") &&
                        (node->Pt == nullptr || node->Pt->Length == 0)) {
                        node->Pt = "T#1S";
                        added++;
                    }
                }
            }
        }
        if (added > 0) return "Auto-added PT for " + added + " timers";
        return "";
    }

    static String^ AutoAddPv(P3Dsl^ dsl) {
        int added = 0;
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "ctu" || t == "ctd" || t == "ctud") &&
                        (node->Pv == nullptr || node->Pv->Length == 0)) {
                        node->Pv = "1";
                        added++;
                    }
                }
            }
        }
        if (added > 0) return "Auto-added PV for " + added + " counters";
        return "";
    }

    static bool IsOutputType(String^ nodeType) {
        String^ t = nodeType->ToLower();
        return t == "coil" || t == "set" || t == "reset" || t == "scoil" || t == "rcoil" ||
            t == "ton" || t == "tof" || t == "tp" || t == "ctu" || t == "ctd" || t == "ctud" ||
            t == "move" || t == "add" || t == "sub" || t == "mul" || t == "div" || t == "mod" ||
            t == "jump" || t == "jmp" || t == "ret" || t == "nop";
    }

    static String^ FindStopInput(P3Dsl^ dsl) {
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr && v->Scope == "input") {
                String^ n = v->Name->ToLower();
                if (n->Contains("stop") || n->Contains("halt")) return v->Name;
            }
        }
        return "Stop_Btn";
    }

    static String^ RemoveDuplicateVariables(P3Dsl^ dsl) {
        HashSet<String^>^ seen = gcnew HashSet<String^>();
        List<P3Variable^>^ unique = gcnew List<P3Variable^>();
        int removed = 0;
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr && seen->Contains(v->Name)) {
                removed++;
            }
            else {
                if (v->Name != nullptr) seen->Add(v->Name);
                unique->Add(v);
            }
        }
        if (removed > 0) {
            dsl->Variables->Clear();
            for each (P3Variable^ v in unique) dsl->Variables->Add(v);
            return "Removed " + removed + " duplicate variable(s)";
        }
        return "";
    }

    static String^ FixDoubleCoil(P3Dsl^ dsl) {
        Dictionary<String^, List<P3Network^>^>^ coilNetworks = gcnew Dictionary<String^, List<P3Network^>^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr && node->Tag != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "coil" && node->Tag->Length > 0) {
                        if (!coilNetworks->ContainsKey(node->Tag))
                            coilNetworks[node->Tag] = gcnew List<P3Network^>();
                        coilNetworks[node->Tag]->Add(net);
                    }
                }
            }
        }
        int fixed_ = 0;
        for each (KeyValuePair<String^, List<P3Network^>^>^ kv in coilNetworks) {
            if (kv->Value->Count > 1) {
                for each (P3Network^ net in kv->Value) {
                    for (int i = 0; i < net->Items->Count; i++) {
                        P3Node^ node = dynamic_cast<P3Node^>(net->Items[i]);
                        if (node != nullptr && node->Tag == kv->Key && node->NodeType->ToLower() == "coil") {
                            node->NodeType = "set";
                            fixed_++;
                            break;
                        }
                    }
                }
                bool hasReset = false;
                for each (P3Network^ net in dsl->Networks) {
                    for each (Object^ item in net->Items) {
                        P3Node^ node = dynamic_cast<P3Node^>(item);
                        if (node != nullptr && node->Tag == kv->Key && node->NodeType->ToLower() == "reset") {
                            hasReset = true;
                        }
                    }
                }
                if (!hasReset) {
                    P3Network^ resetNet = gcnew P3Network();
                    resetNet->Title = "Auto-Reset " + kv->Key;
                    resetNet->Items = gcnew List<Object^>();
                    P3Node^ stopContact = gcnew P3Node();
                    stopContact->NodeType = "contact";
                    stopContact->Tag = FindStopInput(dsl);
                    stopContact->NormallyOpen = false;
                    resetNet->Items->Add(stopContact);
                    P3Node^ resetCoil = gcnew P3Node();
                    resetCoil->NodeType = "reset";
                    resetCoil->Tag = kv->Key;
                    resetNet->Items->Add(resetCoil);
                    dsl->Networks->Add(resetNet);
                }
            }
        }
        if (fixed_ > 0) return "Fixed " + fixed_ + " double coil(s): converted to SET/RESET pattern";
        return "";
    }

    static String^ FixInvalidConnections(P3Dsl^ dsl) {
        int fixed_ = 0;
        for each (P3Network^ net in dsl->Networks) {
            List<Object^>^ inputs = gcnew List<Object^>();
            List<Object^>^ outputs = gcnew List<Object^>();
            bool foundOutput = false;
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (IsOutputType(t)) {
                        foundOutput = true;
                        outputs->Add(node);
                    }
                    else if (foundOutput && (t == "contact" || t == "noc" || t == "nc")) {
                        inputs->Add(node);
                        fixed_++;
                    }
                    else {
                        inputs->Add(node);
                    }
                }
                else {
                    if (foundOutput) outputs->Add(item);
                    else inputs->Add(item);
                }
            }
            if (fixed_ > 0) {
                net->Items->Clear();
                for each (Object^ item in inputs) net->Items->Add(item);
                for each (Object^ item in outputs) net->Items->Add(item);
            }
        }
        if (fixed_ > 0) return "Fixed " + fixed_ + " invalid connection(s): moved contacts before outputs";
        return "";
    }

    static String^ FixUnresetCounter(P3Dsl^ dsl) {
        HashSet<String^>^ counters = gcnew HashSet<String^>();
        HashSet<String^>^ resetCounters = gcnew HashSet<String^>();
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "ctu" || t == "ctd" || t == "ctud") && node->Instance != nullptr)
                        counters->Add(node->Instance);
                    if (t == "reset" && node->Tag != nullptr && counters->Contains(node->Tag))
                        resetCounters->Add(node->Tag);
                }
            }
        }
        int added = 0;
        for each (String^ c in counters) {
            if (!resetCounters->Contains(c)) {
                P3Network^ resetNet = gcnew P3Network();
                resetNet->Title = "Auto-Reset Counter " + c;
                resetNet->Items = gcnew List<Object^>();
                P3Node^ stopContact = gcnew P3Node();
                stopContact->NodeType = "contact";
                stopContact->Tag = FindStopInput(dsl);
                stopContact->NormallyOpen = false;
                resetNet->Items->Add(stopContact);
                P3Node^ resetCoil = gcnew P3Node();
                resetCoil->NodeType = "reset";
                resetCoil->Tag = c;
                resetNet->Items->Add(resetCoil);
                dsl->Networks->Add(resetNet);
                added++;
            }
        }
        if (added > 0) return "Auto-added RESET for " + added + " unreset counter(s)";
        return "";
    }

    static String^ FixDanglingNodes(P3Dsl^ dsl) {
        int fixed_ = 0;
        for each (P3Network^ net in dsl->Networks) {
            for (int i = net->Items->Count - 1; i >= 0; i--) {
                P3Node^ node = dynamic_cast<P3Node^>(net->Items[i]);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "set" || t == "reset" || t == "coil") &&
                        (node->Tag == nullptr || node->Tag->Length == 0)) {
                        net->Items->RemoveAt(i);
                        fixed_++;
                    }
                }
            }
        }
        if (fixed_ > 0) return "Removed " + fixed_ + " dangling node(s) with empty tag";
        return "";
    }

    static String^ FixTimerDeadlock(P3Dsl^ dsl) {
        int fixed_ = 0;
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "ton" || t == "tof" || t == "tp") && node->Instance != nullptr) {
                        bool hasInput = false;
                        for each (Object^ item2 in net->Items) {
                            P3Node^ n2 = dynamic_cast<P3Node^>(item2);
                            if (n2 != nullptr && n2 != node && n2->NodeType->ToLower() == "contact") {
                                hasInput = true;
                            }
                        }
                        if (!hasInput) {
                            P3Node^ autoContact = gcnew P3Node();
                            autoContact->NodeType = "contact";
                            autoContact->Tag = "Always_True";
                            autoContact->NormallyOpen = true;
                            net->Items->Insert(0, autoContact);
                            fixed_++;
                        }
                    }
                }
            }
        }
        if (fixed_ > 0) {
            bool hasAlwaysTrue = false;
            for each (P3Variable^ v in dsl->Variables) {
                if (v->Name == "Always_True") { hasAlwaysTrue = true; break; }
            }
            if (!hasAlwaysTrue) {
                P3Variable^ av = gcnew P3Variable();
                av->Name = "Always_True";
                av->Type = "Bool";
                av->Scope = "internal";
                av->Comment = L"常ON触点（自动修复添加）";
                dsl->Variables->Add(av);
            }
            return "Added input conditions for " + fixed_ + " timer(s) without input";
        }
        return "";
    }

    static String^ FixCounterDeadlock(P3Dsl^ dsl) {
        int fixed_ = 0;
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "ctu" || t == "ctd" || t == "ctud") && node->Instance != nullptr) {
                        bool hasInput = false;
                        for each (Object^ item2 in net->Items) {
                            P3Node^ n2 = dynamic_cast<P3Node^>(item2);
                            if (n2 != nullptr && n2 != node && n2->NodeType->ToLower() == "contact") {
                                hasInput = true;
                            }
                        }
                        if (!hasInput) {
                            P3Node^ autoContact = gcnew P3Node();
                            autoContact->NodeType = "contact";
                            autoContact->Tag = "Always_True";
                            autoContact->NormallyOpen = true;
                            net->Items->Insert(0, autoContact);
                            fixed_++;
                        }
                    }
                }
            }
        }
        if (fixed_ > 0) {
            bool hasAlwaysTrue = false;
            for each (P3Variable^ v in dsl->Variables) {
                if (v->Name == "Always_True") { hasAlwaysTrue = true; break; }
            }
            if (!hasAlwaysTrue) {
                P3Variable^ av = gcnew P3Variable();
                av->Name = "Always_True";
                av->Type = "Bool";
                av->Scope = "internal";
                av->Comment = L"常ON触点（自动修复添加）";
                dsl->Variables->Add(av);
            }
            return "Added input conditions for " + fixed_ + " counter(s) without input";
        }
        return "";
    }

    static String^ FixJumpLoop(P3Dsl^ dsl) {
        Dictionary<String^, int>^ labelNetworkIndex = gcnew Dictionary<String^, int>();
        List<KeyValuePair<int, String^>>^ backwardJumps = gcnew List<KeyValuePair<int, String^>>();
        for (int i = 0; i < dsl->Networks->Count; i++) {
            for each (Object^ item in dsl->Networks[i]->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if (t == "label" && node->Label != nullptr)
                        labelNetworkIndex[node->Label] = i;
                    if (t == "jump" || t == "jmp") {
                        String^ target = (node->Label != nullptr && node->Label->Length > 0) ? node->Label : node->Tag;
                        if (target != nullptr && labelNetworkIndex->ContainsKey(target)) {
                            if (labelNetworkIndex[target] <= i) {
                                backwardJumps->Add(KeyValuePair<int, String^>(i, target));
                            }
                        }
                    }
                }
            }
        }
        int fixed_ = 0;
        for each (KeyValuePair<int, String^>^ kv in backwardJumps) {
            bool hasExit = false;
            for each (P3Network^ net in dsl->Networks) {
                for each (Object^ item in net->Items) {
                    P3Node^ node = dynamic_cast<P3Node^>(item);
                    if (node != nullptr) {
                        String^ t = node->NodeType->ToLower();
                        if (t == "ctu" || t == "ctd" || t == "ctud" ||
                            t == "compare_eq" || t == "compare_gt" || t == "compare_ge" ||
                            t == "compare_lt" || t == "compare_le" || t == "compare_ne") {
                            hasExit = true;
                        }
                    }
                }
            }
            if (!hasExit) {
                String^ counterName = "LoopCounter_" + kv->Value;
                P3Network^ exitNet = gcnew P3Network();
                exitNet->Title = "Auto-Exit Loop " + kv->Value;
                exitNet->Items = gcnew List<Object^>();
                P3Node^ ctuNode = gcnew P3Node();
                ctuNode->NodeType = "ctu";
                ctuNode->Instance = counterName;
                ctuNode->Pv = "100";
                exitNet->Items->Add(ctuNode);
                dsl->Networks->Add(exitNet);

                P3Variable^ cv = gcnew P3Variable();
                cv->Name = counterName;
                cv->Type = "Counter";
                cv->CounterType = "CTU";
                cv->Scope = "internal";
                cv->Comment = L"循环退出计数器（自动修复添加）";
                dsl->Variables->Add(cv);
                fixed_++;
            }
        }
        if (fixed_ > 0) return "Added " + fixed_ + " loop exit counter(s) for backward jump(s)";
        return "";
    }

    static String^ DeclareMissingTimerCounterInstances(P3Dsl^ dsl) {
        HashSet<String^>^ declared = gcnew HashSet<String^>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Name != nullptr) declared->Add(v->Name);
        }
        int added = 0;
        for each (P3Network^ net in dsl->Networks) {
            for each (Object^ item in net->Items) {
                P3Node^ node = dynamic_cast<P3Node^>(item);
                if (node != nullptr) {
                    String^ t = node->NodeType->ToLower();
                    if ((t == "ton" || t == "tof" || t == "tp") && node->Instance != nullptr && !declared->Contains(node->Instance)) {
                        P3Variable^ tv = gcnew P3Variable();
                        tv->Name = node->Instance;
                        tv->Type = "Timer";
                        tv->TimerType = t->ToUpper();
                        tv->Scope = "internal";
                        tv->Comment = L"定时器（自动修复添加）";
                        dsl->Variables->Add(tv);
                        declared->Add(node->Instance);
                        added++;
                    }
                    if ((t == "ctu" || t == "ctd" || t == "ctud") && node->Instance != nullptr && !declared->Contains(node->Instance)) {
                        P3Variable^ cv = gcnew P3Variable();
                        cv->Name = node->Instance;
                        cv->Type = "Counter";
                        cv->CounterType = t->ToUpper();
                        cv->Scope = "internal";
                        cv->Comment = L"计数器（自动修复添加）";
                        dsl->Variables->Add(cv);
                        declared->Add(node->Instance);
                        added++;
                    }
                }
            }
        }
        if (added > 0) return "Declared " + added + " missing timer/counter instance(s)";
        return "";
    }
};

ref class P5VisualizationBackend : P5IBackend {
public:
    virtual String^ Generate(P5IRProgram^ program) {
        P5CompilerLog::Info("VIS", "Generating visualization output...");
        StringBuilder^ sb = gcnew StringBuilder();

        sb->AppendLine("digraph PLC_Program {");
        sb->AppendLine("  rankdir=LR;");
        sb->AppendLine("  node [shape=box];");
        sb->AppendLine();

        sb->AppendLine("  subgraph cluster_logicgraph {");
        sb->AppendLine("    label=\"LogicGraph\";");
        for (int i = 0; i < program->Networks->Count; i++) {
            sb->AppendLine("    net" + i + " [label=\"" + program->Networks[i]->Title + "\"];");
        }
        for (int i = 0; i < program->Networks->Count - 1; i++) {
            sb->AppendLine("    net" + i + " -> net" + (i + 1) + ";");
        }
        sb->AppendLine("  }");
        sb->AppendLine();

        sb->AppendLine("  subgraph cluster_ir {");
        sb->AppendLine("    label=\"IR\";");
        for (int i = 0; i < program->Networks->Count; i++) {
            for each (P5IRInstruction^ instr in program->Networks[i]->Instructions) {
                String^ label = GetInstrLabel(instr);
                sb->AppendLine("    ir" + instr->UId + " [label=\"" + label + "\"];");
            }
        }
        for (int i = 0; i < program->Networks->Count; i++) {
            List<P5IRInstruction^>^ instrs = program->Networks[i]->Instructions;
            for (int j = 0; j < instrs->Count - 1; j++) {
                sb->AppendLine("    ir" + instrs[j]->UId + " -> ir" + instrs[j + 1]->UId + ";");
            }
        }
        sb->AppendLine("  }");

        sb->AppendLine("}");
        return sb->ToString();
    }

    static String^ GenerateCfgDot(ControlFlowGraph^ cfg) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("digraph CFG {");
        sb->AppendLine("  rankdir=TB;");
        for each (CfgNode^ node in cfg->Nodes) {
            sb->AppendLine("  n" + node->Id + " [label=\"" + node->Tag + "\"];");
        }
        for each (KeyValuePair<int, List<int>^>^ kv in cfg->Adjacency) {
            for each (int target in kv->Value) {
                sb->AppendLine("  n" + kv->Key + " -> n" + target + ";");
            }
        }
        sb->AppendLine("}");
        return sb->ToString();
    }

    static String^ GenerateStateMachineDot(P4SemanticPlan^ plan) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("digraph StateMachine {");
        sb->AppendLine("  rankdir=LR;");
        if (plan != nullptr && plan->States != nullptr) {
            for each (P4State^ state in plan->States) {
                sb->AppendLine("  \"" + state->Name + "\" [shape=ellipse];");
                if (state->NextState != nullptr && state->NextState->Length > 0) {
                    sb->AppendLine("  \"" + state->Name + "\" -> \"" + state->NextState + "\" [label=\"" + state->TransitionCondition + "\"];");
                }
            }
        }
        sb->AppendLine("}");
        return sb->ToString();
    }

    static String^ GenerateNetworkTopology(P5IRProgram^ program) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("digraph NetworkTopology {");
        sb->AppendLine("  rankdir=LR;");
        HashSet<String^>^ tags = gcnew HashSet<String^>();
        for each (P5IRNetwork^ net in program->Networks) {
            for each (P5IRInstruction^ instr in net->Instructions) {
                P5IRContact^ c = dynamic_cast<P5IRContact^>(instr);
                if (c != nullptr && c->Tag != nullptr) tags->Add(c->Tag);
                P5IRCoil^ coil = dynamic_cast<P5IRCoil^>(instr);
                if (coil != nullptr && coil->Tag != nullptr) tags->Add(coil->Tag);
            }
        }
        for each (String^ tag in tags) {
            sb->AppendLine("  \"" + tag + "\" [shape=box];");
        }
        for each (P5IRNetwork^ net in program->Networks) {
            List<String^>^ inputs = gcnew List<String^>();
            List<String^>^ outputs = gcnew List<String^>();
            for each (P5IRInstruction^ instr in net->Instructions) {
                P5IRContact^ c = dynamic_cast<P5IRContact^>(instr);
                if (c != nullptr && c->Tag != nullptr) inputs->Add(c->Tag);
                P5IRCoil^ coil = dynamic_cast<P5IRCoil^>(instr);
                if (coil != nullptr && coil->Tag != nullptr) outputs->Add(coil->Tag);
            }
            for each (String^ inp in inputs) {
                for each (String^ outp in outputs) {
                    sb->AppendLine("  \"" + inp + "\" -> \"" + outp + "\" [label=\"" + net->Title + "\"];");
                }
            }
        }
        sb->AppendLine("}");
        return sb->ToString();
    }

private:
    static String^ GetInstrLabel(P5IRInstruction^ instr) {
        P5IRContact^ c = dynamic_cast<P5IRContact^>(instr);
        if (c != nullptr) return (c->Negated ? "NC " : "") + c->Tag;
        P5IRCoil^ coil = dynamic_cast<P5IRCoil^>(instr);
        if (coil != nullptr) return "Coil " + coil->Tag;
        P5IRCall^ call = dynamic_cast<P5IRCall^>(instr);
        if (call != nullptr) return call->Name;
        P5IRBranch^ b = dynamic_cast<P5IRBranch^>(instr);
        if (b != nullptr) return "Branch";
        return "?";
    }
};

ref class P5SimulationBackend : P5IBackend {
public:
    virtual String^ Generate(P5IRProgram^ program) {
        P5CompilerLog::Info("SIM", "Generating simulation model...");
        StringBuilder^ sb = gcnew StringBuilder();

        sb->AppendLine("PLC_Simulation_Model {");
        sb->AppendLine("  variables {");
        for each (String^ var in program->VariableNames) {
            sb->AppendLine("    " + var + " : BOOL := FALSE;");
        }
        sb->AppendLine("  }");
        sb->AppendLine();
        sb->AppendLine("  scan_cycle {");
        for each (P5IRNetwork^ net in program->Networks) {
            sb->AppendLine("    // " + net->Title);
            for each (P5IRInstruction^ instr in net->Instructions) {
                P5IRContact^ c = dynamic_cast<P5IRContact^>(instr);
                if (c != nullptr) {
                    sb->AppendLine("    READ(" + (c->Negated ? "NOT " : "") + c->Tag + ");");
                    continue;
                }
                P5IRCoil^ coil = dynamic_cast<P5IRCoil^>(instr);
                if (coil != nullptr) {
                    String^ action = "WRITE(" + coil->Tag + ", ";
                    if (coil->CoilType == P5CoilType::Set) action += "SET)";
                    else if (coil->CoilType == P5CoilType::Reset) action += "RESET)";
                    else action += "TRUE)";
                    sb->AppendLine("    " + action + ";");
                    continue;
                }
                P5IRCall^ call = dynamic_cast<P5IRCall^>(instr);
                if (call != nullptr) {
                    sb->AppendLine("    EXEC(" + call->Name + ");");
                }
            }
        }
        sb->AppendLine("  }");
        sb->AppendLine("}");

        P5CompilerLog::Info("SIM", "Simulation model generated: " + program->Networks->Count + " networks");
        return sb->ToString();
    }

    static Dictionary<String^, bool>^ SimulateCycle(P5IRProgram^ program, Dictionary<String^, bool>^ inputs) {
        Dictionary<String^, bool>^ state = gcnew Dictionary<String^, bool>(inputs);
        for each (P5IRNetwork^ net in program->Networks) {
            bool condition = true;
            for each (P5IRInstruction^ instr in net->Instructions) {
                P5IRContact^ c = dynamic_cast<P5IRContact^>(instr);
                if (c != nullptr && c->Tag != nullptr) {
                    bool val = false;
                    if (state->ContainsKey(c->Tag)) val = state[c->Tag];
                    condition = condition && (c->Negated ? !val : val);
                }
            }
            if (condition) {
                for each (P5IRInstruction^ instr in net->Instructions) {
                    P5IRCoil^ coil = dynamic_cast<P5IRCoil^>(instr);
                    if (coil != nullptr && coil->Tag != nullptr) {
                        if (coil->CoilType == P5CoilType::Reset) state[coil->Tag] = false;
                        else state[coil->Tag] = true;
                    }
                }
            }
        }
        return state;
    }
};

ref class P5BackendDispatcher {
public:
    static P5IBackend^ Create(P5BackendType type) {
        switch (type) {
        case P5BackendType::LAD:
            return gcnew P5LadBackend();
        case P5BackendType::SCL:
            return gcnew P5SclBackend();
        case P5BackendType::FBD:
            return gcnew P5FbdBackend();
        case P5BackendType::STL:
            return gcnew P5StlBackend();
        case P5BackendType::Visualization:
            return gcnew P5VisualizationBackend();
        case P5BackendType::Simulation:
            return gcnew P5SimulationBackend();
        default:
            return gcnew P5LadBackend();
        }
    }

    static String^ Generate(P5IRProgram^ program, P5BackendType type) {
        P5IBackend^ backend = Create(type);
        return backend->Generate(program);
    }
};

ref class P5DebugPipeline {
public:
    static void Dump(P5IRProgram^ program, String^ stage) {
        P5CompilerLog::Info("DEBUG", "=== Dump: " + stage + " ===");
        P5CompilerLog::Info("DEBUG", "Program: " + program->Name);
        P5CompilerLog::Info("DEBUG", "Networks: " + program->Networks->Count);
        for each (P5IRNetwork^ net in program->Networks) {
            P5CompilerLog::Info("DEBUG", "  Net: " + net->Title + " (" + net->Instructions->Count + " instrs)");
        }
    }

    static void Trace(P5IRProgram^ program, String^ fromStage, String^ toStage) {
        P5CompilerLog::Info("TRACE", fromStage + " -> " + toStage);
        P5CompilerLog::Info("TRACE", "  Networks: " + program->Networks->Count);
        int totalInstrs = 0;
        for each (P5IRNetwork^ net in program->Networks) totalInstrs += net->Instructions->Count;
        P5CompilerLog::Info("TRACE", "  Total instructions: " + totalInstrs);
    }

    static String^ GenerateErrorReport(P3ValidationResult^ validation) {
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("=== Error Report ===");
        if (validation->Errors->Count > 0) {
            sb->AppendLine("ERRORS (" + validation->Errors->Count + "):");
            for each (String^ e in validation->Errors) sb->AppendLine("  - " + e);
        }
        if (validation->Warnings->Count > 0) {
            sb->AppendLine("WARNINGS (" + validation->Warnings->Count + "):");
            for each (String^ w in validation->Warnings) sb->AppendLine("  - " + w);
        }
        if (validation->IsValid) sb->AppendLine("STATUS: PASSED");
        else sb->AppendLine("STATUS: FAILED");
        return sb->ToString();
    }

    static String^ Graph(P5IRProgram^ program) {
        P5CompilerLog::Info("DEBUG", "Generating IR graph...");
        StringBuilder^ sb = gcnew StringBuilder();
        sb->AppendLine("digraph IR_Debug {");
        sb->AppendLine("  rankdir=LR;");
        for each (P5IRNetwork^ net in program->Networks) {
            sb->AppendLine("  subgraph cluster_" + net->Title + " {");
            sb->AppendLine("    label=\"" + net->Title + "\";");
            for each (P5IRInstruction^ instr in net->Instructions) {
                String^ label = "?";
                P5IRContact^ c = dynamic_cast<P5IRContact^>(instr);
                if (c != nullptr) label = (c->Negated ? "NC " : "") + c->Tag;
                P5IRCoil^ coil = dynamic_cast<P5IRCoil^>(instr);
                if (coil != nullptr) label = "Coil " + coil->Tag;
                P5IRCall^ call = dynamic_cast<P5IRCall^>(instr);
                if (call != nullptr) label = call->Name;
                P5IRBranch^ b = dynamic_cast<P5IRBranch^>(instr);
                if (b != nullptr) label = "Branch(" + b->Children->Count + ")";
                sb->AppendLine("    ir" + instr->UId + " [label=\"" + label + "\"];");
            }
            for (int i = 0; i < net->Instructions->Count - 1; i++) {
                sb->AppendLine("    ir" + net->Instructions[i]->UId + " -> ir" + net->Instructions[i + 1]->UId + ";");
            }
            sb->AppendLine("  }");
        }
        sb->AppendLine("}");
        return sb->ToString();
    }
};

ref class P5Pipeline {
public:
    static P3PipelineResult^ RunPhase5(String^ problem, P3Config^ config, String^ templateXmlPath) {
        Console::WriteLine("=== Phase 5 Compiler Platform Pipeline ===");
        Console::WriteLine();
        P5CompilerLog::Info("FRONTEND", "Problem: " + problem);

        P5CompilerLog::Info("FRONTEND", "Step 1: Requirement Parsing...");
        P4Requirement^ requirement = P4RequirementParser::Parse(problem, config);
        if (requirement->Inputs->Count == 0 && requirement->Outputs->Count == 0) {
            P5CompilerLog::Warn("FRONTEND", "Requirement parsing failed, falling back to Phase 3");
            return P3Pipeline::Run(problem, config, templateXmlPath);
        }

        P5CompilerLog::Info("SEMANTIC", "Step 2: Semantic Planning...");
        P4SemanticPlan^ semanticPlan = P4SemanticPlanner::Plan(problem, requirement, config);
        if (semanticPlan->States->Count == 0 && !semanticPlan->IsSequential) {
            P5CompilerLog::Warn("SEMANTIC", "Non-sequential plan, falling back");
            return P3Pipeline::Run(problem, config, templateXmlPath);
        }

        P5CompilerLog::Info("VAR", "Step 3: Variable Planning...");
        P4VariablePlan^ varPlan = P4VariablePlanner::Plan(requirement, semanticPlan);

        P5CompilerLog::Info("LOGICGRAPH", "Step 4: LogicGraph Generation...");
        P3Dsl^ dsl = P4StateMachineBuilder::Build(requirement, semanticPlan, varPlan);
        P5CompilerLog::Info("LOGICGRAPH", "Variables: " + dsl->Variables->Count + ", Networks: " + dsl->Networks->Count);

        P5CompilerLog::Info("VAR", "Step 5: Auto-generating missing variables...");
        P3VariableEngine::AutoGenerateVariables(dsl);

        P5CompilerLog::Info("CFG", "Step 6: CFG Analysis...");
        ControlFlowGraph^ cfg = P5CfgAnalyzer::BuildCfg(dsl);
        List<String^>^ cycles = P5CfgAnalyzer::DetectCycles(cfg);
        List<String^>^ deadStates = P5CfgAnalyzer::DetectDeadStates(dsl);
        List<String^>^ unreachable = P5CfgAnalyzer::AnalyzeReachability(cfg);
        List<String^>^ branches = P5CfgAnalyzer::AnalyzeBranches(cfg);
        for each (String^ c in cycles) P5CompilerLog::Warn("CFG", c);
        for each (String^ d in deadStates) P5CompilerLog::Warn("CFG", d);
        for each (String^ u in unreachable) P5CompilerLog::Warn("CFG", u);

        P5CompilerLog::Info("CFG", "Step 6b: State Compression...");
        P5CfgAnalyzer::CompressStates(dsl);

        P5CompilerLog::Info("IR", "Step 7: Converting to P5 IR...");
        P5IRProgram^ ir = P5DslToIrConverter::Convert(dsl);
        P5DebugPipeline::Dump(ir, "IR");

        P5CompilerLog::Info("OPT", "Step 8: Optimization...");
        P5Optimizer::Optimize(ir);
        P5DebugPipeline::Trace(ir, "OPT", "POST-OPT");

        P5CompilerLog::Info("VAL", "Step 9: Validation (Phase 5 - 10 checks)...");
        P3ValidationResult^ validation = P5SemanticValidator::Validate(dsl);
        if (validation->Warnings->Count > 0) {
            for each (String^ w in validation->Warnings) P5CompilerLog::Warn("VAL", w);
        }
        if (!validation->IsValid) {
            for each (String^ e in validation->Errors) P5CompilerLog::Error("VAL", e);

            P5CompilerLog::Info("REPAIR", "Step 9b: Phase 5 Auto-Repair...");
            P5RepairEngine::AutoRepair(dsl);
            validation = P5SemanticValidator::Validate(dsl);
        }
        if (validation->IsValid) {
            P5CompilerLog::Info("VAL", "Validation: PASSED");
        }
        else {
            P5CompilerLog::Warn("VAL", "Validation: FAILED (proceeding with best-effort)");
        }

        P5CompilerLog::Info("BACKEND", "Step 10: Backend Generation...");

        P5CompilerLog::Info("LAD", "  Generating LAD XML...");
        LadDsl^ ladDsl = P3DslConverter::ToLadDsl(dsl);
        String^ xml = BuildLadXml(ladDsl, templateXmlPath);

        P5CompilerLog::Info("SCL", "  Generating SCL code...");
        String^ sclCode = P5BackendDispatcher::Generate(ir, P5BackendType::SCL);

        P5CompilerLog::Info("FBD", "  Generating FBD representation...");
        String^ fbdCode = P5BackendDispatcher::Generate(ir, P5BackendType::FBD);

        P5CompilerLog::Info("STL", "  Generating STL code...");
        String^ stlCode = P5BackendDispatcher::Generate(ir, P5BackendType::STL);

        P5CompilerLog::Info("VIS", "  Generating Visualization (5 types)...");
        P5VisualizationBackend^ visBackend = gcnew P5VisualizationBackend();
        String^ logicGraphDot = visBackend->Generate(ir);
        String^ cfgDot = P5VisualizationBackend::GenerateCfgDot(cfg);
        String^ irDot = visBackend->Generate(ir);
        String^ stateMachineDot = P5VisualizationBackend::GenerateStateMachineDot(semanticPlan);
        String^ networkTopologyDot = P5VisualizationBackend::GenerateNetworkTopology(ir);

        P5CompilerLog::Info("SIM", "  Generating Simulation model...");
        String^ simCode = P5BackendDispatcher::Generate(ir, P5BackendType::Simulation);

        P5CompilerLog::Info("SIM", "  Running dry-run simulation cycle...");
        Dictionary<String^, bool>^ initInputs = gcnew Dictionary<String^, bool>();
        for each (P3Variable^ v in dsl->Variables) {
            if (v->Scope == "input" && v->Name != nullptr) initInputs[v->Name] = false;
        }
        Dictionary<String^, bool>^ simResult = P5SimulationBackend::SimulateCycle(ir, initInputs);
        P5CompilerLog::Info("SIM", "  Simulation cycle completed: " + simResult->Count + " variables");

        P5CompilerLog::Info("DEBUG", "  Generating Error Report...");
        String^ errorReport = P5DebugPipeline::GenerateErrorReport(validation);

        P5CompilerLog::Info("DEBUG", "  Generating Debug Graph...");
        String^ debugGraph = P5DebugPipeline::Graph(ir);

        if (xml == nullptr || xml->Length == 0) {
            P3PipelineResult^ r = gcnew P3PipelineResult();
            r->Dsl = dsl;
            r->ErrorMessage = "Failed to compile LAD to XML";
            return r;
        }

        Console::WriteLine();
        Console::WriteLine("=== Phase 5 Pipeline Complete ===");
        Console::WriteLine("  LAD XML: " + xml->Length + " chars");
        Console::WriteLine("  SCL Code: " + sclCode->Length + " chars");
        Console::WriteLine("  FBD Code: " + fbdCode->Length + " chars");
        Console::WriteLine("  STL Code: " + stlCode->Length + " chars");
        Console::WriteLine("  Visualization: 5 graphs (LogicGraph/CFG/IR/StateMachine/NetworkTopology)");
        Console::WriteLine("  Simulation: " + simResult->Count + " variables simulated");
        Console::WriteLine("  Debug: ErrorReport + Graph generated");

        P3PipelineResult^ result = gcnew P3PipelineResult();
        result->Xml = xml;
        result->Dsl = dsl;
        result->TagTableXml = P3TagTableGenerator::GenerateTagTableXml(dsl);
        result->Success = true;
        return result;
    }
};
