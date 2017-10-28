FROM ubuntu:xenial
RUN DEBIAN_FRONTEND=noninteractive apt-get update && \
    apt-get install -y git \
                       curl \
                       build-essential \
                       libssl-dev \
                       gawk \
                       libreadline6-dev \
                       libyaml-dev \
                       libsqlite3-dev\
                       sqlite3 \
                       autoconf \
                       libgmp-dev \
                       libgdbm-dev \
                       libncurses5-dev \
                       automake \
                       libtool \
                       bison \
                       pkg-config \
                       libffi-dev \
                       automake \
                       nasm  \
                       libpng-dev\
                       libwebp-dev \
                       python \
                       python-dev \
                       python-pip \
                       python-virtualenv && \
     rm -rf /var/lib/apt/lists/*

RUN rm /bin/sh && ln -s /bin/bash /bin/sh

ENV NVM_DIR /root/.nvm
ENV NODE_VERSION stable

# Install nvm
RUN git clone https://github.com/creationix/nvm.git $NVM_DIR && \
    cd $NVM_DIR && \
    git checkout `git describe --abbrev=0 --tags`

# Install default version of Node.js
RUN source $NVM_DIR/nvm.sh && \
    nvm install $NODE_VERSION && \
    nvm alias default $NODE_VERSION && \
    nvm use default

# Add nvm.sh to .bashrc for startup...
RUN echo "source ${NVM_DIR}/nvm.sh" > $HOME/.bashrc && \
    source $HOME/.bashrc

ENV NODE_PATH $NVM_DIR/v$NODE_VERSION/lib/node_modules
RUN mkdir /app
COPY . /app
WORKDIR /app/build/linux
# RUN ./build.sh
