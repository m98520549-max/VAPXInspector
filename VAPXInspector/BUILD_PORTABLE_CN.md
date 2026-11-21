# 制作 Windows 绿色免安装版（不需要在你的电脑安装 Qt）

本项目提供 GitHub Actions 云端构建配置。Qt、Visual Studio 和 CMake 都由 GitHub 的 Windows 构建机安装；你的电脑只需要浏览器和 GitHub 账号。

## 操作步骤

1. 登录 https://github.com/ 并创建一个空仓库，例如 `VAPXInspector`。仓库可设为 Private。
2. 在仓库页面选择 **Add file → Upload files**，把本项目解压后的全部文件和文件夹上传。注意要上传 `.github` 隐藏目录；如果网页看不到它，可用 GitHub Desktop 上传整个目录。
3. 点击 **Commit changes**。
4. 打开仓库的 **Actions** 页面，选择左侧 **Build Windows portable VAPX Inspector**。
5. 点击 **Run workflow → Run workflow**。首次构建需要等待几分钟。
6. 构建完成后点开绿色成功的运行记录，在 **Artifacts** 区域下载 `VAPXInspector-Windows-Portable`。
7. 解压下载的 ZIP，双击 `VAPXInspector.exe`。这个目录已由 `windeployqt` 收集 Qt 运行库，不需要在目标电脑安装 Qt。

## 注意

- 这是云端自动构建配置，不是已经编译好的 EXE。必须先完成以上 GitHub Actions 构建，才能下载绿色版。
- 若 Actions 失败，打开失败步骤，把红色错误日志复制给助手即可定位。
- Qt Multimedia 的视频格式支持取决于 Windows 多媒体后端和视频编码格式。
- 当前程序属于 MVP：可以解析 `vapc` 配置并进行基础图片/文字叠加预览；尚未完整实现 VAPX 的 `mFrame`、`mt`、Mask、Alpha 通道还原等效果，不能保证与官方播放器逐像素一致。
