import QtQuick 2.0
import org.kde.kirigami 2.8 as Kirigami
import QtQuick.Controls 2.10 as Controls

Kirigami.ScrollablePage {
    title: i18n("Settings")

    property QtObject settings


    Kirigami.FormLayout {
        Controls.TextField {
            id: deleteAfterCount
            text: settings.deleteAfterCount
            Kirigami.FormData.label: i18n("Delete posts after:")
        }
        Controls.ComboBox {
            id: deleteAfterType
            currentIndex: settings.deleteAfterType
            model: [i18n("Posts"), i18n("Days"), i18n("Weeks"), i18n("Months")]
        }
        Controls.Button {
            text: i18n("Save")
            onClicked: {
                settings.deleteAfterCount = deleteAfterCount.text
                settings.deleteAfterType = deleteAfterType.currentIndex
                settings.save()
                pageStack.pop()
            }
        }
    }
}
