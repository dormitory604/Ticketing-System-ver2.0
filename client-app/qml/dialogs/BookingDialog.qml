import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// 预订对话框：用于显示航班预订界面的弹出对话框
Popup {
    id: bookingDialog
    width: 650
    height: 750
    modal: true  // 模态对话框：阻止与父窗口交互
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside  // 关闭策略：按ESC或点击外部关闭
    anchors.centerIn: parent  // 控件：将对话框定位在父窗口中心
    
    // 对话框数据属性
    property var bridge  // 桥接对象：用于与 C++ 后端通信
    property int flightId: -1  // 航班ID：当前选中的航班编号
    property var flightData: ({})  // 航班数据：包含航班详细信息（航班号、起降地、价格等）
    property bool paymentCompleted: false  // 支付状态：标记用户是否已确认支付
    
    signal bookingConfirmed()  // 信号：订单确认完成后触发
    
    // 阴影效果：为对话框添加外部阴影
    Rectangle {
        anchors.fill: parent
        anchors.margins: -10
        color: "#40000000"
        radius: 12
        opacity: 0.3
        z: -1  // 控件：z轴为-1，使阴影显示在对话框后面
    }
    
    // 函数：打开对话框并初始化数据
    function openDialog(flight) {
        if (flight) {
            flightId = flight.flight_id || -1
            flightData = flight
            paymentCompleted = false
            passengerField.text = bridge ? bridge.currentUsername : ""  // 控件：自动填充当前用户名到乘机人姓名输入框
            baggageCombo.currentIndex = 0  // 控件：重置托运行李下拉框为第一项
            paymentCombo.currentIndex = 0  // 控件：重置支付方式下拉框为第一项
            regenerateQrCode()  // 重新生成支付二维码
            bookingDialog.open()
        }
    }
    
    // 主容器：对话框的白色背景容器
    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"
        radius: 12  // 控件：圆角半径12px
        
        // 标题栏：带蓝色渐变的顶部标题区域
        Rectangle {
            id: titleBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 60  // 控件：标题栏高度60px
            radius: 12
            
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#2196F3" }  // 渐变起始颜色：浅蓝
                GradientStop { position: 1.0; color: "#1976D2" }  // 渐变结束颜色：深蓝
            }
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 25
                anchors.rightMargin: 15
                spacing: 15
                
                // 装饰图标：半透明白色圆圈内的飞机图标
                Rectangle {
                    width: 40
                    height: 40
                    radius: 20  // 控件：圆形（半径为宽度的一半）
                    color: "#FFFFFF"
                    opacity: 0.2
                    
                    Text {
                        anchors.centerIn: parent
                        text: "✈"  // 控件：显示飞机emoji图标
                        font.pixelSize: 20
                        color: "white"
                    }
                }
                
                // 标题文字："预订航班"
                Text {
                    Layout.fillWidth: true
                    text: "预订航班"
                    font.pixelSize: 20
                    font.bold: true
                    color: "white"
                }
                
                // 关闭按钮：点击关闭对话框
                Button {
                    width: 32
                    height: 32
                    background: Rectangle {
                        color: parent.hovered ? "#FFFFFF" : "transparent"  // 控件：鼠标悬停时显示半透明白色背景
                        radius: 16
                        opacity: parent.hovered ? 0.2 : 0.1
                    }
                    contentItem: Text {
                        text: "×"  // 控件：显示关闭符号
                        font.pixelSize: 24
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: bookingDialog.close()  // 控件：点击关闭对话框
                }
            }
        }
        
        // 可滚动内容区域：包含航班信息和表单的滚动视图
        ScrollView {
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: buttonBar.top
            clip: true  // 控件：裁剪超出边界的内容
            
            Flickable {
                contentWidth: bookingDialog.width
                contentHeight: contentColumn.height + 40  // 控件：内容高度由子元素决定
                boundsBehavior: Flickable.StopAtBounds  // 控件：滚动到边界时停止
                
                Column {
                    id: contentColumn
                    width: bookingDialog.width
                    spacing: 0
                    topPadding: 20
                    bottomPadding: 20
                    
                    // 航班信息卡片：突出显示当前航班的关键信息
                    Rectangle {
                        width: parent.width - 50
                        height: 110
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: "#F5F9FF"  // 控件：淡蓝色背景
                        radius: 8
                        border.color: "#E3F2FD"
                        border.width: 2
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 20
                            spacing: 20
                            
                            // 航班图标：渐变蓝色圆形背景内的飞机图标
                            Rectangle {
                                Layout.preferredWidth: 60
                                Layout.preferredHeight: 60
                                radius: 30  // 控件：圆形（半径为宽度的一半）
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#2196F3" }
                                    GradientStop { position: 1.0; color: "#1976D2" }
                                }
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "✈"  // 控件：飞机图标
                                    font.pixelSize: 28
                                    color: "white"
                                }
                            }
                            
                            // 航班信息：显示航班号和起降地
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
                                        text: flightData.flight_number || ""  // 控件：显示航班编号
                                        font.pixelSize: 18
                                        font.bold: true
                                        color: "#2196F3"
                                    }
                                }
                                
                                Row {
                                    spacing: 10
                                    Text {
                                        text: "●"  // 控件：装饰性圆点图标
                                        font.pixelSize: 8
                                        color: "#4CAF50"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: (flightData.origin || "") + " → " + (flightData.destination || "")  // 控件：显示"起点→终点"
                                        font.pixelSize: 15
                                        color: "#333"
                                    }
                                }
                            }
                            
                            // 价格显示：右侧橙红色突出显示票价
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
                                    text: "¥" + (flightData.price || 0)  // 控件：显示票价金额
                                    font.pixelSize: 28
                                    font.bold: true
                                    color: "#FF5722"  // 控件：橙红色强调价格
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                        }
                    }
                    
                    Item { width: 1; height: 20 } // 控件：20px间距占位符
                    
                    // 分隔线：灰色横线分隔航班信息和表单区域
                    Rectangle {
                        width: parent.width - 50
                        height: 1
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: "#E0E0E0"
                    }
                    
                    Item { width: 1; height: 20 } // 控件：20px间距占位符
                    
                    // 表单区域：包含乘机人、行李额、支付方式和二维码
                    Column {
                        width: parent.width - 50
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 18
                        
                        // 乘机人姓名输入区域
                        Column {
                            width: parent.width
                            spacing: 8
                            
                            Row {
                                spacing: 10
                                
                                Text {
                                    text: "乘机人姓名"  // 控件：标签文本
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#333"
                                }
                                
                                Text {
                                    text: "请输入乘机人姓名"  // 控件：提示文本
                                    font.pixelSize: 12
                                    color: passengerField.text.trim().length === 0 ? "#D32F2F" : "#999"  // 控件：为空时显示红色
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            
                            Rectangle {
                                width: parent.width
                                height: 45
                                color: "#FAFAFA"
                                radius: 6
                                border.color: passengerField.focus ? "#2196F3" : "#E0E0E0"  // 控件：获得焦点时边框变蓝
                                border.width: passengerField.focus ? 2 : 1
                                
                                TextField {
                                    id: passengerField  // 控件ID：用于获取输入的乘机人姓名
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    anchors.topMargin: 4
                                    anchors.bottomMargin: 4
                                    placeholderText: ""  // 控件：占位符已移至标签右侧
                                    font.pixelSize: 15
                                    verticalAlignment: TextInput.AlignVCenter  // 控件：垂直居中对齐
                                    background: Item {}  // 控件：移除默认背景，使用父容器的背景
                                }
                            }
                        }
                        
                        // 行李额和支付方式：两个下拉框并排显示
                        Row {
                            width: parent.width
                            spacing: 15
                            
                            // 托运行李额下拉框
                            Column {
                                width: (parent.width - parent.spacing) / 2  // 控件：占据一半宽度
                                spacing: 8
                                
                                Text {
                                    text: "托运行李额"  // 控件：标签文本
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#333"
                                }
                                
                                Rectangle {
                                    width: parent.width
                                    height: 45
                                    color: "#FAFAFA"
                                    radius: 6
                                    border.color: baggageCombo.hovered ? "#2196F3" : "#E0E0E0"  // 控件：鼠标悬停时边框变蓝
                                    border.width: baggageCombo.hovered ? 2 : 1
                                    
                                    ComboBox {
                                        id: baggageCombo  // 控件ID：托运行李额下拉选择框
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        anchors.topMargin: 4
                                        anchors.bottomMargin: 4
                                        model: ["15kg", "20kg", "25kg", "30kg"]  // 控件：可选的行李额选项
                                        font.pixelSize: 15
                                        background: Item {}
                                        contentItem: Text {
                                            text: baggageCombo.displayText
                                            font: baggageCombo.font
                                            color: "#333"
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: 5
                                        }
                                        delegate: ItemDelegate {
                                            width: baggageCombo.width
                                            text: modelData  // 控件：显示选项文本
                                            font.pixelSize: 15
                                            padding: 10
                                        }
                                    }
                                }
                            }
                            
                            // 支付方式下拉框
                            Column {
                                width: (parent.width - parent.spacing) / 2  // 控件：占据另一半宽度
                                spacing: 8
                                
                                Text {
                                    text: "支付方式"  // 控件：标签文本
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#333"
                                }
                                
                                Rectangle {
                                    width: parent.width
                                    height: 45
                                    color: "#FAFAFA"
                                    radius: 6
                                    border.color: paymentCombo.hovered ? "#2196F3" : "#E0E0E0"  // 控件：鼠标悬停时边框变蓝
                                    border.width: paymentCombo.hovered ? 2 : 1
                                    
                                    ComboBox {
                                        id: paymentCombo  // 控件ID：支付方式下拉选择框
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        anchors.topMargin: 4
                                        anchors.bottomMargin: 4
                                        model: ["支付宝", "微信", "银联", "信用卡"]  // 控件：可选的支付方式选项
                                        font.pixelSize: 15
                                        background: Item {}
                                        contentItem: Text {
                                            text: paymentCombo.displayText
                                            font: paymentCombo.font
                                            color: "#333"
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: 5
                                        }
                                        delegate: ItemDelegate {
                                            width: paymentCombo.width
                                            text: modelData  // 控件：显示选项文本
                                            font.pixelSize: 15
                                            padding: 10
                                        }
                                    }
                                }
                            }
                        }
                        
                        Item { width: 1; height: 10 } // 控件：10px间距占位符
                        
                        // 分隔线：灰色横线分隔表单和二维码区域
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: "#E0E0E0"
                        }
                        
                        Item { width: 1; height: 10 } // 控件：10px间距占位符
                        
                        // 二维码支付区域：包含二维码显示、刷新按钮和状态提示
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
                                    text: "支付二维码"  // 控件：二维码区域标题
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#333"
                                }
                                
                                // 二维码显示容器：白色背景内显示随机生成的二维码图案
                                Rectangle {
                                    width: 180
                                    height: 180
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    color: "white"
                                    radius: 8
                                    border.color: "#E0E0E0"
                                    border.width: 1
                                    
                                    // 阴影效果：为二维码容器添加外部阴影
                                    Rectangle {
                                        anchors.fill: parent
                                        anchors.margins: -2
                                        color: "#20000000"
                                        radius: 10
                                        z: -1  // 控件：z轴为-1，使阴影显示在容器后面
                                    }
                                    
                                    Canvas {
                                        id: qrCanvas  // 控件ID：用于绘制二维码的画布元素
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        
                                        // 控件：绘制二维码图案（25x25网格的随机黑白方块）
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
                                        
                                        Component.onCompleted: requestPaint()  // 控件：组件加载完成后立即绘制
                                    }
                                }
                                
                                // 刷新二维码按钮：点击重新生成二维码并重置支付状态
                                Button {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: 140
                                    height: 36
                                    text: "🔄 刷新二维码"  // 控件：按钮文本带刷新图标
                                    font.pixelSize: 13
                                    background: Rectangle {
                                        color: parent.pressed ? "#1976D2" : (parent.hovered ? "#42A5F5" : "#2196F3")  // 控件：按下/悬停/普通三种状态的颜色
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
                                        regenerateQrCode()  // 控件：点击调用重新生成二维码函数
                                    }
                                }
                                
                                // 状态提示框：根据支付状态显示不同颜色和文字的提示信息
                                Rectangle {
                                    width: parent.width
                                    height: 40
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    color: paymentCompleted ? "#E8F5E9" : "#FFF3E0"  // 控件：已支付显示绿色，未支付显示橙色
                                    radius: 6
                                    border.color: paymentCompleted ? "#C8E6C9" : "#FFE0B2"
                                    border.width: 1
                                    
                                    Text {
                                        id: statusText  // 控件ID：用于动态更新支付状态文本
                                        anchors.centerIn: parent
                                        text: "二维码已更新，请支付 ¥" + (flightData.price || 0) + "。"  // 控件：显示待支付金额
                                        font.pixelSize: 13
                                        color: paymentCompleted ? "#2E7D32" : "#E65100"  // 控件：已支付显示深绿色，未支付显示橙色
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                }
                            }
                        }
                        
                        Item { width: 1; height: 15 } // 控件：15px间距占位符
                        
                        // 支付确认按钮：用户点击表示已完成支付操作
                        Button {
                            width: parent.width
                            height: 50
                            text: paymentCompleted ? "✓ 支付已确认" : "💳 我已完成支付"  // 控件：根据支付状态显示不同文本
                            enabled: !paymentCompleted  // 控件：支付确认后禁用按钮防止重复点击
                            background: Rectangle {
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: parent.enabled ? (parent.pressed ? "#43A047" : "#66BB6A") : "#CCCCCC" }  // 控件：启用时绿色，禁用时灰色
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
                                paymentCompleted = true  // 控件：设置支付完成标志
                                statusText.text = "✓ 已确认支付 ¥" + (flightData.price || 0) + "，可提交订单"  // 控件：更新状态提示文本
                                statusText.color = "#2E7D32"  // 控件：将状态文本颜色改为绿色
                                submitButton.enabled = true  // 控件：启用底部的提交订单按钮
                            }
                        }
                    }
                }
            }
        }
        
        // 底部按钮栏：包含提交订单和取消按钮的固定底部区域
        Rectangle {
            id: buttonBar
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 75  // 控件：底部栏高度75px
            color: "#FAFAFA"
            radius: 0
            
            Row {
                anchors.fill: parent
                anchors.leftMargin: 25
                anchors.rightMargin: 25
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12
                
                // 提交订单按钮：点击向服务器提交航班预订请求
                Button {
                    width: (parent.parent.width - 50 - parent.spacing) * 0.7  // 控件：占据70%宽度
                    height: 48
                    id: submitButton  // 控件ID：提交订单按钮，初始禁用直到支付确认
                    text: "✓ 提交订单"
                    enabled: paymentCompleted && passengerField.text.length > 0  // 控件：仅当支付完成且填写乘机人姓名后启用
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
                        // 控件：验证支付状态
                        if (!paymentCompleted) {
                            statusText.text = "⚠ 请先完成支付确认"
                            statusText.color = "#D32F2F"
                            return
                        }
                        
                        // 控件：验证乘机人姓名
                        if (passengerField.text.trim().length === 0) {
                            statusText.text = "⚠ 请输入乘机人姓名"
                            statusText.color = "#D32F2F"
                            return
                        }
                        
                        // 控件：提交订单到服务器
                        if (bridge && flightId > 0) {
                            bridge.bookFlight(flightId)  // 调用 C++ 桥接对象的预订航班方法
                            bookingDialog.close()  // 关闭对话框
                            bookingConfirmed()  // 触发订单确认信号
                        }
                    }
                }
                
                // 取消按钮：点击关闭对话框不提交订单
                Button {
                    width: (parent.parent.width - 50 - parent.spacing) * 0.3  // 控件：占据30%宽度
                    height: 48
                    text: "取消"
                    background: Rectangle {
                        color: parent.pressed ? "#616161" : (parent.hovered ? "#757575" : "#9E9E9E")  // 控件：灰色背景，悬停/按下时变暗
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
                        bookingDialog.close()  // 控件：点击关闭对话框
                    }
                }
            }
        }
    }
    
    // 函数：重新生成二维码并重置支付状态
    function regenerateQrCode() {
        paymentCompleted = false  // 重置支付完成标志
        submitButton.enabled = false  // 禁用提交订单按钮
        statusText.text = "二维码已更新，请支付 ¥" + (flightData.price || 0) + "。"  // 更新状态提示文本
        statusText.color = "#E65100"  // 将状态文本颜色改为橙色
        qrCanvas.requestPaint()  // 触发二维码画布重新绘制
    }
}
