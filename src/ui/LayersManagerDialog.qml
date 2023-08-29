import QtQuick
import QtQuick.Controls


Dialog
{
    id: root
    x: parent.width / 2 - root.width/2
    y: parent.height / 2 - root.height/2
    width: 400
    height: 300
    opacity: 1
    closePolicy: Popup.NoAutoClose
    modal: true

    property int currentLayer: 0
    signal operator(int code, int index)

    Text
    {
        width: parent.width
        height: 30
        anchors.top: parent.top
        text: "Layers"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        MouseArea
        {
            property point clickPoint: "0,0"

            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onPressed:
            {
                clickPoint  = Qt.point(mouseX, mouseY)
            }
            onPositionChanged:
            {
                var dlgX = mouseX - clickPoint.x
                var dlgY = mouseY - clickPoint.y
                if(root.x + dlgX < 0)
                {
                    root.x = 0
                }
                else if(root.x + dlgX > root.parent.width - root.width)
                {
                    root.x = root.parent.width - root.width
                }
                else
                {
                    root.x = root.x + dlgX
                }
                if(root.y + dlgY < 0)
                {
                    root.y = 0
                }
                else if(root.y + dlgY > root.parent.height - root.height)
                {
                    root.y = root.parent.height - root.height
                }
                else
                {
                    root.y = root.y + dlgY
                }
            }
        }
    }

    Menu
    {
        id: options

        MenuItem
        {
            text: "ON/OFF"
            onTriggered:
            {
                layersModel.get(layersView.currentIndex)["v"] = !layersModel.get(layersView.currentIndex)["v"]
                operator(0, layersView.currentIndex)
            }
        }
        MenuItem
        {
            text: "Add"
            onTriggered:
            {
                layersModel.insert(0, {name:String(layersModel.count), v:true})
                operator(1, 0)
            }
        }
        MenuItem
        {
            text: "Insert"
            onTriggered:
            {
                layersModel.insert(layersView.currentIndex, {name:String(layersModel.count), v:true})
                operator(2, layersView.currentIndex)
            }
        }
        MenuItem
        {
            text: "Remove"
            onTriggered:
            {
                layersModel.remove(layersView.currentIndex)
                operator(3, layersView.currentIndex)
            }
        }
    }

    ListView
    {
        id: layersView
        anchors.fill: parent
        anchors.topMargin: 30
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.bottomMargin: 30
        spacing: 1

        model: ListModel
        {
            id: layersModel
        }
        delegate: Rectangle
        {
            id: shape
            width: 380
            height: 18
            radius: 2
            border.width: ListView.isCurrentItem ? 1 : 0
            anchors.horizontalCenter: parent.horizontalCenter
            color: index === currentLayer ? "#F0F0F0" : "#FFFFFF"

            Text
            {
                anchors.fill: parent
                anchors.leftMargin: 2
                text: (v ? "O " : "X ") + name
            }

            MouseArea
            {
                anchors.fill: parent
                acceptedButtons: "LeftButton"
                onClicked:
                {
                    layersView.currentIndex = index
                }
                onDoubleClicked:
                {
                    inputRect.parent = parent
                    input.text = layersModel.get(index)["name"]
                    inputRect.index = index
                    inputRect.visible = true
                }
            }
        }

        MouseArea
        {
            anchors.fill: parent
            acceptedButtons: "RightButton"
            onClicked:
            {
                options.popup()
            }
        }
    }

    Rectangle
    {
        id: inputRect
        anchors.fill: parent
        visible: false
        property int index: 0

        TextInput
        {
            id: input
            width: parent.width - 2
            height: parent.height
            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.top: parent.top
            onEditingFinished:
            {
                layersModel.get(inputRect.index)["name"] = text
                inputRect.visible = false
            }
        }
    }

    Button
    {
        text: "Close"
        onClicked: root.close()
        width: parent.width / 3
        anchors.bottom:  parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }

    function load(names, visibles)
    {
        layersModel.clear()
        for (var i = 0, count = names.length; i < count; ++i)
        {
            layersModel.insert(0, {name:naems[i], v:visibles[i]})
        }
    }
}





