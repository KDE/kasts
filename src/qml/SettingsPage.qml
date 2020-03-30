import QtQuick 2.0
import org.kde.kirigami 2.8 as Kirigami
import QtQuick.Controls 2.10 as Controls

Kirigami.ScrollablePage {
    title: "Settings"

    property QtObject settings


    Kirigami.FormLayout {
        Controls.TextField {
            id: deleteAfterCount
            text: settings.deleteAfterCount
            Kirigami.FormData.label: "Delete posts after:"
        }
        Controls.ComboBox {
            id: deleteAfterType
            currentIndex: settings.deleteAfterType
            model: ["Posts", "Days", "Weeks", "Months"]
        }
        Controls.Button {
            text: "Save"
            onClicked: {
                settings.deleteAfterCount = deleteAfterCount.text
                settings.deleteAfterType = deleteAfterType.currentIndex
                settings.save()
                pageStack.pop()
            }
        }
    }
}
