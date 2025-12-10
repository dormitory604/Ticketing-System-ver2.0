import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: favoritesWindow
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
                    onClicked: favoritesWindow.backToSearch()
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
                    text: "我的收藏"
                    font.pixelSize: 24
                    font.bold: true
                    color: "white"
                }
                
                Item { Layout.fillWidth: true }
                
                Button {
                    text: "刷新"
                    onClicked: bridge.getMyFavorites()
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
                id: favoritesListView
                model: bridge ? bridge.myFavorites : []
                spacing: 10
                delegate: Rectangle {
                    width: favoritesListView.width - 20
                    height: 120
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "white"
                    border.color: "#e0e0e0"
                    border.width: 1
                    radius: 4
                    
                    property var favoriteData: modelData || {}
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 20
                        
                        Column {
                            Layout.fillWidth: true
                            spacing: 5
                            
                            Text {
                                text: favoriteData.flight_number || ""
                                font.pixelSize: 18
                                font.bold: true
                                color: "#2196F3"
                            }
                            
                            Text {
                                text: (favoriteData.origin || "") + " → " + (favoriteData.destination || "")
                                font.pixelSize: 14
                                color: "#666"
                            }
                            
                            Text {
                                text: "收藏时间: " + formatDateTime(favoriteData.created_at || "")
                                font.pixelSize: 12
                                color: "#999"
                            }
                        }
                        
                        Text {
                            text: "¥" + (favoriteData.price || 0)
                            font.pixelSize: 24
                            font.bold: true
                            color: "#FF9800"
                        }
                        
                        Button {
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 35
                            text: "取消收藏"
                            background: Rectangle {
                                color: parent.pressed ? "#f44336" : "#e57373"
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
                                if (favoriteData.flight_id) {
                                    bridge.removeFavorite(favoriteData.flight_id)
                                }
                            }
                        }
                    }
                    
                    // 辅助函数：格式化日期时间
                    function formatDateTime(dateTimeString) {
                        if (!dateTimeString || dateTimeString === "") return ""
                        var parts = dateTimeString.split("T")
                        if (parts.length === 2) {
                            var datePart = parts[0].split("-")
                            var timePart = parts[1].substring(0, 5)
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
    
    Component.onCompleted: {
        bridge.getMyFavorites()
    }
}


