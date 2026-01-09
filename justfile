# KernelPatch 跨平台构建脚本
# 在 macOS 本地执行 just 命令进行跨平台编译

# 设置变量
DOCKER_IMAGE := "kernelpatch-builder"
BUILD_DIR := "build"

# 默认目标：显示所有可用命令
default:
    @just --list

# 设置构建环境
setup:
    @echo "设置构建环境..."
    @echo "构建 Docker 镜像: {{DOCKER_IMAGE}}"
    docker build -t {{DOCKER_IMAGE}} .
    @echo "构建环境设置完成"

# 检查 Docker 镜像是否存在
check-docker:
    #!/usr/bin/env bash
    if ! docker images {{DOCKER_IMAGE}} -q | grep -q .; then
        echo "Docker 镜像不存在，正在构建..."
        just setup
    fi

# 创建构建目录
init-build:
    mkdir -p {{BUILD_DIR}}/android
    mkdir -p {{BUILD_DIR}}/linux  
    mkdir -p {{BUILD_DIR}}/mac
    mkdir -p {{BUILD_DIR}}/kpms

# 获取版本号
get-version:
    #!/usr/bin/env bash
    MAJOR=$(grep '#define MAJOR' version | awk '{print $3}')
    MINOR=$(grep '#define MINOR' version | awk '{print $3}')
    PATCH=$(grep '#define PATCH' version | awk '{print $3}')
    echo "${MAJOR}.${MINOR}.${PATCH}"

# 构建所有平台的所有组件
build-all: check-docker init-build
    @echo "构建所有平台的所有组件..."
    just build-kpimg
    just build-kptools
    just build-kpms
    just build-mac
    @echo "所有构建完成"

# 构建 kpimg（Android 和 Linux 版本）
build-kpimg: check-docker init-build
    @echo "构建 kpimg..."
    @echo "构建 Android 版本..."
    docker run --rm -v $(pwd):/workspace -w /workspace {{DOCKER_IMAGE}} bash -c "\
        export TARGET_COMPILE=/opt/arm-toolchain/bin/aarch64-none-elf- && \
        cd kernel && \
        export ANDROID=1 && \
        make clean && make && \
        mv kpimg ../{{BUILD_DIR}}/android/kpimg-android && \
        mv kpimg.elf ../{{BUILD_DIR}}/android/kpimg.elf-android"
    
    @echo "构建 Linux 版本..."
    docker run --rm -v $(pwd):/workspace -w /workspace {{DOCKER_IMAGE}} bash -c "\
        export TARGET_COMPILE=/opt/arm-toolchain/bin/aarch64-none-elf- && \
        cd kernel && \
        unset ANDROID && \
        make clean && make && \
        mv kpimg ../{{BUILD_DIR}}/linux/kpimg-linux && \
        mv kpimg.elf ../{{BUILD_DIR}}/linux/kpimg.elf-linux"
    
    @echo "kpimg 构建完成"

# 构建所有平台的 kptools
build-kptools: check-docker init-build
    @echo "构建 kptools..."
    just kptools-android
    just kptools-linux
    @echo "kptools 构建完成"

# 构建 Android 版本的 kptools
kptools-android: check-docker
    @echo "构建 Android 版本 kptools..."
    docker run --rm -v $(pwd):/workspace -w /workspace {{DOCKER_IMAGE}} bash -c "\
        export TARGET_COMPILE=placeholder && \
        cd kernel && make hdr && cd .. && \
        cd tools && \
        mkdir -p build/android && cd build/android && \
        cmake \
            -DCMAKE_TOOLCHAIN_FILE=/opt/android-ndk/build/cmake/android.toolchain.cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DANDROID_PLATFORM=android-33 \
            -DANDROID_ABI=arm64-v8a ../.. && \
        cmake --build . && \
        mv kptools ../../{{BUILD_DIR}}/android/kptools-android"

# 构建Linux版本的 kptools  
kptools-linux: check-docker
    @echo "构建 Linux 版本 kptools..."
    docker run --rm -v $(pwd):/workspace -w /workspace {{DOCKER_IMAGE}} bash -c "\
        export TARGET_COMPILE=placeholder && \
        cd kernel && make hdr && cd .. && \
        cd tools && \
        mkdir -p build/linux && cd build/linux && \
        cmake .. && \
        make && \
        mv kptools ../../{{BUILD_DIR}}/linux/kptools-linux"

# 构建macOS版本kptools
mac: init-build
    #!/usr/bin/env bash
    echo "构建 macOS 版本..."
    echo "生成头文件..."
    cd kernel && TARGET_COMPILE=placeholder make hdr && cd ..
    
    echo "构建 kptools-mac..."
    cd tools
    mkdir -p build/mac 
    cd build/mac
    cmake ../..
    make
    mv kptools ../../{{BUILD_DIR}}/mac/kptools-mac
    echo "macOS 版本构建完成"

# 构建示例 KPM 模块
build-kpms: check-docker init-build
    @echo "构建示例 KPM 模块..."
    docker run --rm -v $(pwd):/workspace -w /workspace {{DOCKER_IMAGE}} bash -c "\
        export TARGET_COMPILE=/opt/arm-toolchain/bin/aarch64-none-elf- && \
        cd kpms && \
        cd demo-hello && make && mv hello.kpm ../../{{BUILD_DIR}}/kpms/demo-hello.kpm && cd .. && \
        cd demo-inlinehook && make && mv inlinehook.kpm ../../{{BUILD_DIR}}/kpms/demo-inlinehook.kpm && cd .. && \
        cd demo-syscallhook && make && mv syscallhook.kpm ../../{{BUILD_DIR}}/kpms/demo-syscallhook.kpm"
    
    @echo "KPM 模块构建完成"

# 平台特定构建命令
android: check-docker init-build
    @echo "构建 Android 平台所有组件..."
    docker run --rm -v $(pwd):/workspace -w /workspace {{DOCKER_IMAGE}} bash -c "\
        export TARGET_COMPILE=/opt/arm-toolchain/bin/aarch64-none-elf- && \
        cd kernel && export ANDROID=1 && make clean && make && \
        mv kpimg ../{{BUILD_DIR}}/android/kpimg-android && \
        mv kpimg.elf ../{{BUILD_DIR}}/android/kpimg.elf-android && \
        cd .. && \
        export TARGET_COMPILE=placeholder && \
        cd kernel && make hdr && cd .. && \
        cd tools && mkdir -p build/android && cd build/android && \
        cmake -DCMAKE_TOOLCHAIN_FILE=/opt/android-ndk/build/cmake/android.toolchain.cmake \
              -DCMAKE_BUILD_TYPE=Release -DANDROID_PLATFORM=android-33 \
              -DANDROID_ABI=arm64-v8a ../.. && \
        cmake --build . && \
        mv kptools ../../{{BUILD_DIR}}/android/kptools-android"

# 构建Linux平台所有组件
linux: check-docker init-build  
    @echo "构建 Linux 平台所有组件..."
    docker run --rm -v $(pwd):/workspace -w /workspace {{DOCKER_IMAGE}} bash -c "\
        export TARGET_COMPILE=/opt/arm-toolchain/bin/aarch64-none-elf- && \
        cd kernel && unset ANDROID && make clean && make && \
        mv kpimg ../{{BUILD_DIR}}/linux/kpimg-linux && \
        mv kpimg.elf ../{{BUILD_DIR}}/linux/kpimg.elf-linux && \
        cd .. && \
        export TARGET_COMPILE=placeholder && \
        cd kernel && make hdr && cd .. && \
        cd tools && mkdir -p build/linux && cd build/linux && \
        cmake ../.. && make && \
        mv kptools ../../{{BUILD_DIR}}/linux/kptools-linux"

# 显示构建信息
info:
    #!/usr/bin/env bash
    VERSION=$(just get-version)
    echo "KernelPatch 构建信息"
    echo "版本: $VERSION" 
    echo "构建目录: {{BUILD_DIR}}"
    echo "Docker 镜像: {{DOCKER_IMAGE}}"
    echo ""
    echo "构建状态:"
    if [ -d "{{BUILD_DIR}}" ]; then
        echo "  构建目录存在: ✓"
        find {{BUILD_DIR}} -name "*kpimg*" -o -name "*kptools*" -o -name "*.kpm" 2>/dev/null | \
        sed 's|^{{BUILD_DIR}}/|  |' | sort
    else
        echo "  构建目录不存在: ✗"
    fi
    echo ""
    if docker images {{DOCKER_IMAGE}} -q 2>/dev/null | grep -q .; then
        echo "  Docker 镜像存在: ✓"
    else
        echo "  Docker 镜像不存在: ✗ (运行 'just setup' 创建)"
    fi

# 清理构建文件
clean:
    @echo "清理构建文件..."
    rm -rf {{BUILD_DIR}}
    cd kernel && make clean
    cd tools && rm -rf build
    cd kpms/demo-hello && make clean
    cd kpms/demo-inlinehook && make clean  
    cd kpms/demo-syscallhook && make clean
    @echo "清理完成"

# 清理所有文件包括 Docker 镜像
clean-all: clean
    @echo "清理 Docker 镜像..."
    -docker rmi {{DOCKER_IMAGE}}
    -docker system prune -f
    @echo "全部清理完成"

# 开发调试命令
dev-shell: check-docker
    @echo "启动开发调试 shell..."
    docker run --rm -it -v $(pwd):/workspace -w /workspace {{DOCKER_IMAGE}} bash

# 测试构建环境
test-env: check-docker
    @echo "测试构建环境..."
    docker run --rm -v $(pwd):/workspace -w /workspace {{DOCKER_IMAGE}} bash -c "\
        echo '=== 测试 ARM 工具链 ===' && \
        /opt/arm-toolchain/bin/aarch64-none-elf-gcc --version && \
        echo '=== 测试 Android NDK ===' && \
        ls -la /opt/android-ndk/build/cmake/ && \
        echo '=== 测试 CMake ===' && \
        cmake --version && \
        echo '=== 测试 Make ===' && \
        make --version && \
        echo '=== 环境测试完成 ==='"
