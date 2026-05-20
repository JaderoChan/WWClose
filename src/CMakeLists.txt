if(NOT WIN32)
    message(FATAL_ERROR "Unsupported platform.")
endif()

#############
# 收集源文件 #
#############

# 配置 config 头文件。
configure_file(
    ${CMAKE_TEMPLATES_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/config.h
    @ONLY
)

# 头文件（可用于在 Visual Studio IDE 的项目文件树中显示头文件）
file(
    GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
# 源文件
file(
    GLOB_RECURSE SRCS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
set(PROJECT_SOURCES ${HEADERS} ${SRCS})

###########
# 构建程序 #
###########

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

###########
# 配置依赖 #
###########

# 配置头文件路径。
target_include_directories(
    ${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

# 链接依赖库。
target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    easy_translate::easy_translate
    keyboard_tools::keyboard_tools
    shell32
    advapi32
    comctl32
)

###############
# 配置生成目标 #
###############

# 如果为 MSVC 编译器，显式指定编码，防止乱码情况。
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# 指定为 GUI 应用程序。
option(KEEP_TERMINAL "Keep terminal window" OFF)
set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE $<NOT:$<BOOL:${KEEP_TERMINAL}>>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<BOOL:${KEEP_TERMINAL}>:KEEP_TERMINAL>)

# 程序退出后自动更新语言文件的翻译ID列表。（开发时使用，用于提取翻译ID）
option(UPDATE_TRANSLATIONS_FILES "Update the translations files. Only for dev dump Text ID list." OFF)
target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
    $<$<BOOL:${UPDATE_TRANSLATIONS_FILES}>:EASY_TRANSLATE_UPDATE_TRANSLATIONS_FILES>
)

# 配置应用资源/属性。
set(APP_ICON ${APP_ICON_DIR}/app.ico)

set(APP_MANIFEST ${CMAKE_CURRENT_BINARY_DIR}/app.manifest)
configure_file(
    ${CMAKE_TEMPLATES_DIR}/app.manifest.in
    ${APP_MANIFEST}
    @ONLY
)

# 是否将 app.manifest 文件加入到资源文件中。
option(EMBED_APP_MANIFEST "Add the app.manifest file to the rc file." ON)
if(EMBED_APP_MANIFEST)
    # MSVC 默认会自动嵌入一个 manifest，禁用以避免与资源文件中的 RT_MANIFEST 冲突。
    target_link_options(${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MANIFEST:NO>)
endif()

set(RC_FILE ${CMAKE_CURRENT_BINARY_DIR}/app.rc)
configure_file(
    ${CMAKE_TEMPLATES_DIR}/app.rc.in
    ${RC_FILE}
    @ONLY
)
target_sources(${PROJECT_NAME} PRIVATE ${RC_FILE})

#################
# 生成后额外命令 #
#################

# 复制语言文件至输出目录。
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
    ${LANGUAGES_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>/languages
)
