add_executable(Y.Disquette main.cpp composition_root.cpp main_content_widget.cpp settings_dialog.cpp json_config.cpp main_content.ui settings_dialog.ui ../auth/ui/login_widget.cpp ../auth/infrastructure/oauth_callback_scheme_handler.cpp)
target_link_libraries(Y.Disquette PRIVATE
  y_disquette_core
  Qt6::Core
  Qt6::Widgets
  Qt6::Network
  Qt6::WebEngineCore
  Qt6::WebEngineWidgets
)
set_target_properties(Y.Disquette PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_custom_command(TARGET Y.Disquette POST_BUILD COMMAND ${CMAKE_STRIP} $<TARGET_FILE:Y.Disquette>)
endif()

qt_add_translations(Y.Disquette TS_FILES i18n/y_disquette_ru.ts
  QM_FILES_OUTPUT_VARIABLE _qm_files
  QM_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
add_custom_target(l10n ALL DEPENDS Y.Disquette_lrelease)
