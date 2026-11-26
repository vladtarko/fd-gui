import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import FdGui

Item {
    id: root
    property string path: ""
    property var preview: ({})
    property int previewRequestId: 0

    onPathChanged: {
        root.preview = ({})
        root.previewRequestId = root.path.length > 0 ? Preview.request(root.path) : 0
    }

    Connections {
        target: Preview
        function onPreviewReady(requestId, path, data) {
            if (requestId === root.previewRequestId && path === root.path) {
                root.preview = data
            }
        }
    }

    Label {
        anchors.centerIn: parent
        visible: root.path.length === 0
        color: palette.mid
        text: "Select a result to preview"
    }

    Rectangle {
        anchors.fill: parent
        visible: root.path.length > 0
        color: palette.base
        border.color: palette.mid
        border.width: 1
        radius: 8

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: root.preview.name || Launcher.fileUrl(root.path).fileName
                        font.pixelSize: 18
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.preview.displayPath || root.path
                        color: palette.mid
                        font.pixelSize: 11
                        wrapMode: Text.WrapAnywhere
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                }

                Button {
                    text: "Open"
                    onClicked: Launcher.open(Launcher.fileUrl(root.path))
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                Label {
                    text: root.preview.typeName || ""
                    color: palette.mid
                    font.pixelSize: 11
                }

                Label {
                    text: root.preview.sizeText || ""
                    color: palette.mid
                    font.pixelSize: 11
                }

                Label {
                    text: root.preview.modified || ""
                    color: palette.mid
                    font.pixelSize: 11
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: palette.mid
                opacity: 0.35
            }

            Label {
                Layout.fillWidth: true
                visible: text.length > 0
                color: palette.mid
                font.pixelSize: 10
                wrapMode: Text.Wrap
                text: {
                    const notes = []
                    if (root.preview.note) {
                        notes.push(root.preview.note)
                    }
                    if (root.preview.truncated) {
                        notes.push("Preview truncated to 256 KiB")
                    }
                    return notes.join(" · ")
                }
            }

            Loader {
                id: previewLoader
                Layout.fillWidth: true
                Layout.fillHeight: true
                sourceComponent: {
                    const kind = String(root.preview.kind || "")
                    if (kind === "image") {
                        return imageComponent
                    }
                    if (kind === "markdown") {
                        return markdownComponent
                    }
                    if (kind === "text" || kind === "dir") {
                        return textComponent
                    }
                    return infoComponent
                }
            }
        }
    }

    Component {
        id: imageComponent

        Image {
            anchors.fill: parent
            source: Launcher.fileUrl(root.path)
            asynchronous: true
            cache: false
            fillMode: Image.PreserveAspectFit
        }
    }

    Component {
        id: textComponent

        Flickable {
            id: textFlickable
            anchors.fill: parent
            clip: true
            contentWidth: width
            contentHeight: textItem.contentHeight
            boundsBehavior: Flickable.StopAtBounds

            Text {
                id: textItem
                width: textFlickable.width
                text: root.preview.kind === "dir" ? (root.preview.entries || "")
                                                  : (root.preview.content || "")
                color: palette.text
                font.family: "monospace"
                font.pixelSize: 12
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }
        }
    }

    Component {
        id: markdownComponent

        Flickable {
            id: markdownFlickable
            anchors.fill: parent
            clip: true
            contentWidth: width
            contentHeight: markdownItem.contentHeight
            boundsBehavior: Flickable.StopAtBounds

            Text {
                id: markdownItem
                width: markdownFlickable.width
                text: root.preview.content || ""
                color: palette.text
                linkColor: Material.accent
                font.pixelSize: 13
                textFormat: Text.MarkdownText
                wrapMode: Text.Wrap
                onLinkActivated: function(link) {
                    Launcher.open(link)
                }
            }
        }
    }

    Component {
        id: infoComponent

        Item {
            anchors.fill: parent

            Label {
                anchors.centerIn: parent
                width: parent.width - 40
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WrapAnywhere
                color: palette.mid
                font.pixelSize: 14
                text: {
                    const kind = String(root.preview.kind || "")
                    if (kind === "pdf") {
                        return "PDF — use Open to view it in the default application"
                    }
                    if (kind === "binary") {
                        return "Binary file — no text preview available"
                    }
                    if (kind === "unreadable") {
                        return root.preview.error || "File could not be read"
                    }
                    if (kind === "missing") {
                        return "File no longer exists"
                    }
                    return "Loading preview…"
                }
            }
        }
    }
}
