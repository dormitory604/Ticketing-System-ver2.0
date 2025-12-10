import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: bookingDialog
    width: 650
    height: 750
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent
    
    property var bridge
    property int flightId: -1
    property var flightData: ({})
    property bool paymentCompleted: false
    
    signal bookingConfirmed()
    
    // 阴影效果
    Rectangle {
        anchors.fill: parent
        anchors.margins: -10
        color: "#40000000"
        radius: 12
        opacity: 0.3
        z: -1
    }
    
    function openDialog(flight) {
        if (flight) {
            flightId = flight.flight_id || -1
            flightData = flight
            paymentCompleted = false
            passengerField.text = bridge ? bridge.currentUsername : ""
            baggageCombo.currentIndex = 0
            paymentCombo.currentIndex = 0
            regenerateQrCode()
            bookingDialog.open()
        }
    }
    
    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"
        radius: 12
        
        // 标题栏（带渐变效果）
        Rectangle {
            id: titleBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 60
            radius: 12
            
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#2196F3" }
                GradientStop { position: 1.0; color: "#1976D2" }
            }
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 25
                anchors.rightMargin: 15
                spacing: 15
                
                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: "#FFFFFF"
                    opacity: 0.2
                    
                    Text {
                        anchors.centerIn: parent
                        text: "✈"
                        font.pixelSize: 20
                        color: "white"
                    }
                }
                
                Text {
                    Layout.fillWidth: true
                    text: "预订航班"
                    font.pixelSize: 20
                    font.bold: true
                    color: "white"
                }
                
                Button {
                    width: 32
                    height: 32
                    background: Rectangle {
                        color: parent.hovered ? "#FFFFFF" : "transparent"
                        radius: 16
                        opacity: parent.hovered ? 0.2 : 0.1
                    }
                    contentItem: Text {
                        text: "×"
                        font.pixelSize: 24
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: bookingDialog.close()
                }
            }
        }
        
        ScrollView {
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: buttonBar.top
            clip: true
            
            Flickable {
                contentWidth: bookingDialog.width
                contentHeight: contentColumn.height + 40
                boundsBehavior: Flickable.StopAtBounds
                
                Column {
                    id: contentColumn
                    width: bookingDialog.width
                    spacing: 0
                    topPadding: 20
                    bottomPadding: 20
                    
                    // 航班信息卡片（突出显示）
                    Rectangle {
                        width: parent.width - 50
                        height: 110
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: "#F5F9FF"
                        radius: 8
                        border.color: "#E3F2FD"
                        border.width: 2
                        anchors.margins: 25
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 20
                            spacing: 20
                            
                            // 左侧：航班图标
                            Rectangle {
                                Layout.preferredWidth: 60
                                Layout.preferredHeight: 60
                                radius: 30
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#2196F3" }
                                    GradientStop { position: 1.0; color: "#1976D2" }
                                }
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "✈"
                                    font.pixelSize: 28
                                    color: "white"
                                }
                            }
                            
                            // 中间：航班信息
                            Column {
                                Layout.fillWidth: true
                                spacing: 8
                                
                                Row {
                                    spacing: 15
                                    Text {
                                        text: "航班号"
                                        font.pixelSize: 12
                                        color: "#999"
                                    }
                                    Text {
                                        text: flightData.flight_number || ""
                                        font.pixelSize: 18
                                        font.bold: true
                                        color: "#2196F3"
                                    }
                                }
                                
                                Row {
                                    spacing: 10
                                    Text {
                                        text: "●"
                                        font.pixelSize: 8
                                        color: "#4CAF50"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: (flightData.origin || "") + " → " + (flightData.destination || "")
                                        font.pixelSize: 15
                                        color: "#333"
                                    }
                                }
                            }
                            
                            // 右侧：价格（突出显示）
                            Column {
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                spacing: 5
                                
                                Text {
                                    text: "票价"
                                    font.pixelSize: 12
                                    color: "#999"
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                
                                Text {
                                    text: "¥" + (flightData.price || 0)
                                    font.pixelSize: 28
                                    font.bold: true
                                    color: "#FF5722"
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                        }
                    }
                    
                    Item { width: 1; height: 20 } // 间距
                    
                    // 分隔线
                    Rectangle {
                        width: parent.width - 50
                        height: 1
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: "#E0E0E0"
                    }
                    
                    Item { width: 1; height: 20 } // 间距
                    
                    // 表单区域
                    Column {
                        width: parent.width - 50
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 18
                        
                        // 乘机人信息
                        Column {
                            width: parent.width
                            spacing: 8
                            
                            Text {
                                text: "乘机人姓名"
                                font.pixelSize: 14
                                font.bold: true
                                color: "#333"
                            }
                            
                            Rectangle {
                                width: parent.width
                                height: 45
                                color: "#FAFAFA"
                                radius: 6
                                border.color: passengerField.focus ? "#2196F3" : "#E0E0E0"
                                border.width: passengerField.focus ? 2 : 1
                                
                                TextField {
                                    id: passengerField
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    placeholderText: "请输入乘机人姓名"
                                    font.pixelSize: 15
                                    background: Item {}
                                }
                            }
                        }
                        
                        // 行李额和支付方式（并排显示）
                        Row {
                            width: parent.width
                            spacing: 15
                            
                            // 托运行李额
                            Column {
                                width: (parent.width - parent.spacing) / 2
                                spacing: 8
                                
                                Text {
                                    text: "托运行李额"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#333"
                                }
                                
                                Rectangle {
                                    width: parent.width
                                    height: 45
                                    color: "#FAFAFA"
                                    radius: 6
                                    border.color: baggageCombo.hovered ? "#2196F3" : "#E0E0E0"
                                    border.width: baggageCombo.hovered ? 2 : 1
                                    
                                    ComboBox {
                                        id: baggageCombo
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        model: ["15kg", "20kg", "25kg", "30kg"]
                                        font.pixelSize: 15
                                        background: Item {}
                                        delegate: ItemDelegate {
                                            width: baggageCombo.width
                                            text: modelData
                                            font.pixelSize: 15
                                            padding: 10
                                        }
                                    }
                                }
                            }
                            
                            // 支付方式
                            Column {
                                width: (parent.width - parent.spacing) / 2
                                spacing: 8
                                
                                Text {
                                    text: "支付方式"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#333"
                                }
                                
                                Rectangle {
                                    width: parent.width
                                    height: 45
                                    color: "#FAFAFA"
                                    radius: 6
                                    border.color: paymentCombo.hovered ? "#2196F3" : "#E0E0E0"
                                    border.width: paymentCombo.hovered ? 2 : 1
                                    
                                    ComboBox {
                                        id: paymentCombo
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        model: ["支付宝", "微信", "银联", "信用卡"]
                                        font.pixelSize: 15
                                        background: Item {}
                                        delegate: ItemDelegate {
                                            width: paymentCombo.width
                                            text: modelData
                                            font.pixelSize: 15
                                            padding: 10
                                        }
                                    }
                                }
                            }
                        }
                        
                        Item { width: 1; height: 10 } // 间距
                        
                        // 分隔线
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: "#E0E0E0"
                        }
                        
                        Item { width: 1; height: 10 } // 间距
                        
                        // 二维码支付区域
                        Rectangle {
                            width: parent.width
                            height: 280
                            color: "#FAFAFA"
                            radius: 8
                            border.color: "#E0E0E0"
                            border.width: 1
                            
                            Column {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 15
                                
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "支付二维码"
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#333"
                                }
                                
                                // 二维码预览（带背景）
                                Rectangle {
                                    width: 180
                                    height: 180
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    color: "white"
                                    radius: 8
                                    border.color: "#E0E0E0"
                                    border.width: 1
                                    
                                    // 阴影效果
                                    Rectangle {
                                        anchors.fill: parent
                                        anchors.margins: -2
                                        color: "#20000000"
                                        radius: 10
                                        z: -1
                                    }
                                    
                                    Canvas {
                                        id: qrCanvas
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        
                                        onPaint: {
                                            var ctx = getContext("2d")
                                            ctx.fillStyle = "#FFFFFF"
                                            ctx.fillRect(0, 0, width, height)
                                            
                                            // 生成随机二维码图案
                                            var cellSize = Math.floor(width / 25)
                                            var gridCount = 25
                                            
                                            for (var y = 0; y < gridCount; y++) {
                                                for (var x = 0; x < gridCount; x++) {
                                                    var fill = Math.random() < 0.5
                                                    ctx.fillStyle = fill ? "#000000" : "#FFFFFF"
                                                    ctx.fillRect(x * cellSize, y * cellSize, cellSize, cellSize)
                                                }
                                            }
                                        }
                                        
                                        Component.onCompleted: requestPaint()
                                    }
                                }
                                
                                Button {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: 140
                                    height: 36
                                    text: "🔄 刷新二维码"
                                    font.pixelSize: 13
                                    background: Rectangle {
                                        color: parent.pressed ? "#1976D2" : (parent.hovered ? "#42A5F5" : "#2196F3")
                                        radius: 6
                                    }
                                    contentItem: Text {
                                        text: parent.text
                                        font: parent.font
                                        color: "white"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: {
                                        regenerateQrCode()
                                    }
                                }
                                
                                // 状态提示（更美观）
                                Rectangle {
                                    width: parent.width
                                    height: 40
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    color: paymentCompleted ? "#E8F5E9" : "#FFF3E0"
                                    radius: 6
                                    border.color: paymentCompleted ? "#C8E6C9" : "#FFE0B2"
                                    border.width: 1
                                    
                                    Text {
                                        id: statusText
                                        anchors.centerIn: parent
                                        text: "二维码已更新，请支付 ¥" + (flightData.price || 0) + "。"
                                        font.pixelSize: 13
                                        color: paymentCompleted ? "#2E7D32" : "#E65100"
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                }
                            }
                        }
                        
                        Item { width: 1; height: 15 } // 间距
                        
                        // 支付确认按钮（更突出）
                        Button {
                            width: parent.width
                            height: 50
                            text: paymentCompleted ? "✓ 支付已确认" : "💳 我已完成支付"
                            enabled: !paymentCompleted
                            background: Rectangle {
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: parent.enabled ? (parent.pressed ? "#43A047" : "#66BB6A") : "#CCCCCC" }
                                    GradientStop { position: 1.0; color: parent.enabled ? (parent.pressed ? "#388E3C" : "#4CAF50") : "#CCCCCC" }
                                }
                                radius: 8
                                border.color: parent.enabled ? "#43A047" : "#CCCCCC"
                                border.width: 1
                            }
                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 16
                                font.bold: true
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                paymentCompleted = true
                                statusText.text = "✓ 已确认支付 ¥" + (flightData.price || 0) + "，可提交订单"
                                statusText.color = "#2E7D32"
                                submitButton.enabled = true
                            }
                        }
                    }
                }
            }
        }
        
        // 底部按钮栏
        Rectangle {
            id: buttonBar
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 75
            color: "#FAFAFA"
            radius: 0
            
            Row {
                anchors.fill: parent
                anchors.leftMargin: 25
                anchors.rightMargin: 25
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12
                
                Button {
                    width: (parent.parent.width - 50 - parent.spacing) * 0.7
                    height: 48
                    id: submitButton
                    text: "✓ 提交订单"
                    enabled: paymentCompleted && passengerField.text.length > 0
                    background: Rectangle {
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: parent.enabled ? (parent.pressed ? "#1976D2" : "#2196F3") : "#CCCCCC" }
                            GradientStop { position: 1.0; color: parent.enabled ? (parent.pressed ? "#1565C0" : "#1976D2") : "#CCCCCC" }
                        }
                        radius: 8
                        border.color: parent.enabled ? "#1976D2" : "#CCCCCC"
                        border.width: 1
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 16
                        font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (!paymentCompleted) {
                            statusText.text = "⚠ 请先完成支付确认"
                            statusText.color = "#D32F2F"
                            return
                        }
                        
                        if (passengerField.text.trim().length === 0) {
                            statusText.text = "⚠ 请输入乘机人姓名"
                            statusText.color = "#D32F2F"
                            return
                        }
                        
                        // 提交订单
                        if (bridge && flightId > 0) {
                            bridge.bookFlight(flightId)
                            bookingDialog.close()
                            bookingConfirmed()
                        }
                    }
                }
                
                Button {
                    width: (parent.parent.width - 50 - parent.spacing) * 0.3
                    height: 48
                    text: "取消"
                    background: Rectangle {
                        color: parent.pressed ? "#616161" : (parent.hovered ? "#757575" : "#9E9E9E")
                        radius: 8
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 15
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        bookingDialog.close()
                    }
                }
            }
        }
    }
    
    function regenerateQrCode() {
        paymentCompleted = false
        submitButton.enabled = false
        statusText.text = "二维码已更新，请支付 ¥" + (flightData.price || 0) + "。"
        statusText.color = "#E65100"
        qrCanvas.requestPaint()
    }
}
