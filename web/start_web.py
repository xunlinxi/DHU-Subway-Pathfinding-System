#  start_web.py
#  上海地铁路径规划系统 —— 一键启动脚本 (替代 bat)

import os
import subprocess
import sys
from pathlib import Path

# ==== 路径初始化 ====
SCRIPT_DIR = Path(__file__).parent.resolve()        # web/
PROJECT_ROOT = SCRIPT_DIR.parent.resolve()           # 项目根
MAIN_WEB_CPP = SCRIPT_DIR / "main_web.cpp"
MAIN_WEB_EXE = SCRIPT_DIR / "main_web.exe"
SERVER_PY = SCRIPT_DIR / "server.py"

DIST_MODE = "--dist" in sys.argv
SETUP_MODE = "--setup" in sys.argv


def find_exe(name: str, search_paths: list) -> str | None:
    """在多条路径中查找可执行文件"""
    for p in search_paths:
        exe = Path(p) / name
        if exe.is_file():
            return str(exe)
    # 尝试系统 PATH
    import shutil
    found = shutil.which(name)
    if found:
        return found
    return None


def find_gxx() -> str:
    """自动查找 g++.exe"""
    search = [
        r"D:\Software\mingw64\bin",
        r"C:\msys64\mingw64\bin",
        r"C:\mingw64\bin",
        r"C:\MinGW\bin",
        r"C:\TDM-GCC-64\bin",
    ]
    result = find_exe("g++.exe", search)
    if not result:
        print("[ERROR] g++.exe 未找到，请安装 MinGW-w64 或将其 bin 目录加入 PATH。")
        sys.exit(1)
    return result


def find_python() -> str:
    """查找 python.exe（优先使用当前运行的解释器）"""
    return sys.executable


def compile_exe(gxx: str):
    """编译 main_web.cpp -> main_web.exe"""
    print("[1/3] Compiling main_web.cpp ...")
    result = subprocess.run(
        [gxx, "-std=c++11", "-O2", "-o", str(MAIN_WEB_EXE), str(MAIN_WEB_CPP)],
        cwd=str(SCRIPT_DIR),
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print("[ERROR] g++ compile failed.")
        print(result.stderr)
        sys.exit(1)
    print(f"       [OK] web/main_web.exe built.\n")


def setup_deps(python: str, target_dir: str | None = None):
    """安装 Python 依赖"""
    deps = ["fastapi", "uvicorn"]
    if target_dir:
        print(f"[2/3] Installing Python deps to {target_dir} ...")
        subprocess.run([python, "-m", "pip", "install", "--target", target_dir] + deps,
                       check=True)
    else:
        print("[2/3] Checking Python deps ...")
        for dep in deps:
            try:
                __import__(dep)
            except ImportError:
                print(f"       Installing {dep} ...")
                subprocess.run([python, "-m", "pip", "install", dep], check=True)
    print("       [OK] Python deps ready.\n")


def start_server(python: str):
    """启动 FastAPI 服务"""
    print("[3/3] Starting FastAPI server ...")
    print("       Open http://127.0.0.1:3724 in your browser.")
    print("       Press Ctrl+C in this window to stop.\n")
    env = os.environ.copy()
    if DIST_MODE:
        # 打包模式：将本地 packages/ 加入路径
        packages_dir = str(SCRIPT_DIR / "packages")
        existing = env.get("PYTHONPATH", "")
        env["PYTHONPATH"] = f"{packages_dir}{os.pathsep}{existing}" if existing else packages_dir
    subprocess.run([python, str(SERVER_PY)], cwd=str(SCRIPT_DIR), env=env)


def main():
    print("=" * 44)
    print("  Shanghai Subway Web - One-click Starter")
    print("=" * 44)
    print()

    gxx = find_gxx()
    print(f"[INFO] g++    = {gxx}")
    python = find_python()
    print(f"[INFO] python = {python}")
    print()

    # 编译 C++ 后端
    compile_exe(gxx)

    if SETUP_MODE:
        packages_dir = str(SCRIPT_DIR / "packages")
        setup_deps(python, target_dir=packages_dir)
        print("[INFO] 依赖已安装到 web/packages/，可直接分发。")
        return

    if DIST_MODE:
        packages_dir = str(SCRIPT_DIR / "packages")
        if not Path(packages_dir).is_dir():
            print("[ERROR] 打包模式需要先运行: python start_web.py --setup")
            sys.exit(1)
        setup_deps(python, target_dir=packages_dir)
    else:
        setup_deps(python)

    # 启动服务
    start_server(python)


if __name__ == "__main__":
    main()
