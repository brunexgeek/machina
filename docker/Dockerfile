FROM amd64/ubuntu:20.04

RUN apt update && apt -y install tar curl xz-utils make cmake && apt clean

RUN cd /opt && \
    curl -L "https://developer.arm.com/-/media/Files/downloads/gnu-a/10.2-2020.11/binrel/gcc-arm-10.2-2020.11-x86_64-aarch64-none-elf.tar.xz?revision=79f65c42-1a1b-43f2-acb7-a795c8427085&la=en&hash=3BEE623628664E6E1736ABBE7CE5AD76B65B51EA" | tar -Jxf -

#ENTRYPOINT ["/bin/bash"]