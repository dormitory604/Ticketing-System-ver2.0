import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: calendarPicker
    width: 800
    height: 600
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    
    property date selectedDate: new Date()
    property var flightPrices: ({}) // { "2025-12-11": 395, "2025-12-12": 400, ... }
    property date currentMonth: new Date()
    
    signal dateSelected(date selectedDate)
    
    Rectangle {
        anchors.fill: parent
        color: "white"
        radius: 8
        
        Column {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15
            
            // 顶部：月份导航
            RowLayout {
                width: parent.width
                spacing: 10
                
                Button {
                    width: 40
                    height: 40
                    text: "←"
                    onClicked: {
                        currentMonth = new Date(currentMonth.getFullYear(), currentMonth.getMonth() - 1, 1)
                    }
                    background: Rectangle {
                        color: parent.pressed ? "#E0E0E0" : "#F5F5F5"
                        radius: 4
                    }
                }
                
                Item { Layout.fillWidth: true }
                
                Button {
                    width: 40
                    height: 40
                    text: "→"
                    onClicked: {
                        currentMonth = new Date(currentMonth.getFullYear(), currentMonth.getMonth() + 1, 1)
                    }
                    background: Rectangle {
                        color: parent.pressed ? "#E0E0E0" : "#F5F5F5"
                        radius: 4
                    }
                }
            }
            
            // 双月日历视图
            RowLayout {
                width: parent.width
                spacing: 25
                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                
                // 第一个月
                Column {
                    Layout.preferredWidth: (parent.width - parent.spacing) / 2
                    Layout.fillHeight: false
                    spacing: 12
                    
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: currentMonth.getFullYear() + "年" + (currentMonth.getMonth() + 1) + "月"
                        font.pixelSize: 17
                        font.bold: true
                        color: "#333"
                        height: 25
                    }
                    
                    CalendarGrid {
                        id: month1Grid
                        width: parent.width
                        month: currentMonth
                        selectedDate: calendarPicker.selectedDate
                        flightPrices: calendarPicker.flightPrices
                        onDateClicked: function(clickedDate) {
                            calendarPicker.selectedDate = clickedDate
                            calendarPicker.dateSelected(clickedDate)
                        }
                    }
                }
                
                // 第二个月
                Column {
                    Layout.preferredWidth: (parent.width - parent.spacing) / 2
                    Layout.fillHeight: false
                    spacing: 12
                    
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: {
                            var nextMonth = new Date(currentMonth.getFullYear(), currentMonth.getMonth() + 1, 1)
                            return nextMonth.getFullYear() + "年" + (nextMonth.getMonth() + 1) + "月"
                        }
                        font.pixelSize: 17
                        font.bold: true
                        color: "#333"
                        height: 25
                    }
                    
                    CalendarGrid {
                        id: month2Grid
                        width: parent.width
                        month: {
                            var nextMonth = new Date(currentMonth.getFullYear(), currentMonth.getMonth() + 1, 1)
                            return nextMonth
                        }
                        selectedDate: calendarPicker.selectedDate
                        flightPrices: calendarPicker.flightPrices
                        onDateClicked: function(clickedDate) {
                            calendarPicker.selectedDate = clickedDate
                            calendarPicker.dateSelected(clickedDate)
                        }
                    }
                }
            }
        }
    }
    
    // 日历网格组件
    component CalendarGrid: Column {
        property date month
        property date selectedDate
        property var flightPrices: ({})
        signal dateClicked(date clickedDate)
        
        width: parent.width
        spacing: 8
        
        // 星期标题行
        Row {
            width: parent.width
            spacing: 4
            Repeater {
                model: ["日", "一", "二", "三", "四", "五", "六"]
                Item {
                    width: (parent.width - parent.spacing * 6) / 7
                    height: 24
                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        font.pixelSize: 13
                        color: "#666"
                    }
                }
            }
        }
        
        // 日期网格容器（固定高度确保对称）
        Grid {
            id: dateGrid
            width: parent.width
            columns: 7
            rowSpacing: 4
            columnSpacing: 4
            
            // 空白填充（月份第一天之前）
            Repeater {
                model: getFirstDayOfMonth()
                Item {
                    width: (dateGrid.width - dateGrid.columnSpacing * 6) / 7
                    height: 52
                }
            }
            
            // 日期单元格（固定6行以确保对称，即使月份只有30天或31天）
            Repeater {
                model: 42 // 固定 6 行 x 7 列，后续通过 visible 控制
                delegate: Rectangle {
                    width: (dateGrid.width - dateGrid.columnSpacing * 6) / 7
                    height: 52
                    visible: index < getDaysInMonth()
                    enabled: visible
                    
                    property int dayNumber: index + 1
                    color: {
                        if (!visible) return "transparent"
                        var dateStr = formatDate(dayNumber)
                        var isSelected = isDateSelected(dayNumber)
                        var isToday = isTodayDate(dayNumber)
                        
                        if (isSelected) return "#E3F2FD"
                        if (isToday) return "#FFF3E0"
                        return "#F5F5F5"
                    }
                    radius: 4
                    border.color: {
                        if (!visible) return "transparent"
                        var isSelected = isDateSelected(dayNumber)
                        var isToday = isTodayDate(dayNumber)
                        if (isSelected) return "#2196F3"
                        if (isToday) return "#FF9800"
                        return "#E0E0E0"
                    }
                    border.width: {
                        if (!visible) return 0
                        var isSelected = isDateSelected(dayNumber)
                        var isToday = isTodayDate(dayNumber)
                        if (isSelected) return 2
                        if (isToday) return 2
                        return 1
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        enabled: parent.visible
                        onClicked: {
                            if (parent.visible) {
                                var newDate = new Date(month.getFullYear(), month.getMonth(), parent.dayNumber)
                                dateClicked(newDate)
                            }
                        }
                    }
                    
                    Column {
                        anchors.fill: parent
                        anchors.margins: 3
                        spacing: 1
                        visible: parent.visible
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: {
                                var isToday = isTodayDate(dayNumber)
                                if (isToday) return "今天"
                                return dayNumber.toString()
                            }
                            font.pixelSize: {
                                var isSelected = isDateSelected(dayNumber)
                                var isToday = isTodayDate(dayNumber)
                                return (isSelected || isToday) ? 14 : 13
                            }
                            font.bold: isDateSelected(dayNumber) || isTodayDate(dayNumber)
                            color: {
                                var isSelected = isDateSelected(dayNumber)
                                var isToday = isTodayDate(dayNumber)
                                if (isSelected) return "#2196F3"
                                if (isToday) return "#FF9800"
                                return "#333"
                            }
                        }
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            visible: {
                                var dateStr = formatDate(dayNumber)
                                return flightPrices[dateStr] !== undefined && flightPrices[dateStr] > 0
                            }
                            text: {
                                var dateStr = formatDate(dayNumber)
                                return "¥" + (flightPrices[dateStr] || 0)
                            }
                            font.pixelSize: 10
                            font.bold: true
                            color: "#FF5722"
                        }
                        
                        // 特殊日期标记
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            visible: {
                                var special = getSpecialDateLabel(dayNumber)
                                return special !== ""
                            }
                            text: getSpecialDateLabel(dayNumber)
                            font.pixelSize: 9
                            color: "#666"
                        }
                    }
                    
                    function formatDate(day) {
                        var year = month.getFullYear()
                        var monthNum = month.getMonth() + 1
                        return year + "-" + (monthNum < 10 ? "0" + monthNum : monthNum) + "-" + (day < 10 ? "0" + day : day)
                    }
                    
                    function isDateSelected(day) {
                        var dateStr = formatDate(day)
                        var selectedStr = selectedDate.getFullYear() + "-" + 
                                         (selectedDate.getMonth() + 1 < 10 ? "0" + (selectedDate.getMonth() + 1) : (selectedDate.getMonth() + 1)) + "-" + 
                                         (selectedDate.getDate() < 10 ? "0" + selectedDate.getDate() : selectedDate.getDate())
                        return dateStr === selectedStr && 
                               month.getMonth() === selectedDate.getMonth() &&
                               month.getFullYear() === selectedDate.getFullYear()
                    }
                    
                    function isTodayDate(day) {
                        var today = new Date()
                        return day === today.getDate() &&
                               month.getMonth() === today.getMonth() &&
                               month.getFullYear() === today.getFullYear()
                    }
                    
                    function getSpecialDateLabel(day) {
                        var dateStr = formatDate(day)
                        var parts = dateStr.split("-")
                        if (parts.length === 3) {
                            var year = parseInt(parts[0])
                            var monthNum = parseInt(parts[1])
                            var dayNum = parseInt(parts[2])
                            
                            // 公历固定节假日
                            // 1月1日 - 元旦
                            if (monthNum === 1 && dayNum === 1) return "元旦"
                            // 3月8日 - 妇女节
                            if (monthNum === 3 && dayNum === 8) return "妇女节"
                            // 3月12日 - 植树节
                            if (monthNum === 3 && dayNum === 12) return "植树节"
                            // 4月1日 - 愚人节
                            if (monthNum === 4 && dayNum === 1) return "愚人节"
                            // 5月1日 - 劳动节
                            if (monthNum === 5 && dayNum === 1) return "劳动节"
                            // 5月4日 - 青年节
                            if (monthNum === 5 && dayNum === 4) return "青年节"
                            // 6月1日 - 儿童节
                            if (monthNum === 6 && dayNum === 1) return "儿童节"
                            // 7月1日 - 建党节
                            if (monthNum === 7 && dayNum === 1) return "建党节"
                            // 8月1日 - 建军节
                            if (monthNum === 8 && dayNum === 1) return "建军节"
                            // 9月10日 - 教师节
                            if (monthNum === 9 && dayNum === 10) return "教师节"
                            // 10月1日 - 国庆节
                            if (monthNum === 10 && dayNum === 1) return "国庆节"
                            // 11月11日 - 光棍节
                            if (monthNum === 11 && dayNum === 11) return "光棍节"
                            // 12月25日 - 圣诞节
                            if (monthNum === 12 && dayNum === 25) return "圣诞节"
                            
                            // 清明节（通常是4月4日或4月5日，根据年份略有变化）
                            if (monthNum === 4) {
                                // 清明节的公历日期：通常是4月4日或4月5日
                                var qingmingDay = getQingmingDay(year)
                                if (dayNum === qingmingDay) return "清明节"
                            }
                            
                            // 农历节日的公历映射（2024-2030年）
                            var lunarHoliday = getLunarHoliday(year, monthNum, dayNum)
                            if (lunarHoliday !== "") return lunarHoliday
                        }
                        return ""
                    }
                    
                    // 获取清明节的日期（根据年份，通常是4月4日或5日）
                    function getQingmingDay(year) {
                        // 清明节的简单计算：1900-2100年间，大部分年份是4月5日，部分年份是4月4日
                        // 这里使用一个简化的算法
                        var century = Math.floor(year / 100)
                        var yearInCentury = year % 100
                        // 简化的清明日期计算
                        if ((year % 4 === 0 && year % 100 !== 0) || year % 400 === 0) {
                            // 闰年
                            if (yearInCentury < 20) return 4
                            return 5
                        } else {
                            // 平年
                            if (yearInCentury < 20) return 5
                            return 4
                        }
                    }
                    
                    // 获取农历节日的公历日期（2024-2030年）
                    function getLunarHoliday(year, month, day) {
                        // 春节日期映射表（2024-2030年）
                        var springFestival = {
                            2024: { month: 2, day: 10 },
                            2025: { month: 1, day: 29 },
                            2026: { month: 2, day: 17 },
                            2027: { month: 2, day: 6 },
                            2028: { month: 1, day: 26 },
                            2029: { month: 2, day: 13 },
                            2030: { month: 2, day: 3 }
                        }
                        
                        // 端午节日期映射表（农历五月初五，2024-2030年）
                        var dragonFestival = {
                            2024: { month: 6, day: 10 },
                            2025: { month: 5, day: 31 },
                            2026: { month: 6, day: 19 },
                            2027: { month: 6, day: 9 },
                            2028: { month: 5, day: 29 },
                            2029: { month: 6, day: 16 },
                            2030: { month: 6, day: 6 }
                        }
                        
                        // 中秋节日期映射表（农历八月十五，2024-2030年）
                        var midAutumnFestival = {
                            2024: { month: 9, day: 17 },
                            2025: { month: 10, day: 6 },
                            2026: { month: 9, day: 25 },
                            2027: { month: 9, day: 15 },
                            2028: { month: 10, day: 3 },
                            2029: { month: 9, day: 22 },
                            2030: { month: 9, day: 12 }
                        }
                        
                        // 检查春节
                        if (springFestival[year]) {
                            var sf = springFestival[year]
                            if (month === sf.month && day === sf.day) return "春节"
                            // 春节前后几天也标记
                            if (month === sf.month) {
                                if (day === sf.day - 1) return "除夕"
                                if (day === sf.day + 1) return "初二"
                                if (day === sf.day + 2) return "初三"
                                if (day === sf.day + 3) return "初四"
                                if (day === sf.day + 4) return "初五"
                                if (day === sf.day + 5) return "初六"
                            }
                            // 跨月情况
                            if (year === 2025 && month === 1 && day === 28) return "除夕"
                            if (year === 2025 && month === 1 && day === 30) return "初二"
                            if (year === 2025 && month === 1 && day === 31) return "初三"
                        }
                        
                        // 检查端午节
                        if (dragonFestival[year]) {
                            var df = dragonFestival[year]
                            if (month === df.month && day === df.day) return "端午节"
                        }
                        
                        // 检查中秋节
                        if (midAutumnFestival[year]) {
                            var maf = midAutumnFestival[year]
                            if (month === maf.month && day === maf.day) return "中秋节"
                        }
                        
                        return ""
                    }
                }
            }
        }
        
        function getFirstDayOfMonth() {
            var firstDay = new Date(month.getFullYear(), month.getMonth(), 1)
            return firstDay.getDay() // 0 = Sunday, 1 = Monday, etc.
        }
        
        function getDaysInMonth() {
            return new Date(month.getFullYear(), month.getMonth() + 1, 0).getDate()
        }
    }
    
    function setDateFromString(dateString) {
        if (!dateString || dateString === "") {
            selectedDate = new Date()
            currentMonth = new Date()
            return
        }
        var parts = dateString.split("-")
        if (parts.length === 3) {
            selectedDate = new Date(parseInt(parts[0]), parseInt(parts[1]) - 1, parseInt(parts[2]))
            currentMonth = new Date(parseInt(parts[0]), parseInt(parts[1]) - 1, 1)
        }
    }
    
    function getDateString() {
        var year = selectedDate.getFullYear()
        var month = selectedDate.getMonth() + 1
        var day = selectedDate.getDate()
        return year + "-" + (month < 10 ? "0" + month : month) + "-" + (day < 10 ? "0" + day : day)
    }
}

