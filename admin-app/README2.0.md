# admin-app 开发进度报告 (v2.0)

**日期：** 2025-11-30
**状态：** 客户端核心逻辑完成 / 等待服务器联调 / 待进行 UI 美化

---

## 1. 本次更新概览
本次更新完成了 `admin-app` 的所有核心业务逻辑开发，修复了网络通信中的身份认证与粘包问题，并实现了完整的 GUI 交互流程。

### ✅ 已完成模块
- [x] **NetworkManager 升级**：实现了身份认证机制（Tag）与 TCP 粘包处理。
- [x] **登录模块**：实现了管理员身份鉴权与错误处理。
- [x] **主控台 (Dashboard)**：实现了航班增删改查、用户查看、订单查看的 UI 与逻辑。
- [x] **航班弹窗 (FlightDialog)**：实现了航班信息的表单录入与数据打包。

---

## 2. 详细更新内容

### 2.1 网络通信层 (`network_manager`)
1.  **身份认证 (Tag Auth)**：
    *   在 `onConnected` 中增加了发送 `{"tag": "admin_xxxx"}` 的逻辑。
    *   解决连接后无法立即通信的问题（适配 Server 端的防重复连接机制）。
2.  **管理员接口实现**：
    *   实现了 `sendAdminAddFlight` (增)、`sendAdminDeleteFlight` (删)、`sendAdminUpdateFlight` (改)。
    *   实现了 `sendAdminGetAllUsers` (查用户)、`sendAdminGetAllBookings` (查订单)。
3.  **信号系统**：
    *   增加了 `adminOperationSuccess` / `adminOperationFailed` 等信号，用于通知 UI 层操作结果。

### 2.2 界面交互层 (`admin_dashboard`)
1.  **TCP 粘包修复**：
    *   在“刷新数据”时引入 `QTimer::singleShot`，将多个查询请求间隔 200ms 发送，彻底解决服务器解析 `Invalid JSON` 的问题。
2.  **增删改查闭环**：
    *   **添加/修改**：通过 `FlightDialog` 收集数据 -> 调用 `NetworkManager` 发送 -> 接收成功信号 -> 自动刷新列表。
    *   **删除**：增加了二次确认弹窗 (`QMessageBox`)，防止误操作。
3.  **UI 细节**：
    *   表格设置为只读 (`NoEditTriggers`) 和整行选中 (`SelectRows`)。
    *   实现了数据回显（点击“修改”时，旧数据会自动填入弹窗）。

### 2.3 辅助模块 (`flightdialog`)
1.  **表单验证**：完成了 `.ui` 布局修复（解决了 `uic` 编译报错问题）。
2.  **数据封装**：实现了 `getFlightData()` 函数，将界面上的输入框内容自动打包为标准 JSON 对象。

---

## 3. 关键依赖：Server 端待办事项 (Blockers)
**目前 Admin 端功能无法完全跑通，是因为 Server 端尚未实现对应的处理逻辑。**
请 Server 端开发人员尽快补全 `tcp_server.cpp` 中的以下 5 个接口：

| Action (客户端指令) | 缺失的 Server 函数 | 描述 |
| :--- | :--- | :--- |
| `admin_add_flight` | `handleAdminAddFlight` | 接收航班 JSON，执行 SQL `INSERT` 插入到 `Flight` 表。**注意：需自动计算 `arrival_time`。** |
| `admin_delete_flight` | `handleAdminDeleteFlight` | 接收 `flight_id`，执行 SQL `DELETE` 删除航班。 |
| `admin_update_flight` | `handleAdminUpdateFlight` | 接收 `flight_id` 及变更数据，执行 SQL `UPDATE`。 |
| `admin_get_all_users` | `handleAdminGetAllUsers` | 执行 SQL `SELECT * FROM User`，返回 JSON 数组。 |
| `admin_get_all_bookings`| `handleAdminGetAllBookings`| 执行 SQL `SELECT * FROM Booking`，返回 JSON 数组。 |

---

## 4. Admin 端后续操作计划 (Next Steps)

### 第一步：启用完整查询 (联调准备)
Server 端代码更新后，请在 `admin_dashboard.cpp` 中**解开以下注释**：

```cpp
// 文件：admin_dashboard.cpp -> on_btnRefresh_clicked()
void AdminDashboard::on_btnRefresh_clicked()
{
    NetworkManager::instance().sendGetAllFlightsRequest();

    // [TODO] Server端更新后解开：
    QTimer::singleShot(200, [this](){
        NetworkManager::instance().sendAdminGetAllUsersRequest();
    });
    QTimer::singleShot(400, [this](){
        NetworkManager::instance().sendAdminGetAllBookingsRequest();
    });
}
```

### 第二步：全流程联调测试
1.  **功能测试**：测试增、删、改航班是否能实时同步到数据库。
2.  **多页面测试**：切换到用户管理/订单管理 Tab，确认数据加载正常。

### 第三步：UI/UX 界面与体验优化 (重要)
目前界面较为基础，建议在功能跑通后进行以下美化：

1.  **样式表 (QSS) 美化**：
    *   给按钮添加颜色（如：删除按钮设为红色警告色，添加按钮设为绿色）。
    *   优化表格样式（设置隔行变色 `setAlternatingRowColors(true)`，调整行高）。
2.  **表格体验优化**：
    *   隐藏不必要的 ID 列（如 `flight_id` 用户不需要看，可以用 `setColumnHidden` 隐藏）。
    *   优化列宽，让“出发地/目的地”等短字段紧凑，“时间”字段宽敞。
3.  **输入校验增强**：
    *   在 `FlightDialog` 中，限制价格不能为负数。
    *   限制起飞时间不能选择“过去的时间”。