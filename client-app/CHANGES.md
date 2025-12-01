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
| `search_window.h/.cpp/.ui` | 航班搜索主界面，包含查询条件、结果表格、预订逻辑以及进入“我的订单”入口。 |
| `myorders_window.h/.cpp/.ui` | “我的订单”窗口，展示订单列表并允许取消订单。 |
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
  - UI 引入“查询条件”分组、动作按钮区、交替行色表格及状态栏。
  - 新增舱位下拉框与“带儿童/带婴儿”复选框，通过 `currentCabinClassCode`、`selectedPassengerTypes`、`passengerSummaryText` 等辅助函数实现提示与参数同步。
  - 集成“我的订单”入口、当前用户摘要以及 `ProfileWindow` 快捷访问。
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
- 点击“我已完成支付”后才启用“提交订单”；提交会发射 `bookingConfirmed` 信号，由 `SearchWindow` 转成 `NetworkManager::bookFlightRequest`，不向服务器新增任何字段。

## 后续可选工作
1. 根据最终 UI 设计补充 `search_window.ui` 与 `myorders_window.ui` 的布局细节与样式。
2. 与服务器端约定响应字段后，进一步完善表格列（例如价格格式、状态颜色）。
3. 为 `NetworkManager` 拆分更多失败信号（如 `searchFailed`, `ordersFailed`），减少对 `generalError` 的依赖。
