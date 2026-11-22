
cmake_minimum_required(VERSION 3.16)

project(GeminiQuizCppQt VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    Network
)

option(ENABLE_MUPDF "Habilita extração real de PDF via MuPDF" OFF)

set(MUPDF_INCLUDE_DIR "" CACHE PATH
    "Diretório include do MuPDF")

set(MUPDF_LIBRARY "" CACHE FILEPATH
    "Biblioteca MuPDF compilada para a plataforma de destino")

# --------------------------------------------------
# Backend
# --------------------------------------------------

add_library(quiz_backend STATIC
    src/backend/MuPdfExtractor.h
    src/backend/MuPdfExtractor.cpp
    src/backend/GeminiQuizGenerator.h
    src/backend/GeminiQuizGenerator.cpp
    src/backend/QuizEngine.h
    src/backend/QuizEngine.cpp
)

set_target_properties(quiz_backend PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(quiz_backend PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/backend"
)

target_link_libraries(quiz_backend PUBLIC
    Qt6::Core
    Qt6::Network
)

# --------------------------------------------------
# MuPDF
# --------------------------------------------------

if(ENABLE_MUPDF)

    message(STATUS "Configurando suporte ao MuPDF")

    # Verifica o diretório de cabeçalhos

    if(NOT MUPDF_INCLUDE_DIR)
        message(FATAL_ERROR
            "ENABLE_MUPDF está ativo, mas MUPDF_INCLUDE_DIR não foi informado."
        )
    endif()

    if(NOT EXISTS "${MUPDF_INCLUDE_DIR}/mupdf/fitz.h")
        message(FATAL_ERROR
            "Cabeçalho MuPDF não encontrado em: ${MUPDF_INCLUDE_DIR}"
        )
    endif()

    # Verifica a biblioteca compilada

    if(NOT MUPDF_LIBRARY OR NOT EXISTS "${MUPDF_LIBRARY}")
        message(FATAL_ERROR
            "Biblioteca MuPDF não encontrada: ${MUPDF_LIBRARY}"
        )
    endif()

    # Configurações específicas do Android

    if(ANDROID)

        message(STATUS "Configurando MuPDF para Android")

        if(NOT ANDROID_ABI STREQUAL "arm64-v8a")
            message(FATAL_ERROR
                "ABI Android não suportada: ${ANDROID_ABI}. "
                "Esperado: arm64-v8a."
            )
        endif()

    endif()

    # Biblioteca estática importada

    add_library(MuPDF::MuPDF STATIC IMPORTED GLOBAL)

    set_target_properties(MuPDF::MuPDF PROPERTIES
        IMPORTED_LOCATION "${MUPDF_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MUPDF_INCLUDE_DIR}"
    )

    # Ativa a implementação MuPDF no código C++

    target_compile_definitions(quiz_backend PUBLIC
        HAVE_MUPDF=1
    )

    # Vincula a biblioteca MuPDF

    target_link_libraries(quiz_backend PUBLIC
        MuPDF::MuPDF
    )

    # Dependências nativas do MuPDF no Android

    if(ANDROID)

        target_link_libraries(quiz_backend PUBLIC
            z
            m
            log
        )

    endif()

    message(STATUS "--------------------------------")
    message(STATUS "MuPDF habilitado")
    message(STATUS "Include: ${MUPDF_INCLUDE_DIR}")
    message(STATUS "Library: ${MUPDF_LIBRARY}")
    message(STATUS "--------------------------------")

else()

    # O Android exige a implementação real de extração PDF

    if(ANDROID)
        message(FATAL_ERROR
            "MuPDF está desabilitado para Android. "
            "Configure ENABLE_MUPDF=ON e forneça a biblioteca ARM64."
        )
    endif()

    message(STATUS "MuPDF desabilitado")

endif()

# --------------------------------------------------
# Aplicativo terminal (desktop)
# --------------------------------------------------

if(NOT ANDROID)

    add_executable(quiz_terminal
        src/terminal/main.cpp
    )

    target_link_libraries(quiz_terminal PRIVATE
        quiz_backend
    )

endif()

# --------------------------------------------------
# Interface Qt Quick
# --------------------------------------------------

qt_add_executable(quiz_frontend
    src/frontend/main_frontend.cpp
)

qt_add_qml_module(quiz_frontend
    URI QuizApp
    VERSION 1.0
    QML_FILES
        src/frontend/Main.qml
)

target_link_libraries(quiz_frontend PRIVATE
    quiz_backend
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
)

# --------------------------------------------------
# Configurações Android
# --------------------------------------------------

if(ANDROID)

    set_target_properties(quiz_frontend PROPERTIES
        QT_ANDROID_PACKAGE_NAME "com.exemplo.geminiquiz"
        QT_ANDROID_VERSION_CODE 2
        QT_ANDROID_VERSION_NAME "0.2"
    )

    message(STATUS "================================")
    message(STATUS "Configurando aplicativo Android")
    message(STATUS "Projeto: ${PROJECT_NAME}")
    message(STATUS "Versão: ${PROJECT_VERSION}")
    message(STATUS "ABI: ${ANDROID_ABI}")
    message(STATUS "MuPDF: ${ENABLE_MUPDF}")
    message(STATUS "Biblioteca: ${MUPDF_LIBRARY}")
    message(STATUS "================================")

endif()
