import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import DSV.Canvas
import DSV.Setting

Window
{
    id: root
    visible: true
    width: 1200
    height: 710
    color: "#8E8E8E"
    title: "DSV"

    FileDialog
    {
        id: openFileDialog
        title: qsTr("Open")
        nameFilters: ["All Files (*.*)", "JSON: (*.json *.JSON)", "PLT (*.plt *.PLT)", "PDF (*.pdf *.PDF)"]
        fileMode: FileDialog.OpenFile
        onAccepted:
        {
            filePath.text = openFileDialog.file.toString().slice(8)
            if (filePath.text.length > 0)
            {
                canvas.open_file(filePath.text)
            }
        }
    }

    FileDialog
    {
        id: saveFileDialog
        title: qsTr("Save")
        nameFilters: ["JSON: (*.json *.JSON)"]
        fileMode: FileDialog.SaveFile
        onAccepted:
        {
            filePath.text = saveFileDialog.file.toString().slice(8)
            if (filePath.text.length > 0)
            {
                canvas.save_file(filePath.text)
            }
        }
    }

    Timer
    {
        id: autoSaveTimer
        interval: 3000
        repeat: true
        triggeredOnStart: true
        onTriggered:
        {
            if (filePath.text.length > 0 && (filePath.text.endWith(".json") || filePath.text.endWith(".JSON")))
            {
                save.triggered()
            }
        }
    }

    DSVSetting
    {
        id: setting
    }

    MenuBar
    {
        Menu
        {
            title: "file"
            MenuItem
            {
                text: "open"
                onTriggered:
                {
                    if (canvas.modifed)
                    {
                        saveAs.triggered()
                    }
                    openFileDialog.selectedNameFilter.index = openFileDialog.nameFilters.indexOf(setting.FileType)
                    openFileDialog.open()
                }
            }
            MenuItem
            {
                id: save
                text: "save"
                onTriggered:
                {
                    if (filePath.text.length > 0 && (filePath.txt.endWith(".json") || filePath.text.endWith(".JSON")))
                    {
                        canvas.save_file(filePath.text)
                    }
                    else
                    {
                        saveAs.triggered()
                    }
                }
            }
            MenuItem
            {
                id: saveAs
                text: "save as"
                onTriggered: saveFileDialog.open()
            }
            MenuItem
            {
                id: autoSave
                text: "auto save"
                checkable: true
                onCheckedChanged:
                {
                    if (checked)
                    {
                        autoSaveTimer.start()
                    }
                    else
                    {
                        autoSaveTimer.stop()
                    }
                    setting.AutoSave = checked
                }
            }
        }

        Menu
        {
            title: "setting"
            MenuItem
            {
                id: autoLayering
                text: "auto layering"
                checkable: true
                onCheckedChanged: setting.AutoLayering = checked
            }
            MenuItem
            {
                id: rememberFileType
                text: "remember file type"
                checkable: true
                onCheckedChanged: setting.RememberFileType = checked
            }
            MenuItem
            {
                id: autoAligning
                text: "auto aligning"
                onCheckedChanged: setting.AutoAligning = checked
            }
            MenuItem
            {
                text: "advanced"
            }
        }
    }

    Text
    {
        id: currentTool
        width: 120
        height: 24
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 10
        anchors.bottomMargin: 10

        font.pointSize: 12
        horizontalAlignment: Text.AlignHCenter
    }

    Button
    {
        id: noTool
        width: 102
        height: 25
        anchors.horizontalCenter: currentTool.horizontalCenter
        anchors.top: currentTool.bottom
        anchors.topMargin: 10
        anchors.bottomMargin: 10

        text: "←"
        font.pointSize: 10
        onClicked:
        {
            canvas.use_last_tool()
        }
    }

    Button
    {
        id: polylineTool
        width: 102
        height: 25
        anchors.horizontalCenter: currentTool.horizontalCenter
        anchors.top: noTool.bottom
        anchors.topMargin: 10
        anchors.bottomMargin: 10

        text: "Polyline"
        font.pointSize: 10

        onClicked: canvas.use_tool(1)
    }

    Button
    {
        id: rectangleTool
        width: 102
        height: 25
        anchors.horizontalCenter: currentTool.horizontalCenter
        anchors.top: polylineTool.bottom
        anchors.topMargin: 10
        anchors.bottomMargin: 10

        text: "Rectangle"
        font.pointSize: 10

        onClicked: canvas.use_tool(2)
    }

    Button
    {
        id: circleTool
        width: 102
        height: 25
        anchors.horizontalCenter: currentTool.horizontalCenter
        anchors.top: rectangleTool.bottom
        anchors.topMargin: 10
        anchors.bottomMargin: 10

        text: "Circle"
        font.pointSize: 10

        onClicked: canvas.use_tool(0)
    }

    Button
    {
        id: curveTool
        width: 52
        height: 25
        anchors.top: circleTool.bottom
        anchors.left: circleTool.left
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        anchors.rightMargin: 3

        text: "Curve"
        font.pointSize: 10

        onClicked:
        {
            canvas.set_bezier_order(curveOrder.value)
            canvas.use_tool(3)
        }
    }

    SpinBox
    {
        id: curveOrder
        width: 46
        height: 24
        anchors.top: circleTool.bottom
        anchors.left: curveTool.right
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        anchors.leftMargin: 3

        from: 2
        to: 5
        value: 3
    }

    Button
    {
        id: rotateTool
        width: 52
        height: 25
        anchors.top: curveTool.bottom
        anchors.left: circleTool.left
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        anchors.rightMargin: 3

        text: "Rotate"
        font.pointSize: 10
    }

    Rectangle
    {
        width: 46
        height: 25
        anchors.top: curveTool.bottom
        anchors.left: rotateTool.right
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        anchors.leftMargin: 3
        radius: 2

        TextInput
        {
            id: rotateAnlge
            width: 28
            height: 18
            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.bottom: parent.bottom
            anchors.topMargin: 8
            clip: true

            font.pointSize: 10
            validator: IntValidator
            {
                bottom: -360
                top: 360
            }
        }

        Label
        {
            width: 8
            height: 25
            anchors.right: parent.right
            anchors.top: rotateAnlge.top
            text: "°"
        }
    }

    Button
    {
        id: flipXTool
        width: 50
        height: 25
        anchors.top: rotateTool.bottom
        anchors.left: rotateTool.left
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        anchors.rightMargin: 3

        text: "FlipX"
        font.pointSize: 10
    }

    Button
    {
        id: flipYTool
        width: 50
        height: 25
        anchors.top: rotateTool.bottom
        anchors.left: flipXTool.right
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        anchors.leftMargin: 3

        text: "FlipY"
        font.pointSize: 10
    }

    Rectangle
    {
        width: parent.width - 147
        height: parent.height - 4
        anchors.left: parent.left
        anchors.leftMargin: 145
        anchors.top: parent.top
        anchors.topMargin: 4

        DSVCanvas
        {
            id: canvas
            anchors.fill: parent
        }

        Rectangle
        {
            id: textInputRect
            visible: false
            TextEdit
            {
                id: textInput
                anchors.fill: parent
                wrapMode: TextEdit.Wrap
            }
        }
    }    

    Rectangle
    {
        width: parent.width
        height: 30
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        color: "#8E8E8E"

        Text
        {
            id: coord
            height: 24
            width: Math.max(contentWidth, 80)
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.leftMargin: 2
            anchors.rightMargin: 2

            font.pointSize: 12
            text: "X" + canvas.mouseX + " Y" + canvas.mouseY
        }

        Rectangle
        {
            id: sparator1
            width: 1
            height: 24
            anchors.left: coord.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            color: "black"
        }

        Text
        {
            id: infomation
            height: 24
            width: Math.max(contentWidth, 140)
            anchors.left: sparator1.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 2
            anchors.rightMargin: 2

            font.pointSize: 12
        }

        Rectangle
        {
            id: sparator2
            width: 1
            height: 24
            anchors.left: infomation.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            color: "black"
        }

        Text
        {
            id: filePath
            height: 24
            width: 240
            anchors.left: sparator2.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 2
            anchors.rightMargin: 2

            font.pointSize: 12
        }

        ComboBox
        {
            id: layersCbx
            width: 60
            height: 24

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 6
        }

        Button
        {
            id: layers
            width: 52
            height: 26
            anchors.right: layersCbx.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 4
            background: Rectangle
            {
                color: "#8D8D8D"
                border.width: 1
                radius: 2
            }

            text: "Layers"
        }
    }


    Connections
    {
        target: canvas
        function onToolChanged(tool)
        {
            switch(tool)
            {
            case 0:
                currentTool.text = "Circle"
                break
            case 1:
                currentTool.text = "Polyline"
                break
            case 2:
                currentTool.text = "Rectangle"
                break
            case 3:
                currentTool.text = "Bezier Curve"
                break
            default:
                currentTool.text = ""
            }
        }
    }

    Connections
    {
        target: canvas
        function onInfoChanged(info)
        {
           infomation.text = info
        }
    }

    Connections
    {
        target: canvas
        function onSaveFile()
        {
           save.triggered()
        }
    }

    Connections
    {
        target: canvas
        function onInputText(x, y, w, h, str)
        {
            textInputRect.x = x
            textInputRect.y = y
            textInputRect.width = w
            textInputRect.height = h
            textInputRect.visible = true
            textInput.text = str
            textInput.focus = true
        }
    }

    Connections
    {
        target: canvas
        function onInputTextOver()
        {
            textInputRect.visible = false
            canvas.set_container_text(textInput.text)
            textInput.focus = false
            canvas.focus = true
        }
    }

    Component.onCompleted:
    {
        autoSave.checked = setting.AutoSave
        autoLayering.checked = setting.AutoLayering
        rememberFileType.checked = setting.RememberFileType
        autoAligning.checked = setting.AutoAligning
    }
}
