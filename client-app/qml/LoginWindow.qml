import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: loginWindow
    color: "#f5f5f5"
    
    property var bridge
    signal requestRegister()
    signal loginSuccess()
    
    ColumnLayout {
        anchors.centerIn: parent
        width: 400
        spacing: 20
        
        // 标题
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "SkyTravel"
            font.pixelSize: 36
            font.bold: true
            color: "#2196F3"
        }
        
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "机票预订系统"
            font.pixelSize: 18
            color: "#666"
        }
        
        // 登录表单
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 300
            color: "white"
            radius: 8
            border.color: "#e0e0e0"
            border.width: 1
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 30
                spacing: 20
                
                Text {
                    Layout.fillWidth: true
                    text: "用户登录"
                    font.pixelSize: 24
                    font.bold: true
                }
                
                TextField {
                    id: usernameField
                    Layout.fillWidth: true
                    placeholderText: "用户名"
                    font.pixelSize: 16
                }
                
                TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    placeholderText: "密码"
                    echoMode: TextInput.Password
                    font.pixelSize: 16
                }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45
                    text: "登录"
                    font.pixelSize: 16
                    background: Rectangle {
                        color: parent.pressed ? "#1976D2" : "#2196F3"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        bridge.login(usernameField.text, passwordField.text)
                    }
                }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    text: "注册新用户"
                    font.pixelSize: 14
                    background: Rectangle {
                        color: "transparent"
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#2196F3"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        loginWindow.requestRegister()
                    }
                }
                
                // 错误提示
                Text {
                    id: errorText
                    Layout.fillWidth: true
                    visible: false
                    color: "red"
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }
            }
        }
    }
    
    Connections {
        target: bridge
        function onLoginSuccess(userData) {
            loginWindow.loginSuccess()
        }
        function onLoginFailed(message) {
            errorText.text = message
            errorText.visible = true
        }
        function onErrorOccurred(message) {
            errorText.text = message
            errorText.visible = true
        }
    }
}

