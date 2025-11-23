# admin-app 更新日志

### 0. NetworkManager 复制并添加函数

- [x] **复制基础代码**
    - [x] 从 `Client-app` 中复制 `network_manager.h` 和 `network_manager.cpp` 到 `admin-app`。

- [x] **添加发送函数**
    *依据 README 项目接口文档 3.3，增加以下5个函数：*
    - [x] 实现 `sendAdminAddFlightRequest()` (增航班 -> 对应 `handleAdminAddFlight`)
    - [x] 实现 `sendAdminUpdateFlightRequest()` (改航班 -> 对应 `handleAdminUpdateFlight`)
    - [x] 实现 `sendAdminDeleteFlightRequest()` (删航班 -> 对应 `handleAdminDeleteFlight`)
    - [x] 实现 `sendAdminGetAllUsersRequest()` (查所有用户 -> 对应 `handleAdminGetAllUsers`)
    - [x] 实现 `sendAdminGetAllBookingsRequest()` (查所有订单 -> 对应 `handleAdminGetAllBookings`)

- [x] **添加信号 (Signals)**
    *在 `network_manager.h` 中添加以下信号：*
    - [x] `adminOperationSuccess()`: 增、删、改航班成功时触发。
    - [x] `adminOperationFailed()`: 增、删、改航班失败时触发。
    - [x] `allUsersReceived()`: 收到用户列表时触发。
    - [x] `allBookingsReceived()`: 收到订单列表时触发。

- [x] **管理员登录逻辑**
    - [x] 实现 `sendAdminLoginRequest()`: 发送登录请求。
    - [x] 添加信号 `loginResult` (参数包含结果和 `is_admin`)。
    - [x] 完善 `onReadyRead()`: 解析登录结果信号。

---

### 1. 管理员登录窗口 (AdminLoginWindow类)

- [x] **界面设计**
    - [x] 完成 `admin_login_window.ui` 的布局。

- [x] **业务逻辑实现**
    - [x] 点击“登录”按钮：调用 `NetworkManager` 发送请求。
    - [x] **处理登录回复 (收到信号后):**
        - [x] 判断 `is_admin` 字段。
        - [x] **是 (1)**: 登录成功，跳转到管理员主页面 (关闭当前窗口)。
        - [x] **否 (0)**: 登录失败，提示“非管理员权限”。
        - [x] **密码/账号错误**: 提示“输入的账号或密码不对”。

---

### 2. 管理员主页面 (AdminDashboard类)

- [x] **文件创建**
    - [x] 创建 `admin_dashboard.h`, `admin_dashboard.cpp`, `admin_dashboard.ui`。

- [ ] **界面设计 (UI)**
    - [ ] 使用 `QTabWidget` 分为3个标签页。

- [ ] **标签页 1: 航班管理**
    - [ ] 放置表格 `QTableWidget`。
    - [ ] 设置3个按钮：**添加**、**删除**、**修改**。
    - [ ] 逻辑：调用 `NetworkManager` 获取数据并填进表格。

- [ ] **标签页 2: 用户列表 (只读)**
    - [ ] 放置表格 `QTableWidget`。
    - [ ] 逻辑：调用 `NetworkManager` 获取数据并填进表格。

- [ ] **标签页 3: 订单列表 (只读)**
    - [ ] 放置表格 `QTableWidget`。
    - [ ] 逻辑：调用 `NetworkManager` 获取数据并填进表格。
