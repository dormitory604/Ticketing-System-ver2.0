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
| `favorites_window.h/.cpp/.ui` | "我的收藏"窗口，展示用户收藏的航班列表，支持查看、刷新和取消收藏功能。 |
| `CHANGES.md` | 当前说明文档。 |

## 主要改动

### 1. CMake 构建（`client-app/CMakeLists.txt`）
- 将所有 Session/Window/Dialog 文件加入 `PROJECT_SOURCES`，确保一站式构建。
- 新增 `CLIENT_APP_USE_FAKE_SERVER` 选项。配置 `-DCLIENT_APP_USE_FAKE_SERVER=ON` 会为 `client-app` 目标定义 `USE_FAKE_SERVER` 宏，以开启本地假数据模式。

### 2. NetworkManager 扩展
- `network_manager.h/.cpp`：新增 `sendRegisterRequest`, `bookFlightRequest`, `getMyOrdersRequest`, `cancelOrderRequest`, `updateProfileRequest` 以及配套成功/失败信号；`onReadyRead()` 根据 `action_response` 精细路由。
- **收藏功能扩展**：新增 `addFavoriteRequest(int userId, int flightId)`, `removeFavoriteRequest(int userId, int flightId)`, `getMyFavoritesRequest(int userId)` 三个请求函数，对应服务器 action：`"add_favorite"`, `"remove_favorite"`, `"get_my_favorites"`。
- **收藏相关信号**：新增 `addFavoriteSuccess`, `addFavoriteFailed`, `removeFavoriteSuccess`, `removeFavoriteFailed`, `myFavoritesResult` 五个信号，用于 UI 层响应收藏操作的结果。
- `USE_FAKE_SERVER` 模式下，`NetworkManager` 以 `emitFake*` 函数模拟登录/搜索/注册/预订/订单/取消/资料更新/收藏的响应，方便离线调试；默认情况下维持真实 TCP 通信。
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
  - **收藏功能集成**：
    - 新增"添加到收藏"按钮（`addToFavoritesButton`），点击时检查是否选中航班，调用 `NetworkManager::addFavoriteRequest`。
    - 新增"我的收藏"按钮（`myFavoritesButton`），懒加载并显示 `FavoritesWindow`。
    - 连接 `addFavoriteSuccess` 和 `addFavoriteFailed` 信号，显示操作结果提示。
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

### 9. 我的收藏功能（`favorites_window.h/.cpp/.ui`）
- **功能概述**：允许用户收藏感兴趣的航班，并在专门的窗口中查看和管理收藏列表。
- **窗口界面**：
  - 使用 `QTableWidget` 展示收藏的航班信息，包含 7 列：航班号、出发地、目的地、起飞时间、到达时间、价格、收藏时间。
  - 显示当前用户信息摘要（`userSummaryLabel`）。
  - 提供"取消收藏"、"刷新"、"返回"三个操作按钮。
- **核心功能实现**：
  - **查看收藏列表**（`getMyFavoritesRequest`）：
    - 窗口显示时（`showEvent`）自动请求当前用户的收藏列表。
    - 点击"刷新"按钮可手动刷新列表。
    - 收到 `myFavoritesResult` 信号后，调用 `populateFavoritesTable` 填充表格。
  - **添加收藏**（`addFavoriteRequest`）：
    - 在 `SearchWindow` 中实现，用户选中航班后点击"添加到收藏"按钮即可。
    - 成功后显示提示消息。
  - **取消收藏**（`removeFavoriteRequest`）：
    - 用户选择表格中的航班行后，"取消收藏"按钮自动启用（通过 `itemSelectionChanged` 信号控制）。
    - 取消收藏成功后自动刷新列表，保持数据同步。
- **数据存储**：
  - 使用 `Qt::UserRole` 和 `Qt::UserRole + 1` 在表格项中存储 `favorite_id` 和 `flight_id`，用于取消收藏时的数据关联。
  - 窗口内部维护 `m_latestFavorites` 数组，保存最新的收藏数据。
- **信号连接**：
  ```cpp
  connect(&NetworkManager::instance(), &NetworkManager::myFavoritesResult, ...);
  connect(&NetworkManager::instance(), &NetworkManager::removeFavoriteSuccess, ...);
  connect(&NetworkManager::instance(), &NetworkManager::removeFavoriteFailed, ...);
  connect(&NetworkManager::instance(), &NetworkManager::generalError, ...);
  ```
- **时间格式化**：
  - 起飞/到达时间：使用 `QDateTime::fromString(..., Qt::ISODate)` 解析 ISO 格式时间，显示为 "MM-dd hh:mm"。
  - 收藏时间：同样格式化为 "MM-dd hh:mm"，展示用户何时添加该收藏。
- **用户体验优化**：
  - 表格自动调整列宽（`resizeColumnsToContents`），最后一列自动拉伸（`setStretchLastSection`）。
  - 未选中航班时禁用"取消收藏"按钮，防止误操作。
  - 取消收藏成功后自动刷新，无需手动操作。
- **懒加载设计**：
  - `FavoritesWindow` 在 `SearchWindow` 中采用懒加载，首次点击"我的收藏"按钮时才创建实例。
  - 窗口关闭时保留实例（不立即删除），再次打开时重用，减少资源开销。

---

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

## 后续可选工作
1. 根据最终 UI 设计补充 `search_window.ui` 与 `myorders_window.ui` 的布局细节与样式。
2. 与服务器端约定响应字段后，进一步完善表格列（例如价格格式、状态颜色）。
3. 为 `NetworkManager` 拆分更多失败信号（如 `searchFailed`, `ordersFailed`），减少对 `generalError` 的依赖。
4. **多客户端优化**：添加连接状态指示器、重连机制、连接池管理等高级功能。
