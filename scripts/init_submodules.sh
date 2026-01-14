#!/usr/bin/env bash
set -euo pipefail

# FirstEngine: init/update git submodules with retries.
# 适用于网络不稳定/偶发超时的环境（例如首次拉取 third_party 子模块）。

MAX_RETRIES="${MAX_RETRIES:-4}"
SLEEP_BASE_SECONDS="${SLEEP_BASE_SECONDS:-4}"
JOBS="${SUBMODULE_JOBS:-4}"

say() { printf '%s\n' "$*"; }

backoff_sleep() {
  local attempt="$1"
  # 4s, 8s, 16s, 32s...
  local seconds=$(( SLEEP_BASE_SECONDS << (attempt - 1) ))
  say "等待 ${seconds}s 后重试..."
  sleep "${seconds}"
}

try_update_with_jobs() {
  # Some git versions don't support --jobs; we fall back automatically.
  git submodule update --init --recursive --jobs "${JOBS}"
}

try_update_no_jobs() {
  git submodule update --init --recursive
}

say "== FirstEngine 子模块初始化 =="
say "分支: $(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'unknown')"
say "重试次数: ${MAX_RETRIES}；并行 jobs: ${JOBS}"
say ""

git submodule sync --recursive

attempt=1
while true; do
  say "[${attempt}/${MAX_RETRIES}] 拉取/更新子模块..."
  if try_update_with_jobs; then
    break
  fi

say "提示：你的 git 可能不支持 --jobs，改用兼容模式重试..."
  if try_update_no_jobs; then
    break
  fi

  if [[ "${attempt}" -ge "${MAX_RETRIES}" ]]; then
    say ""
    say "子模块拉取失败（已重试 ${MAX_RETRIES} 次）。常见原因/处理："
    say "- 网络/代理：配置 http_proxy/https_proxy 后重试"
    say "- DNS：检查是否能解析 github.com"
    say "- 公司网络拦截 GitHub：换网络或使用可访问 GitHub 的代理"
    say ""
    say "你也可以手动执行："
    say "  git submodule sync --recursive"
    say "  git submodule update --init --recursive"
    exit 1
  fi

  backoff_sleep "${attempt}"
  attempt=$((attempt + 1))
done

say ""
say "完成：子模块已初始化。当前状态："
git submodule status
