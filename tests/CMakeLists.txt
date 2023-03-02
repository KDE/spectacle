include_directories(${PROJECT_SOURCE_DIR}/src)

SET(FILENAME_TEST_SRCS
    FilenameTest.cpp
    ../src/CaptureModeModel.cpp
    ../src/ShortcutActions.cpp
    ../src/ExportManager.cpp
    ../src/Platforms/Platform.cpp
)


kconfig_add_kcfg_files(FILENAME_TEST_SRCS GENERATE_MOC ${PROJECT_SOURCE_DIR}/src/Gui/SettingsDialog/settings.kcfgc)

ecm_add_test(
    ${FILENAME_TEST_SRCS}
    TEST_NAME "filename_test"
    LINK_LIBRARIES  Qt::Test
        Qt::PrintSupport KF${QT_MAJOR_VERSION}::I18n KF${QT_MAJOR_VERSION}::ConfigCore KF${QT_MAJOR_VERSION}::GlobalAccel KF${QT_MAJOR_VERSION}::KIOCore KF${QT_MAJOR_VERSION}::WindowSystem KF${QT_MAJOR_VERSION}::XmlGui KF${QT_MAJOR_VERSION}::GuiAddons
)
