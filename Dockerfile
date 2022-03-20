FROM alpine:3.14 as builder
ARG COMMIT=feature/cilium-compatibility-patch

RUN apk add alpine-sdk build-base autoconf readline-dev bison flex ncurses-dev\
             libssh linux-headers

RUN git clone --depth 1 --branch ${COMMIT} https://github.com/tailify/bird
WORKDIR bird

RUN autoreconf --install

RUN CFLAGS="-fcommon" ./configure --prefix=/usr --sysconfdir=/etc --runstatedir=/run

RUN make -j && \
    make install

FROM alpine:3.14

LABEL "org.opencontainers.image.source"="https://github.com/tailify/bird"

ENV TZ=UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apk add --no-cache readline ncurses libssh bash

COPY --from=builder /usr/sbin/bird* /usr/sbin/

ENTRYPOINT /usr/sbin/bird -f

