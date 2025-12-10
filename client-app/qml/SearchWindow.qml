import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

Rectangle {
    id: searchWindow
    color: "#f5f5f5"
    
    property var bridge
    signal requestOrders()
    signal requestFavorites()
    signal requestProfile()
    signal requestLogout()
    
    // 预订对话框
    BookingDialog {
        id: bookingDialog
        bridge: searchWindow.bridge
        onBookingConfirmed: {
            // 预订已确认，等待服务器响应
        }
    }
    
    // 日历选择器
    CalendarPicker {
        id: calendarPicker
        parent: searchWindow
        anchors.centerIn: parent
        onDateSelected: function(selectedDate) {
            dateField.text = calendarPicker.getDateString()
            calendarPicker.close()
            // 可以根据出发地和目的地加载该日期附近的价格数据
            if (originField.text && destField.text) {
                loadFlightPricesForCalendar(originField.text, destField.text, calendarPicker.getDateString())
            }
        }
        
        Component.onCompleted: {
            // 初始化当前日期
            calendarPicker.selectedDate = new Date()
            if (dateField.text) {
                calendarPicker.setDateFromString(dateField.text)
            }
        }
    }
    
    // 加载日历价格数据的函数（可以后续连接到服务器）
    function loadFlightPricesForCalendar(origin, dest, dateStr) {
        // 这里可以添加从服务器获取价格的逻辑
        // 目前使用示例价格数据，模拟真实的价格波动
        var prices = {}
        var baseDate = dateStr ? new Date(dateStr) : new Date()
        for (var i = -15; i <= 45; i++) {
            var checkDate = new Date(baseDate)
            checkDate.setDate(checkDate.getDate() + i)
            var dateKey = checkDate.getFullYear() + "-" + 
                         (checkDate.getMonth() + 1 < 10 ? "0" + (checkDate.getMonth() + 1) : (checkDate.getMonth() + 1)) + "-" + 
                         (checkDate.getDate() < 10 ? "0" + checkDate.getDate() : checkDate.getDate())
            // 生成随机价格，周末稍贵一些
            var dayOfWeek = checkDate.getDay()
            var basePrice = 320
            if (dayOfWeek === 0 || dayOfWeek === 6) {
                // 周末价格
                basePrice = 380
            }
            // 节假日价格更高
            var month = checkDate.getMonth() + 1
            var day = checkDate.getDate()
            if ((month === 12 && day === 25) || (month === 1 && day === 1)) {
                basePrice += 50
            }
            prices[dateKey] = basePrice + Math.floor(Math.random() * 150)
        }
        calendarPicker.flightPrices = prices
    }
    
    function getTodayString() {
        var today = new Date()
        var year = today.getFullYear()
        var month = today.getMonth() + 1
        var day = today.getDate()
        return year + "-" + (month < 10 ? "0" + month : month) + "-" + (day < 10 ? "0" + day : day)
    }
    
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
                
                Button {
                    id: dateSelectButton
                    Layout.preferredWidth: 180
                    Layout.preferredHeight: 40
                    text: {
                        if (dateField.text) {
                            var parts = dateField.text.split("-")
                            if (parts.length === 3) {
                                var date = new Date(parseInt(parts[0]), parseInt(parts[1]) - 1, parseInt(parts[2]))
                                var today = new Date()
                                var tomorrow = new Date(today)
                                tomorrow.setDate(tomorrow.getDate() + 1)
                                if (date.getDate() === today.getDate() && 
                                    date.getMonth() === today.getMonth() && 
                                    date.getFullYear() === today.getFullYear()) {
                                    return dateField.text + " 今天"
                                } else if (date.getDate() === tomorrow.getDate() && 
                                           date.getMonth() === tomorrow.getMonth() && 
                                           date.getFullYear() === tomorrow.getFullYear()) {
                                    return dateField.text + " 明天"
                                }
                                return dateField.text
                            }
                            return dateField.text
                        }
                        return "选择日期"
                    }
                    onClicked: {
                        // 打开日历前加载价格数据
                        if (originField.text && destField.text) {
                            loadFlightPricesForCalendar(originField.text, destField.text, dateField.text || getTodayString())
                        }
                        calendarPicker.open()
                    }
                    background: Rectangle {
                        color: dateField.text ? (parent.pressed ? "#E3F2FD" : "#FFFFFF") : "#FAFAFA"
                        border.color: dateField.text ? "#2196F3" : "#E0E0E0"
                        border.width: dateField.text ? 2 : 1
                        radius: 4
                    }
                    contentItem: Text {
                        text: dateSelectButton.text
                        font.pixelSize: 14
                        color: dateField.text ? "#2196F3" : "#999"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                
                // 隐藏的日期字段（用于存储）
                TextField {
                    id: dateField
                    visible: false
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
                        if (flightListView.currentIndex >= 0 && bridge && bridge.searchResults) {
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
                        if (flightListView.currentIndex >= 0 && bridge && bridge.searchResults) {
                            var flight = bridge.searchResults[flightListView.currentIndex]
                            if (flight && flight.flight_id) {
                                bookingDialog.openDialog(flight)
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
                    
                    // 正确访问模型数据 - QVariantList 的每个元素作为 modelData
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
                            Layout.preferredWidth: 180
                            spacing: 5
                            
                            Text {
                                text: (flightData && flightData.flight_number) ? flightData.flight_number : ""
                                font.pixelSize: 18
                                font.bold: true
                                color: "#2196F3"
                            }
                            
                            Text {
                                text: ((flightData && flightData.origin) ? flightData.origin : "") + 
                                      " → " + 
                                      ((flightData && flightData.destination) ? flightData.destination : "")
                                font.pixelSize: 14
                                color: "#666"
                            }
                        }
                        
                        Column {
                            Layout.preferredWidth: 220
                            spacing: 5
                            
                            Text {
                                text: "起飞: " + formatDateTime(flightData ? flightData.departure_time : "")
                                font.pixelSize: 14
                                color: "#333"
                            }
                            
                            Text {
                                text: "到达: " + formatDateTime(flightData ? flightData.arrival_time : "")
                                font.pixelSize: 14
                                color: "#333"
                            }
                        }
                        
                        Column {
                            Layout.preferredWidth: 120
                            spacing: 5
                            
                            Text {
                                text: "剩余座位: " + (flightData && flightData.remaining_seats !== undefined ? flightData.remaining_seats : 0)
                                font.pixelSize: 14
                                color: "#666"
                            }
                            
                            Text {
                                visible: flightData && flightData.remaining_seats !== undefined
                                text: {
                                    if (!flightData || flightData.remaining_seats === undefined) return ""
                                    var seats = flightData.remaining_seats
                                    return seats > 10 ? "充足" : seats > 0 ? "紧张" : "售罄"
                                }
                                font.pixelSize: 12
                                color: {
                                    if (!flightData || flightData.remaining_seats === undefined) return "#666"
                                    var seats = flightData.remaining_seats
                                    return seats > 10 ? "#4CAF50" : seats > 0 ? "#FF9800" : "#f44336"
                                }
                            }
                        }
                        
                        Item { Layout.fillWidth: true }
                        
                        Column {
                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                            spacing: 5
                            
                            Text {
                                text: "¥" + (flightData && flightData.price !== undefined ? flightData.price : 0)
                                font.pixelSize: 24
                                font.bold: true
                                color: "#FF9800"
                            }
                        }
                    }
                    
                    // 辅助函数：格式化日期时间
                    function formatDateTime(dateTimeString) {
                        if (!dateTimeString || dateTimeString === "") return ""
                        // 处理 ISO 格式: "2025-12-01T08:00:00" -> "12-01 08:00"
                        var parts = dateTimeString.split("T")
                        if (parts.length === 2) {
                            var datePart = parts[0].split("-")
                            var timePart = parts[1].substring(0, 5) // 只取 HH:MM
                            if (datePart.length === 3) {
                                return datePart[1] + "-" + datePart[2] + " " + timePart
                            }
                        }
                        return dateTimeString
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

