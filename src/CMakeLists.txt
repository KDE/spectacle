# common - configure file and version definitions

set(SPECTACLE_QML_URI "org.kde.spectacle.private")
set(SPECTACLE_QML_PATH "qrc:/qt/qml/org/kde/spectacle/private")
configure_file(Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Config.h)

# target

add_library(SpectacleCore)

target_sources(SpectacleCore PRIVATE
    SpectacleCore.cpp
    ExportManager.cpp
    CaptureModeModel.cpp
    CommandLineOptions.cpp
    RecordingModeModel.cpp
    Geometry.cpp
    PlasmaVersion.cpp
    ScreenShotEffect.cpp
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
)

kconfig_add_kcfg_files(SpectacleCore GENERATE_MOC Gui/SettingsDialog/settings.kcfgc)

ecm_qt_declare_logging_category(SpectacleCore
    HEADER spectacle_debug.h
    IDENTIFIER SPECTACLE_LOG
    CATEGORY_NAME spectacle
    DESCRIPTION "spectacle (general)"
    EXPORT SPECTACLE
)

ecm_generate_export_header(SpectacleCore
    BASE_NAME SpectacleCore
    VERSION ${PROJECT_VERSION}
    DEPRECATED_BASE_VERSION 0
    EXCLUDE_DEPRECATED_BEFORE_AND_AT ${EXCLUDE_DEPRECATED_BEFORE_AND_AT}
)

target_link_libraries(SpectacleCore PUBLIC
    Qt6::DBus
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickTemplates2

    Qt6::Widgets
    Qt6::PrintSupport

    KF6::CoreAddons
    KF6::GuiAddons
    KF6::DBusAddons
    KF6::WidgetsAddons
    KF6::I18n
    KF6::WindowSystem
    KF6::ConfigGui
    KF6::GlobalAccel
    KF6::KirigamiPlatform
    KF6::StatusNotifierItem
    KF6::Notifications

    KF6::KIOWidgets
    # KF6::PrisonScanner
    KF6::XmlGui

    LayerShellQt::Interface

        # Qt::Concurrent
    # Qt::DBus
    # Qt::Qml
    # Qt::Quick
    # Qt::QuickControls2
    # Qt::QuickTemplates2
    # Qt::GuiPrivate
    # Qt::WaylandClient
    # Qt6::QWebpPlugin
    # KF6::XmlGui
    # KF6::Crash
)

add_subdirectory(Platforms)

if(PURPOSE_FOUND)
    target_link_libraries(SpectacleCore PRIVATE KF6::PurposeWidgets)
endif()

add_executable(spectacle
    ${SPECTACLE_SRCS}
    Main.cpp

    # Platforms/PlatformLoader.cpp
    # Platforms/ImagePlatform.cpp
    # Platforms/PlatformNull.cpp
    # Platforms/VideoPlatform.cpp
    # Platforms/ImagePlatformKWin.cpp
    # Platforms/VideoPlatformWayland.cpp
    # Platforms/screencasting.cpp
)

# if(XCB_FOUND)
#     target_sources(spectacle PRIVATE Platforms/ImagePlatformXcb.cpp Platforms/ImagePlatformXcb.h)
#
#     target_link_libraries(spectacle PRIVATE
#         XCB::XFIXES
#         XCB::IMAGE
#         XCB::CURSOR
#         XCB::UTIL
#         XCB::RANDR
#     )
#     target_link_libraries(spectacle PRIVATE Qt6::GuiPrivate) # Gui/private/qtx11extras_p.h
# endif()

# if (Qt6_VERSION VERSION_GREATER_EQUAL "6.8.0")
#     set(private_code_option  "PRIVATE_CODE")
# endif()
# qt6_generate_wayland_protocol_client_sources(spectacle FILES
#     ${PLASMA_WAYLAND_PROTOCOLS_DIR}/zkde-screencast-unstable-v1.xml
#     ${private_code_option}
# )

#[[
# if(PURPOSE_FOUND)
#     target_link_libraries(spectacle PRIVATE KF6::PurposeWidgets)
# endif()

ki18n_wrap_ui(spectacle
    Gui/SettingsDialog/GeneralOptions.ui
    Gui/SettingsDialog/ImageSaveOptions.ui
    Gui/SettingsDialog/VideoSaveOptions.ui
)]]

# Needed to compile with OpenCV
target_compile_options (spectacle PRIVATE -fexceptions)

# target_include_directories(spectacle PUBLIC ${OpenCV_INCLUDE_DIRS})

target_link_libraries(spectacle PRIVATE
    SpectacleCore
    # Qt::Concurrent
    # Qt::DBus
    # Qt::PrintSupport
    # Qt::Qml
    # Qt::Quick
    # Qt::QuickControls2
    # Qt::QuickTemplates2
    # Qt::GuiPrivate
    # Qt::WaylandClient
    # Qt6::QWebpPlugin
    # KF6::CoreAddons
    # KF6::DBusAddons
    # KF6::WidgetsAddons
    # KF6::Notifications
    # KF6::ConfigCore
    # KF6::I18n
    # KF6::KIOWidgets
    # KF6::WindowSystem
    # KF6::GlobalAccel
    # KF6::XmlGui
    # KF6::GuiAddons
    # KF6::KirigamiPlatform
    # KF6::StatusNotifierItem
    # KF6::PrisonScanner
    KF6::Crash
    # K::KPipeWireRecord
    # Wayland::Client
    # LayerShellQt::Interface
    # ${OpenCV_LIBRARIES}
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

install(TARGETS SpectacleCore ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS spectacle ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
