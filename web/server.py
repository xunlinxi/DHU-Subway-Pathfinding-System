# =============================================================
#  server.py
#  上海地铁路径规划系统 —— Python 桥接服务 (FastAPI)
#
#  文件位置: d:\Codes\数据结构课程设计\web\server.py
#
# 职责:
#    1. 接收浏览器 fetch 请求
#    2. 用 subprocess 调用同目录的 main_web.exe
#    3. 解析 C++ 结构化输出, 转 JSON 返回
#    4. 托管同目录的 index.html
#
# 启动:
#    cd d:\Codes\数据结构课程设计
#    pip install fastapi uvicorn
#    python web\server.py
# 访问: http://127.0.0.1:3724
# =============================================================
import os
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

from fastapi import FastAPI, HTTPException, Query
from fastapi.responses import FileResponse, JSONResponse
from pydantic import BaseModel, Field

# ============= 路径配置 =============
# 支持三种运行模式:
#   1. 开发环境: python web/server.py  (main_web.exe 在 web/ 下)
#   2. 发布环境: server.exe (PyInstaller) 在 dist/ 根, main_web.exe 同行
#   3. 分发环境: python start_web.py (dist 目录下)
FROZEN = getattr(sys, "frozen", False)
if FROZEN:
    # PyInstaller 打包: exe 在 dist/ 根, data/ 同行
    EXE_DIR   = Path(sys.executable).parent.resolve()
    BASE_DIR  = EXE_DIR                                   # dist/
    EXE_NAME  = "main_web.exe" if sys.platform == "win32" else "main_web"
    MAIN_WEB  = str(EXE_DIR / EXE_NAME)                    # dist/main_web.exe
    INDEX_HTML = str(Path(sys._MEIPASS) / "index.html")    # PyInstaller 内置
else:
    BASE_DIR  = Path(__file__).parent.parent.resolve()     # 项目根
    WEB_DIR   = Path(__file__).parent.resolve()             # web/
    EXE_NAME  = "main_web.exe" if sys.platform == "win32" else "main_web"
    MAIN_WEB  = str(WEB_DIR / EXE_NAME)                    # web/main_web.exe
    INDEX_HTML = str(WEB_DIR / "index.html")                # web/index.html
DATA_DIR = BASE_DIR / "data"                                # data/ 目录

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

    def collect_keyvalue_block(start: int) -> (Dict[str, Any], int):
        """从 start 开始读 KEY=VALUE 行, 直到非 KEY=VALUE 行, 返回 (dict, 下一行索引)"""
        rec: Dict[str, Any] = {}
        while i < n and lines[i] != "ITEM_END" and lines[i] != "PATH_END" \
              and lines[i] != "SAME_LINE_ADJ_END":
            if "=" in lines[i]:
                k, v = lines[i].split("=", 1)
                rec.setdefault(k.strip().lower(), []).append(_to_int(v.strip()))
            i += 1
        # 把单元素列表降为标量
        flat = {k: v[0] if len(v) == 1 else v for k, v in rec.items()}
        return flat, i

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
                    k = k.strip().lower(); v = v.strip()
                    v_conv = _to_int(v)
                    if k in rec:
                        # 同名字段 -> 转成列表 (list_lines 等场景)
                        if not isinstance(rec[k], list):
                            rec[k] = [rec[k]]
                        rec[k].append(v_conv)
                    else:
                        rec[k] = v_conv
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

        # 影响分析: SAME_LINE_ADJ_BEGIN ... SAME_LINE_ADJ_END
        if line == "SAME_LINE_ADJ_BEGIN":
            i += 1
            adj: List[Dict[str, Any]] = []
            cur: Dict[str, Any] = {}
            while i < n and lines[i] != "SAME_LINE_ADJ_END":
                if "=" in lines[i]:
                    k, v = lines[i].split("=", 1)
                    k = k.strip().lower(); v = v.strip()
                    if k in ("id",) and cur:
                        adj.append(cur); cur = {}
                    cur[k] = _to_int(v) if k == "id" else v
                i += 1
            if cur: adj.append(cur)
            i += 1
            data["same_line_adj"] = adj
            continue

        # 连通性: COMP_BEGIN ... COMP_END (含各分量站点明细)
        if line == "COMP_BEGIN":
            i += 1
            comps: List[Dict[str, Any]] = []
            cur: Dict[str, Any] = {}
            while i < n and lines[i] != "COMP_END":
                if lines[i] == "STATIONS_BEGIN":
                    i += 1
                    stns: List[Dict[str, Any]] = []
                    cur_st: Dict[str, Any] = {}
                    while i < n and lines[i] != "STATIONS_END":
                        if "=" in lines[i]:
                            k, v = lines[i].split("=", 1)
                            k = k.strip().lower(); v = v.strip()
                            if k == "id" and cur_st:
                                stns.append(cur_st); cur_st = {}
                            if k == "id":
                                cur_st[k] = _to_int(v)
                            else:
                                cur_st[k] = v
                        i += 1
                    if cur_st:
                        stns.append(cur_st)
                    i += 1  # 跳过 STATIONS_END
                    cur["stations"] = stns
                    continue
                if "=" in lines[i]:
                    k, v = lines[i].split("=", 1)
                    k = k.strip().lower(); v = v.strip()
                    if k == "comp_id" and cur:
                        comps.append(cur); cur = {}
                    cur[k] = _to_int(v)
                i += 1
            if cur:
                comps.append(cur)
            i += 1  # 跳过 COMP_END
            data["components"] = comps
            continue

        # 顶层 key=value
        if "=" in line:
            k, v = line.split("=", 1)
            data[k.strip().lower()] = _to_int(v.strip())
        i += 1

    # AFFECTED_LINES 逗号分隔
    if "affected_lines" in data and isinstance(data["affected_lines"], str):
        data["affected_lines"] = [x for x in data["affected_lines"].split(",") if x]

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
            cwd=None,  # exe 自行通过 getExeDir() 定位 data/，不依赖 CWD
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

class CloseTransferRequest(BaseModel):
    name: str = Field(..., description="换乘站名称, 如 人民广场")

class LineToggleRequest(BaseModel):
    line:   str = Field(..., description="线路名, 如 1号线")
    action: str = Field(..., description="open 或 close")

class NetworkToggleRequest(BaseModel):
    action: str = Field(..., description="open 或 close")

class ImpactRequest(BaseModel):
    name: str = Field(..., description="站点名")
    line: str = Field(..., description="线路名")

# ---- 路径查询：单条最优 / K 条候选（时间 / 换乘）----
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

# ---- 站点状态：切换 / 批量更新 ----
@app.post("/api/subway/station/toggle")
def api_toggle(req: ToggleRequest):
    if req.status not in ("开启", "关闭"):
        raise HTTPException(status_code=400, detail="status 必须是 '开启' 或 '关闭'")
    result = run_main_web(["toggle", req.name, req.line, req.status])
    if not result.get("ok"):
        raise HTTPException(status_code=400, detail=result.get("message", "更新失败"))
    return JSONResponse(result)

# ---- 站点列表：全部 / 仅关闭 / 按线路 / 所有线路名 ----
@app.get("/api/subway/stations/all")
def api_stations_all():
    return JSONResponse(run_main_web(["list_all"]))

@app.get("/api/subway/stations/closed")
def api_stations_closed():
    return JSONResponse(run_main_web(["list_closed"]))

@app.get("/api/subway/stations/line/{line}")
def api_stations_line(line: str):
    return JSONResponse(run_main_web(["list_line", line]))

# ---- 运营管理：恢复初始 / 换乘站关闭 / 线路停运 / 全网停运 ----
@app.post("/api/subway/reset")
def api_reset():
    return JSONResponse(run_main_web(["reset"]))

class BatchUpdateRequest(BaseModel):
    path:    Optional[str] = Field(None, description="CSV 文件路径, 留空则用 data/update_station_status.csv")
    content: Optional[str] = Field(None, description="CSV 文本内容, 提供时写入临时文件后批量更新")

@app.post("/api/subway/station/batch-update")
def api_batch_update(req: BatchUpdateRequest):
    import tempfile
    tmp_path = None
    try:
        if req.content:
            # 把粘贴的 CSV 内容写入 data/ 下临时文件, 再交给 main_web 读取
            tmp_fd, tmp_path = tempfile.mkstemp(suffix=".csv", dir=str(DATA_DIR), text=True)
            with os.fdopen(tmp_fd, "w", encoding="utf-8") as f:
                f.write(req.content)
            result = run_main_web(["batch_update", tmp_path])
        else:
            args = ["batch_update"] + ([req.path] if req.path else [])
            result = run_main_web(args)
        if not result.get("ok"):
            raise HTTPException(status_code=400, detail=result.get("message", "批量更新失败"))
        return JSONResponse(result)
    finally:
        if tmp_path and os.path.exists(tmp_path):
            try:
                os.remove(tmp_path)
            except Exception:
                pass

@app.post("/api/subway/station/close-transfer")
def api_close_transfer(req: CloseTransferRequest):
    result = run_main_web(["close-transfer", req.name])
    if not result.get("ok"):
        raise HTTPException(status_code=400, detail=result.get("message", "操作失败"))
    return JSONResponse(result)

@app.post("/api/subway/line/toggle")
def api_line_toggle(req: LineToggleRequest):
    if req.action not in ("open", "close"):
        raise HTTPException(status_code=400, detail="action 必须是 open 或 close")
    result = run_main_web(["line-toggle", req.line, req.action])
    if not result.get("ok"):
        raise HTTPException(status_code=400, detail=result.get("message", "操作失败"))
    return JSONResponse(result)

@app.post("/api/subway/network/toggle")
def api_network_toggle(req: NetworkToggleRequest):
    if req.action not in ("open", "close"):
        raise HTTPException(status_code=400, detail="action 必须是 open 或 close")
    result = run_main_web(["network-toggle", req.action])
    if not result.get("ok"):
        raise HTTPException(status_code=400, detail=result.get("message", "操作失败"))
    return JSONResponse(result)

# ---- 分析：受关闭影响 / 网络连通性 ----
@app.post("/api/subway/analyze/impact")
def api_impact(req: ImpactRequest):
    result = run_main_web(["impact", req.name, req.line])
    if not result.get("ok"):
        raise HTTPException(status_code=400, detail=result.get("message", "分析失败"))
    return JSONResponse(result)

@app.get("/api/subway/analyze/network")
def api_network():
    return JSONResponse(run_main_web(["network"]))

@app.get("/api/subway/lines")
def api_lines():
    return JSONResponse(run_main_web(["list_lines"]))

# ---- 静态页面与健康检查 ----
@app.get("/")
def index():
    if not os.path.exists(INDEX_HTML):
        raise HTTPException(status_code=404, detail="index.html 不存在")
    return FileResponse(INDEX_HTML, media_type="text/html")

@app.get("/healthz")
def healthz():
    return {"ok": True, "main_web_exists": os.path.exists(MAIN_WEB)}


# 启动入口
if __name__ == "__main__":
    import uvicorn
    print(f"[INFO] BASE_DIR     = {BASE_DIR}")
    print(f"[INFO] MAIN_WEB     = {MAIN_WEB}")
    print(f"[INFO] INDEX_HTML   = {INDEX_HTML}")
    print(f"[INFO] Server ready at http://127.0.0.1:3724")
    uvicorn.run(app, host="127.0.0.1", port=3724)
