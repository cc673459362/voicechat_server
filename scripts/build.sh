#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

ROOT_DIR="$(pwd)"
BUILD_DIR="build"
if [ -d "${BUILD_DIR}" ]; then
    rm -rf "${BUILD_DIR}"
fi
mkdir -p "$BUILD_DIR"
BUILD_DIR_PATH="${ROOT_DIR}/${BUILD_DIR}"
cd "$BUILD_DIR"

# 可选：设 SANITIZE=1 环境变量启用 ASAN
if [ "${SANITIZE:-0}" = "1" ]; then
    echo "Configuring with AddressSanitizer..."
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
else
    cmake .. -DCMAKE_BUILD_TYPE=Debug
fi

cmake --build . -- -j"$(nproc)"

# 将 logs 放在项目根目录，若已存在则备份后重建
LOGS_DIR="${ROOT_DIR}/logs"
if [ -d "${LOGS_DIR}" ]; then
   rm -rf ${ROOT_DIR}/logs
fi
mkdir -p "${LOGS_DIR}"


# 如果已有同路径的可执行在跑，则尝试停止（安全尝试）
if pgrep -f "${PWD}/chat_server" > /dev/null 2>&1; then
    echo "Stopping existing chat_server..."
    pkill -f "${PWD}/chat_server" || true
    sleep 1
fi

# 后台启动并重定向日志
nohup "${PWD}/chat_server" > ${LOGS_DIR}/chat_server.log 2>&1 &

echo "Server started (PID $!). Logs: ${LOGS_DIR}/chat_server.log"