FROM debian:latest                                                                                                                                                                                       
RUN  apt-get update \
 && apt-get install -y --no-install-recommends build-essential \
 	  git make openssh-client unzip uuid-dev mingw-w64
RUN apt-get install --reinstall -y ca-certificates
RUN git clone https://github.com/premake/premake-core.git
RUN make -C premake-core -f Bootstrap.mak linux
RUN cp premake-core/bin/release/premake5 /bin/premake5
RUN rm -rf premake-core
RUN git clone https://github.com/andryblack/llae.git
RUN cd llae && premake5 download
RUN cd llae && premake5 unpack
RUN cd llae && premake5 gmake2
RUN make -C llae/build verbose=1 config=release 
RUN cd llae && LUA_PATH="tools/?.lua;scripts/?.lua" ./bin/llae-bootstrap bootstrap
RUN ln -s $HOME/.llae/bin/llae /bin/llae
RUN rm -rf llae
RUN llae --help
