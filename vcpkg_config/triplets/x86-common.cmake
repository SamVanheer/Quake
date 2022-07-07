# Included by platform-specific triplets to configure third party dependencies.
# Do not use as a triplet directly.

# Default to static linkage for all dependencies.
set(VCPKG_LIBRARY_LINKAGE static)

# Override linkage to dynamic only for these.
if(PORT MATCHES "(sdl2|openal-soft)")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()

set(VCPKG_C_FLAGS "")
set(VCPKG_CXX_FLAGS "-std=c++20")

# We don't need debug builds.
# TODO: this breaks due to different runtime libraries being used.
#set(VCPKG_BUILD_TYPE "release")
