# Translation Procedure

This guide explains how to handle translations when adding a new `qsTr` in a QML file and how to use the `update_translations` and `release_translations` options in CMakeLists.

## Note

1. An initial global translation has been done on all QML files with `qsTr` defined.
2. The CMakeLists file has been modified to handle translations when `LOCAL_QML` is set to `OFF`.

## Adding a New Translation

1. **Add `qsTr` to QML File:**
   When you add a new text element in a QML file, wrap the text with the `qsTr` function to mark it for translation.

   ```qml
   Text {
       text: qsTr("New Text to Translate")
   }
   ```

2. **Update Translation Files:**
   After adding the new `qsTr` text, you need to update the translation files to include the new text.

## Using CMakeLists Options

### `update_translations`

This option updates the translation files with new strings marked by `qsTr`.

1. Open your terminal or command prompt.
2. Navigate to your project directory.
3. Run the following command to update the translation files:

   ```sh
   cmake --build . --target update_translations
   ```

   This command will scan the QML files for new `qsTr` strings and update the `.ts` files accordingly.

### Translating `.ts` Files

Before releasing the translations, you need to translate the `.ts` files. You can do this using Qt Linguist or an automatic translation tool like ChatGPT.

1. **Using Qt Linguist:**
   - Open the `.ts` file in Qt Linguist.
   - Translate the strings manually.
   - Save the translated `.ts` file.

2. **Using ChatGPT or Other Automatic Translation Tools:**
   - Extract the strings from the `.ts` file.
   - Use the translation tool to translate the strings.
   - Update the `.ts` file with the translated strings.

### `release_translations`

This option generates the final `.qm` files from the `.ts` files, which are used by the application for translations.

1. Open your terminal or command prompt.
2. Navigate to your project directory.
3. Run the following command to release the translations:

   ```sh
   cmake --build . --target release_translations
   ```

   This command will compile the `.ts` files into `.qm` files.

## Setting Language Choice in `formgps_ui.cpp`

Before compiling the project, you need to set the language choice in `formgps_ui.cpp`.

1. Open `formgps_ui.cpp`.
2. Set the desired language by loading the appropriate translation file.

   ```cpp
   // filepath: e:\projects\AOG\cmake_qml\src\formgps_ui.cpp
   // ...existing code...
   // translate the QML
   QString language = "fr"; // Change this variable to "fr" or "en" as needed
   QString translationPath = QString("qml_%1.qm").arg(language);
   QTranslator translator;
   if (translator.load(translationPath)) {
       qDebug() << "Translation loaded from" << translationPath;
       QCoreApplication::installTranslator(&translator);
   } else {
       qDebug() << "Translation not loaded";
   }
   // ...existing code...
   ```

## Summary

1. Add `qsTr` to new text elements in QML files.
2. Run `cmake --build . --target update_translations` to update the translation files.
3. Translate the `.ts` files using Qt Linguist or an automatic translation tool.
4. Set the language choice in `formgps_ui.cpp`.
5. Run `cmake --build . --target release_translations` to generate the final translation files.

By following these steps, you ensure that all new text elements are properly translated and included in the application's language files.
