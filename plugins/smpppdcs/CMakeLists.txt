
add_subdirectory( icons ) 
add_subdirectory( unittest ) 

include_directories( 
${KOPETE_INCLUDES}
${CMAKE_CURRENT_SOURCE_DIR}/libsmpppdclient
)

add_definitions(-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS -DKDE3_SUPPORT)
add_definitions(-DNOKINTERNETDCOP)

########### next target ###############

set(smpppdclient_PART_SRCS
   libsmpppdclient/smpppdclient.cpp
   libsmpppdclient/smpppdready.cpp
   libsmpppdclient/smpppdstate.cpp
   libsmpppdclient/smpppdunsettled.cpp )

set(kopete_smpppdcs_PART_SRCS
   ${smpppdclient_PART_SRCS} 
   smpppdcsplugin.cpp 
   onlineinquiry.cpp 
   detectordcop.cpp 
   detectorsmpppd.cpp 
   detectornetstat.cpp 
#   detectornetworkstatus.cpp 
   smpppdcsiface.h
   kinternetiface.h )

qt4_add_dbus_adaptor(kopete_smpppdcs_SRCS org.kde.kopete.plugin.smpppdcs.xml smpppdcsplugin.h SMPPPDCSPlugin)

kde4_add_kcfg_files(kopete_smpppdcs_PART_SRCS smpppdcsconfig.kcfgc )

kde4_add_plugin(kopete_smpppdcs ${kopete_smpppdcs_PART_SRCS})


target_link_libraries(kopete_smpppdcs  ${KDE4_KDECORE_LIBS} ${QT_QT3SUPPORT_LIBRARY} ${KDE4_KDE3SUPPORT_LIBS} kopete crypto )

install(TARGETS kopete_smpppdcs  DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(kcm_kopete_smpppdcs_PART_SRCS
   ${smpppdclient_PART_SRCS}
   smpppdcspreferences.cpp 
   smpppdsearcher.cpp 
   smpppdcsprefsimpl.cpp 
   smpppdlocationwidget.cpp )


kde4_add_ui_files(kcm_kopete_smpppdcs_PART_SRCS smpppdcsprefsbase.ui smpppdlocationbase.ui )

kde4_add_kcfg_files(kcm_kopete_smpppdcs_PART_SRCS smpppdcsconfig.kcfgc )

kde4_add_plugin(kcm_kopete_smpppdcs ${kcm_kopete_smpppdcs_PART_SRCS})


target_link_libraries(kcm_kopete_smpppdcs kopete ${KDE4_KUTILS_LIBS} ${OPENSSL_LIBRARIES} ${QT_QT3SUPPORT_LIBRARY} ${KDE4_KDE3SUPPORT_LIBS} crypto )

install(TARGETS kcm_kopete_smpppdcs  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES smpppdcs.kcfg  DESTINATION ${KCFG_INSTALL_DIR})
install( FILES kopete_smpppdcs.desktop  DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES kopete_smpppdcs_config.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/kconfiguredialog)




