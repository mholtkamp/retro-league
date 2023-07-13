# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.22.1)

# Declares and names the project.

project("rocket")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

include(AndroidNdkModules)
android_ndk_import_module_native_app_glue()

add_definitions(-DPLATFORM_ANDROID=1 -DAPI_VULKAN=1 -DVK_USE_PLATFORM_ANDROID_KHR=1)

file(GLOB SrcLua "../../../../../../Octave/External/Lua/*.c")
file(GLOB SrcVorbis "../../../../../../Octave/External/Vorbis/*.c")
file(GLOB SrcBullet
        "../../../../../../Octave/External/Bullet/BulletCollision/BroadphaseCollision/*.cpp"
        "../../../../../../Octave/External/Bullet/BulletCollision/CollisionDispatch/*.cpp"
        "../../../../../../Octave/External/Bullet/BulletCollision/CollisionShapes/*.cpp"
        "../../../../../../Octave/External/Bullet/BulletCollision/Gimpact/*.cpp"
        "../../../../../../Octave/External/Bullet/BulletCollision/NarrowPhaseCollision/*.cpp"
        "../../../../../../Octave/External/Bullet/BulletDynamics/Character/*.cpp"
        "../../../../../../Octave/External/Bullet/BulletDynamics/ConstraintSolver/*.cpp"
        "../../../../../../Octave/External/Bullet/BulletDynamics/Dynamics/*.cpp"
        "../../../../../../Octave/External/Bullet/BulletDynamics/Featherstone/*.cpp"
        "../../../../../../Octave/External/Bullet/BulletDynamics/MLCPSolvers/*.cpp"
        "../../../../../../Octave/External/Bullet/BulletDynamics/Vehicle/*.cpp"
        "../../../../../../Octave/External/Bullet/LinearMath/*.cpp")
file(GLOB_RECURSE SrcEngine "../../../../../../Octave/Engine/Source/*.cpp")
file(GLOB_RECURSE SrcRocket "../../../../../../Rocket/Source/*.cpp")

include_directories("../../../../../../Octave/External/Lua/")
include_directories("../../../../../../Octave/External/Vorbis/")
include_directories("../../../../../../Octave/External/Bullet/")
include_directories("../../../../../../Octave/Engine/Include/"
        "../../../../../../Octave/Engine/Include/Engine/"
        "../../../../../../Octave/External/"
        "../../../../../../Octave/External/Bullet/")
include_directories("../../../../../../Rocket/Include")

add_library( # Sets the name of the library.
        rocket

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${SrcLua} ${SrcVorbis} ${SrcBullet} ${SrcEngine} ${SrcRocket})

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

find_library( # Sets the name of the path variable.
        vulkan-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        vulkan)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        rocket
        android
        OpenSLES
        native_app_glue
        ${vulkan-lib}
        ${log-lib})