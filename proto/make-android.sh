
ndk=$HOME/Install/android-ndk-r10e

(cd android/jni ; $ndk/ndk-build)
(cd android; ant debug install)
