# Locates the sphinx-build executable, including pip user-scheme install
# locations that aren't normally on PATH. Deliberately doesn't rely on
# find_package(Python3) to point at the interpreter `pip install` used --
# on a machine with multiple Pythons installed (Windows Store alias,
# py.org installer, ...) CMake's resolver can pick a different one than
# whichever `pip` put sphinx-build under.

set(_eden_sphinx_hints)

if(WIN32)
    file(GLOB _eden_sphinx_hints
        "$ENV{APPDATA}/Python/Python*/Scripts"
        "$ENV{LOCALAPPDATA}/Programs/Python/Python*/Scripts"
    )
else()
    list(APPEND _eden_sphinx_hints "$ENV{HOME}/.local/bin")
endif()

find_program(SPHINX_EXECUTABLE
    NAMES sphinx-build sphinx-build.exe
    HINTS ${_eden_sphinx_hints}
    DOC "Path to the sphinx-build executable"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sphinx
    "Failed to find sphinx-build. Install with: pip install -r docs/requirements.txt"
    SPHINX_EXECUTABLE
)

mark_as_advanced(SPHINX_EXECUTABLE)
