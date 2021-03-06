#
# Shared code between in-process and out-of-process UI
#
set(gammaray_ui_srcs
  ${CMAKE_SOURCE_DIR}/3rdparty/kde/krecursivefilterproxymodel.cpp
  ${CMAKE_SOURCE_DIR}/3rdparty/kde/kfilterproxysearchline.cpp

  ${CMAKE_SOURCE_DIR}/3rdparty/other/modelutils.cpp

  palettemodel.cpp
  methodinvocationdialog.cpp
  propertywidget.cpp
  deferredresizemodesetter.cpp
  deferredtreeviewconfiguration.cpp
  splashscreen.cpp
  editabletypesmodel.cpp

  propertyeditor/propertycoloreditor.cpp
  propertyeditor/propertydoublepaireditor.cpp
  propertyeditor/propertyeditorfactory.cpp
  propertyeditor/propertyextendededitor.cpp
  propertyeditor/propertyfonteditor.cpp
  propertyeditor/propertyintpaireditor.cpp
  propertyeditor/propertypaletteeditor.cpp
  propertyeditor/palettedialog.cpp
  propertyeditor/propertyeditordelegate.cpp
)

if(NOT Qt5Core_VERSION VERSION_LESS 5.2.0)
  list(APPEND gammaray_ui_srcs
    variantcontainermodel.cpp
  )
endif()

qt4_wrap_ui(gammaray_ui_srcs
  methodinvocationdialog.ui
  propertywidget.ui

  propertyeditor/propertydoublepaireditor.ui
  propertyeditor/propertyextendededitor.ui
  propertyeditor/propertyintpaireditor.ui
  propertyeditor/palettedialog.ui
)

add_library(gammaray_ui SHARED ${gammaray_ui_srcs})
set_target_properties(gammaray_ui PROPERTIES
  DEFINE_SYMBOL MAKE_GAMMARAY_UI_LIB
  SOVERSION ${GAMMARAY_SOVERSION}
  VERSION ${GAMMARAY_SOVERSION}
  OUTPUT_NAME gammaray_ui-${GAMMARAY_PROBE_ABI}
)

target_link_libraries(gammaray_ui
  LINK_PUBLIC ${QT_QTCORE_LIBRARIES} ${QT_QTGUI_LIBRARIES} gammaray_common
  LINK_PRIVATE gammaray_common_internal
)

install(TARGETS gammaray_ui EXPORT GammaRayTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

set(gammaray_ui_internal_srcs
  sidepane.cpp
  aboutpluginsdialog.cpp
  mainwindow.cpp
  aboutdialog.cpp
  proxytooluifactory.cpp
  clienttoolmodel.cpp

  tools/connectioninspector/connectioninspectorwidget.cpp
  tools/localeinspector/localeinspectorwidget.cpp
  tools/messagehandler/messagehandlerwidget.cpp
  tools/messagehandler/messagehandlerclient.cpp
  ../core/tools/messagehandler/messagehandlerinterface.cpp
  tools/metaobjectbrowser/metaobjectbrowserwidget.cpp
  tools/metatypebrowser/metatypebrowserwidget.cpp
  tools/mimetypes/mimetypeswidget.cpp
  tools/modelinspector/modelinspectorwidget.cpp
  tools/modelinspector/modelinspectorclient.cpp
  tools/objectinspector/objectinspectorwidget.cpp
  tools/resourcebrowser/clientresourcemodel.cpp
  tools/resourcebrowser/resourcebrowserwidget.cpp
  tools/resourcebrowser/resourcebrowserclient.cpp
  tools/standardpaths/standardpathswidget.cpp
  tools/textdocumentinspector/textdocumentinspectorwidget.cpp
  tools/textdocumentinspector/textdocumentcontentview.cpp
)

qt4_wrap_ui(gammaray_ui_internal_srcs
  mainwindow.ui
  aboutdialog.ui

  tools/connectioninspector/connectioninspectorwidget.ui
  tools/localeinspector/localeinspectorwidget.ui
  tools/messagehandler/messagehandlerwidget.ui
  tools/metatypebrowser/metatypebrowserwidget.ui
  tools/mimetypes/mimetypeswidget.ui
  tools/modelinspector/modelinspectorwidget.ui
  tools/objectinspector/objectinspectorwidget.ui
  tools/resourcebrowser/resourcebrowserwidget.ui
  tools/standardpaths/standardpathswidget.ui
  tools/textdocumentinspector/textdocumentinspectorwidget.ui
)

if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

add_library(gammaray_ui_internal STATIC ${gammaray_ui_internal_srcs})

target_link_libraries(gammaray_ui_internal
  ${QT_QTCORE_LIBRARIES}
  ${QT_QTGUI_LIBRARIES}
  gammaray_common_internal
  gammaray_ui
)

gammaray_install_headers(
  gammaray_ui_export.h
  deferredresizemodesetter.h
  deferredtreeviewconfiguration.h
  palettemodel.h
  propertywidget.h
  tooluifactory.h
)
