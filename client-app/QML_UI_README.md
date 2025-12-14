# Qt Quick (QML) UI 使用说明

## 概述

本项目已从 Qt Widgets 转换为 Qt Quick (QML) 界面，功能保持完全一致，但使用更现代化的声明式 UI 框架。

## 文件结构

### C++ 桥接类
- `qml_bridge.h/.cpp` - QML 和 C++ 逻辑之间的桥接类，负责将所有业务逻辑暴露给 QML

### QML 界面文件
- `qml/main.qml` - 主应用窗口，管理页面导航
- `qml/LoginWindow.qml` - 登录窗口
- `qml/RegisterWindow.qml` - 注册窗口
- `qml/SearchWindow.qml` - 航班搜索窗口
- `qml/OrdersWindow.qml` - 我的订单窗口
- `qml/ProfileWindow.qml` - 个人资料窗口

### 资源文件
- `qml.qrc` - QML 资源文件，将 QML 文件打包到应用程序中

### 主程序入口
- `main_qml.cpp` - QML 版本的主程序入口，使用 `QQmlApplicationEngine`

## 构建说明

### CMakeLists.txt 变更

1. **添加了 Qt Quick 模块**：
   ```cmake
   find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools Network Quick)
   find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS 
       Quick
       QuickControls2
   )
   ```

2. **更新了源文件列表**：
   - 使用 `main_qml.cpp` 替代 `main.cpp`
   - 添加了 `qml_bridge.cpp/.h`
   - 添加了 `qml.qrc` 资源文件
   - 原有的 Widgets 文件已注释，保留作为参考

3. **更新了链接库**：
   - 添加了 `Qt::Quick` 和 `Qt::QuickControls2`
   - 可选：如果不需要 Widgets，可以移除 `Qt::Widgets`

### 构建步骤

1. **重新配置 CMake**：
   ```bash
   cd build
   cmake ..
   ```

2. **编译项目**：
   ```bash
   cmake --build .
   ```

   或在 Qt Creator 中：
   - **构建** → **重新构建项目**

## 功能对照

所有原有功能均已完整实现：

| 功能 | Widgets 版本 | QML 版本 | 状态 |
|------|-------------|---------|------|
| 用户登录 | MainWindow | LoginWindow.qml | ✅ |
| 用户注册 | RegisterWindow | RegisterWindow.qml | ✅ |
| 航班搜索 | SearchWindow | SearchWindow.qml | ✅ |
| 航班预订 | SearchWindow | SearchWindow.qml | ✅ |
| 我的订单 | MyOrdersWindow | OrdersWindow.qml | ✅ |
| 取消订单 | MyOrdersWindow | OrdersWindow.qml | ✅ |
| 个人资料 | ProfileWindow | ProfileWindow.qml | ✅ |
| 网络通信 | NetworkManager | 通过 QmlBridge | ✅ |

## 架构说明

### QmlBridge 类

`QmlBridge` 是一个 `QObject` 派生类，充当 C++ 后端和 QML 前端之间的桥梁：

1. **属性（Properties）**：
   - `currentUsername` - 当前用户名
   - `currentUserId` - 当前用户 ID
   - `isLoggedIn` - 是否已登录
   - `searchResults` - 搜索结果
   - `myOrders` - 我的订单

2. **槽函数（Slots）**：
   - 所有业务操作函数（login, register, search, book 等）
   - 窗口管理函数

3. **信号（Signals）**：
   - 所有操作结果信号
   - 窗口切换信号

### 页面导航

使用 `StackView` 管理页面导航：
- 登录页 → 注册页（可返回）
- 登录页 → 搜索页（登录成功后）
- 搜索页 → 订单页/资料页（可返回）

### 数据绑定

QML 通过属性绑定自动更新 UI：
```qml
Text {
    text: "当前用户: " + (bridge ? bridge.currentUsername : "")
}
```

当 `bridge.currentUsername` 改变时，文本会自动更新。

## 切换到 QML UI

### 如果当前使用 Widgets 版本

要使用新的 QML UI，只需要：

1. **确保 CMakeLists.txt 使用 QML 配置**（已默认设置）

2. **重新构建项目**

3. **运行应用程序**

### 切换回 Widgets 版本

如果需要切换回 Widgets 版本：

1. 在 `CMakeLists.txt` 中：
   - 将 `main_qml.cpp` 改为 `main.cpp`
   - 取消注释 Widgets 相关源文件
   - 注释掉 QML 相关源文件
   - 将 `QGuiApplication` 改为 `QApplication`

2. 重新构建

## 自定义 UI 样式

QML UI 可以轻松自定义样式：

1. **修改颜色主题**：
   在各个 `.qml` 文件中修改 `color` 属性

2. **修改布局**：
   调整 `ColumnLayout`、`RowLayout` 的 `spacing` 和 `anchors`

3. **添加动画**：
   使用 Qt Quick 的动画系统（`NumberAnimation`、`PropertyAnimation` 等）

4. **使用 Material 样式**：
   在 `main_qml.cpp` 中取消注释：
   ```cpp
   QQuickStyle::setStyle("Material");
   ```

## 注意事项

1. **QGuiApplication vs QApplication**：
   - QML 版本使用 `QGuiApplication`（轻量级）
   - Widgets 版本使用 `QApplication`（完整功能）

2. **资源文件**：
   - QML 文件必须通过 `qrc` 文件包含
   - 修改 QML 文件后需要重新编译 `qrc`

3. **性能**：
   - QML UI 启动可能稍快
   - 界面响应更流畅
   - 更适合移动设备（如果将来需要）

4. **调试**：
   - 可以使用 Qt Creator 的 QML 调试器
   - 支持 QML Profiler 进行性能分析

## 兼容性

- ✅ 所有业务逻辑保持不变
- ✅ NetworkManager 完全兼容
- ✅ AppSession 完全兼容
- ✅ 服务器通信协议不变
- ✅ 数据库结构不变

## 未来扩展

QML UI 为以下扩展提供了更好的基础：
- 移动端应用（Android/iOS）
- 更丰富的动画效果
- 更好的响应式设计
- 主题切换功能
- 国际化支持


