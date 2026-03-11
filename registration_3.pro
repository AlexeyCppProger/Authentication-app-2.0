QT += core gui sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++11
SOURCES += \
    AuthService.cpp \
    DBConnection.cpp \
    Initialization_db.cpp \
    PasswordHasher.cpp \
    UserRepository.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    AuthService.h \
    DBConnection.h \
    Initialization_db.h \
    PasswordHasher.h \
    UserRepository.h \
    mainwindow.h

FORMS += \
    mainwindow.ui
# путь к установленному vcpkg (измените, если у вас другой)
VCPKG_ROOT = C:/vcpkg/installed/x86-mingw-dynamic

win32-g++ {
    # Заголовки
    INCLUDEPATH += $$VCPKG_ROOT/include
    DEPENDPATH += $$VCPKG_ROOT/include

    # Линковка: явная ссылка на импорт-архив (надежнее для MinGW)
    LIBS += $$quote($$VCPKG_ROOT/lib/libsodium.dll.a)
}

# Диагностяхические сообщения при qmake
exists($$VCPKG_ROOT/include/sodium.h) {
    message("vcpkg: found sodium.h in $$VCPKG_ROOT/include")
} else {
    message("Warning: sodium.h NOT found in $$VCPKG_ROOT/include")
}

exists($$VCPKG_ROOT/lib/libsodium.dll.a) {
    message("vcpkg: found libsodium import archive $$VCPKG_ROOT/lib/libsodium.dll.a")
} else {
    message("Warning: libsodium import archive NOT found in $$VCPKG_ROOT/lib")
}

exists($$VCPKG_ROOT/bin/libsodium-26.dll) {
    message("vcpkg: found runtime DLL libsodium-26.dll")
} else {
    message("Note: runtime DLL libsodium-26.dll not found; wildcard copy will try to match libsodium*.dll")
}

DISTFILES +=
