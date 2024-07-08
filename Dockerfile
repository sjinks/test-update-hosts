FROM --platform=${BUILDPLATFORM} tonistiigi/xx:latest AS xx
FROM --platform=${BUILDPLATFORM} crazymax/osxcross:latest-alpine AS osxcross

FROM --platform=${BUILDPLATFORM} alpine:3.20.1 AS build
COPY --from=xx / /
COPY --from=osxcross / /
RUN ln -s /osxcross/SDK /xx-sdk

RUN apk add --no-cache clang mingw-w64-gcc i686-mingw-w64-gcc file
RUN TARGETPLATFORM=linux/amd64 xx-apk add --no-cache musl-dev gcc
RUN TARGETPLATFORM=linux/386 xx-apk add --no-cache musl-dev gcc
RUN TARGETPLATFORM=linux/arm64 xx-apk add --no-cache musl-dev gcc

WORKDIR /src
COPY . .

RUN mkdir -p output

RUN TARGETPLATFORM=linux/amd64   xx-clang -Wall -O2 -static -o output/dev-env-update-hosts-linux-amd64 cc/dev-env-update-hosts.c
RUN TARGETPLATFORM=linux/386     xx-clang -Wall -O2 -static -o output/dev-env-update-hosts-linux-386 cc/dev-env-update-hosts.c
RUN TARGETPLATFORM=linux/arm64   xx-clang -Wall -O2 -static -o output/dev-env-update-hosts-linux-arm64 cc/dev-env-update-hosts.c
RUN TARGETPLATFORM=darwin/arm64  xx-clang -Wall -O2 -o output/dev-env-update-hosts-darwin-arm64 cc/dev-env-update-hosts.c
RUN TARGETPLATFORM=darwin/amd64  xx-clang -Wall -O2 -o output/dev-env-update-hosts-darwin-amd64 cc/dev-env-update-hosts.c

RUN \
    export TARGETPLATFORM=windows/amd64; \
    x86_64-w64-mingw32-windres --input cc/dev-env-update-hosts.rc --output dev-env-update-hosts.res --output-format=coff; \
    xx-clang -Wall -O2 -static -o output/dev-env-update-hosts-windows-amd64.exe cc/dev-env-update-hosts.c dev-env-update-hosts.res

RUN \
    export TARGETPLATFORM=windows/386; \
    i686-w64-mingw32-windres --input cc/dev-env-update-hosts.rc --output dev-env-update-hosts.res --output-format=coff; \
    xx-clang -Wall -O2 -static -o output/dev-env-update-hosts-windows-386.exe cc/dev-env-update-hosts.c dev-env-update-hosts.res

FROM scratch

COPY --from=build /src/output/* /
