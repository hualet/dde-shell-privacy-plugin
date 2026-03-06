// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

AppletItem {
    id: privacyIndicator

    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 10
    
    // 调试模式：设为 true 可强制显示插件（红色背景）
    property bool debugMode: false
    
    // 图标大小
    property int iconSize: Math.min(Panel.rootObject.dockItemMaxSize * 0.6, 32)
    
    // 计算激活的图标数量
    property int activeIconCount: (Applet.cameraInUse ? 1 : 0) + (Applet.microphoneInUse ? 1 : 0) + (Applet.locationInUse ? 1 : 0)

    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : (debugMode ? iconSize : (Applet.anyDeviceInUse ? activeIconCount * iconSize : 0))
    implicitHeight: useColumnLayout ? (debugMode ? iconSize : (Applet.anyDeviceInUse ? activeIconCount * iconSize : 0)) : Panel.rootObject.dockSize

    visible: (debugMode || Applet.anyDeviceInUse) || hasAnimatingIcons
    
    // 是否有图标正在动画中
    property bool hasAnimatingIcons: cameraIcon.animating || micIcon.animating || locationIcon.animating
    
    // 调试背景色
    Rectangle {
        anchors.fill: parent
        color: debugMode ? "red" : "transparent"
        visible: debugMode
        opacity: 0.5
    }

    // 提示框
    PanelToolTip {
        id: toolTip
        text: Applet.getTooltipText() || ""
        toolTipX: DockPanelPositioner.x
        toolTipY: DockPanelPositioner.y
    }

    // 图标容器 - 使用 Item 手动计算位置
    Item {
        id: iconContainer
        anchors.centerIn: parent
        width: parent.width
        height: parent.height

        // 摄像头图标
        AnimatedIcon {
            id: cameraIcon
            shouldShow: Applet.cameraInUse || debugMode
            source: "icons/camera-video.svg"
            iconSize: privacyIndicator.iconSize
            iconIndex: 0
            totalCount: activeIconCount
        }

        // 麦克风图标
        AnimatedIcon {
            id: micIcon
            shouldShow: Applet.microphoneInUse || debugMode
            source: "icons/audio-input-microphone.svg"
            iconSize: privacyIndicator.iconSize
            iconIndex: (Applet.cameraInUse || debugMode) ? 1 : 0
            totalCount: activeIconCount
        }

        // 位置图标
        AnimatedIcon {
            id: locationIcon
            shouldShow: Applet.locationInUse || debugMode
            source: "icons/gps.svg"
            iconSize: privacyIndicator.iconSize
            iconIndex: ((Applet.cameraInUse || debugMode) ? 1 : 0) + ((Applet.microphoneInUse || debugMode) ? 1 : 0)
            totalCount: activeIconCount
        }
    }

    // 鼠标区域
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        onEntered: {
            if (Applet.anyDeviceInUse || debugMode) {
                var point = privacyIndicator.mapToItem(null, privacyIndicator.width / 2, privacyIndicator.height / 2)
                toolTip.DockPanelPositioner.bounding = Qt.rect(point.x, point.y, toolTip.width, toolTip.height)
                toolTip.open()
            }
        }

        onExited: {
            toolTip.close()
        }
    }

    Component.onCompleted: {
        console.log("Privacy indicator plugin loaded, debugMode:", debugMode)
        console.log("Camera:", Applet.cameraInUse, "Microphone:", Applet.microphoneInUse, "Location:", Applet.locationInUse)
    }

    // 带动画的图标组件
    component AnimatedIcon: Item {
        id: animatedIconRoot
        
        property bool shouldShow: false
        property string source
        property int iconSize: 32
        property int iconIndex: 0
        property int totalCount: 0
        
        // 是否正在动画中
        property bool animating: false
        
        // 动画期间冻结的位置
        property int frozenIndex: 0
        property int frozenCount: 0
        
        width: iconSize
        height: iconSize
        
        // 使用冻结的位置（如果正在动画）或当前位置
        x: useColumnLayout ? (parent.width - width) / 2 : calculateX(animating ? frozenIndex : iconIndex, animating ? frozenCount : totalCount)
        y: useColumnLayout ? calculateY(animating ? frozenIndex : iconIndex, animating ? frozenCount : totalCount) : (parent.height - height) / 2
        
        // 位置变化动画（补位动画）
        Behavior on x {
            enabled: !animating
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutCubic
            }
        }
        Behavior on y {
            enabled: !animating
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutCubic
            }
        }
        
        // 计算水平位置
        function calculateX(index, count) {
            if (count === 0) return 0
            var totalWidth = count * iconSize + (count - 1) * 4
            var startX = (parent.width - totalWidth) / 2
            return startX + index * (iconSize + 4)
        }
        
        // 计算垂直位置
        function calculateY(index, count) {
            if (count === 0) return 0
            var totalHeight = count * iconSize + (count - 1) * 4
            var startY = (parent.height - totalHeight) / 2
            return startY + index * (iconSize + 4)
        }

        Image {
            id: iconImage
            anchors.fill: parent
            source: parent.source
            sourceSize: Qt.size(parent.iconSize, parent.iconSize)
            opacity: 0
            scale: 0.5
        }

        // 出现动画（缩放+渐现）
        ParallelAnimation {
            id: showAnimation
            NumberAnimation {
                target: iconImage
                property: "opacity"
                from: 0
                to: 1
                duration: 250
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: iconImage
                property: "scale"
                from: 0.5
                to: 1
                duration: 250
                easing.type: Easing.OutBack
            }
            onStarted: {
                animatedIconRoot.frozenIndex = animatedIconRoot.iconIndex
                animatedIconRoot.frozenCount = animatedIconRoot.totalCount
                animatedIconRoot.animating = true
            }
            onStopped: animatedIconRoot.animating = false
        }

        // 消失动画（缩放+渐隐）
        ParallelAnimation {
            id: hideAnimation
            NumberAnimation {
                target: iconImage
                property: "opacity"
                from: 1
                to: 0
                duration: 200
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                target: iconImage
                property: "scale"
                from: 1
                to: 0.5
                duration: 200
                easing.type: Easing.InCubic
            }
            onStarted: {
                animatedIconRoot.frozenIndex = animatedIconRoot.iconIndex
                animatedIconRoot.frozenCount = animatedIconRoot.totalCount
                animatedIconRoot.animating = true
            }
            onStopped: animatedIconRoot.animating = false
        }

        onShouldShowChanged: {
            if (shouldShow) {
                hideAnimation.stop()
                showAnimation.restart()
            } else {
                showAnimation.stop()
                hideAnimation.restart()
            }
        }

        Component.onCompleted: {
            if (shouldShow) {
                iconImage.opacity = 1
                iconImage.scale = 1
            }
        }
    }
}
