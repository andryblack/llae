FROM build-llae:latest
RUN  apt-get update \
  && apt-get install -y --no-install-recommends mingw-w64
