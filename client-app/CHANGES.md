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
| `CHANGES.md` | 当前说明文档。 |

## 主要改动
1. **CMake 构建**（`client-app/CMakeLists.txt`）
   - 将新建的 Session/Window 文件加入 `PROJECT_SOURCES`，确保会随项目一起编译。
   - 新增 `CLIENT_APP_USE_FAKE_SERVER` 选项。配置时传入 `-DCLIENT_APP_USE_FAKE_SERVER=ON` 会为 `client-app` 目标定义 `USE_FAKE_SERVER` 宏，以开启本地假数据模式。

2. **NetworkManager 扩展**
   - `network_manager.h/.cpp`：新增 `sendRegisterRequest`, `bookFlightRequest`, `getMyOrdersRequest`, `cancelOrderRequest` 以及对应成功/失败信号；`onReadyRead()` 根据 `action_response` 精细路由；错误时触发更具体的信号满足 README 的发射器/信号规范。
   - 当 `USE_FAKE_SERVER` 宏开启时，`NetworkManager` 不再创建真实 `QTcpSocket`，而是在各个 `send*Request` 中直接调用 `emitFake*` 系列函数，通过 `QTimer` 异步发射与服务器响应一致的信号（登录/搜索/注册/预订/订单/取消），方便离线测试 UI 流程；默认情况下该宏未启用，仍保持对真实服务器的 TCP 通信。
   - `sendSearchRequest` 现在支持可选的 `cabin_class` 及 `passenger_types` 参数，并在 JSON 请求中附带，保证网络层仍遵循 README 的接口约定。

3. **登录窗口改造**
   - `mainwindow.ui`：重建 UI，包含用户名/密码输入、登录按钮与“注册新用户”按钮，并提示用户流程。
   - `mainwindow.h/.cpp`：增加槽函数 `on_registerButton_clicked`, `onGeneralError`，并在登录成功后使用 `AppSession` 保存用户信息、打开 `SearchWindow`。

4. **注册流程**
   - `RegisterWindow`（三件套）：
     - 表单校验（空值、密码一致性）。
     - 发起 `NetworkManager::sendRegisterRequest`。
     - UI 优化：`register_window.ui` 引入“填写信息”分组、密码规则提示与说明标签，让用户明白输入要求并及时看到注册状态。
     - 订阅 `registerSuccess/registerFailed/generalError` 信号以反馈结果并在成功后关闭窗口。

5. **航班查询与预订**
   - `SearchWindow`：
     - 读取日期/出发地/目的地，调用 `sendSearchRequest`。
     - UI 优化：`search_window.ui` 引入“查询条件”分组、动作按钮区以及带交替行色的 `QTableWidget` 和状态栏，便于与槽函数联动。
     - 使用 `QTableWidget` 展示航班数组 (`searchResults`)，并允许选中航班后调用 `bookFlightRequest`。
     - 集成“我的订单”按钮打开 `MyOrdersWindow`。
     - 显示当前登录用户信息（依赖 `AppSession`）。
     - 新增舱位下拉框（不限/经济/公务头等）与“带儿童/带婴儿”复选框，`search_window.cpp` 中新增 `currentCabinClassCode/selectedPassengerTypes` 等辅助函数，用户勾选后既能在提示文案中看到概要，也能随按钮点击同步传入 `sendSearchRequest`。

6. **我的订单 & 取消**
   - `MyOrdersWindow`：
     - 在 `showEvent` 与“刷新”按钮时调用 `getMyOrdersRequest`。
     - UI 优化：`myorders_window.ui` 加入标题+刷新区、中部只读订单表、底部状态+取消按钮，使数据流向更清晰。
     - 订阅 `myOrdersResult` 更新表格；对 `cancelOrderSuccess/cancelOrderFailed` 做 UI 提示并在成功后刷新。
     - 选中表格行后调用 `cancelOrderRequest`。

7. **个人主页 & 资料修改**
   - 新增 `profile_window.h/.cpp/.ui`：
     - 窗口展示当前用户 ID / 用户名，允许输入新的用户名与密码并执行表单校验（空值、长度、确认密码一致）。
     - `AppSession` 的 `userChanged` 信号与窗口联动，保持多窗数据同步；提交后根据结果更新提示及输入状态。
   - `search_window.ui/.cpp/.h`：
     - 新增“个人主页”按钮，并在 `SearchWindow` 中懒加载 `ProfileWindow` 以便登录后随时修改资料。
   - `network_manager.h/.cpp`：
     - 保持原有接口不动，新增 `updateProfileRequest` 发送函数与 `profileUpdateSuccess/profileUpdateFailed` 信号；在 `onReadyRead()` 中路由 `update_profile` 成功/失败响应。
     - FAKE_SERVER 模式补充 `emitFakeProfileUpdateResponse`，便于离线验证个人主页流程。
   - `client-app/CMakeLists.txt`：将 `profile_window` 三件套加入 `PROJECT_SOURCES`，确保构建。

## 后续可选工作
1. 根据最终 UI 设计补充 `search_window.ui`、`myorders_window.ui` 的布局细节与样式。
2. 与服务器端约定响应字段后，进一步完善表格列（例如价格格式、状态枚举颜色）。
3. 为 `NetworkManager` 新增更细粒度的失败信号（如 `searchFailed`, `ordersFailed`）以减少对 `generalError` 的依赖。
