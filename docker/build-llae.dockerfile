FROM debian:latest
RUN  apt-get update \
 && apt-get install -y --no-install-recommends build-essential \
 	  git make openssh-client zip uuid-dev
RUN apt-get install --reinstall -y ca-certificates
RUN git clone https://github.com/premake/premake-core.git --branch v5.0.0-beta7 --depth 1
RUN make -C premake-core -f Bootstrap.mak linux
RUN cp premake-core/bin/release/premake5 /bin/premake5
RUN rm -rf premake-core
