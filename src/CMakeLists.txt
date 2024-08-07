# common - configure file and version definitions

set(SPECTACLE_QML_URI "org.kde.spectacle.private")
set(SPECTACLE_QML_PATH "qrc:/qt/qml/org/kde/spectacle/private")
configure_file(Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Config.h)

# target

kconfig_add_kcfg_files(SPECTACLE_SRCS GENERATE_MOC Gui/SettingsDialog/settings.kcfgc)

ecm_qt_declare_logging_category(SPECTACLE_SRCS
    HEADER spectacle_debug.h
    IDENTIFIER SPECTACLE_LOG
    CATEGORY_NAME spectacle
    DESCRIPTION "spectacle (general)"
    EXPORT SPECTACLE
)

add_executable(spectacle
    ${SPECTACLE_SRCS}
    Main.cpp
    CaptureModeModel.cpp
    CommandLineOptions.cpp
    RecordingModeModel.cpp
    ExportManager.cpp
    Geometry.cpp
    PlasmaVersion.cpp
    ScreenShotEffect.cpp
    SpectacleCore.cpp
    SpectacleDBusAdapter.cpp
    ShortcutActions.cpp
    VideoFormatModel.cpp
    Gui/CaptureWindow.cpp
    Gui/ExportMenu.cpp
    Gui/HelpMenu.cpp
    Gui/OptionsMenu.cpp
    Gui/SmartSpinBox.cpp
    Gui/Selection.cpp
    Gui/SelectionEditor.cpp
    Gui/SpectacleWindow.cpp
    Gui/SpectacleMenu.cpp
    Gui/ViewerWindow.cpp
    Gui/Annotations/AnnotationDocument.cpp
    Gui/Annotations/AnnotationTool.cpp
    Gui/Annotations/AnnotationViewport.cpp
    Gui/Annotations/EffectUtils.cpp
    Gui/Annotations/History.cpp
    Gui/Annotations/QmlPainterPath.cpp
    Gui/Annotations/Traits.cpp
    Gui/SettingsDialog/ImageSaveOptionsPage.cpp
    Gui/SettingsDialog/VideoFormatComboBox.cpp
    Gui/SettingsDialog/VideoSaveOptionsPage.cpp
    Gui/SettingsDialog/SettingsDialog.cpp
    Gui/SettingsDialog/GeneralOptionsPage.cpp
    Gui/SettingsDialog/ShortcutsOptionsPage.cpp
    Platforms/PlatformLoader.cpp
    Platforms/ImagePlatform.cpp
    Platforms/PlatformNull.cpp
    Platforms/ImagePlatformKWin.cpp
    Platforms/VideoPlatform.cpp
    Platforms/VideoPlatformWayland.cpp
    Platforms/screencasting.cpp
)

if(XCB_FOUND)
    target_sources(spectacle PRIVATE Platforms/ImagePlatformXcb.cpp Platforms/ImagePlatformXcb.h)

    target_link_libraries(spectacle PRIVATE
        XCB::XFIXES
        XCB::IMAGE
        XCB::CURSOR
        XCB::UTIL
        XCB::RANDR
    )
    target_link_libraries(spectacle PRIVATE Qt6::GuiPrivate) # Gui/private/qtx11extras_p.h
endif()

qt6_generate_wayland_protocol_client_sources(spectacle FILES
    ${PLASMA_WAYLAND_PROTOCOLS_DIR}/zkde-screencast-unstable-v1.xml)


if(PURPOSE_FOUND)
    target_link_libraries(spectacle PRIVATE KF6::PurposeWidgets)
endif()

ki18n_wrap_ui(spectacle
    Gui/SettingsDialog/GeneralOptions.ui
    Gui/SettingsDialog/ImageSaveOptions.ui
    Gui/SettingsDialog/VideoSaveOptions.ui
)

# Needed to compile with OpenCV
target_compile_options (spectacle PRIVATE -fexceptions)

target_include_directories(spectacle PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${OpenCV_INCLUDE_DIRS})

target_link_libraries(spectacle PRIVATE
    Qt::Concurrent
    Qt::DBus
    Qt::PrintSupport
    Qt::Qml
    Qt::Quick
    Qt::QuickControls2
    Qt::QuickTemplates2
    Qt::GuiPrivate
    Qt::WaylandClient
    KF6::CoreAddons
    KF6::DBusAddons
    KF6::WidgetsAddons
    KF6::Notifications
    KF6::ConfigCore
    KF6::I18n
    KF6::KIOWidgets
    KF6::WindowSystem
    KF6::GlobalAccel
    KF6::XmlGui
    KF6::GuiAddons
    KF6::KirigamiPlatform
    KF6::StatusNotifierItem
    KF6::PrisonScanner
    KF6::Crash
    K::KPipeWireRecord
    Wayland::Client
    LayerShellQt::Interface
    ${OpenCV_LIBRARIES}
)

set_source_files_properties(Gui/QmlUtils.qml PROPERTIES
  QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(spectacle
    URI ${SPECTACLE_QML_URI}
    QML_FILES
    Gui/AnimatedLoader.qml
    Gui/AnnotationOptionsToolBarContents.qml
    Gui/Annotations/AnnotationEditor.qml
    Gui/Annotations/CropTool.qml
    Gui/Annotations/HoverOutline.qml
    Gui/Annotations/SelectionTool.qml
    Gui/Annotations/TextTool.qml
    Gui/AnnotationsToolBarContents.qml
    Gui/ButtonGrid.qml
    Gui/CaptureModeButtonsColumn.qml
    Gui/CaptureOptions.qml
    Gui/CaptureSettingsColumn.qml
    Gui/CopiedMessage.qml
    Gui/DashedOutline.qml
    Gui/DelaySpinBox.qml
    Gui/DialogPage.qml
    Gui/EmptyPage.qml
    Gui/FloatingBackground.qml
    Gui/FloatingToolBar.qml
    Gui/Handle.qml
    Gui/ImageCaptureOverlay.qml
    Gui/ImageView.qml
    Gui/InlineMessage.qml
    Gui/LocationCopiedMessage.qml
    Gui/Magnifier.qml
    Gui/MainToolBarContents.qml
    Gui/Outline.qml
    Gui/QRCodeScannedMessage.qml
    Gui/QmlUtils.qml
    Gui/RecordOptions.qml
    Gui/RecordingFailedMessage.qml
    Gui/RecordingModeButtonsColumn.qml
    Gui/RecordingSettingsColumn.qml
    Gui/RecordingView.qml
    Gui/SavedAndCopiedMessage.qml
    Gui/SavedAndLocationCopied.qml
    Gui/SavedMessage.qml
    Gui/ScreenshotFailedMessage.qml
    Gui/ScreenshotView.qml
    Gui/ShareErrorMessage.qml
    Gui/SharedMessage.qml
    Gui/ShortcutsTextBox.qml
    Gui/SizeLabel.qml
    Gui/UndoRedoGroup.qml
    Gui/VideoCaptureOverlay.qml
)

install(TARGETS spectacle ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
