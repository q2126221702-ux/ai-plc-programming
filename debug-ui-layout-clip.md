# Debug Session: ui-layout-clip

**Status**: [OPEN]
**Created**: 2026-05-23
**Bug**: UI布局控件显示不全 - 左侧树节点截断/不可见，右侧输入控件不可见

## Hypotheses

1. **控件宽度溢出父容器**: 子控件初始宽度超过父Panel可用宽度，导致控件被裁剪
2. **SplitContainer实际可用空间不足**: form高度(900)减去MenuStrip/ToolStrip/StatusStrip后，splitRight的Panel1实际高度远小于SplitterDistance
3. **Padding/Location坐标计算错误**: pnlInput的Padding与子控件Location产生偏移，导致部分控件在可视区域外
4. **Form尺寸超出屏幕**: 1400x900的窗口在常见1920x1080屏幕(含任务栏)上可能被裁剪
5. **DPI缩放问题**: 系统DPI缩放(125%/150%)导致像素计算偏移

## Observation Points

- 运行时各容器实际ClientSize
- 各控件实际Location和Size
- Form实际位置和屏幕工作区大小
- DPI设置

## Evidence Log

(待收集)