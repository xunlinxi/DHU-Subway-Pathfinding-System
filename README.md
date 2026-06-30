# DHU-Subway-Pathfinding-System

东华大学数据结构课程设计项目：上海地铁路径规划与管理系统。

基于邻接表建模上海地铁网络，采用状态扩展 Dijkstra 变种与 Yen K 短路算法，提供时间最短 / 换乘最少双目标路径规划、动态站点状态管理、受影响区域分析、网络连通性分析等功能，并附带 FastAPI + 静态前端 的 Web 版本。

## 项目结构

```text
.
├── main.cpp                 # CLI 主程序入口（Unity Build）
├── station.h / station.cpp  # 站点数据与运营状态管理
├── graph.h / graph.cpp      # 邻接表图结构（线路边 + 换乘边）
├── pathfinder.h / pathfinder.cpp  # Dijkstra 变种 + Yen K 短路 + 影响分析
├── menu.h / menu.cpp        # 控制台交互菜单
├── data/
│   ├── Station.csv          # 站点数据（id,name,line,status）
│   ├── Station_init.csv     # 初始状态快照（用于一键恢复）
│   ├── Edge.csv             # 边数据（from,to,line,direction,time）
│   └── update_station_status.csv  # 批量更新样例
├── web/
│   ├── main_web.cpp         # Web 入口（结构化文本输出供 Python 解析）
│   ├── server.py            # FastAPI 桥接服务（端口 3724）
│   ├── index.html           # 静态前端
│   └── start_web.bat        # 一键启动脚本（自动编译 + 装依赖）
├── 小组任务指导书.md
├── 小组任务测试文档.md
└── WORK_LOG.md
```

## 功能特性

### 路径规划
- 单条所需时间最短路径（Dijkstra，主键=时间，副键=换乘数）
- 单条所需换乘次数最少路径（Dijkstra，主键=换乘，副键=时间）
- 前 3 条候选路径查询（Yen 算法，偏离点 + 受限 Dijkstra + signature 去重）
- 4 号线环线内圈 / 外圈方向显示
- 起点为换乘站时不计初始换乘

### 站点与运营管理
- 批量 CSV 更新站点状态（支持 `id,status` 与 `name,line,status` 两种格式）
- 手工更新（循环操作 + 实时落盘到 `Station.csv`）
- 一键恢复初始状态（基于 `Station_init.csv` 快照）
- 换乘站整体关闭 / 线路停运恢复 / 全网停运恢复
- 所有状态变更自动同步回 `data/Station.csv`

### 分析功能
- 受关闭站点影响分析：受影响线路 + 同线直接相邻站点
- 网络连通性分析：DFS 统计开放站点连通分量，定位网络断裂

### 输入与健壮性
- 模糊搜索站点，换乘站提供二次选择
- 菜单输入严格校验（只允许 `[+/-]` + 数字，拒绝小数 / 字母 / 符号）
- CSV 解析处理 UTF-8 BOM、全角空格、空行、中英文表头

## 编译

需要支持 C++11 的 g++（MinGW-w64 / MSYS2 / Linux gcc 均可）。

Unity Build（推荐，一行编译）：

```bash
g++ -std=c++11 -O2 -Wall -Wextra -o metro.exe main.cpp
```

传统多文件编译：

```bash
g++ -std=c++11 -O2 -Wall -Wextra -DNO_UNITY_BUILD \
    main.cpp station.cpp graph.cpp pathfinder.cpp menu.cpp -o metro.exe
```

Web 入口编译（在 `web/` 目录下执行）：

```bash
cd web
g++ -std=c++11 -O2 -Wall -Wextra -I.. -o main_web.exe main_web.cpp
```

## 运行

### 命令行版本

交互式菜单：

```bash
./metro.exe
```

一键演示模式（跑预设测试用例，无需交互，适合截图与验收）：

```bash
./metro.exe --demo
```

帮助：

```bash
./metro.exe --help
```

### Web 版本

一键启动（自动检测 g++ / python，编译 C++ 入口，安装 FastAPI 依赖，启动服务）：

```bash
web\start_web.bat
```

手动启动：

```bash
cd web
g++ -std=c++11 -O2 -I.. -o main_web.exe main_web.cpp
pip install fastapi uvicorn
python server.py
```

访问 [http://127.0.0.1:3724](http://127.0.0.1:3724)。

## 菜单结构

```
主菜单
├── 1. 线路站点信息/运营状态管理
│   ├── 1. 批量更新站点状态
│   ├── 2. 手工更新站点状态
│   ├── 3. 显示当前关闭站点
│   ├── 4. 恢复所有站点初始状态
│   ├── 5. 显示线路站点信息
│   ├── 6. 换乘站整体关闭管理
│   ├── 7. 线路停运管理
│   ├── 8. 全网停运/恢复管理
│   ├── 9. 受关闭站点影响站点分析
│   └── 10. 网络连通性分析
├── 2. 所需时间最短路径规划
│   ├── 1. 单条所需时间最短路径
│   └── 2. 3 条所需时间最短路径
├── 3. 所需换乘次数最少路径规划
│   ├── 1. 单条换乘次数最少路径
│   └── 2. 3 条换乘次数最少路径
└── 4. 退出系统
```

## 路径输出格式

```
起点[线路]：
起点 -> 途经站 -> ... -> 换乘站
站内换乘至[新线路]
换乘站 -> ... -> 终点
  总耗时: N 分钟 | 换乘: N 次 | 途经: N 站 | 实际经过: N 站
```

4 号线环线会标注方向，如 `4号线·内圈方向` / `站内换乘至[4号线 (内圈方向)]`。

## Web API

| 方法 | 路径 | 说明 |
|---|---|---|
| GET | `/api/subway/query` | 路径查询（start/end/start_line/end_line/k/mode） |
| POST | `/api/subway/station/toggle` | 切换站点状态 |
| GET | `/api/subway/stations/all` | 全部站点 |
| GET | `/api/subway/stations/closed` | 关闭站点 |
| GET | `/api/subway/stations/line/{line}` | 按线路查询站点 |
| GET | `/api/subway/lines` | 所有线路名 |
| POST | `/api/subway/station/close-transfer` | 换乘站整体关闭 |
| POST | `/api/subway/line/toggle` | 线路停运/恢复 |
| POST | `/api/subway/network/toggle` | 全网停运/恢复 |
| POST | `/api/subway/reset` | 恢复初始状态 |
| POST | `/api/subway/analyze/impact` | 关闭影响分析 |
| GET | `/api/subway/analyze/network` | 网络连通性分析 |
| GET | `/healthz` | 健康检查 |

## 数据规模与性能

| 项目 | 数值 |
|---|---|
| 站点数 | 523 |
| 图顶点数 | 5138 |
| 边数 | 1000+ |
| 单条最短路径查询 | < 50 ms |
| K 短路查询（K=3） | < 200 ms |

## 技术要点

- **状态扩展 Dijkstra**：状态 = `(stationId, arrivedLine)`，因为同一站位于不同线路时已发生的换乘次数不同；双键代价 `(主键, 副键)` 按目标切换
- **剪枝策略**：关闭站点过滤、状态过期丢弃、blocked 节点、禁用边
- **Yen K 短路**：偏离点 + 受限 Dijkstra + signature 字符串去重
- **换乘建模**：同名换乘站之间构建 5 分钟双向换乘边，按 `(from, to, line)` 三元组去重
- **Unity Build**：`g++ main.cpp` 一行编译，`-DNO_UNITY_BUILD` 切换传统多文件编译

## 注意事项

- 控制台中文输出依赖 UTF-8，Windows 下程序已自动调用 `SetConsoleOutputCP(CP_UTF8)`
- Web 版 `main_web.cpp` 与 CLI 版 `main.cpp` 互不影响，共享核心模块但入口独立
- 站点状态变更会实时写回 `data/Station.csv`（带 UTF-8 BOM，防止 Windows 记事本乱码）
