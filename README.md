## 架构分析文档(v1.0)
### 关于数据库的选择
一开始在SQLlite和mySQL之间犹豫。但是由于我们并没有高并发需求，SQLlite也可以实现1秒钟上千次的读写，根据“如无必要，勿增实体”，选择了SQLlite。
并且由于SQLlite内置于Qt，大大方便了四个合作者的数据库配置。
### 关于多客户端分离
采用了规范且简明的方式，分为以下三个。
```
server-app 
	命令行界面，用来处理各种和数据库相关的信息。
admin-app
	图形化界面，给管理者设置航班，查看信息。
client-app
	图形化界面，提供给用户来查询航班信息，提供订阅航班服务。
```
## 日常开发流程

## 功能需求文档(v1.0)
1. **用户端 (Client) 功能：**    
    - **用户模块：** 注册、登录、退出。
    - **航班查询模块：** 按日期、出发地、目的地搜索航班。
	    基本功能：同时拥有日期、出发地、目的地三个信息进行搜索。
	    附加功能：只拥有其中的1/2个信息进行搜索。
		搜索结果需要按时间进行排序。
    - **航班预订模块：** 选择航班、下单、支付（模拟支付即可）。
    - **订单管理模块：** 查看“我的订单”（已预订、已取消）、取消订单。
    
2. **服务器端 (Server / Admin) 功能：**
    
    - **管理员登录：** （与用户登录分开）。
    - **航班管理：** 航班信息（航班号、机型、座位数）的增、删、改、查。
    - **票务管理：** 设置某航班的票价、起飞/降落时间、剩余票数。
    - **用户管理：** 查看所有用户信息。
    - **订单查看：** 查看系统中的所有订单。

## 项目接口文档(v1.0)
- **服务器part:** 你的任务是**实现** `tcpserver.cpp`，使其能正确**响应**本文档定义的JSON。
    
- **客户端part:** 任务是**使用** `NetworkManager`，**发送**本文档定义的JSON请求，并**处理**响应。
### 1. 基础通信规范 (JSON 信封)

所有客户端 (C2S) 和服务器 (S2C) 之间的通信**必须**使用标准JSON格式。

#### C2S (客户端 -> 服务器) 请求格式

```
{
  "action": "string",
  "data": { ... }
}
```

- `action`: 一个字符串，告诉服务器你想"干什么" (例如: `"login"`)。
    
- `data`: 一个JSON对象，包含执行此`action`所需的所有数据。
    
#### S2C (服务器 -> 客户端) 响应格式

```
{
  "status": "success" | "error",
  "message": "string",
  "data": { ... } | [ ... ] | null
}
```

- `status`: `"success"` (成功) 或 `"error"` (失败)。
    
- `message`: 给客户端的提示信息 (例如: `"登录成功"` 或 `"用户名已存在"`)。
    
- `data`: `status`为`success`时返回的数据。可以是单个对象 `{}`, 数组 `[]`, 或 `null`。
    

### 2. 【重要】NetworkManager 使用指南 (给 Admin和Client)

你们**永远不需要**手动创建JSON或`QTcpSocket`。你们只需要使用 `NetworkManager` 这个单例。

**使用流程分为 3 步：**

1. **(逻辑层)** 在 `NetworkManager.h/.cpp` 中定义“发射器”和“信号”。
    
2. **(界面层)** 在窗口的`.cpp`**构造函数**中，`connect` 信号到槽。
    
3. **(界面层)** 在按钮点击时**调用** `NetworkManager` 函数，并在**槽函数**中处理UI更新。
    

#### 所有的功能实现只要用到信息与槽（或者说只要用到了数据库），都要遵循这个模式。

### 3. API 接口详情（这个可以在具体实现时再看，只要确保server端和admin, client端能够保持一致即可）

> 注意 下面的api接口规范全部都可以更改，具体实现时注意server和其他两端的协同即可。

#### 3.1 基础用户接口 (供 `client-app` 和 `admin-app` 使用)

##### `handleRegister` (用户注册)

- `action`: `"register"`
    
- **C2S `data`:**
    
    ```
    {
      "username": "new_user",
      "password": "password123"
    }
    ```
    
- **S2C `data` (成功):** `null` (或用户信息)
    
    ```
    { "status": "success", "message": "注册成功", "data": null }
    ```
    
- **S2C (失败):**
    
    ```
    { "status": "error", "message": "用户名 'new_user' 已存在", "data": null }
    ```
    

##### `handleLogin` (登录)

- `action`: `"login"`
    
- **C2S `data`:**
    
    ```
    {
      "username": "user123",
      "password": "password123"
    }
    ```
    
- **S2C `data` (成功):** 返回用户信息，用于客户端判断是否为管理员。
    
    ```
    {
      "status": "success",
      "message": "登录成功",
      "data": {
        "user_id": 15,
        "username": "user123",
        "is_admin": 0 
      }
    }
    ```
    
- **S2C (失败):**
    
    ```
    { "status": "error", "message": "用户名或密码错误", "data": null }
    ```
    

#### 3.2 航班票务接口 (供 `client-app` 使用)

##### `handleSearchFlights` (航班查询)

- `action`: `"search_flights"`
    
- **C2S `data`:** (允许字段缺失，服务器应动态构建SQL)
    
    ```
    {
      "origin": "北京",
      "destination": "上海",
      "date": "2025-12-01" 
    }
    ```
    
- **S2C `data` (成功):** 返回航班信息数组 (按时间排序)。
    
    ```
    {
      "status": "success",
      "message": "查询成功",
      "data": [
        {
          "flight_id": 101,
          "flight_number": "CA101",
          "origin": "北京",
          "destination": "上海",
          "departure_time": "2025-12-01T08:00:00",
          "arrival_time": "2025-12-01T10:15:00",
          "price": 850.0,
          "remaining_seats": 25
        }
      ]
    }
    ```
    

##### `handleBookFlight` (预订航班)

- `action`: `"book_flight"`
    
- **C2S `data`:** ( `user_id` 应由客户端登录后保存)
    
    ```
    {
      "user_id": 15,
      "flight_id": 101
    }
    ```
    
- **S2C `data` (成功):** 返回新创建的订单信息。
    
    ```
    {
      "status": "success",
      "message": "预订成功",
      "data": {
        "booking_id": 501,
        "user_id": 15,
        "flight_id": 101,
        "status": "confirmed"
      }
    }
    ```
    
- **S2C (失败):**
    
    ```
    { "status": "error", "message": "票已售罄", "data": null }
    ```
    

##### `handleGetMyOrders` (获取我的订单)

- `action`: `"get_my_orders"`
    
- **C2S `data`:**
    
    ```
    {
      "user_id": 15
    }
    ```
    
- **S2C `data` (成功):** 返回该用户的所有订单（包含航班信息）。
    
    ```
    {
      "status": "success",
      "message": "查询成功",
      "data": [
        {
          "booking_id": 501,
          "flight_id": 101,
          "status": "confirmed",
          "flight_number": "CA101", 
          "origin": "北京",
          "destination": "上海",
          "departure_time": "2025-12-01T08:00:00"
        }
      ]
    }
    ```
    

##### `handleCancelOrder` (取消订单)

- `action`: `"cancel_order"`
    
- **C2S `data`:**
    
    ```
    {
      "booking_id": 501
    }
    ```
    
- **S2C `data` (成功):** `null`
    
    ```
    { "status": "success", "message": "订单已成功取消", "data": null }
    ```
    

#### 3.3 管理员接口 (供 `admin-app` 使用)

##### `handleAdminAddFlight` (增航班)

- `action`: `"admin_add_flight"`
    
- **C2S `data`:** (包含所有 `Flight` 表字段)
    
    ```
    {
      "flight_number": "HO110",
      "model": "A320",
      "origin": "南京",
      "destination": "成都",
      "departure_time": "2025-12-05T10:00:00",
      "arrival_time": "2025-12-05T12:30:00",
      "total_seats": 150,
      "price": 900.0
    }
    ```
    
- **S2C (成功):**
    
    ```
    { "status": "success", "message": "航班添加成功", "data": null }
    ```
    

##### `handleAdminUpdateFlight` (改航班)

- `action`: `"admin_update_flight"`
    
- **C2S `data`:** ( `flight_id` 必填，其他字段选填)
    
    ```
    {
      "flight_id": 101,
      "price": 950.0,
      "remaining_seats": 30
    }
    ```
    
- **S2C (成功):**
    
    ```
    { "status": "success", "message": "航班更新成功", "data": null }
    ```
    

##### `handleAdminDeleteFlight` (删航班)

- `action`: `"admin_delete_flight"`
    
- **C2S `data`:**
    
    ```
    {
      "flight_id": 101
    }
    ```
    
- **S2C (成功):**
    
    ```
    { "status": "success", "message": "航班删除成功", "data": null }
    ```
    

##### `handleAdminGetAllUsers` (查所有用户)

- `action`: `"admin_get_all_users"`
    
- **C2S `data`:** `{}`
    
- **S2C `data` (成功):**
    
    ```
    {
      "status": "success",
      "message": "查询成功",
      "data": [
        { "user_id": 15, "username": "user123", "created_at": "..." },
        { "user_id": 16, "username": "other_user", "created_at": "..." }
      ]
    }
    ```
    

##### `handleAdminGetAllBookings` (查所有订单)

- `action`: `"admin_get_all_bookings"`
    
- **C2S `data`:** `{}`
    
- **S2C `data` (成功):**
    
    ```
    {
      "status": "success",
      "message": "查询成功",
      "data": [
        { "booking_id": 501, "user_id": 15, "flight_id": 101, "status": "confirmed" },
        { "booking_id": 502, "user_id": 16, "flight_id": 102, "status": "cancelled" }
      ]
    }
    ```

## 数据库表格文档(v1.0)
### 核心表格
#### 表一：`User` (用户表)
这张表同时存储**普通用户**和**管理员**。我们用一个 `is_admin` 字段来区分他们（0=普通用户, 1=管理员）。
```SQL
CREATE TABLE IF NOT EXISTS User (
    user_id         INTEGER PRIMARY KEY AUTOINCREMENT,
    username        TEXT NOT NULL UNIQUE,
    password        TEXT NOT NULL,
    is_admin        INTEGER NOT NULL DEFAULT 0,  -- 0 = 普通用户, 1 = 管理员
    created_at      DATETIME DEFAULT CURRENT_TIMESTAMP
);
```
- `user_id`: 用户唯一ID（自动增长）。
- `username`: 用户名（`UNIQUE` 保证不能重名）。
- `password`: 密码（大作业阶段存明文也行，进阶一点可以存MD5）。
- `is_admin`: **核心字段**。解决你“管理员登录与用户登录分开”的需求。服务器在处理登录`action`时，查询这个字段就知道他是谁。
    
#### 表二：`Flight` (航班表)
这张表是**核心中的核心**。它同时满足了你的“航班管理”和“票务管理”两大需求。我们把一个具体的、可售卖的航班作为一行。
```SQL
CREATE TABLE IF NOT EXISTS Flight (
    flight_id         INTEGER PRIMARY KEY AUTOINCREMENT,
    flight_number     TEXT NOT NULL,          -- 航班号 (如 "CA101")
    model             TEXT,                   -- 机型 (如 "Boeing 737")
    origin            TEXT NOT NULL,          -- 出发地
    destination       TEXT NOT NULL,          -- 目的地
    departure_time    DATETIME NOT NULL,      -- 起飞时间 (格式: 'YYYY-MM-DD HH:MM:SS')
    arrival_time      DATETIME NOT NULL,      -- 降落时间
    total_seats       INTEGER NOT NULL,         -- 总座位数
    remaining_seats   INTEGER NOT NULL,         -- 剩余座位数
    price             REAL NOT NULL           -- 价格
);
```
- `departure_time`: **核心字段**。客户端的“按日期搜索”和“按时间排序”都依赖它。
- `remaining_seats`: **核心字段**。
    - **管理员**设置票务时，应让 `remaining_seats` 初始值等于 `total_seats`。
    - **用户**预订航班时，服务器逻辑需要 `UPDATE Flight SET remaining_seats = remaining_seats - 1 WHERE flight_id = ?`。
    - **用户**取消订单时，服务器逻辑需要 `UPDATE Flight SET remaining_seats = remaining_seats + 1 WHERE flight_id = ?`。
#### 表三：`Booking` (订单表)
这张表用于连接“哪个用户”预订了“哪个航班”。
```SQL
CREATE TABLE IF NOT EXISTS Booking (
    booking_id      INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id         INTEGER NOT NULL,
    flight_id       INTEGER NOT NULL,
    booking_time    DATETIME DEFAULT CURRENT_TIMESTAMP, -- 预订时间
    status          TEXT NOT NULL, -- "confirmed" (已预订), "cancelled" (已取消)

    FOREIGN KEY (user_id) REFERENCES User (user_id),
    FOREIGN KEY (flight_id) REFERENCES Flight (flight_id)
);
```
- `user_id`: 关联到 `User` 表，才知道这是谁的订单。
- `flight_id`: 关联到 `Flight` 表，才知道订的是哪一班。
- `status`: **核心字段**。满足“查看已预订、已取消”和“取消订单”的需求。
---
### 默认管理员帐户
> 可以用这个管理员帐户在各个数据表中畅游。
```
用户名：
jaisonZheng
密码：
admin123
```

## 项目代码规范(v1.1)
### 在每个程序文件顶部写上实现的功能及调用方式
比如：
```c++
/*
该程序负责建立和管理数据库
注意，所有需要使用数据库的部分，都需要通过调用该文件当中的DatabaseManager类来实现。
也就是说，在server-app的任何地方，如果你需要调用数据库，请使用DatabaseManager::instance().database()
在main.cpp中初始化该数据库
*/
```

### 命名法
类采用大驼峰命名法，
函数、变量采用小驼峰命名法，
文件采用蛇形命名法，
项目采用串式命名法。
比如：
```c++
// 类名 -> 大驼峰
class FlightService {
    // 变量/函数 -> 小驼峰  
    std::string userName;
    void bookFlight();
};

// 文件名: flight_service.h (蛇形)
// 项目名: flight-system (串式)
```


## TODO List(v1.1)
项目大体架构已搭建完毕，下面是几份分开的任务。
**重要规则:**
1. **API 优先:** 所有的开发都必须**严格遵守**《API接口文档》。
2. **前后端分离:** 任何界面 (GUI) **禁止**直接操作数据库。所有数据必须通过 `NetworkManager` (客户端) 和 `TcpServer` (服务器) 进行 JSON 通信。
3. **Git 规范:**
	没有规范，主要就是push之前最好build一下测试一下，或者把代码扔给AI review一下。
	如果出了问题可以回退，不要紧张。

### 1、架构
- [x] 搭建 `server-app` (控制台) 和 `client-app` (GUI) 项目结构。
- [x] 搭建 `admin-app` (GUI) 项目结构。
- [x] 完成 `DatabaseManager`，实现数据库自动创建。
- [x] 提供 `TcpServer` 和 `NetworkManager` 的骨架代码。
- [ ] 维护和更新《API接口文档》和《数据库设计文档》。

### 2. `server-app` 服务器部分

**职责：** 在 `tcpserver.cpp` 中实现所有 `handleRequest` 的业务逻辑。这是系统的“大脑”。
- [x] `handleLogin` (登录)：已在骨架中实现，待测试。
- [x] `handleSearchFlights` (航班查询)：已在骨架中实现，待测试。
- [ ] `handleRegister` (用户注册)：
    - 从 JSON 中获取 `username`, `password`。
    - 执行 `INSERT INTO User ...`。
    - 注意处理 `username` 已存在的错误 (UNIQUE 约束)。
        
- [ ] `handleBookFlight` (预订航班)：
    - 从 JSON 中获取 `user_id`, `flight_id`。
    - **(重要!)** 使用数据库**事务 (Transaction)**。
	    其实不用也行（毕竟我们也没有高并发），但是一定要有这个意识，就是比如说一个用户订了票，千万不能只改booking表中的东西，还要改flight和user表中的东西。
    - 检查 `Flight` 表中 `remaining_seats > 0`。
    - `UPDATE Flight SET remaining_seats = remaining_seats - 1 WHERE flight_id = ?`。
    - `INSERT INTO Booking (user_id, flight_id, status) VALUES (?, ?, 'confirmed')`。
        
    - 返回成功或失败的 JSON。
- [ ] `handleGetMyOrders` (获取我的订单)：
    
    - 从 JSON 中获取 `user_id`。
    - `SELECT * FROM Booking JOIN Flight ON Booking.flight_id = Flight.flight_id WHERE Booking.user_id = ?`。
    - 将查询结果打包成一个 JSON 数组并返回。
        
- [ ] `handleCancelOrder` (取消订单)：
    - 从 JSON 中获取 `booking_id`。
    - **(重要!)** 使用数据库**事务**。（就是说不要直接操控数据库）
    - `UPDATE Booking SET status = 'cancelled' WHERE booking_id = ?`。
        
    - `UPDATE Flight SET remaining_seats = remaining_seats + 1 WHERE flight_id = (SELECT flight_id FROM Booking WHERE booking_id = ?)`。
        
    - 返回成功 JSON。
        
- [ ] **(管理员接口)** `handleAdminAddFlight` (增航班)：实现 `INSERT INTO Flight ...`。
    
- [ ] **(管理员接口)** `handleAdminUpdateFlight` (改航班)：实现 `UPDATE Flight SET ... WHERE flight_id = ?`。
    
- [ ] **(管理员接口)** `handleAdminDeleteFlight` (删航班)：实现 `DELETE FROM Flight WHERE flight_id = ?`。
    
- [ ] **(管理员接口)** `handleAdminGetAllUsers` (查用户)：实现 `SELECT user_id, username, created_at FROM User WHERE is_admin = 0`。
    
- [ ] **(管理员接口)** `handleAdminGetAllBookings` (查订单)：实现 `SELECT * FROM Booking`。
    

### 3. `client-app`用户端部分

**职责：** 开发用户界面，并通过 `NetworkManager` 与服务器通信。
- [ ] **`NetworkManager` 
    - [ ] 添加 `sendRegisterRequest(...)` 函数。
    - [ ] 添加 `bookFlightRequest(...)` 函数。
        
    - [ ] 添加 `getMyOrdersRequest()` 函数。
        
    - [ ] 添加 `cancelOrderRequest(...)` 函数。
        
    - [ ] 添加对应的 `signals` (如 `registerSuccess`, `registerFailed`, `bookingSuccess` 等)。
        
    - [ ] 在 `onReadyRead()` 中添加对新响应的路由逻辑 (emit 对应的信号)。
- [ ] **GUI：注册
    - [ ] 创建 `RegisterWindow.ui` (注册窗口)。
    - [ ] 添加 "用户名", "密码", "确认密码" 输入框。
    - [ ] (队友D) "注册" 按钮点击时，调用 `NetworkManager::instance().sendRegisterRequest(...)`。
    - [ ] (队友D) `connect` `registerSuccess` 信号，提示成功并返回登录页。
- [ ] **GUI：航班查询 **
    - [ ] 创建 `SearchWindow.ui` (查询主窗口)。
    - [ ] 添加 `QDateEdit` (日期), `QLineEdit` (出发地), `QLineEdit` (目的地)。
    - [ ] 添加一个 `QTableWidget` (表格) 来显示航班结果。
    - [ ] "查询" 按钮点击时，调用 `NetworkManager::instance().sendSearchRequest(...)`。
    - [ ] `connect` `searchResults(QJsonArray flights)` 信号，将 `flights` 数组解析并填充到 `QTableWidget` 中。
    - [ ] 在表格旁添加 "预订" 按钮，点击时获取选中行的 `flight_id`，调用 `NetworkManager::instance().bookFlightRequest(...)`。
        
- [ ] **GUI：我的订单
    - [ ] 创建 `MyOrdersWindow.ui` (我的订单窗口)。
    - [ ] 添加一个 `QTableWidget` (表格) 来显示订单。
    - [ ] 在窗口加载时 (e.g., `showEvent()`)，调用 `NetworkManager::instance().getMyOrdersRequest()`。
    - [ ]  `connect` 信号 (如 `myOrdersResult(QJsonArray orders)`)，将订单填充到表格中。
    - [ ]  在表格中添加 "取消订单" 按钮，点击时获取 `booking_id`，调用 `NetworkManager::instance().cancelOrderRequest(...)`。
        

### 4. `admin-app` 管理员部分

**职责：** 开发管理员专属的GUI界面，同样通过 `NetworkManager` 与服务器通信。

- [ ] **`NetworkManager` (管理员版)**
    - [ ] **复制** `client-app` 的 `networkmanager.h/.cpp` 到 `admin-app`。
    - [ ] 添加所有 `admin_...` 相关的发送函数 (e.g., `sendAdminAddFlight(...)`)。
    - [ ] 添加所有 `admin_...` 相关的接收信号 (e.g., `adminFlightsResult(QJsonArray)`)。
    - [ ] 在 `onReadyRead()` 中添加对管理员响应的路由。
        
- [ ] **GUI：登录**
    
    - [ ] 创建 `AdminLoginWindow.ui`。
        
    - [ ] "登录" 按钮调用 `NetworkManager::instance().sendLoginRequest(...)`。
        
    - [ ] `connect` `loginSuccess` 信号，**检查**返回的 JSON 中 `is_admin == 1`，如果为 1，则打开管理员主面板，否则提示“非管理员账户”。
        
- [ ] **GUI：管理员主面板 (AdminDashboard.ui)**
    - [ ] 推荐使用 `QTabWidget` (选项卡)。
    - [ ] **Tab 1: 航班管理**
        - [ ] 添加 `QTableWidget` 显示所有航班。
        - [ ] 添加 "添加", "修改", "删除" 按钮和对应的表单 (QLineEdit)。
        - [ ] 将按钮功能连接到 `NetworkManager` 的 `sendAdminAddFlight` 等函数。
            
    - [ ] **Tab 2: 用户管理**
        - [ ] 添加 `QTableWidget` 显示所有用户。
        - [ ] (在 Tab 显示时) 调用 `NetworkManager` 获取所有用户数据。
            
    - [ ] **Tab 3: 订单管理**
        
        - [ ] 添加 `QTableWidget` 显示所有订单。
        - [ ] (在 Tab 显示时) 调用 `NetworkManager` 获取所有订单数据。
