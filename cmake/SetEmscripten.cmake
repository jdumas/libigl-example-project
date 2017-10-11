find_path(EMSCRIPTEN_DIR
	emcc
	HINTS
		ENV EMSCRIPTEN
	PATHS
		"C:/Program Files/emscripten"
		 /usr/lib/emscripten
)

set(CMAKE_C_COMPILER "${EMSCRIPTEN_DIR}/emcc" CACHE FILEPATH "")
set(CMAKE_CXX_COMPILER "${EMSCRIPTEN_DIR}/em++" CACHE FILEPATH "")
set(CMAKE_AR "${EMSCRIPTEN_DIR}/emar" CACHE FILEPATH "")
set(CMAKE_RANLIB "${EMSCRIPTEN_DIR}/emranlib" CACHE FILEPATH "")
set(CMAKE_LINKER "${EMSCRIPTEN_DIR}/emcc" CACHE FILEPATH "")

include(${EMSCRIPTEN_DIR}/cmake/Modules/Platform/Emscripten.cmake)

# Emscripten optimization flags:
# see:
# https://kripken.github.io/emscripten-site/docs/optimizing/Optimizing-Code.html
# Note: they are added to CMAKE CXX and C flags later on, because the
# way add_flags() works may remove the second "-s" argument.
# Note: TOTAL_MEMORY needs to be a multiple of 16M
set(EM_FLAGS_RELEASE "-m64 -O3 -s USE_GLFW=3 -s TOTAL_MEMORY=268435456")
set(EM_FLAGS_DEBUG "-m64 -O2 -s ASSERTIONS=2 -s SAFE_HEAP=1 -g -s USE_GLFW=3 -s TOTAL_MEMORY=268435456")

set(CMAKE_C_FLAGS ${EM_FLAGS_DEBUG} CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS ${EM_FLAGS_DEBUG} CACHE STRING "" FORCE)
