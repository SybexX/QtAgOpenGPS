To do

1. ~~ABLine and Contour line UI stuff out of the GL drawing function.~~ (pretty sure that was done a long time ago)
2. ~~Move section lookahead stuff out of GL drawing functinone somehow.
   Maybe after rendering frame, save a pixmap of the entire widget.~~ (done a long time ago)
3. Create the "Top Field View"/"Grid Rotate" window
4. Finish out the "Steer Settings" Pages.
 * Anyone working on this, please keep your branch up-do-date with David's "dev" branch, as he will be working on this in there
 * FormSteerWiz.cs see btnStartSA_Click, btnStartSA_Left_Click, Timer1_Tick
   * Everything can be done in javascript and qml, accessing settings, and some other properties in aog, vehicleInterface, etc. Anything else we need will have to be brought in.
5. Add "Auto reconnect" to UDP code in QtAgIO.
7. Finish the reset logic in 'reset_all' in SteerConfigSettings.qml
8. ~~IsAutoSteerBtnOn~~ (done)
9. Trams
10. Create lines from boundary dialog
 * Will be similar to HeadAche and HeadLine as far as how the rendering will work.  Does not use OpenGL. Uses QML canvaas and arrays of lines, etc.
11. Fix TimedMessage.qml to be a proper width and height to actually show a message.
12. Implement device-independent scaling functions for screen units in QML
 * similar to what Android native apps use.
 * dp() - device independent pixels where 160 pixels is 1 real inch
 * sp() - similar to dp but for text.
 * https://stackoverflow.com/questions/2025282/what-is-the-difference-between-px-dip-dp-and-sp
 13. Set the categories to consistent in the ini file.
 14. Add the necessary code to turn blockage on/off, and merge into dev.
 15. Show AB and AC on main screen, implement timer like in AOG
16. Add NTRIP to bluetooth
17. Serial. Add NTRIP to bluetooth

