import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: ordersWindow
    color: "#f5f5f5"
    
    property var bridge
    signal backToSearch()
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 顶部栏
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
                    onClicked: ordersWindow.backToSearch()
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
                    text: "我的订单"
                    font.pixelSize: 24
                    font.bold: true
                    color: "white"
                }
                
                Item { Layout.fillWidth: true }
                
                Button {
                    text: "刷新"
                    onClicked: bridge.getMyOrders()
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
        
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            ListView {
                id: ordersListView
                model: bridge ? bridge.myOrders : []
                spacing: 10
                delegate: Rectangle {
                    width: ordersListView.width - 20
                    height: 100
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "white"
                    border.color: "#e0e0e0"
                    border.width: 1
                    radius: 4
                    
                    property var orderData: modelData || {}
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 20
                        
                        Column {
                            Layout.fillWidth: true
                            spacing: 5
                            
                            Text {
                                text: "订单号: " + (orderData.booking_id || "")
                                font.pixelSize: 16
                                font.bold: true
                            }
                            
                            Text {
                                text: (orderData.flight_number || "") + " | " + 
                                      (orderData.origin || "") + " → " + (orderData.destination || "")
                                font.pixelSize: 14
                                color: "#666"
                            }
                            
                            Text {
                                text: "状态: " + (orderData.status === "confirmed" ? "已确认" : 
                                                  orderData.status === "cancelled" ? "已取消" : orderData.status || "")
                                font.pixelSize: 14
                                color: orderData.status === "confirmed" ? "#4CAF50" : "#f44336"
                            }
                        }
                        
                        Button {
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 35
                            text: "取消订单"
                            enabled: orderData.status === "confirmed"
                            visible: orderData.status === "confirmed"
                            background: Rectangle {
                                color: parent.enabled ? (parent.pressed ? "#f44336" : "#e57373") : "#cccccc"
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
                                if (orderData.booking_id) {
                                    bridge.cancelOrder(orderData.booking_id)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    Component.onCompleted: {
        bridge.getMyOrders()
    }
    
    Connections {
        target: bridge
        function onCancelOrderSuccess(message) {
            bridge.getMyOrders()
        }
    }
}


