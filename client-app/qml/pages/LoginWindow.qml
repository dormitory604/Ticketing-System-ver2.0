import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml 2.15

// 登录窗口：应用的主登录界面，提供用户名密码登录功能
Rectangle {
    id: loginWindow
    
    property var bridge  // 桥接对象：用于与 C++ 后端通信
    signal requestRegister()  // 信号：用户点击注册按钮时触发，通知主窗口切换到注册页面
    signal loginSuccess()  // 信号：登录成功后触发，通知主窗口切换到搜索页面
    
    // 背景图片：全屏背景图（background.jpeg）
    Image {
        anchors.fill: parent  // 控件：填充整个父容器
        source: "qrc:/qml/assets/images/background.jpeg"  // 控件：加载背景图片资源
        fillMode: Image.PreserveAspectCrop  // 控件：保持图片比例并裁剪以填满区域
        
        // 半透明黑色遮罩：增强文字可读性
        Rectangle {
            anchors.fill: parent
            color: "#000000"  // 控件：纯黑色
            opacity: 0.4  // 控件：40%不透明度，使背景图略微变暗
        }
    }
    
    // 主布局容器：垂直排列标题、副标题和登录表单
    ColumnLayout {
        anchors.centerIn: parent  // 控件：整体居中显示
        anchors.verticalCenterOffset: -10  // 控件：垂直偏移量，向上移动10像素
        width: 400  // 控件：固定宽度400px
        spacing: 20  // 控件：子元素之间的垂直间距
        
        // 主标题："SkyTravel"
        Text {
            Layout.alignment: Qt.AlignHCenter  // 控件：水平居中对齐
            text: "SkyTravel"  // 控件：显示应用名称
            font.pixelSize: 36  // 控件：大号字体
            font.bold: true  // 控件：加粗
            color: "#2196F3"  // 控件：蓝色文字
        }
        
        // 副标题："机票预订系统"
        Text {
            Layout.alignment: Qt.AlignHCenter  // 控件：水平居中对齐
            text: "机票预订系统"  // 控件：显示系统说明
            font.pixelSize: 18  // 控件：中号字体
            color: Qt.rgba(1, 1, 1, 0.88)  // 控件：白色文字，不透明度
            font.bold: true  // 控件：加粗
        }
        
        // 登录表单容器：包含标题、输入框和按钮的半透明白色卡片
        Rectangle {
            Layout.fillWidth: true  // 控件：填充父容器宽度
            Layout.preferredHeight: 300  // 控件：固定高度300px
            color: "white"  // 控件：白色背景
            opacity: 0.78  // 控件：不透明度，形成半透明效果
            radius: 8  // 控件：圆角半径8px
            border.color: "#e0e0e0"  // 控件：浅灰色边框
            border.width: 1  // 控件：1px边框宽度
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 30  // 控件：内边距30px
                spacing: 20  // 控件：子元素之间的垂直间距
                
                // 表单标题："用户登录"
                Text {
                    Layout.fillWidth: true
                    text: "用户登录"  // 控件：显示表单标题
                    font.pixelSize: 24  // 控件：大号字体
                    font.bold: true  // 控件：加粗
                }
                
                // 用户名输入框
                TextField {
                    id: usernameField  // 控件ID：用于获取输入的用户名
                    Layout.fillWidth: true  // 控件：填充父容器宽度
                    placeholderText: "用户名"  // 控件：占位符提示文本
                    font.pixelSize: 16  // 控件：字体大小
                    validator: RegularExpressionValidator { regularExpression: /\S*/ } // 不允许输入空格
                    onTextEdited: {
                        const cleaned = text.replace(/\s+/g, "")
                        if (cleaned !== text) {
                            const delta = text.length - cleaned.length
                            text = cleaned
                            cursorPosition = Math.max(0, cursorPosition - delta)
                        }
                    }
                }
                
                // 密码输入框
                TextField {
                    id: passwordField  // 控件ID：用于获取输入的密码
                    Layout.fillWidth: true  // 控件：填充父容器宽度
                    placeholderText: "密码"  // 控件：占位符提示文本
                    echoMode: TextInput.Password  // 控件：密码模式，输入内容显示为圆点
                    font.pixelSize: 16  // 控件：字体大小
                    validator: RegularExpressionValidator { regularExpression: /\S*/ } // 不允许输入空格
                    onTextEdited: {
                        const cleaned = text.replace(/\s+/g, "")
                        if (cleaned !== text) {
                            const delta = text.length - cleaned.length
                            text = cleaned
                            cursorPosition = Math.max(0, cursorPosition - delta)
                        }
                    }
                }
                
                // 登录按钮：提交用户名和密码到服务器验证
                Button {
                    Layout.fillWidth: true  // 控件：填充父容器宽度
                    Layout.preferredHeight: 45  // 控件：高度45px
                    text: "登录"  // 控件：按钮文字
                    font.pixelSize: 17  // 控件：字体大小
                    font.bold: true  // 控件：加粗
                    background: Rectangle {
                        color: parent.pressed ? "#1976D2" : "#2196F3"  // 控件：按下时深蓝色，否则浅蓝色
                        radius: 4  // 控件：圆角半径4px
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "white"  // 控件：白色文字
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        bridge.login(usernameField.text, passwordField.text)  // 控件：调用 C++ 桥接对象的登录方法
                    }
                }
                
                // 间距占位符：在登录按钮和注册按钮之间增加10px间距
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 10  // 控件：10px高度的空白间距
                }
                
                // 注册按钮：点击跳转到注册页面
                Button {
                    Layout.fillWidth: true  // 控件：填充父容器宽度
                    Layout.preferredHeight: 40  // 控件：高度40px
                    text: "注册新用户"  // 控件：按钮文字
                    font.pixelSize: 20  // 控件：字体大小
                    font.bold: true  // 控件：加粗
                    background: Rectangle {
                        color: "transparent"  // 控件：透明背景（文字按钮）
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#2196F3"  // 控件：蓝色文字
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        loginWindow.requestRegister()  // 控件：触发信号，通知主窗口切换到注册页面
                    }
                }
                
                // 错误提示文本：显示登录失败或网络错误信息，带渐入渐出动画
                Text {
                    id: errorText  // 控件ID：用于动态设置错误消息
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter  // 控件：水平居中
                    visible: false  // 控件：默认隐藏，仅在出错时显示
                    color: "#f44336"  // 控件：红色文字（Material Design 错误色）
                    font.pixelSize: 16  // 控件：字体大小
                    font.bold: true  // 控件：加粗
                    horizontalAlignment: Text.AlignHCenter  // 控件：文本居中对齐
                    wrapMode: Text.Wrap  // 控件：文本自动换行
                    
                    opacity: 0  // 控件：初始透明度为0（完全透明）
                    
                    // 透明度动画：实现渐入渐出效果
                    Behavior on opacity {
                        NumberAnimation {
                            duration: 300  // 控件：动画持续时间300ms
                            easing.type: Easing.InOutQuad  // 控件：缓动曲线（先加速后减速）
                        }
                    }
                    
                    // 控件：当 visible 属性改变时触发
                    onVisibleChanged: {
                        if (visible) {
                            opacity = 1  // 控件：显示时渐入（透明度变为1）
                            hideTimer.restart()  // 控件：启动自动隐藏定时器
                        } else {
                            opacity = 0  // 控件：隐藏时渐出（透明度变为0）
                        }
                    }
                    
                    // 自动隐藏定时器：3秒后自动隐藏错误提示
                    Timer {
                        id: hideTimer  // 控件ID：错误提示的自动隐藏定时器
                        interval: 3000  // 控件：3000ms（3秒）后触发
                        onTriggered: {
                            errorText.opacity = 0  // 控件：开始渐出动画
                            Qt.callLater(function() {
                                if (errorText.opacity === 0) {
                                    errorText.visible = false  // 控件：动画完成后完全隐藏
                                }
                            })
                        }
                    }
                }
            }
        }
    }
    
    // 信号连接：监听来自 C++ bridge 的登录结果信号
    Connections {
        target: bridge  // 控件：监听 bridge 对象的信号
        function onLoginSuccess(userData) {
            loginWindow.loginSuccess()  // 控件：登录成功，触发信号通知主窗口切换页面
        }
        function onLoginFailed(message) {
            errorText.text = message  // 控件：设置错误提示文本内容
            errorText.visible = true  // 控件：显示错误提示
        }
        function onErrorOccurred(message) {
            errorText.text = message  // 控件：设置错误提示文本内容
            errorText.visible = true  // 控件：显示错误提示
        }
    }
}


