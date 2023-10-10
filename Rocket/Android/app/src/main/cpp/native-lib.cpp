#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_solarscapegames_rocket_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Project Rocket!";
    return env->NewStringUTF(hello.c_str());
}