To do

 * Create the "Top Field View"/"Grid Rotate" window
 * Finish out the "Steer Settings" Pages.
   * Anyone working on this, please keep your branch up-do-date with David's "dev" branch, as he will be working on this in there
   * FormSteerWiz.cs see btnStartSA_Click, btnStartSA_Left_Click, Timer1_Tick
   * Everything can be done in javascript and qml, accessing settings, and some other properties in aog, vehicleInterface, etc. Anything else we need will have to be brought in.
 * Finish the reset logic in 'reset_all' in SteerConfigSettings.qml
 * Trams
 * Create lines from boundary dialog
   * Will be similar to HeadAche and HeadLine as far as how the rendering will work.  Does not use OpenGL. Uses QML canvaas and arrays of lines, etc.
 * Fix TimedMessage.qml to be a proper width and height to actually show a message.
 * Implement device-independent scaling functions for screen units in QML
   * similar to what Android native apps use.
   * dp() - device independent pixels where 160 pixels is 1 real inch
   * sp() - similar to dp but for text.
   * https://stackoverflow.com/questions/2025282/what-is-the-difference-between-px-dip-dp-and-sp
 * Consider using QML_SINGLETON to embed things like CTrack and CVehicle in the QML space.  Would have to pull pointers back to FormGPS, though.  For now I'm going to stick with setContextProperty.
   * concerned a bit with threading.  Reading properties is probably thread safe, but setting properties might not be.
 * Fix world grid lines. 
 * Add the necessary code to turn blockage on/off.
 * Show AB and AC on main screen, implement timer like in AOG
 * Add NTRIP to bluetooth
 * Serial. Add NTRIP to bluetooth

Done
 * ~~Set the categories to consistent in the ini file.~~
 * ~~Add "Auto reconnect" to UDP code in QtAgIO.~~
 * ~~ABLine and Contour line UI stuff out of the GL drawing function.~~ (pretty sure that was done a long time ago)
 * ~~Move section lookahead stuff out of GL drawing functinone somehow.
   Maybe after rendering frame, save a pixmap of the entire widget.~~ (done a long time ago)
 * ~~IsAutoSteerBtnOn~~ (done)
