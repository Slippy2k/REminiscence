
ndk=~/Install/android-ndk-r10e

(cd android/jni ; $ndk/ndk-build TARGET_PLATFORM=10)
(cd android; ant debug)
