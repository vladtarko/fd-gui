import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import FdGui

ApplicationWindow {
    id: window
    width: 1280
    height: 820
    minimumWidth: 820
    minimumHeight: 540
    visible: true
    title: "fd-gui"

    readonly property color fallbackPrimary: DmsTheme.dark ? "#D0BCFF" : "#6750A4"
    readonly property color fallbackBackground: DmsTheme.dark ? "#141218" : "#FEF7FF"
    readonly property color fallbackForeground: DmsTheme.dark ? "#E6E0E9" : "#1D1B20"
    readonly property color fallbackSurface: DmsTheme.dark ? "#211F24" : "#F2ECF4"

    property string selectedPath: ""
    property var selectedPaths: []
    property int selectionAnchorRow: -1
    property string errorText: ""

    Material.theme: DmsTheme.dark ? Material.Dark : Material.Light
    Material.primary: DmsTheme.color("primary", fallbackPrimary)
    Material.accent: DmsTheme.color("primary", fallbackPrimary)
    Material.background: DmsTheme.color("surface", fallbackBackground)
    Material.foreground: DmsTheme.color("on_surface", fallbackForeground)

    palette.window: DmsTheme.color("background", fallbackBackground)
    palette.windowText: DmsTheme.color("on_background", fallbackForeground)
    palette.base: DmsTheme.color("surface", fallbackBackground)
    palette.alternateBase: DmsTheme.color("surface_container", fallbackSurface)
    palette.text: DmsTheme.color("on_surface", fallbackForeground)
    palette.button: DmsTheme.color("surface_container_high", fallbackSurface)
    palette.buttonText: DmsTheme.color("on_surface", fallbackForeground)
    palette.highlight: DmsTheme.color("primary", fallbackPrimary)
    palette.highlightedText: DmsTheme.color("on_primary", DmsTheme.dark ? "#381E72" : "#FFFFFF")

    function runSearch() {
        window.errorText = ""
        FdSearch.search(searchField.text, extensionField.text, ignoreField.text, hiddenBox.checked)
    }

    function choose(row, path, modifiers) {
        if (row < 0 || path.length === 0) {
            return
        }

        listView.currentIndex = row

        if ((modifiers & Qt.ShiftModifier) && window.selectionAnchorRow >= 0) {
            const from = Math.min(window.selectionAnchorRow, row)
            const to = Math.max(window.selectionAnchorRow, row)
            const rows = []
            for (let i = from; i <= to; ++i) {
                const candidate = ResultModel.pathAt(i)
                if (candidate.length > 0 && rows.indexOf(candidate) < 0) {
                    rows.push(candidate)
                }
            }
            window.selectedPaths = rows
        } else if (modifiers & Qt.ControlModifier) {
            const rows = window.selectedPaths.slice()
            const index = rows.indexOf(path)
            if (index >= 0) {
                rows.splice(index, 1)
            } else {
                rows.push(path)
            }
            window.selectedPaths = rows
        } else {
            window.selectedPaths = [path]
            window.selectionAnchorRow = row
        }

        if (!(modifiers & Qt.ShiftModifier)) {
            window.selectionAnchorRow = row
        }

        window.selectedPath = path
    }

    function dragUris(path) {
        const paths = window.selectedPaths.length > 0 && window.selectedPaths.indexOf(path) >= 0
                ? window.selectedPaths
                : [path]
        return paths.map(function(item) {
            return Launcher.fileUrl(item).toString()
        }).join("\n")
    }

    Connections {
        target: FdSearch
        function onSearchError(message) {
            window.errorText = message
        }
    }

    Component.onCompleted: {
        extensionField.text = SettingsStore.extension
        ignoreField.text = SettingsStore.ignore
        hiddenBox.checked = SettingsStore.hidden
        runSearch()
        searchField.forceActiveFocus()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search files and directories..."
                selectByMouse: true
                onTextChanged: window.runSearch()
                onAccepted: window.runSearch()
            }

            TextField {
                id: extensionField
                Layout.preferredWidth: 130
                placeholderText: "extension"
                selectByMouse: true
                ToolTip.visible: hovered
                ToolTip.text: "Comma- or space-separated extensions"
                onTextChanged: {
                    SettingsStore.extension = text
                    window.runSearch()
                }
            }
        }

        TextField {
            id: ignoreField
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            placeholderText: "Ignore folders — one glob per line (for example: node_modules, .cache)"
            selectByMouse: true
            onTextChanged: {
                SettingsStore.ignore = text
                window.runSearch()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ComboBox {
                id: sortBox
                Layout.preferredWidth: 200
                model: ["Time (newest first)", "Name", "Size (largest first)", "Type name"]
                onCurrentIndexChanged: ResultModel.setSortKey(currentIndex)
            }

            CheckBox {
                id: foldersFirstBox
                text: "Folders first"
                checked: true
                onToggled: ResultModel.foldersFirst = checked
            }

            CheckBox {
                id: hiddenBox
                text: "Hidden"
                onToggled: {
                    SettingsStore.hidden = checked
                    window.runSearch()
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                visible: window.errorText.length > 0
                Layout.maximumWidth: 300
                color: Material.error
                text: window.errorText
                elide: Text.ElideRight
            }

            Label {
                text: FdSearch.scanning ? "scanning…" : ResultModel.count + " results"
                color: palette.mid
            }

            Button {
                text: "Stop"
                enabled: FdSearch.scanning
                onClicked: FdSearch.cancel()
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            handle: Rectangle {
                implicitWidth: 6
                color: palette.mid
                opacity: 0.4
            }

            Item {
                id: listPane
                SplitView.preferredWidth: 560
                SplitView.minimumWidth: 320

                ListView {
                    id: listView
                    anchors.fill: parent
                    model: ResultModel
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: ResultDelegate {
                        width: listView.width
                        row: index
                        path: model.path
                        name: model.name
                        displayPath: model.displayPath
                        url: model.url
                        isDir: model.isDir
                        typeName: model.typeName
                        size: model.size
                        selected: window.selectedPaths.indexOf(model.path) >= 0
                        dragUris: window.dragUris(model.path)
                        onChosen: function(row, path, modifiers) {
                            window.choose(row, path, modifiers)
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: ResultModel.count === 0 && !FdSearch.scanning
                    color: palette.mid
                    text: "No results"
                }
            }

            PreviewPane {
                id: previewPane
                SplitView.fillWidth: true
                SplitView.minimumWidth: 320
                path: window.selectedPath
            }
        }
    }
}
