多客户端升级说明（客户端需要的改动）
服务器已从“单客户端模式”升级为“多客户端并发支持”，并引入 tag 机制 为每个客户端分配唯一身份标识。

为了保证客户端能够继续与服务器正常通信，请按照以下规范修改客户端代码。

1. 客户端连接后必须首先发送“注册 tag”消息
连接成功后，客户端必须发送 第一条 JSON：

{
    "tag": "your_unique_tag"
}
tag 用于标识客户端的唯一身份，可以使用：

用户 ID（推荐）
设备 ID
随机 UUID
其他唯一字符串
若未发送 tag，服务器将立即断开连接。

2. tag 必须唯一（客户端保证）
服务器会拒绝使用重复 tag 的连接。

推荐使用用户 ID，例如：

{ "tag": "user_15" }
若客户端没有固定用户，可自动生成：

QString tag = QUuid::createUuid().toString(QUuid::WithoutBraces);
3. 收到服务器返回 “Tag registered” 后才能继续发送业务请求
服务器成功注册 tag 后，会返回：

{
    "status": "success",
    "message": "Tag registered"
}
客户端必须 等待收到此消息后 才能发送后续的业务 action（如 login、register、search_flights 等）。

4. 所有业务 action 请求保持格式不变，但必须从第二条消息开始发送
以前可能会在连接后立即发送：

{
    "action": "login",
    "data": { ... }
}
现在必须按以下顺序：

第 1 条消息：注册 tag
{ "tag": "user_15" }
第 2 条及之后：正常业务请求
{
    "action": "login",
    "data": { ... }
}
5. 原有 action / data 协议完全不变
业务请求保持原样，例如：

{
    "action": "search_flights",
    "data": {
        "origin": "北京",
        "destination": "上海"
    }
}
客户端只需确保 所有业务 action 均在 tag 注册成功后发送。