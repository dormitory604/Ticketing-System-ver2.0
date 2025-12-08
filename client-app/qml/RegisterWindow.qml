import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: registerWindow
    color: "#f5f5f5"
    
    property var bridge
    signal backToLogin()
    signal registerSuccess()
    
    ColumnLayout {
        anchors.centerIn: parent
        width: 400
        spacing: 20
        
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "用户注册"
            font.pixelSize: 32
            font.bold: true
            color: "#2196F3"
        }
        
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 380
            color: "white"
            radius: 8
            border.color: "#e0e0e0"
            border.width: 1
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 30
                spacing: 20
                
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
                
                TextField {
                    id: confirmPasswordField
                    Layout.fillWidth: true
                    placeholderText: "确认密码"
                    echoMode: TextInput.Password
                    font.pixelSize: 16
                }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45
                    text: "注册"
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
                        if (passwordField.text !== confirmPasswordField.text) {
                            errorText.text = "两次输入的密码不一致"
                            errorText.visible = true
                            return
                        }
                        if (usernameField.text.length === 0 || passwordField.text.length === 0) {
                            errorText.text = "用户名和密码不能为空"
                            errorText.visible = true
                            return
                        }
                        bridge.registerUser(usernameField.text, passwordField.text)
                    }
                }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    text: "返回登录"
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
                        registerWindow.backToLogin()
                    }
                }
                
                Text {
                    id: errorText
                    Layout.fillWidth: true
                    visible: false
                    color: "red"
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }
                
                Text {
                    id: successText
                    Layout.fillWidth: true
                    visible: false
                    color: "green"
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }
            }
        }
    }
    
    Connections {
        target: bridge
        function onRegisterSuccess(message) {
            successText.text = message
            successText.visible = true
            errorText.visible = false
            Qt.callLater(function() {
                registerWindow.registerSuccess()
            }, 1500)
        }
        function onRegisterFailed(message) {
            errorText.text = message
            errorText.visible = true
            successText.visible = false
        }
        function onErrorOccurred(message) {
            errorText.text = message
            errorText.visible = true
            successText.visible = false
        }
    }
}

