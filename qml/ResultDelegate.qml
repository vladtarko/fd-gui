import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import FdGui

Rectangle {
    id: root
    width: 560
    height: 72
    radius: 6
    color: root.selected ? Qt.alpha(Material.accent, 0.24)
                         : (dragArea.hovered ? palette.alternateBase : "transparent")
    border.color: root.selected ? Material.accent : "transparent"
    border.width: root.selected ? 1 : 0

    property int row: -1
    property string path: ""
    property string name: ""
    property string displayPath: ""
    property url url: ""
    property bool isDir: false
    property string typeName: ""
    property double size: 0
    property bool selected: false
    property string dragUris: ""

    signal chosen(int row, string path, var modifiers)

    function formatSize(bytes) {
        if (bytes < 1024) {
            return bytes + " B"
        }
        if (bytes < 1048576) {
            return (bytes / 1024).toFixed(1) + " KiB"
        }
        if (bytes < 1073741824) {
            return (bytes / 1048576).toFixed(1) + " MiB"
        }
        return (bytes / 1073741824).toFixed(1) + " GiB"
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: 10

        Rectangle {
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
            radius: 8
            color: root.isDir ? Qt.alpha(Material.primary, 0.22)
                              : Qt.alpha(palette.mid, 0.18)

            Label {
                anchors.centerIn: parent
                text: root.isDir ? "D" : "F"
                color: root.isDir ? Material.primary : palette.text
                font.bold: true
                font.pixelSize: 14
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: root.name
                font.pixelSize: 15
                font.bold: true
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                Layout.maximumHeight: 30
                text: root.displayPath
                color: palette.mid
                font.pixelSize: 11
                wrapMode: Text.WrapAnywhere
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }

        Column {
            Layout.preferredWidth: 110
            spacing: 2
            visible: !root.isDir

            Label {
                width: parent.width
                text: root.formatSize(root.size)
                horizontalAlignment: Text.AlignRight
                color: palette.mid
                font.pixelSize: 11
            }

            Label {
                width: parent.width
                text: root.typeName
                horizontalAlignment: Text.AlignRight
                color: palette.mid
                font.pixelSize: 10
                elide: Text.ElideLeft
            }
        }
    }

    Drag.active: dragArea.drag.active
    Drag.source: root
    Drag.dragType: Drag.Automatic
    Drag.supportedActions: Qt.CopyAction
    Drag.hotSpot: Qt.point(18, 18)
    Drag.mimeData: ({
        "text/uri-list": root.dragUris,
        "text/plain": root.path
    })

    Item {
        id: dragProxy
        width: root.width
        height: root.height
        visible: false
    }

    MouseArea {
        id: dragArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        drag.target: dragProxy
        drag.threshold: 8
        onClicked: root.chosen(root.row, root.path, mouse.modifiers)
        onDoubleClicked: Launcher.open(root.url)
    }
}
