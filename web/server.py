# =============================================================
#  server.py
#  上海地铁路径规划系统 —— Python 桥接服务 (FastAPI)
#
#  文件位置: d:\Codes\数据结构课程设计\web\server.py
#
#  职责:
#    1. 接收浏览器 fetch 请求
#    2. 用 subprocess 调用上一级目录的 main_web.exe
#    3. 解析 C++ 结构化输出, 转 JSON 返回
#    4. 托管同目录的 index.html
#
#  启动:
#    cd d:\Codes\数据结构课程设计
#    pip install fastapi uvicorn
#    python web\server.py
#  访问: http://127.0.0.1:8000
# =============================================================
import os
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List

from fastapi import FastAPI, HTTPException, Query
from fastapi.responses import FileResponse, JSONResponse
from pydantic import BaseModel, Field

# ============= 路径配置 =============
# server.py 在 web/ 下, 所以项目根 = 父目录的父目录
BASE_DIR   = Path(__file__).parent.parent.resolve()
EXE_NAME   = "main_web.exe" if sys.platform == "win32" else "main_web"
MAIN_WEB   = str(BASE_DIR / EXE_NAME)            # 项目根/main_web.exe
INDEX_HTML = str(Path(__file__).parent / "index.html")  # web/index.html
DATA_DIR   = BASE_DIR / "data"                    # 项目根/data/

app = FastAPI(title="上海地铁路径规划 Web 服务", version="1.0.0")


# ============= 解析 main_web 的结构化输出 =============
def _to_int(s: str):
    """把字符串转 int, 失败则原样返回"""
    try:
        return int(s)
    except ValueError:
        return s

def parse_output(stdout: str) -> Dict[str, Any]:
    """把 main_web 的 OK/ERR + key=value + BEGIN/END 块解析为 dict"""
    if not stdout:
        return {"ok": False, "message": "子进程无输出"}
    lines = stdout.splitlines()
    if not lines:
        return {"ok": False, "message": "子进程无输出"}
    head = lines[0].strip()

    if head == "ERR":
        msg = "未知错误"
        for ln in lines[1:]:
            if ln.startswith("MESSAGE="):
                msg = ln[len("MESSAGE="):].strip()
        return {"ok": False, "message": msg}
    if head != "OK":
        return {"ok": False, "message": "无法识别的输出", "raw": stdout}

    data: Dict[str, Any] = {"ok": True}
    i, n = 1, len(lines)

    while i < n:
        line = lines[i]

        # 单条 PRETTY 块 (query / qtransfer)
        if line == "PRETTY_BEGIN":
            i += 1
            buf = []
            while i < n and lines[i] != "PRETTY_END":
                buf.append(lines[i]); i += 1
            i += 1  # 跳过 PRETTY_END
            if "pretty" in data:
                # 已经存在一个, 转成列表
                data.setdefault("pretty_list", [data.pop("pretty")])
                data["pretty_list"].append("\n".join(buf))
            else:
                data["pretty"] = "\n".join(buf)
            continue

        # 单条 ITEM 块 (list_* 命令)
        if line == "ITEM_BEGIN":
            i += 1
            rec: Dict[str, Any] = {}
            while i < n and lines[i] != "ITEM_END":
                if "=" in lines[i]:
                    k, v = lines[i].split("=", 1)
                    rec[k.strip().lower()] = _to_int(v.strip())
                i += 1
            i += 1  # 跳过 ITEM_END
            data.setdefault("items", []).append(rec)
            continue

        # 整条 PATH 块 (k* 命令)
        if line == "PATH_BEGIN":
            i += 1
            rec: Dict[str, Any] = {}
            while i < n and lines[i] != "PATH_END":
                if lines[i] == "PRETTY_BEGIN":
                    i += 1
                    buf = []
                    while i < n and lines[i] != "PRETTY_END":
                        buf.append(lines[i]); i += 1
                    i += 1
                    rec["pretty"] = "\n".join(buf)
                    continue
                if "=" in lines[i]:
                    k, v = lines[i].split("=", 1)
                    rec[k.strip().lower()] = _to_int(v.strip())
                i += 1
            i += 1  # 跳过 PATH_END
            data.setdefault("paths", []).append(rec)
            continue

        # 顶层 key=value
        if "=" in line:
            k, v = line.split("=", 1)
            data[k.strip().lower()] = _to_int(v.strip())
        i += 1

    return data


def run_main_web(args: List[str], timeout: int = 30) -> Dict[str, Any]:
    """调 main_web.exe, 解析输出, 捕获错误"""
    if not os.path.exists(MAIN_WEB):
        return {"ok": False, "message": f"找不到可执行文件: {MAIN_WEB} (请先编译 main_web.cpp)"}
    if not DATA_DIR.exists():
        return {"ok": False, "message": f"找不到 data 目录: {DATA_DIR}"}
    try:
        result = subprocess.run(
            [MAIN_WEB] + args,
            capture_output=True,
            text=True,
            timeout=timeout,
            encoding="utf-8",
            errors="replace",
            cwd=str(BASE_DIR),  # 在项目根执行, 让 main_web 能找到 data/
        )
    except subprocess.TimeoutExpired:
        return {"ok": False, "message": f"C++ 子进程超时 ({timeout}s)"}
    except Exception as e:
        return {"ok": False, "message": f"调用失败: {e}"}

    if result.returncode != 0 and not result.stdout.startswith(("OK", "ERR")):
        return {"ok": False, "message": f"子进程退出码 {result.returncode}",
                "stderr": result.stderr}
    return parse_output(result.stdout)


# ============= API 路由 =============
class ToggleRequest(BaseModel):
    name:   str = Field(..., description="站点名")
    line:   str = Field(..., description="线路, 如 1号线")
    status: str = Field(..., description="开启 或 关闭")

@app.get("/api/subway/query")
def api_query(
    start:      str  = Query(..., description="起点站名"),
    start_line: str  = Query("",  description="起点线路(同名换乘站需要)"),
    end:        str  = Query(..., description="终点站名"),
    end_line:   str  = Query("",  description="终点线路"),
    k:          int  = Query(0,   description="0=单条最优; >0=K 条候选"),
    mode:       str  = Query("time", description="time 或 transfer"),
):
    if mode not in ("time", "transfer"):
        raise HTTPException(status_code=400, detail="mode 必须是 time 或 transfer")
    if k <= 0:
        cmd = "query" if mode == "time" else "qtransfer"
        result = run_main_web([cmd, start, start_line, end, end_line])
    else:
        cmd = "ktime" if mode == "time" else "ktransfer"
        result = run_main_web([cmd, start, start_line, end, end_line, str(k)])
    if not result.get("ok"):
        raise HTTPException(status_code=400, detail=result.get("message", "查询失败"))
    return JSONResponse(result)

@app.post("/api/subway/station/toggle")
def api_toggle(req: ToggleRequest):
    if req.status not in ("开启", "关闭"):
        raise HTTPException(status_code=400, detail="status 必须是 '开启' 或 '关闭'")
    result = run_main_web(["toggle", req.name, req.line, req.status])
    if not result.get("ok"):
        raise HTTPException(status_code=400, detail=result.get("message", "更新失败"))
    return JSONResponse(result)

@app.get("/api/subway/stations/all")
def api_stations_all():
    return JSONResponse(run_main_web(["list_all"]))

@app.get("/api/subway/stations/closed")
def api_stations_closed():
    return JSONResponse(run_main_web(["list_closed"]))

@app.get("/api/subway/stations/line/{line}")
def api_stations_line(line: str):
    return JSONResponse(run_main_web(["list_line", line]))

@app.post("/api/subway/reset")
def api_reset():
    return JSONResponse(run_main_web(["reset"]))

@app.get("/")
def index():
    if not os.path.exists(INDEX_HTML):
        raise HTTPException(status_code=404, detail="index.html 不存在")
    return FileResponse(INDEX_HTML, media_type="text/html")

@app.get("/healthz")
def healthz():
    return {"ok": True, "main_web_exists": os.path.exists(MAIN_WEB)}


if __name__ == "__main__":
    import uvicorn
    print(f"[INFO] BASE_DIR     = {BASE_DIR}")
    print(f"[INFO] MAIN_WEB     = {MAIN_WEB}")
    print(f"[INFO] INDEX_HTML   = {INDEX_HTML}")
    print(f"[INFO] Server ready at http://127.0.0.1:8000")
    uvicorn.run(app, host="127.0.0.1", port=8000)
