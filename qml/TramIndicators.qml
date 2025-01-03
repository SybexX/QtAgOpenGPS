import QtQuick

//Make a round circle, for tram indicators
Rectangle{
    id: root
    property bool manual: false
    property bool toggleState: false
    width: 50
    height: width
    radius: width / 2 // set to circle -- hackish
    border.width: 2
    border.color: "black"
    opacity: .5
    color: "red"
    onManualChanged: if(manual){
                        timer1.running = true
                         timer1.repeat = true
                     }
                     else{
                         timer1.running = false
                         root.color = "yellow"
                     }


    MouseArea{ // so we can set to manual or auto
        anchors.fill: root
        onClicked: manual = !manual
    }
    Timer{ // flicker for when on manual
        id: timer1
        running: false
        repeat: true
        interval: 500
        onTriggered: toggle()
    }
    function toggle(){
        root.toggleState = !root.toggleState
        if(root.toggleState)
            root.color = "black"
        else
            root.color = "yellow"
    }
}
