cmake_minimum_required(VERSION 3.16)
project(GeminiQuizCppQt VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Network)

option(ENABLE_MUPDF "Habilita extração real de PDF via MuPDF" OFF)
set(MUPDF_INCLUDE_DIR "" CACHE PATH "Diretório include do MuPDF")
set(MUPDF_LIBRARY "" CACHE FILEPATH "Biblioteca estática/compartilhada do MuPDF")

add_library(quiz_backend STATIC
    src/backend/MuPdfExtractor.h
    src/backend/MuPdfExtractor.cpp
    src/backend/GeminiQuizGenerator.h
    src/backend/GeminiQuizGenerator.cpp
    src/backend/QuizEngine.h
    src/backend/QuizEngine.cpp
)
target_include_directories(quiz_backend PUBLIC src/backend)
target_link_libraries(quiz_backend PUBLIC Qt6::Core Qt6::Network)

if(ENABLE_MUPDF AND MUPDF_INCLUDE_DIR AND MUPDF_LIBRARY)
    target_compile_definitions(quiz_backend PUBLIC HAVE_MUPDF=1)
    target_include_directories(quiz_backend PUBLIC "${MUPDF_INCLUDE_DIR}")
    target_link_libraries(quiz_backend PUBLIC "${MUPDF_LIBRARY}")
endif()

if(NOT ANDROID)
    add_executable(quiz_terminal
        src/terminal/main.cpp
    )
    target_link_libraries(quiz_terminal PRIVATE quiz_backend)
endif()

qt_add_executable(quiz_frontend
    src/frontend/main_frontend.cpp
)

qt_add_qml_module(quiz_frontend
    URI QuizApp
    VERSION 1.0
    QML_FILES src/frontend/Main.qml
)

target_link_libraries(quiz_frontend PRIVATE
    quiz_backend
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
)

if(ANDROID)
    set_target_properties(quiz_frontend PROPERTIES
        QT_ANDROID_PACKAGE_NAME "com.exemplo.geminiquiz"
        QT_ANDROID_VERSION_CODE 2
        QT_ANDROID_VERSION_NAME "0.2"
    )
endif()
