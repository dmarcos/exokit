FROM debian:latest

RUN apt-get update -y && \
  apt-get install -y \
    build-essential wget python git \
    unzip default-jdk

RUN groupadd -g 999 appuser && \
    useradd -r -u 999 -g appuser appuser

ENV HOME /home/appuser
ADD . $HOME/app
WORKDIR $HOME/app
RUN echo $HOME

RUN chown -R appuser:appuser $HOME && \
    chmod -R 777 $HOME

USER appuser

RUN \
  wget "https://nodejs.org/dist/v11.6.0/node-v11.6.0-linux-x64.tar.gz" -O node.tar.gz && \
  tar -zxf node.tar.gz && \
  rm node.tar.gz && \
  mv node-v11.6.0-linux-x64 node
RUN \
  wget https://dl.google.com/android/repository/sdk-tools-linux-4333796.zip && \
  unzip sdk-tools-linux-4333796.zip && \
  rm sdk-tools-linux-4333796.zip && \
  mkdir android-sdk && \
  export ANDROID_HOME=$(pwd)/android-sdk && \
  mv tools android-sdk/tools && \
  yes | $ANDROID_HOME/tools/bin/sdkmanager --licenses && \
  $ANDROID_HOME/tools/bin/sdkmanager "platform-tools" "platforms;android-28" && \
  $ANDROID_HOME/tools/bin/sdkmanager "ndk-bundle" && \
  export PATH="$PATH:$(pwd)/node/bin" && \
  scripts/make-toolchain-android.sh && \
  scripts/build-android.sh
RUN \
  mv android/app/build/outputs/apk/debug/app-debug.apk ./exokit.apk
