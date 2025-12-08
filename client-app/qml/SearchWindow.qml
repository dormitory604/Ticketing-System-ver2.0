import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: searchWindow
    color: "#f5f5f5"
    
    property var bridge
    signal requestOrders()
    signal requestFavorites()
    signal requestProfile()
    signal requestLogout()
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 顶部工具栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#2196F3"
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                
                Text {
                    text: "SkyTravel"
                    font.pixelSize: 24
                    font.bold: true
                    color: "white"
                }
                
                Item { Layout.fillWidth: true }
                
                Text {
                    text: "当前用户: " + (bridge ? bridge.currentUsername : "")
                    font.pixelSize: 14
                    color: "white"
                }
                
                Button {
                    text: "我的订单"
                    onClicked: searchWindow.requestOrders()
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
                
                Button {
                    text: "我的收藏"
                    onClicked: searchWindow.requestFavorites()
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
                
                Button {
                    text: "个人资料"
                    onClicked: searchWindow.requestProfile()
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
                
                Button {
                    text: "退出"
                    onClicked: searchWindow.requestLogout()
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
            }
        }
        
        // 搜索表单
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: "white"
            border.color: "#e0e0e0"
            border.width: 1
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15
                
                TextField {
                    id: originField
                    Layout.preferredWidth: 200
                    placeholderText: "出发地"
                    font.pixelSize: 14
                }
                
                TextField {
                    id: destField
                    Layout.preferredWidth: 200
                    placeholderText: "目的地"
                    font.pixelSize: 14
                }
                
                TextField {
                    id: dateField
                    Layout.preferredWidth: 180
                    placeholderText: "日期 (YYYY-MM-DD)"
                    font.pixelSize: 14
                }
                
                Button {
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 40
                    text: "搜索航班"
                    background: Rectangle {
                        color: parent.pressed ? "#1976D2" : "#2196F3"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 14
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        bridge.searchFlights(originField.text, destField.text, dateField.text)
                    }
                }
                
                Button {
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 40
                    text: "添加到收藏"
                    enabled: flightListView.currentIndex >= 0
                    background: Rectangle {
                        color: parent.enabled ? (parent.pressed ? "#4CAF50" : "#66BB6A") : "#cccccc"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 14
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (flightListView.currentIndex >= 0) {
                            var flight = bridge.searchResults[flightListView.currentIndex]
                            if (flight && flight.flight_id) {
                                bridge.addFavorite(flight.flight_id)
                            }
                        }
                    }
                }
                
                Button {
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 40
                    text: "预订"
                    enabled: flightListView.currentIndex >= 0
                    background: Rectangle {
                        color: parent.enabled ? (parent.pressed ? "#FF9800" : "#FFA726") : "#cccccc"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 14
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (flightListView.currentIndex >= 0) {
                            var flight = bridge.searchResults[flightListView.currentIndex]
                            if (flight && flight.flight_id) {
                                bridge.bookFlight(flight.flight_id)
                            }
                        }
                    }
                }
            }
        }
        
        // 航班列表
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            ListView {
                id: flightListView
                model: bridge ? bridge.searchResults : []
                spacing: 10
                delegate: Rectangle {
                    width: flightListView.width - 20
                    height: 120
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: flightListView.currentIndex === index ? "#E3F2FD" : "white"
                    border.color: flightListView.currentIndex === index ? "#2196F3" : "#e0e0e0"
                    border.width: flightListView.currentIndex === index ? 2 : 1
                    radius: 4
                    
                    property var flightData: modelData || {}
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            flightListView.currentIndex = index
                        }
                    }
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 20
                        
                        Column {
                            Layout.preferredWidth: 150
                            spacing: 5
                            
                            Text {
                                text: parent.parent.flightData.flight_number || ""
                                font.pixelSize: 18
                                font.bold: true
                                color: "#2196F3"
                            }
                            
                            Text {
                                text: (parent.parent.flightData.origin || "") + " → " + (parent.parent.flightData.destination || "")
                                font.pixelSize: 14
                                color: "#666"
                            }
                        }
                        
                        Column {
                            Layout.preferredWidth: 200
                            spacing: 5
                            
                            Text {
                                text: "起飞: " + (parent.parent.flightData.departure_time || "")
                                font.pixelSize: 14
                            }
                            
                            Text {
                                text: "到达: " + (parent.parent.flightData.arrival_time || "")
                                font.pixelSize: 14
                            }
                        }
                        
                        Text {
                            text: "剩余座位: " + (parent.parent.flightData.remaining_seats || 0)
                            font.pixelSize: 14
                            color: "#666"
                        }
                        
                        Item { Layout.fillWidth: true }
                        
                        Text {
                            text: "¥" + (parent.parent.flightData.price || 0)
                            font.pixelSize: 24
                            font.bold: true
                            color: "#FF9800"
                        }
                    }
                }
            }
        }
    }
    
    // 消息提示
    Rectangle {
        id: messageBox
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 40
        height: 50
        visible: false
        color: messageType === "error" ? "#f44336" : "#4CAF50"
        radius: 4
        
        property string messageType: "success"
        property string messageText: ""
        
        Text {
            anchors.centerIn: parent
            text: messageBox.messageText
            color: "white"
            font.pixelSize: 14
        }
        
        Timer {
            id: messageTimer
            interval: 3000
            onTriggered: messageBox.visible = false
        }
    }
    
    Connections {
        target: bridge
        function onSearchComplete() {
            // 搜索结果已更新
        }
        function onBookingSuccess(bookingData) {
            messageBox.messageType = "success"
            messageBox.messageText = "预订成功！"
            messageBox.visible = true
            messageTimer.restart()
        }
        function onBookingFailed(message) {
            messageBox.messageType = "error"
            messageBox.messageText = message
            messageBox.visible = true
            messageTimer.restart()
        }
        function onAddFavoriteSuccess(message) {
            messageBox.messageType = "success"
            messageBox.messageText = message
            messageBox.visible = true
            messageTimer.restart()
        }
        function onAddFavoriteFailed(message) {
            messageBox.messageType = "error"
            messageBox.messageText = message
            messageBox.visible = true
            messageTimer.restart()
        }
        function onErrorOccurred(message) {
            messageBox.messageType = "error"
            messageBox.messageText = message
            messageBox.visible = true
            messageTimer.restart()
        }
    }
}

