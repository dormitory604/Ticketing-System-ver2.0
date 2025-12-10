import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: profileWindow
    color: "#f5f5f5"
    
    property var bridge
    signal backToSearch()
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#2196F3"
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                
                Button {
                    text: "← 返回"
                    onClicked: profileWindow.backToSearch()
                    background: Rectangle {
                        color: parent.pressed ? "#1976D2" : "transparent"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 14
                        color: "white"
                    }
                }
                
                Text {
                    text: "个人资料"
                    font.pixelSize: 24
                    font.bold: true
                    color: "white"
                }
                
                Item { Layout.fillWidth: true }
            }
        }
        
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "white"
            
            ColumnLayout {
                anchors.centerIn: parent
                width: 400
                spacing: 20
                
                Text {
                    Layout.fillWidth: true
                    text: "修改个人信息"
                    font.pixelSize: 24
                    font.bold: true
                }
                
                Text {
                    Layout.fillWidth: true
                    text: "当前用户名: " + (bridge ? bridge.currentUsername : "")
                    font.pixelSize: 16
                    color: "#666"
                }
                
                TextField {
                    id: usernameField
                    Layout.fillWidth: true
                    placeholderText: "新用户名"
                    text: bridge ? bridge.currentUsername : ""
                    font.pixelSize: 16
                }
                
                TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    placeholderText: "新密码"
                    echoMode: TextInput.Password
                    font.pixelSize: 16
                }
                
                TextField {
                    id: confirmPasswordField
                    Layout.fillWidth: true
                    placeholderText: "确认新密码"
                    echoMode: TextInput.Password
                    font.pixelSize: 16
                }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45
                    text: "保存"
                    background: Rectangle {
                        color: parent.pressed ? "#1976D2" : "#2196F3"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 16
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
                        bridge.updateProfile(usernameField.text, passwordField.text)
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
        function onProfileUpdateSuccess(message, userData) {
            successText.text = message
            successText.visible = true
            errorText.visible = false
        }
        function onProfileUpdateFailed(message) {
            errorText.text = message
            errorText.visible = true
            successText.visible = false
        }
    }
}


