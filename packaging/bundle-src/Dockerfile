FROM ubuntu:22.04 AS base

# it is an arg so it doesn't persist
ARG DEBIAN_FRONTEND=noninteractive

COPY . /root/diagram-server
WORKDIR /root/diagram-server

RUN apt -y update && apt -y install git

FROM base as deploy-src

RUN mkdir -p /tmp/deploy

RUN git submodule init \
    && git submodule update --recursive \
    && rm -rf .git \
    && tar -czf codevis-src-`date -u +%Y-%m-%d`.tar.gz -C /tmp/deploy --exclude .git/ .
