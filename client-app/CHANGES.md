# Client-App Implementation Notes

## 概览
- 依据 `README.md` 第 3 节规范，补齐用户端必须的网络请求、信号与界面。
- 保持 `NetworkManager` 既有接口不变，在其基础上**新增**行为以支持注册、预订、查询订单与取消订单。
- 新增注册/查询/订单窗口，并以 `AppSession` 单例缓存登录信息，串联完整的用户流程。

## 新增文件
| 文件 | 说明 |
| --- | --- |
| `app_session.h/.cpp` | 负责保存当前登录用户 (`user_id`, `username`, `is_admin`)，供 UI 与网络请求复用。 |
| `register_window.h/.cpp/.ui` | 注册窗口，提供用户名/密码/确认密码输入，调用 `sendRegisterRequest` 并响应成功/失败信号。 |
| `search_window.h/.cpp/.ui` | 航班搜索主界面，包含查询条件、结果表格、预订逻辑以及进入"我的订单"入口。 |
| `myorders_window.h/.cpp/.ui` | "我的订单"窗口，展示订单列表并允许取消订单。 |
| `profile_window.h/.cpp/.ui` | 个人资料编辑窗口，允许修改用户名与密码。 |
| `booking_dialog.h/.cpp/.ui` | 预订填单对话框，用于选择乘机人、行李额与支付方式。 |
| `CHANGES.md` | 当前说明文档。 |

## 主要改动

### 1. CMake 构建（`client-app/CMakeLists.txt`）
- 将所有 Session/Window/Dialog 文件加入 `PROJECT_SOURCES`，确保一站式构建。
- 新增 `CLIENT_APP_USE_FAKE_SERVER` 选项。配置 `-DCLIENT_APP_USE_FAKE_SERVER=ON` 会为 `client-app` 目标定义 `USE_FAKE_SERVER` 宏，以开启本地假数据模式。

### 2. NetworkManager 扩展
- `network_manager.h/.cpp`：新增 `sendRegisterRequest`, `bookFlightRequest`, `getMyOrdersRequest`, `cancelOrderRequest`, `updateProfileRequest` 以及配套成功/失败信号；`onReadyRead()` 根据 `action_response` 精细路由。
- `USE_FAKE_SERVER` 模式下，`NetworkManager` 以 `emitFake*` 函数模拟登录/搜索/注册/预订/订单/取消/资料更新的响应，方便离线调试；默认情况下维持真实 TCP 通信。
- `sendSearchRequest` 支持可选的 `cabin_class` 与 `passenger_types` 字段，并仅在用户选择时注入 JSON，契合 README 接口规范。

### 3. 登录窗口
- `mainwindow.ui`：重建为包含用户名、密码、登录与“注册新用户”按钮的布局。
- `mainwindow.h/.cpp`：添加 `on_registerButton_clicked`、`onGeneralError` 等槽，并在登录成功后写入 `AppSession`、打开 `SearchWindow`。

### 4. 注册流程
- `register_window` 三件套：
  - 表单校验（空值/密码一致性）。
  - 调用 `NetworkManager::sendRegisterRequest` 并订阅 `registerSuccess/registerFailed/generalError`。
  - UI 中加入“填写信息”分组、密码规则与状态提示。

### 5. 航班查询与预订
- `SearchWindow`：
  - 读取日期/出发地/目的地后调用 `sendSearchRequest`，并展示 `searchResults`。
  - UI 引入"查询条件"分组、动作按钮区、交替行色表格及状态栏。
  - 新增舱位下拉框与"带儿童/带婴儿"复选框，通过 `currentCabinClassCode`、`selectedPassengerTypes`、`passengerSummaryText` 等辅助函数实现提示与参数同步。
  - 集成"我的订单"入口、当前用户摘要以及 `ProfileWindow` 快捷访问。
  - 预订按钮弹出 `BookingDialog`（见第 8 节），携带当前航班价格并在确认乘机人/行李/支付方式后才触发 `NetworkManager::bookFlightRequest`，不会改变既有网络协议。

### 6. 我的订单 & 取消
- `MyOrdersWindow`：
  - `showEvent` 和“刷新”按钮均会调用 `getMyOrdersRequest`。
  - UI 采用“标题+刷新”“只读表格”“底部状态+取消按钮”布局。
  - 订阅 `myOrdersResult` 更新表格，并在 `cancelOrderSuccess/cancelOrderFailed` 时提示用户、必要时自动刷新。

### 7. 个人主页 & 资料修改
- `profile_window` 三件套：显示当前用户信息，支持修改用户名/密码并进行校验。
- `AppSession` 的 `userChanged` 信号与多个窗口保持同步；提交后根据服务器响应刷新提示。
- `NetworkManager` 新增 `updateProfileRequest`、`profileUpdateSuccess/profileUpdateFailed` 与对应的 fake 响应，保证在线/离线都可验证。
- `SearchWindow` 提供“个人主页”按钮，懒加载 `ProfileWindow` 以减少资源占用。

### 8. 预订填单窗口（`booking_dialog.h/.cpp/.ui`）
- 新增 `BookingDialog`，并在 `CMakeLists.txt` 中注册。
- 对话框允许用户从候选列表或手动输入乘机人、选择托运行李额、选择支付方式，并使用 `QPainter` 随机生成二维码预览。
- 点击"我已完成支付"后才启用"提交订单"；提交会发射 `bookingConfirmed` 信号，由 `SearchWindow` 转成 `NetworkManager::bookFlightRequest`，不向服务器新增任何字段。


## 多用户端升级 (v2.0)

### 概览
根据 `NEW.md` 规范，在保持原有功能框架不变的前提下，实现多客户端并发支持。引入 **Tag 机制** 为每个客户端分配唯一身份标识，确保服务器能够识别和管理多个同时连接的用户。

### 升级要求
- 严格遵循 `NEW.md` 中的连接流程规范
- 保持所有现有业务接口不变
- 自动化 tag 注册，用户无感知
- 完全向后兼容原有功能

### 主要改动

#### 1. NetworkManager Tag 机制 (`network_manager.h/.cpp`)
**新增公共接口：**
- `sendTagRegistration(const QString& tag)` - 发送 tag 注册请求
- `isTagRegistered() const` - 检查 tag 注册状态
- `generateUniqueTag() const` - 生成唯一客户端标识

**新增信号：**
- `tagRegistered()` - tag 注册成功
- `tagRegistrationFailed(const QString& message)` - tag 注册失败

**核心逻辑变更：**
- 构造函数初始化 `m_tagRegistered = false` 和 `m_clientTag`
- `onConnected()` 自动生成唯一 tag 并发送注册请求
- `onDisconnected()` 重置 tag 注册状态
- `sendJsonRequest()` 增加 tag 注册状态检查，未注册时拒绝业务请求
- `onReadyRead()` 优先处理 tag 注册响应（无 `action_response` 字段）

**Tag 生成策略：**
```cpp
QString generateUniqueTag() const {
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    int random = QRandomGenerator::global()->bounded(1000, 9999);
    return QString("client_%1_%2").arg(timestamp).arg(random);
}
```

#### 2. MainWindow 连接流程优化 (`mainwindow.h/.cpp`)
**新增槽函数：**
- `onConnected()` - 处理服务器连接成功
- `onTagRegistered()` - 处理 tag 注册成功
- `onTagRegistrationFailed(const QString& message)` - 处理 tag 注册失败

**UI 交互增强：**
- 登录按钮点击前检查 tag 注册状态
- 未注册时显示友好提示："正在与服务器建立连接，请稍后再试..."
- tag 注册失败时显示错误对话框

**信号连接更新：**
```cpp
connect(&NetworkManager::instance(), &NetworkManager::connected,
        this, &MainWindow::onConnected);
connect(&NetworkManager::instance(), &NetworkManager::tagRegistered,
        this, &MainWindow::onTagRegistered);
connect(&NetworkManager::instance(), &NetworkManager::tagRegistrationFailed,
        this, &MainWindow::onTagRegistrationFailed);
```

#### 3. 连接时序规范
**严格按照 NEW.md 要求的通信顺序：**

1. **客户端启动** → `main.cpp` 自动连接服务器
2. **连接成功** → 触发 `onConnected()` 信号
3. **自动注册** → 发送第一条消息：`{"tag": "client_1701234567890_1234"}`
4. **等待确认** → 服务器返回：`{"status": "success", "message": "Tag registered"}`
5. **业务可用** → tag 注册成功后可发送正常业务请求

**错误处理机制：**
- 5秒超时自动断开未注册连接（服务器端）
- 客户端显示连接状态和错误提示
- 网络断开自动重置 tag 状态

### 兼容性保证

#### 现有功能完全不变
- 所有业务接口：`login`, `register`, `search_flights`, `book_flight` 等
- UI 界面和用户操作流程
- 数据格式和协议规范
- `AppSession` 会话管理

#### 向后兼容
- 原有 `USE_FAKE_SERVER` 模式继续支持
- 本地模拟模式自动跳过 tag 注册
- 服务器升级不影响客户端基本功能

### 技术特性

#### 多客户端支持
- **唯一标识**：每个客户端生成唯一的 tag（时间戳+随机数）
- **并发管理**：服务器通过 tag 区分不同客户端连接
- **状态隔离**：每个客户端维护独立的连接状态和用户会话

#### 自动化设计
- **零配置**：用户无需手动操作 tag 注册
- **透明处理**：tag 注册过程对用户完全透明
- **智能重连**：断线重连时自动重新注册 tag

#### 错误恢复
- **连接超时**：服务器端 5 秒超时保护
- **状态同步**：客户端实时同步连接状态
- **用户友好**：提供清晰的状态提示和错误信息

### 部署说明

#### 编译要求
- 需要链接 `Qt6::Core` 模块（用于 `QDateTime`, `QRandomGenerator`）
- 现有 CMake 配置无需修改
- 编译选项 `CLIENT_APP_USE_FAKE_SERVER` 继续有效

#### 运行要求
- 服务器端必须支持 tag 注册协议
- 客户端会自动适配支持/不支持 tag 的服务器
- 多客户端可同时连接同一服务器端口

### 测试验证

#### 功能测试
1. **单客户端**：验证原有功能完全正常
2. **多客户端**：验证多个客户端同时连接和使用
3. **异常场景**：验证网络断开、服务器重启等异常情况

#### 性能测试
- 并发连接数测试
- 长时间运行稳定性测试
- 内存泄漏检测

---

## Qt Quick (QML) UI 转换 (v3.0)

### 概述
项目已从 Qt Widgets 转换为 Qt Quick (QML) 界面，功能保持完全一致，使用更现代化的声明式 UI 框架。所有业务逻辑、网络通信和数据处理保持不变，仅 UI 层进行了重构。

### 新增文件

#### C++ 桥接类
- `qml_bridge.h/.cpp` - QML 和 C++ 逻辑之间的桥接类，负责将所有业务逻辑暴露给 QML，使用 Qt 属性系统实现数据绑定

#### QML 界面文件
- `qml/main.qml` - 主应用窗口，使用 StackView 管理页面导航
- `qml/LoginWindow.qml` - 登录窗口
- `qml/RegisterWindow.qml` - 注册窗口
- `qml/SearchWindow.qml` - 航班搜索与预订窗口
- `qml/OrdersWindow.qml` - 我的订单窗口
- `qml/ProfileWindow.qml` - 个人资料窗口

#### 资源与入口
- `qml.qrc` - QML 资源文件，将 QML 文件打包到应用程序中
- `main_qml.cpp` - QML 版本的主程序入口，使用 `QQmlApplicationEngine` 和 `QGuiApplication`

### 主要改动

#### 1. CMake 构建配置更新 (`CMakeLists.txt`)
- **添加 Qt Quick 模块**：`find_package` 添加 `Quick` 和 `QuickControls2` 组件
- **更新源文件列表**：使用 `main_qml.cpp` 替代 `main.cpp`，添加 `qml_bridge` 和 `qml.qrc`
- **更新链接库**：链接 `Qt::Quick` 和 `Qt::QuickControls2`，移除了对 Widgets 的依赖（可选）
- **禁用 AUTOUIC**：QML 模式不需要 UI 编译器

#### 2. QmlBridge 桥接类设计 (`qml_bridge.h/.cpp`)
**核心职责**：连接 C++ 后端和 QML 前端

**属性（Q_PROPERTY）**：
- `currentUsername`、`currentUserId`、`isLoggedIn` - 用户会话信息
- `searchResults`、`myOrders` - 数据列表

**槽函数**：
- 业务操作：`login()`, `registerUser()`, `searchFlights()`, `bookFlight()` 等
- 窗口管理：`showSearchWindow()`, `showOrdersWindow()` 等

**信号连接**：
- 自动连接所有 `NetworkManager` 信号到对应槽函数
- 将操作结果通过信号转发给 QML 层

#### 3. QML UI 设计特点
- **现代化 Material 风格**：蓝色主题 (`#2196F3`)，圆角卡片式设计
- **响应式布局**：使用 `ColumnLayout`、`RowLayout` 自动适配窗口大小
- **流畅导航**：`StackView` 管理页面切换，支持前进/后退
- **数据绑定**：QML 通过属性绑定自动更新 UI，无需手动刷新
- **交互反馈**：按钮状态变化、消息提示、选中高亮等

#### 4. 功能对照
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

### 架构优势

#### 声明式 UI
- QML 使用声明式语法，代码更简洁易读
- 属性绑定自动处理数据更新，减少手动同步代码

#### 性能优化
- `QGuiApplication` 比 `QApplication` 更轻量
- QML 引擎优化的渲染性能
- 更适合移动设备（未来扩展）

#### 易于扩展
- 支持丰富的动画效果（NumberAnimation、PropertyAnimation）
- 响应式设计，适配不同屏幕尺寸
- 主题切换和国际化支持

### 兼容性保证

✅ **完全向后兼容**：
- 所有业务逻辑保持不变（NetworkManager、AppSession）
- 服务器通信协议不变
- 数据库结构不变
- 原有 Widgets 代码保留作为参考（已注释）

### 使用说明

#### 构建与运行
1. CMakeLists.txt 已默认配置为 QML 模式
2. 重新配置 CMake：`cmake ..`
3. 编译项目：`cmake --build .` 或在 Qt Creator 中构建
4. 运行应用程序即可看到新的 QML UI

#### 切换回 Widgets 版本
如需切换回 Widgets 版本：
1. 在 `CMakeLists.txt` 中将 `main_qml.cpp` 改为 `main.cpp`
2. 取消注释 Widgets 相关源文件，注释 QML 相关文件
3. 将 `QGuiApplication` 改为 `QApplication`
4. 重新构建

#### 自定义样式
- 修改各 `.qml` 文件中的 `color` 属性改变主题色
- 调整 `Layout` 的 `spacing` 和 `anchors` 改变布局
- 使用 Qt Quick 动画系统添加过渡效果
- 可选：在 `main_qml.cpp` 中设置 `QQuickStyle::setStyle("Material")` 使用 Material 样式

### 技术细节

#### 页面导航流程
```
登录页 → 注册页（可返回）
登录页 → 搜索页（登录成功后）
搜索页 → 订单页/资料页（可返回，使用 StackView.push/pop）
```

#### 数据绑定示例
```qml
Text {
    text: "当前用户: " + (bridge ? bridge.currentUsername : "")
}
```
当 `bridge.currentUsername` 改变时，文本自动更新，无需手动调用更新函数。

### 未来扩展方向

QML UI 为以下扩展提供了更好的基础：
- 移动端应用（Android/iOS）- QML 原生支持
- 更丰富的动画效果 - Qt Quick 动画系统
- 更好的响应式设计 - Layout 自动适配
- 主题切换功能 - 动态加载 QML 主题文件
- 国际化支持 - QML 的 `qsTr()` 函数

详细使用说明请参考 `QML_UI_README.md`。

---

## 后续可选工作
1. 根据最终 UI 设计补充 `search_window.ui` 与 `myorders_window.ui` 的布局细节与样式。
2. 与服务器端约定响应字段后，进一步完善表格列（例如价格格式、状态颜色）。
3. 为 `NetworkManager` 拆分更多失败信号（如 `searchFailed`, `ordersFailed`），减少对 `generalError` 的依赖。
4. **多客户端优化**：添加连接状态指示器、重连机制、连接池管理等高级功能。
5. **QML UI 增强**：添加更多动画效果、主题切换、响应式优化等。
