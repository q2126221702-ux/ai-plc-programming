# Debug Session: ctu-missing-after-import

**Status:** [OPEN]
**Created:** 2026-05-22
**Bug:** 有CTU的程序导出后，再执行导入，CTU丢失

## 症状
- **实际行为:** 导出包含CTU的程序 → 导入后CTU消失
- **预期行为:** CTU应该完整保留

## 复现步骤
1. 在TIA Portal中创建包含CTU的程序
2. 执行导出（选项2）
3. 执行导入（选项1）
4. 检查导入后的程序，CTU丢失

## 假设
1. **导出时CTU未正确写入XML** - LadConversion.h中CTU的XML生成有问题
2. **导入时CTU未被正确解析** - LadXmlParser.h中CTU的解析逻辑有遗漏
3. **导入时CTU被TIA Portal拒绝** - XML Schema验证失败导致CTU被跳过
4. **CTU的Instance/Wire连接问题** - CTU的Instance DB或Wire连接不正确导致导入失败

## 插桩计划
- 在导出路径添加CTU诊断日志
- 在导入路径添加CTU诊断日志
- 检查生成的XML文件中CTU的完整结构

## 进展
- [ ] 步骤1: 添加插桩日志
- [ ] 步骤2: 复现问题并收集日志
- [ ] 步骤3: 分析证据
- [ ] 步骤4: 实施修复
- [ ] 步骤5: 验证修复
