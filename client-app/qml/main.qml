import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: mainWindow
    width: 1200
    height: 800
    visible: true
    title: "SkyTravel"
    
    property var bridge: qmlBridge
    
    // 窗口栈管理
    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: loginPage
        
        // 登录页面
        Component {
            id: loginPage
            LoginWindow {
                bridge: mainWindow.bridge
                onRequestRegister: {
                    stackView.push(registerPage)
                }
                onLoginSuccess: {
                    stackView.replace(searchPage)
                }
            }
        }
        
        // 注册页面
        Component {
            id: registerPage
            RegisterWindow {
                bridge: mainWindow.bridge
                onBackToLogin: {
                    stackView.pop()
                }
                onRegisterSuccess: {
                    stackView.pop()
                }
            }
        }
        
        // 搜索页面
        Component {
            id: searchPage
            SearchWindow {
                bridge: mainWindow.bridge
                onRequestOrders: {
                    stackView.push(ordersPage)
                }
                onRequestFavorites: {
                    stackView.push(favoritesPage)
                }
                onRequestProfile: {
                    stackView.push(profilePage)
                }
                onRequestLogout: {
                    bridge.logout()
                    stackView.replace(loginPage)
                }
            }
        }
        
        // 订单页面
        Component {
            id: ordersPage
            OrdersWindow {
                bridge: mainWindow.bridge
                onBackToSearch: {
                    stackView.pop()
                }
            }
        }
        
        // 收藏页面
        Component {
            id: favoritesPage
            FavoritesWindow {
                bridge: mainWindow.bridge
                onBackToSearch: {
                    stackView.pop()
                }
            }
        }
        
        // 个人资料页面
        Component {
            id: profilePage
            ProfileWindow {
                bridge: mainWindow.bridge
                onBackToSearch: {
                    stackView.pop()
                }
            }
        }
    }
}

