# VAPX Inspector（Windows / Qt 6）

这是一个可编译的 Windows 桌面 MVP，用于检查 VAP/VAPX 文件中的 `vapc` 配置，并测试图片/文字占位符替换。

## 当前实现

- 读取 MP4 顶层 BOX，定位 `vapc` BOX 并解析 JSON。
- 展示 `info`、`src`、`frame` 中的资源声明。
- 按 `srcTag` 绑定本地图片或文字。
- 用 Qt Multimedia 解码视频帧，并在预览层绘制替换素材。
- 资源缺失、未引用、未知 srcId 等基础检查。
- 支持 JSON 配置单独载入，便于调试/对照。

## 重要限制（请先读）

这是用于占位符配置和资源映射验证的**可编译测试工具骨架**，不是腾讯官方 VAP 播放器的完整等价实现。

VAPX 最终合成还涉及 `aFrame`/`rgbFrame`、Alpha 区域、`mFrame` 遮罩纹理、`mt` 旋转、逐帧对象出现/消失、图片 fitType 规则及不同版本差异。本 MVP 会按 `frame[].obj[].frame` 绘制替换内容，但**尚未实现 VAP 原生 Mask/Porter-Duff 合成与全部 Alpha 还原**。因此它适合检查：
1. vapc 是否读取成功；
2. srcTag/srcId 映射是否正确；
3. 图片/文字资源是否加载；
4. 大致坐标、尺寸、层级是否正确；
5. 资源缺失与配置引用错误。

如果要做到与官方播放器像素级一致，下一阶段需逐项移植腾讯 Web/Android/iOS 的 `VapFrameParser`、Mask 合成及视频 Alpha 处理逻辑。

## 环境

- Windows 10/11 x64
- Visual Studio 2022（Desktop development with C++）
- CMake 3.21+
- Qt 6.5+（MSVC 2022 64-bit，安装 Widgets、Multimedia）
- FFmpeg 不要求单独安装；本项目使用 Qt Multimedia 后端解码，具体可播放编码取决于 Windows/Qt Multimedia 后端。

## 编译

在 Qt 6 的 MSVC x64 开发者命令行中：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64"

cmake --build build --config Release
```

运行：

```powershell
.\build\Release\VAPXInspector.exe
```

若 Qt 安装路径不同，请替换 `CMAKE_PREFIX_PATH`。

部署（在 Qt 环境变量已配置时）：

```powershell
windeployqt .\build\Release\VAPXInspector.exe
```

## 使用

1. 点击“打开 VAP/MP4”，选择 `.vap` 文件。
2. 程序尝试读取文件中的 `vapc` BOX。
3. 如果已有独立 JSON，也可点击“打开 vapc JSON”载入。
4. 在左侧选择 `src` 项。
5. 图片资源：点击“绑定图片”；文字资源：在右侧输入替换文字并点击“应用文字”。
6. 点击“播放”。也可以点击列表中的帧号/拖动进度条查看不同帧。
7. “检查”页会列出缺失绑定、未引用资源和无效 srcId。

## vapc JSON 结构预期

```json
{
  "info": { "w": 720, "h": 1280, "fps": 20, "f": 160, "isVapx": 1 },
  "src": [
    { "srcId": "1", "srcType": "img", "srcTag": "[imgUser]", "w": 100, "h": 100, "fitType": "centerCrop" },
    { "srcId": "2", "srcType": "txt", "srcTag": "[txtName]", "w": 240, "h": 48, "color": "#FFFFFF", "style": "b" }
  ],
  "frame": [
    { "i": 0, "obj": [
      { "srcId": "1", "z": 1, "frame": [20, 30, 100, 100], "mFrame": [0, 0, 100, 100] }
    ]}
  ]
}
```

`srcTag` 中的方括号会在 UI 里自动规范化为 `imgUser` / `txtName` 绑定键；也接受直接使用完整 tag。

## 测试

可先用 `test/sample_vapc.json` 验证 UI，不需要真实 VAP 文件。

## 绿色免安装版

如果不想在本机安装 Qt / Visual Studio / CMake，请看 [BUILD_PORTABLE_CN.md](BUILD_PORTABLE_CN.md)。项目包含 GitHub Actions 云端 Windows 构建流程，构建成功后会生成 `VAPXInspector-Windows-Portable.zip`。
