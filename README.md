# 基于 Qt 的目标检测全流程桌面应用

> 工程基础与创新设计 B — 课程项目  
> 集**图像标注、模型训练、模型量化、模型推理**于一体的桌面应用，以口罩检测为应用场景。

---

## ✨ 功能特性

- 🖼️ **图像标注**：支持矩形框绘制、编辑、删除，YOLO 与 VOC 格式互转
- 🔄 **批量标注**：切换图片自动保存与加载，标注不丢失
- 🚀 **模型训练**：图形化配置 YOLO 超参，实时彩色日志展示
- 📦 **模型量化**：ONNX Runtime INT8 动态量化，模型体积缩小约 4 倍
- 🔍 **模型推理**：自适应 PyTorch (.pt) 与 ONNX (.onnx) 格式
- 💾 **数据持久化**：SQLite 记录训练与推理历史，支持可视化查询
- 🎨 **完整 GUI**：菜单栏、工具栏、状态栏、可停靠面板

---

## 🛠️ 技术栈

| 类别 | 技术 |
|------|------|
| GUI 框架 | Qt 6 + C++ 17 |
| 深度学习 | Python 3.9 + Ultralytics YOLO v11 |
| 模型量化 | ONNX Runtime |
| 数据库 | SQLite |
| 构建工具 | qmake + MinGW |
| 进程通信 | QProcess |

---

## 📁 项目结构

```
AnnotationTool/
├── main.cpp                    程序入口
├── mainwindow.cpp / .h         主窗口 (布局/菜单/信号路由)
├── mainwindow.ui               主窗口 UI 描述文件
│
├── imageview.cpp / .h          图像视图 (标注绘制/缩放/旋转)
├── annotationitem.h            标注框数据结构
├── labelmanager.cpp / .h       类别与颜色管理
│
├── trainwidget.cpp / .h        训练界面 (调用 Python 训练)
├── quantizewidget.cpp / .h     量化界面 (INT8 量化)
├── inferwidget.cpp / .h        推理界面 (模型加载与展示)
│
├── database.cpp / .h           SQLite 数据库封装 (单例)
├── dbviewer.cpp / .h           数据库记录查看器
│
├── quantize.py                 量化脚本 (ONNX Runtime)
├── infer.py                    推理脚本 (Ultralytics YOLO)
│
├── AnnotationTool.pro          Qt 项目配置文件
└── README.md                   项目说明
```

---

## 🚀 快速开始

### 环境要求

- Qt 6.x（推荐 6.5+）
- MinGW 或 MSVC 编译器
- Python 3.9+
- Anaconda（推荐）

### Python 依赖安装

```bash
conda create -n work2_env python=3.9
conda activate work2_env
pip install ultralytics onnxruntime onnx
```

### 编译运行

1. 使用 Qt Creator 打开 `AnnotationTool.pro`
2. 修改以下文件中的 Python 路径为你的实际路径：
   - `trainwidget.cpp` — Python 解释器路径
   - `quantizewidget.cpp` — Python 与 `quantize.py` 路径
   - `inferwidget.cpp` — Python 路径
3. 点击构建并运行

---

## 📖 使用说明

### 标注流程

1. 菜单 `File → Open Folder` 选择图像文件夹
2. 右侧面板选择或添加标签类别
3. 左键拖拽绘制标注框，右键取消选中，Delete 删除
4. 切换图片时自动保存当前标注

### 训练流程

1. 切换到"训练"Tab
2. 配置 Python 路径、数据集 yaml、训练超参（Epochs / Batch / 学习率）
3. 点击"开始训练"，日志框实时显示 YOLO 彩色输出
4. 训练完成后模型信息自动入库

### 量化流程

1. 切换到"量化"Tab
2. 选择训练输出的 `best.pt` 模型
3. 选择 INT8 量化精度
4. 一键完成量化，结果模型自动传递给推理模块

### 推理流程

1. 量化完成后自动跳转到"推理"Tab 并填充模型路径
2. 选择测试图片
3. 点击"运行推理"，右侧显示带检测框的结果图片与类别置信度
4. 推理记录自动入库

### 查看历史记录

- 菜单 `数据库 → 查看记录`，可分别查看训练与推理的历史数据

---

## 💡 核心亮点

### C++ / Python 混合编程

采用 QProcess 调用 Python 子进程，C++ 负责 GUI 与流程控制，Python 负责深度学习计算，充分发挥两种语言的优势。

### ANSI 转 HTML 解析器

自行实现终端颜色控制码解析器，将 YOLO 训练输出的 ANSI 转义序列（如 `\x1B[32m`）转换为 HTML `<span>` 标签，在 Qt 富文本框中完整保留原始配色。

### 模块化设计

四大功能模块通过 `QTabWidget` 分离，各自独立成 Widget，使用 Qt 信号槽机制通信，量化完成后通过 `quantizeFinished(QString)` 信号自动传递模型路径给推理模块，实现无缝衔接。

### 智能标注保存

切换图片时自动保存当前标注为 YOLO 格式 `.txt` 文件，切回时自动加载对应标注，无需手动操作，适合大批量数据标注场景。

---

## 📊 性能指标

| 指标 | 数值 |
|------|------|
| 图像加载延迟 | < 50ms (1080p) |
| 量化前模型大小 | 5.4 MB (YOLO11n) |
| 量化后模型大小 | 1.4 MB (INT8) |
| 压缩比 | ~3.86× |
| 推理耗时 (CPU) | ~500ms / 张 |

---

## 📄 License

MIT License

---

## 👤 作者

Lililyli

---

⭐ 如果觉得有帮助，欢迎 Star 支持！