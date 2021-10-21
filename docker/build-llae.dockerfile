FROM debian:latest
RUN  apt-get update \
 && apt-get install -y --no-install-recommends build-essential \
 	  git make openssh-client zip
RUN apt-get install --reinstall -y ca-certificates
RUN git clone https://github.com/premake/premake-core.git
RUN make -C premake-core -f Bootstrap.mak linux
RUN cp premake-core/bin/release/premake5 /bin/premake5
RUN rm -rf premake-core
