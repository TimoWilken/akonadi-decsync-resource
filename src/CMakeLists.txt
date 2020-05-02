set(decsyncresource_SRCS
    decsyncresource.cpp
)

ecm_qt_declare_logging_category(decsyncresource_SRCS
    HEADER debug.h
    IDENTIFIER log_decsyncresource
    CATEGORY_NAME log_decsyncresource
)

kconfig_add_kcfg_files(decsyncresource_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfgc
)

kcfg_generate_dbus_interface(
    ${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfg
    org.kde.Akonadi.DecSync.Settings
)

qt5_add_dbus_adaptor(decsyncresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.DecSync.Settings.xml
    ${CMAKE_CURRENT_BINARY_DIR}/settings.h
    Settings
)

add_executable(akonadi_decsync_resource ${decsyncresource_SRCS})
set_target_properties(akonadi_decsync_resource PROPERTIES MACOSX_BUNDLE FALSE)

target_link_libraries(akonadi_decsync_resource
    Qt5::DBus
    Qt5::Network
    KF5::AkonadiAgentBase
    KF5::ConfigCore
    KF5::I18n
)

install(TARGETS akonadi_decsync_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES decsyncresource.desktop
    DESTINATION ${KDE_INSTALL_DATAROOTDIR}/akonadi/agents
)
