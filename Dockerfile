# KernelPatch 跨平台构建环境
# 强制使用 x86_64 架构（NDK工具链仅支持 x86_64）
FROM --platform=linux/amd64 ubuntu:24.04

# 设置环境变量
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# 更新包列表并安装基础工具
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    curl \
    wget \
    git \
    unzip \
    xz-utils \
    file \
    pkg-config \
    ca-certificates \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# 创建工具安装目录
RUN mkdir -p /opt

# 安装 ARM64 交叉编译工具链
RUN cd /opt && \
    wget -q https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-elf.tar.xz && \
    tar -Jxf arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-elf.tar.xz && \
    mv arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-elf arm-toolchain && \
    rm arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-elf.tar.xz

# 安装 Android NDK
RUN cd /opt && \
    wget -q https://dl.google.com/android/repository/android-ndk-r26b-linux.zip && \
    unzip -q android-ndk-r26b-linux.zip && \
    mv android-ndk-r26b android-ndk && \
    rm android-ndk-r26b-linux.zip

# 安装 zlib 开发库（用于 kptools）
RUN apt-get update && apt-get install -y \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# 设置工具链环境变量
ENV PATH="/opt/arm-toolchain/bin:/opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin:${PATH}"

# 创建工作目录
WORKDIR /workspace

# 验证工具链安装
RUN echo "=== 验证 ARM 工具链 ===" && \
    /opt/arm-toolchain/bin/aarch64-none-elf-gcc --version && \
    echo "=== 验证 Android NDK ===" && \
    ls -la /opt/android-ndk/build/cmake/android.toolchain.cmake && \
    echo "=== 验证 CMake ===" && \
    cmake --version

# 设置默认命令
CMD ["/bin/bash"]
