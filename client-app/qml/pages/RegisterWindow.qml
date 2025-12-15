// 导入 QtQuick 核心模块，提供基本的 QML 类型（Rectangle、Image、Text 等）
import QtQuick 2.15
// 导入 QtQuick.Controls 模块，提供标准 UI 控件（Button、TextField 等）
import QtQuick.Controls 2.15
// 导入 QtQuick.Layouts 模块，提供布局管理（ColumnLayout 等）
import QtQuick.Layouts 1.15
import QtQml 2.15

// 控件：注册页面的根容器，使用 Rectangle 作为背景层
Rectangle {
    // 控件ID：registerWindow，用于在信号处理中引用自身
    id: registerWindow
    
    // 属性：bridge - 指向 C++ 后端的 QmlBridge 对象，用于调用注册等业务逻辑
    property var bridge
    // 信号：backToLogin - 当用户点击"返回登录"按钮时发出，通知父组件切换回登录页
    signal backToLogin()
    // 信号：registerSuccess - 注册成功后发出，通知父组件切换到登录页或主页
    signal registerSuccess()
    
    // 控件：背景图片 - 显示注册页的全屏背景图
    Image {
        // 布局：填充整个父容器（registerWindow），实现全屏背景效果
        anchors.fill: parent
        // 图片源：从 Qt 资源文件加载背景图（qml/assets/images/background.jpeg）
        source: "qrc:/qml/assets/images/background.jpeg"
        // 填充模式：PreserveAspectCrop 保持纵横比并裁剪多余部分，确保图片完全覆盖区域
        fillMode: Image.PreserveAspectCrop
        
        // 控件：半透明黑色遮罩层 - 降低背景亮度，使前景表单更清晰易读
        Rectangle {
            // 布局：覆盖整个背景图片区域
            anchors.fill: parent
            // 颜色：纯黑色 #000000
            color: "#000000"
            // 透明度：40% 不透明度，形成柔和的暗化效果
            opacity: 0.4
        }
    }
    
    // 控件：主布局容器 - 垂直排列标题、表单和按钮
    ColumnLayout {
        // 布局：居中于父容器（registerWindow）
        anchors.centerIn: parent
        // 宽度：固定 400 像素，与登录页保持一致
        width: 400
        // 间距：子元素之间垂直间距 20 像素
        spacing: 20
        
        // 控件：页面标题文本 - 显示"用户注册"
        Text {
            // 布局：在 ColumnLayout 中水平居中对齐
            Layout.alignment: Qt.AlignHCenter
            // 文本内容：显示"用户注册"四个字
            text: "用户注册"
            // 字体大小：32 像素，大标题尺寸
            font.pixelSize: 32
            // 字体粗细：加粗显示，增强视觉层次
            font.bold: true
            // 字体颜色：白色 #ffffff，与背景形成对比
            color: Qt.rgba(1, 1, 1, 0.88)
        }
        
        // 控件：表单容器 - 白色半透明圆角矩形，承载所有输入框和按钮
        Rectangle {
            // 布局：宽度填充 ColumnLayout 的 400 像素
            Layout.fillWidth: true
            // 高度：固定 380 像素，容纳 3 个输入框 + 2 个按钮 + 2 个提示文本
            Layout.preferredHeight: 380
            // 背景色：白色，提供清晰的表单区域
            color: "white"
            // 透明度：不透明度，与登录页相同的半透明效果
            opacity: 0.78
            // 圆角半径：8 像素，柔和的边角
            radius: 8
            // 边框颜色：浅灰色 #e0e0e0，边缘轮廓
            border.color: "#e0e0e0"
            // 边框宽度：1 像素
            border.width: 1
            
            // 控件：表单内部布局 - 垂直排列所有子控件
            ColumnLayout {
                // 布局：填充整个表单容器
                anchors.fill: parent
                // 外边距：所有边 30 像素，留出呼吸空间
                anchors.margins: 30
                // 间距：子控件之间垂直间距 20 像素
                spacing: 20
                
                // 控件：用户名输入框 - 用于输入要注册的用户名
                TextField {
                    // 控件ID：usernameField，用于在注册逻辑中读取输入值
                    id: usernameField
                    // 布局：宽度填充父容器（减去 margins 后约 340 像素）
                    Layout.fillWidth: true
                    // 占位符文本：显示"用户名"提示，输入内容时自动消失
                    placeholderText: "用户名"
                    // 字体大小：16 像素，与密码框保持一致
                    font.pixelSize: 16
                    validator: RegularExpressionValidator { regularExpression: /\S*/ } // 不允许空格
                    onTextEdited: {
                        const cleaned = text.replace(/\s+/g, "")
                        if (cleaned !== text) {
                            const delta = text.length - cleaned.length
                            text = cleaned
                            cursorPosition = Math.max(0, cursorPosition - delta)
                        }
                    }
                }
                
                // 控件：密码输入框 - 用于输入注册密码
                TextField {
                    // 控件ID：passwordField，用于在验证逻辑中读取密码
                    id: passwordField
                    // 布局：宽度填充父容器
                    Layout.fillWidth: true
                    // 占位符文本：显示"密码"提示
                    placeholderText: "密码"
                    // 回显模式：Password 模式，输入内容显示为圆点（隐藏明文）
                    echoMode: TextInput.Password
                    // 字体大小：16 像素
                    font.pixelSize: 16
                    validator: RegularExpressionValidator { regularExpression: /\S*/ } // 不允许空格
                    onTextEdited: {
                        const cleaned = text.replace(/\s+/g, "")
                        if (cleaned !== text) {
                            const delta = text.length - cleaned.length
                            text = cleaned
                            cursorPosition = Math.max(0, cursorPosition - delta)
                        }
                    }
                }
                
                // 控件：确认密码输入框 - 用于再次输入密码进行验证
                TextField {
                    // 控件ID：confirmPasswordField，用于在验证逻辑中与 passwordField 对比
                    id: confirmPasswordField
                    // 布局：宽度填充父容器
                    Layout.fillWidth: true
                    // 占位符文本：显示"确认密码"提示
                    placeholderText: "确认密码"
                    // 回显模式：Password 模式，隐藏输入内容
                    echoMode: TextInput.Password
                    // 字体大小：16 像素
                    font.pixelSize: 16
                    validator: RegularExpressionValidator { regularExpression: /\S*/ } // 不允许空格
                    onTextEdited: {
                        const cleaned = text.replace(/\s+/g, "")
                        if (cleaned !== text) {
                            const delta = text.length - cleaned.length
                            text = cleaned
                            cursorPosition = Math.max(0, cursorPosition - delta)
                        }
                    }
                }
                
                // 控件：注册按钮 - 触发注册逻辑
                Button {
                    // 布局：宽度填充父容器
                    Layout.fillWidth: true
                    // 高度：50 像素，增加按钮高度使其更醒目
                    Layout.preferredHeight: 50
                    // 按钮文本：显示"注册"
                    text: "注册"
                    // 字体大小：16 像素
                    font.pixelSize: 16
                    // 背景样式：蓝色圆角矩形，按下时颜色加深
                    background: Rectangle {
                        // 颜色：按下时深蓝 #1976D2，否则标准蓝 #2196F3（Material Design 蓝色）
                        color: parent.pressed ? "#1976D2" : "#2196F3"
                        // 圆角半径：4 像素
                        radius: 4
                    }
                    // 内容项：白色文本，居中显示
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        // 文字颜色：白色
                        color: "white"
                        // 水平对齐：居中
                        horizontalAlignment: Text.AlignHCenter
                        // 垂直对齐：居中
                        verticalAlignment: Text.AlignVCenter
                    }
                    // 事件处理：点击按钮时的处理逻辑
                    onClicked: {
                        // 验证：检查两次密码是否一致
                        if (passwordField.text !== confirmPasswordField.text) {
                            errorText.text = "两次输入的密码不一致"
                            errorText.visible = true
                            return
                        }
                        // 验证：检查用户名和密码是否为空
                        if (usernameField.text.length === 0 || passwordField.text.length === 0) {
                            errorText.text = "用户名和密码不能为空"
                            errorText.visible = true
                            return
                        }
                        // 调用后端接口：通过 bridge 调用 registerUser 方法发送注册请求
                        bridge.registerUser(usernameField.text, passwordField.text)
                    }
                }
                
                // 控件：负间距调整器 - 减少注册按钮和返回按钮之间的间距
                Item {
                    // 布局：宽度填充（无实际内容）
                    Layout.fillWidth: true
                    // 高度：负值会压缩上下元素之间的间距，使返回按钮位于注册按钮下方空白区域正中
                    Layout.preferredHeight: 0
                }
                
                // 控件：返回登录按钮 - 切换回登录页面
                Button {
                    // 布局：宽度填充父容器
                    Layout.fillWidth: true
                    // 高度：40 像素，略低于注册按钮（次要操作）
                    Layout.preferredHeight: 40
                    // 布局：顶部外边距，将按钮推到注册按钮下方居中位置
                    Layout.topMargin: -28
                    // 按钮文本：显示"返回登录"
                    text: "返回登录"
                    // 字体大小：18 像素
                    font.pixelSize: 19
                    // 字体粗细：加粗显示，提高识别度
                    font.bold: true
                    // 背景样式：透明背景，无边框（文本按钮风格）
                    background: Rectangle {
                        color: "transparent"
                    }
                    // 内容项：蓝色文本，居中显示
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        // 文字颜色：蓝色 #2196F3（与注册按钮背景色相呼应）
                        color: "#2196F3"
                        // 水平对齐：居中
                        horizontalAlignment: Text.AlignHCenter
                        // 垂直对齐：居中
                        verticalAlignment: Text.AlignVCenter
                    }
                    // 事件处理：点击按钮时发出 backToLogin 信号
                    onClicked: {
                        // 发射信号：通知父组件（main.qml）切换回登录页
                        registerWindow.backToLogin()
                    }
                }
                
                // 控件：错误提示文本 - 显示注册失败或验证错误信息
                Text {
                    // 控件ID：errorText，用于在验证逻辑和 Connections 中设置错误消息
                    id: errorText
                    // 布局：宽度填充父容器
                    Layout.fillWidth: true
                    // 布局：在 ColumnLayout 中水平居中对齐
                    Layout.alignment: Qt.AlignHCenter
                    // 布局：顶部外边距，使错误提示更靠下显示
                    Layout.topMargin: 15
                    // 可见性：默认隐藏，出错时设为 true
                    visible: false
                    // 字体颜色：红色 #f44336（Material Design 红色）
                    color: "#f44336"
                    // 字体大小：16 像素
                    font.pixelSize: 16
                    // 字体粗细：加粗显示，突出错误提示
                    font.bold: true
                    // 水平对齐：文本居中
                    horizontalAlignment: Text.AlignHCenter
                    // 换行模式：自动换行，适应长错误消息
                    wrapMode: Text.Wrap
                    
                    // 初始透明度：0（完全透明），用于淡入动画
                    opacity: 0
                    
                    // 动画行为：opacity 属性变化时触发淡入淡出动画
                    Behavior on opacity {
                        NumberAnimation {
                            // 动画时长：300 毫秒
                            duration: 300
                            // 缓动类型：InOutQuad（先加速后减速，柔和过渡）
                            easing.type: Easing.InOutQuad
                        }
                    }
                    
                    // 事件处理：visible 属性变化时的逻辑
                    onVisibleChanged: {
                        if (visible) {
                            // 显示时：设置 opacity 为 1（触发淡入动画）
                            opacity = 1
                            // 启动自动隐藏定时器（3 秒后自动隐藏）
                            errorHideTimer.restart()
                        } else {
                            // 隐藏时：设置 opacity 为 0（触发淡出动画）
                            opacity = 0
                        }
                    }
                    
                    // 控件：自动隐藏定时器 - 3 秒后自动隐藏错误提示
                    Timer {
                        // 控件ID：errorHideTimer
                        id: errorHideTimer
                        // 定时间隔：3000 毫秒（3 秒）
                        interval: 3000
                        // 事件处理：定时器触发时的逻辑
                        onTriggered: {
                            // 先淡出：设置 opacity 为 0
                            errorText.opacity = 0
                            // 延迟执行：等待淡出动画完成后再隐藏
                            Qt.callLater(function() {
                                // 检查：如果 opacity 仍为 0，才设置 visible 为 false
                                if (errorText.opacity === 0) {
                                    errorText.visible = false
                                }
                            })
                        }
                    }
                }
                
                // 控件：成功提示文本 - 显示注册成功消息
                Text {
                    // 控件ID：successText，用于在 Connections 中设置成功消息
                    id: successText
                    // 布局：宽度填充父容器
                    Layout.fillWidth: true
                    // 布局：在 ColumnLayout 中水平居中对齐
                    Layout.alignment: Qt.AlignHCenter
                    // 可见性：默认隐藏，注册成功时设为 true
                    visible: false
                    // 字体颜色：绿色 #4CAF50（Material Design 绿色）
                    color: "#4CAF50"
                    // 字体大小：16 像素
                    font.pixelSize: 16
                    // 字体粗细：加粗显示，突出成功提示
                    font.bold: true
                    // 水平对齐：文本居中
                    horizontalAlignment: Text.AlignHCenter
                    // 换行模式：自动换行，适应长消息
                    wrapMode: Text.Wrap
                    
                    // 初始透明度：0（完全透明），用于淡入动画
                    opacity: 0
                    
                    // 动画行为：opacity 属性变化时触发淡入淡出动画
                    Behavior on opacity {
                        NumberAnimation {
                            // 动画时长：300 毫秒
                            duration: 300
                            // 缓动类型：InOutQuad（柔和过渡）
                            easing.type: Easing.InOutQuad
                        }
                    }
                    
                    // 事件处理：visible 属性变化时的逻辑
                    onVisibleChanged: {
                        if (visible) {
                            // 显示时：设置 opacity 为 1（触发淡入动画）
                            opacity = 1
                        } else {
                            // 隐藏时：设置 opacity 为 0（触发淡出动画）
                            opacity = 0
                        }
                    }
                }
            }
        }
    }
    
    // 控件：信号连接块 - 监听 bridge 发出的注册相关信号
    Connections {
        // 目标对象：bridge（C++ 后端的 QmlBridge 对象）
        target: bridge
        
        // 信号处理：注册成功 - 显示成功消息，1.5 秒后跳转登录页
        function onRegisterSuccess(message) {
            // 设置成功提示文本内容（服务器返回的消息）
            successText.text = message
            // 显示成功提示
            successText.visible = true
            // 隐藏错误提示（避免同时显示多个提示）
            errorText.visible = false
            // 延迟执行：1500 毫秒（1.5 秒）后发射 registerSuccess 信号
            Qt.callLater(function() {
                // 通知父组件：注册成功，可以切换到登录页或主页
                registerWindow.registerSuccess()
            }, 1500)
        }
        
        // 信号处理：注册失败 - 显示失败消息
        function onRegisterFailed(message) {
            // 设置错误提示文本内容（服务器返回的失败原因）
            errorText.text = message
            // 显示错误提示
            errorText.visible = true
            // 隐藏成功提示
            successText.visible = false
        }
        
        // 信号处理：一般错误 - 显示错误消息（网络错误、解析错误等）
        function onErrorOccurred(message) {
            // 设置错误提示文本内容
            errorText.text = message
            // 显示错误提示
            errorText.visible = true
            // 隐藏成功提示
            successText.visible = false
        }
    }
}


