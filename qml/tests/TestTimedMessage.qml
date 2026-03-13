import QtQuick
import QtQuick.Layouts
import QtQuick.Window


Window {
    width: 800
    height: 600

    components.TimedMessage {
        id: timedMessageBox
    }

    Component.onCompleted: {
	    console.warn("Okay we're ready to display messages.")
        //timedMessageBox.open()
	    timedMessageBox.addMessage(6000,"Long message","This message will last for 6 seconds")
        timedMessageBox.addMessage(4000,"Another message", "This message will last for 4 seconds")
        timedMessageBox.addMessage(2000,"Shorter message", "This will last for only 2 seconds.")
    }
}
