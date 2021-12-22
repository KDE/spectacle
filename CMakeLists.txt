
# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "22")
set(RELEASE_SERVICE_VERSION_MINOR "03")
set(RELEASE_SERVICE_VERSION_MICRO "70")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
set(SPECTACLE_VERSION ${RELEASE_SERVICE_VERSION})
# minimum requirements

cmake_minimum_required (VERSION 3.16 FATAL_ERROR)
# Spectacle project
project(Spectacle VERSION ${SPECTACLE_VERSION})

set(QT_MIN_VERSION     "5.15.0")
set(KF5_MIN_VERSION    "5.89.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# set up standard kde build settings

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

# used cmake macros

include(ECMInstallIcons)
include(ECMSetupVersion)
include(FeatureSummary)
include(ECMQtDeclareLoggingCategory)
include(ECMAddTests)
include(ECMQueryQmake)
include(KDEClangFormat)
include(KDEGitCommitHooks)

# find dependencies

find_package(
    Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED
        Core
        Concurrent
        Widgets
        DBus
        PrintSupport
        Svg
        Test
)

find_package(
    KF5 ${KF5_MIN_VERSION} REQUIRED
        CoreAddons
        WidgetsAddons
        DBusAddons
        Notifications
        Config
        I18n
        KIO
        WindowSystem
        NewStuff
        GlobalAccel
        XmlGui
        Wayland
        GuiAddons
)
# optional components
find_package(KF5DocTools ${KF5_MIN_VERSION})
set_package_properties(KF5DocTools PROPERTIES DESCRIPTION
    "Tools to generate documentation"
    TYPE OPTIONAL
)
find_package(KF5Purpose)
if (KF5Purpose_FOUND)
    set(PURPOSE_FOUND 1)
endif()

find_package(XCB COMPONENTS XFIXES IMAGE UTIL CURSOR)
set(XCB_COMPONENTS_ERRORS FALSE)
if (XCB_FOUND)
	find_package(Qt5X11Extras ${QT_MIN_VERSION} REQUIRED)
endif()
set(XCB_COMPONENTS_FOUND TRUE)
if(NOT XCB_XFIXES_FOUND)
	set(XCB_COMPONENTS_ERRORS "${XCB_COMPONENTS_ERRORS} XCB-XFIXES ")
	set(XCB_COMPONENTS_FOUND FALSE)
endif()
if(NOT XCB_IMAGE_FOUND)
	set(XCB_COMPONENTS_ERRORS "${XCB_COMPONENTS_ERRORS} XCB-IMAGE ")
	set(XCB_COMPONENTS_FOUND FALSE)
endif()
if(NOT XCB_UTIL_FOUND)
	set(XCB_COMPONENTS_ERRORS "${XCB_COMPONENTS_ERRORS} XCB-UTIL ")
	set(XCB_COMPONENTS_FOUND FALSE)
endif()
if(NOT XCB_CURSOR_FOUND)
	set(XCB_COMPONENTS_ERRORS "${XCB_COMPONENTS_ERRORS} XCB-CURSOR ")
	set(XCB_COMPONENTS_FOUND FALSE)
endif()

find_package(kImageAnnotator)

if(kImageAnnotator_FOUND)
    find_package(X11 REQUIRED)
    find_package(kColorPicker REQUIRED)
    set(KIMAGEANNOTATOR_FOUND 1)
    if(NOT kImageAnnotator_VERSION VERSION_LESS 0.5.0)
        set(KIMAGEANNOTATOR_CAN_LOAD_TRANSLATIONS 1)
    endif()
    if(NOT kImageAnnotator_VERSION VERSION_LESS 0.6.0)
        set(KIMAGEANNOTATOR_HAS_EXTRA_TOOLS 1)
    endif()
endif()

# add the spectacle.upd file
install(FILES conf/update/spectacle_clipboard.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(FILES conf/update/50-clipboard_settings_change.py DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})

# fail build if none of the platform backends can be found
if (NOT XCB_FOUND OR NOT XCB_COMPONENTS_FOUND)
    message(FATAL_ERROR "No suitable backend platform was found. Currently supported platforms are: XCB Components Required: ${XCB_COMPONENTS_ERRORS}")
endif()

# locate qdbus in the Qt path because not every distro makes a symlink at /usr/bin/qdbus
query_qmake(QtBinariesDir QT_INSTALL_BINS)

# setup handling of deprecated Qt & KF API

add_definitions(
    -DQT_DISABLE_DEPRECATED_BEFORE=0x050c00
    -DQT_DEPRECATED_WARNINGS_SINCE=0x060000
    -DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055800
    -DKF_DEPRECATED_WARNINGS_SINCE=0x060000
)

# hand off to subdirectories

add_subdirectory(src)
add_subdirectory(dbus)
add_subdirectory(desktop)
add_subdirectory(icons)
add_subdirectory(tests)

if (KF5DocTools_FOUND)
    add_subdirectory(doc)
endif()

ecm_qt_install_logging_categories(EXPORT SPECTACLE FILE spectacle.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

# summaries

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
