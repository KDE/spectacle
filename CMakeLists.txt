
# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "23")
set(RELEASE_SERVICE_VERSION_MINOR "03")
set(RELEASE_SERVICE_VERSION_MICRO "70")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
set(SPECTACLE_VERSION ${RELEASE_SERVICE_VERSION})
# minimum requirements

cmake_minimum_required (VERSION 3.16 FATAL_ERROR)
# Spectacle project
project(Spectacle VERSION ${SPECTACLE_VERSION})

set(QT_MIN_VERSION     "5.15.2")
set(KF_MIN_VERSION    "5.104.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
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
include(ECMDeprecationSettings)
include(ECMQMLModules)

# find dependencies

find_package(
    Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED
        Core
        Concurrent
        Qml
        Quick
        QuickControls2
        QuickTemplates2
        Widgets
        DBus
        PrintSupport
        Test
        WaylandClient
)

find_package(
    KF${QT_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED
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
        GuiAddons
        Kirigami2
)

find_package(Wayland REQUIRED COMPONENTS Client)
find_package(PlasmaWaylandProtocols REQUIRED)
if (QT_MAJOR_VERSION EQUAL "5")
    find_package(QtWaylandScanner REQUIRED)
endif()
find_package(KPipeWire)
set_package_properties(KPipeWire PROPERTIES DESCRIPTION
    "Used to record pipewire streams into a file"
    TYPE REQUIRED
)

# optional components
find_package(KF${QT_MAJOR_VERSION}DocTools ${KF_MIN_VERSION})
set_package_properties(KF${QT_MAJOR_VERSION}DocTools PROPERTIES DESCRIPTION
    "Tools to generate documentation"
    TYPE OPTIONAL
)
find_package(KF${QT_MAJOR_VERSION}Purpose)
if (KF${QT_MAJOR_VERSION}Purpose_FOUND)
    set(PURPOSE_FOUND 1)
endif()

find_package(XCB COMPONENTS XFIXES IMAGE UTIL CURSOR)
set(XCB_COMPONENTS_ERRORS FALSE)
if (XCB_FOUND)
    if (QT_MAJOR_VERSION STREQUAL "5")
        find_package(Qt5X11Extras ${QT_MIN_VERSION} REQUIRED)
    endif()
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

# add the spectacle.upd file
install(FILES conf/update/spectacle_clipboard.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(FILES conf/update/50-clipboard_settings_change.py DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})

# fail build if none of the platform backends can be found
if (NOT XCB_FOUND OR NOT XCB_COMPONENTS_FOUND)
    message(FATAL_ERROR "No suitable backend platform was found. Currently supported platforms are: XCB Components Required: ${XCB_COMPONENTS_ERRORS}")
endif()

# locate qdbus in the Qt path because not every distro makes a symlink at /usr/bin/qdbus
if (QT_MAJOR_VERSION EQUAL "5")
    query_qmake(QtBinariesDir QT_INSTALL_BINS)
else()
    MESSAGE(STATUS "need to find QtBinariesDir")
endif()

# setup handling of deprecated Qt & KF API

ecm_set_disabled_deprecation_versions(QT 5.15.2  KF 5.100.0)

# hand off to subdirectories

add_subdirectory(src)
add_subdirectory(dbus)
add_subdirectory(desktop)
add_subdirectory(icons)
add_subdirectory(tests)

if (KF${QT_MAJOR_VERSION}DocTools_FOUND)
    add_subdirectory(doc)
    kdoctools_install(po)
endif()
ki18n_install(po)

ecm_qt_install_logging_categories(EXPORT SPECTACLE FILE spectacle.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

# summaries

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
