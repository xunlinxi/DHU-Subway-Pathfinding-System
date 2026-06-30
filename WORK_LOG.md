# 小组作业完成日志

> 项目：东华大学 2025-2026 学年《数据结构》课程设计 —— 上海地铁路径规划与管理系统
> 仓库：https://github.com/xunlinxi/DHU-Subway-Pathfinding-System
> 时间范围：4 天（2026-06-29 ~ 2026-07-02）
> 小组成员：4 人

---

## 小组成员分工

| 成员 | 模块 | 关键技术点 |
|---|---|---|
| 成员 A | 地铁路径规划模块 | 邻接表建模、Station/Edge 结构体定义、CSV 健壮解析 |
| 成员 B | 最短路径核心算法 | 优先队列 + Dijkstra 变种、状态剪枝、K 短路 |
| 成员 C | 路径输出与评估 + Web 后端 | 多目标排序、signature 去重、影响分析、main_web.cpp + FastAPI 桥接 |
| 成员 D | 动态站点管理 + 菜单交互 + Web 前端 | 批量 CSV 更新、模糊搜索、运营管理、index.html 可视化 |

> 实际开发中由组长（**我**）主要承担 B/D 模块并统筹 A/C 的接口与代码 review；以下日志按"日期 + 当日完成项 + 遇到的问题 + 第二日计划"的格式记录。

---

## Day 1 — 2026-06-29（周一）

### 当日目标
- 搭建项目骨架
- 解决遗留的**运行超时问题**（参考代码存在性能瓶颈）

### 完成项
- [x] 整理资料：拿到 `Station.csv` / `Edge.csv` / `Station_init.csv` / `update_station_status.csv` 四个数据文件
- [x] 阅读测试文档，明确功能需求（路径查询、K 短路、动态站点管理、模糊搜索、影响分析等）
- [x] **诊断超时问题**：之前的参考实现把"换乘次数"作为唯一目标，导致所有等代价路径全部入队，优先队列爆炸
- [x] 设计新算法思路：
  - 状态从 `(stationId)` 扩展为 `(stationId, arrivedLine)`
  - 双键代价：`(主键, 副键)` —— 优化"时间最短"时主键=时间、副键=换乘数
  - 多重剪枝：关闭站点过滤 / 状态过期丢弃 / blocked 节点 / 禁用边
  - K 短路走 **Yen 算法**（偏离点 + 受限 Dijkstra）
- [x] 创建 GitHub 仓库 `DHU-Subway-Pathfinding-System`

### 遇到的问题
- 🔴 **超时**：第一版 Dijkstra 在上海地铁 500+ 站点规模下查询耗时 > 30 s
  - 解决：状态扩展 + 双键代价后，查询降到 < 50 ms
- 🟡 换乘站同名不同 id 的存储方式一开始想用 `map<int, vector<int>>`，后改为 `unordered_map` 减少哈希开销

### 第二天计划
- 完成 station / graph / pathfinder 三个核心模块的代码

---

## Day 2 — 2026-06-30（周二）

### 当日目标
- 写完三个核心模块
- 跑通核心算法的最小验证

### 完成项
- [x] **`station.h / .cpp`**：实现 `StationManager` 类
  - 加载/保存 CSV、模糊搜索、状态切换、批量更新、一键恢复
  - 健壮处理：UTF-8 BOM / 全角空格 / 空行 / 表头跳过（支持中文"站点名称"与英文"name"两种表头）
- [x] **`graph.h / .cpp`**：实现邻接表
  - `vector<vector<Edge>>` 结构 + `(from, to, line, direction)` 四元组去重
  - 自动识别同名换乘站，构建 5 分钟双向换乘边
  - 4 号线环线内圈/外圈方向显式标注
- [x] **`pathfinder.h / .cpp`**：核心算法
  - 状态扩展 Dijkstra（`StateKey = (id, line)`）
  - Yen K 短路（偏离点 + 受限 Dijkstra + signature 去重）
  - 路径输出格式严格按要求：`起点 -> 途经站(线路) -> ... -> 换乘站 换乘至 [新线路] -> ... -> 终点`
  - 受影响区域分析（受影响线路 + 同线直接相邻站点）
  - 网络连通性分析（DFS 统计开放站点连通分量）
- [x] **第一次跑通测试**：
  - 莘庄 → 人民广场：13 站 / 34 min / 0 换乘 ✓
  - 莘庄 → 陆家嘴：16 站 / 45 min / 1 换乘 ✓
  - K 短路：3 条不同路径 ✓
  - **关键边界测试**：关闭 人民广场(1号线) 后，系统**自动绕行**到 12 号线 ✓

### 遇到的问题
- 🟡 K 短路中出现"1号线 → 换乘 → 2号线"的等价路径（Edge.csv 中 "换乘" 伪线路），通过 `signature()` 字符串哈希去重解决
- 🟡 路径中的"途经站数"会因换乘拆分而多算 1，最后加了 `actualStopCnt = stopCnt - transferCnt` 修正
- 🟡 Dijkstra 方向字段一开始未填充导致去重失效，修复后正常

### 第三天计划
- 对照测试文档补齐 §3.3 各模块接口命名
- 加建站管理（可选加分项）
- 数据迁到 `data/` 子目录
- 改造编译方式（Unity Build + 演示模式）

---

## Day 3 — 2026-07-01（周三）

### 当日目标
- 完善 §3.3 命名 + 可选加分项
- 优化编译和运行方式
- 开发 Web 版本（FastAPI + 静态前端）

### 完成项
- [x] **数据结构调整**：把 4 个 CSV 移到 `data/` 子目录，符合 §3.5 项目结构
- [x] **§3.3 规范命名对齐**：
  - `batchUpdateFromCSV(csvPath)` —— 批量更新（返回更新条数）
  - `restoreInitialStatus()` —— 一键恢复
  - `showClosedStations(ostream&)` —— 打印关闭站点
  - `showStationsByLine(line, ostream&)` —— 按线路打印
- [x] **§3.3 路径输出增强**：
  - `Path` 新增 `actualStopCnt` 字段（实际经过站数 = `stopCnt - transferCnt`）
  - `toPrettyString` 输出"实际经过: N 站"
- [x] **§3.3 5) 建站管理（可选加分项）**：
  - `addStation(name, line, status)` / `removeStation(name, line)`
  - `Graph::addEdge(fromId, toId, line, time)`
  - 菜单集成建站管理功能
- [x] **编译方式改造**：
  - `main.cpp` 加入 **Unity Build**：`g++ main.cpp` 一行直接编译
  - 加 `-DNO_UNITY_BUILD` 开关：切换传统多文件编译
  - 加 `--demo` / `-d` 参数：一键跑预设测试用例（不需要交互）
  - 加 `--help` / `-h` 参数：显示用法
- [x] **菜单结构对齐测试文档**：
  - 主菜单：1.站点管理 2.时间最短路径 3.换乘最少路径 4.退出
  - 路径规划拆分为独立的"时间最短"和"换乘最少"子菜单
  - 站点管理子菜单：批量更新 / 手工更新 / 关闭站点 / 恢复初始 / 线路站点 / 换乘站关闭 / 线路停运 / 全网停运 / 影响分析 / 连通性分析
  - 菜单输入严格校验（只允许 `[+/-]` + 数字，拒绝小数/字母/符号）
  - 手工更新支持循环操作，输入 `exit` 退出
- [x] **Web 版本全栈开发**：
  - `web/main_web.cpp`：Web 入口，接收命令行参数，输出结构化文本（OK/ERR + key=value + BEGIN/END 块）供 Python 解析
  - `web/server.py`：FastAPI 桥接服务（端口 3724），subprocess 调用 main_web.exe，解析结构化输出转 JSON
  - `web/index.html`：静态前端，路径规划时间线可视化 + 站点管理 + 运营管理 + 网络分析
  - `web/start_web.bat`：一键启动脚本（自动检测 g++/python，编译 C++ 入口，安装 FastAPI 依赖，启动服务）
  - **13 个 API 端点**：query / station toggle / stations all / stations closed / stations line / lines / close-transfer / line toggle / network toggle / reset / analyze impact / analyze network / healthz
- [x] **StationManager 静默版本方法**（供 Web 子进程调用，避免污染协议输出）：
  - `restoreInitialStatusSilent()` / `closeTransferStationSilent(name)` / `closeLineStationsSilent(line)` / `openLineStationsSilent(line)` / `closeAllStationsSilent()` / `openAllStationsSilent()`
- [x] **运营管理增强**：
  - 换乘站整体关闭（关闭所有同名站点）
  - 线路停运/恢复（批量切换该线路所有站点状态）
  - 全网停运/恢复
  - 所有状态变更自动同步回 `data/Station.csv`（带 UTF-8 BOM，防止 Windows 记事本乱码）
- [x] **Web 前端优化**：
  - 路径规划时间线组件：绿色起点 / 蓝色途经 / 橙色换乘 / 粉色终点 + 灰色主线 + 线路徽章（上海地铁官方配色）+ 分段时间
  - 站点列表按线路排序，换乘信息括号标注
  - K=3 多路径独立渲染
  - 网络分析下拉框按站点输入动态过滤线路
- [x] **代码注释重构**：
  - 所有 `.h/.cpp` 文件注释精简：每个大函数/方法/类前仅保留一行简单注释
  - 删除繁琐的行内注释和分隔线
- [x] **文档重写**：
  - `README.md` 重写，反映当前代码库（web/ 目录、13 个 API、C++11 编译命令、完整菜单结构、路径输出格式、数据规模、技术要点）
  - `WORK_LOG.md` 重新编撰，框架不变，细节按现有代码更新

### 遇到的问题
- 🟡 静态函数 `rebuildStationsIndex` 在 `loadFromCSV` 中调用，但定义在文件后面，编译报错。修复：把静态函数移到文件顶部
- 🟡 `main.cpp` 整体结构里没区分"数据加载"和"演示模式"，加了一个 `runDemo()` 静态函数统一演示用例
- 🟡 Web 版 `main_web.cpp` 的 Windows argv 是 ANSI 编码，中文站名乱码。解决：加 `ansiToUtf8()` 用 `MultiByteToWideChar` + `WideCharToMultiByte` 转换
- 🟡 Web 版 `reset` 命令输出第一行必须是 `OK`，否则 Python 解析失败。解决：所有 cmd* 函数严格遵循 `OK\n` + key=value 协议
- 🟡 Web 站点状态操作一度导致 `Station.csv` 编码损坏。解决：写文件时显式写入 UTF-8 BOM
- 🟡 `std::stoi` 解析菜单输入会把 `1.5` 截断成 1，改用手动字符串校验（只允许 `[+/-]` + 数字）
- 🟡 网络分析中"孤立/无邻站点"指标恒为 0，移除该指标，把"波及线路"移到影响分析子模块

### 第四天计划
- 写实验报告（设计思路 / 复杂度分析 / 测试用例）
- 制作系统截图（用 demo 模式输出 + 菜单交互 + Web 界面）
- 整理交付物清单

---

## Day 4 — 2026-07-02（周四）

### 当日目标
- 收尾：报告 + 截图 + 文档

### 待完成项
- [ ] 实验报告：
  - §1 设计思路（邻接表 / Dijkstra 变种 / K 短路 / 剪枝 / 换乘建模）
  - §2 系统架构（CLI + Web 双入口 / 5 个核心模块 / FastAPI 桥接）
  - §3 测试用例 + 截图（CLI demo + Web 界面）
  - §4 复杂度分析
  - §5 心得体会
- [ ] 系统截图：5 类
  1. `g++ main.cpp` Unity Build 编译输出
  2. `--demo` 模式跑出的核心功能截图
  3. 菜单交互（路径查询、K 短路、站点管理）
  4. 边界测试（关闭站点后自动绕行）
  5. Web 界面（路径规划时间线 + 站点管理 + 网络分析）
- [ ] 交付物打包：
  - 源码（GitHub 仓库）
  - 编译/运行说明（README.md）
  - 实验报告 PDF
  - 操作演示视频（可选）

### 风险与注意事项
- 控制台中文输出依赖 UTF-8 编码；Windows 下需保证终端为 UTF-8（程序已自动调用 `SetConsoleOutputCP`）
- Web 版需先编译 `web/main_web.exe` 再启动 `server.py`，`start_web.bat` 已自动处理
- 若实验报告要求"分小组写"，把 4 天的成员分工填入对应小节

---

## 整体统计

| 项目 | 数值 |
|---|---|
| 代码文件 | 5 对 `.h/.cpp` + `main.cpp` + `web/main_web.cpp` + `web/server.py` + `web/index.html`（共 15 个文件） |
| 代码行数 | 约 2500 行（C++） + 约 400 行（Python） + 约 600 行（HTML/JS/CSS） |
| 数据规模 | 523 站点 / 5138 图节点 / 1000+ 条边 |
| 算法性能 | 单条最短路径查询 < 50 ms / K 短路 (K=3) < 200 ms |
| CLI 编译命令 | `g++ -std=c++11 -O2 -Wall -Wextra -o metro.exe main.cpp` |
| Web 编译命令 | `g++ -std=c++11 -O2 -I.. -o main_web.exe web/main_web.cpp`（在 web/ 下执行） |
| Web API 端点 | 13 个（query / toggle / stations / lines / close-transfer / line-toggle / network-toggle / reset / impact / network / healthz） |
| 一键演示 | `metro.exe --demo` |
| 一键启动 Web | `web\start_web.bat` |

---

## Git 提交记录

| Commit | 说明 |
|---|---|
| `959ea61` | Initial commit (LICENSE) |
| `22f8f65` | feat: v1.0 — 核心算法 + 菜单交互 |
| `3045217` | feat: v1.1 — 对齐测试文档规范（数据迁移 / 规范命名 / 建站管理） |
| `96c9f18` | fix: 菜单重复问题，新增站点查询 |
| `78c51b2` | fix: 青浦江问 题修复 |
| `041fc6f` | fix: 站内换乘至[换乘] 人民广场问题修复 |
| `c0fd1a7` | fix: 菜单页面输入小数不会返回异常的问题 |
| `d9fdfb3` | feat: 前端初步完成 |
| `0de6f3e` | feat: 后端功能修复,前端补齐 |
| `09b83fd` | feat: 程序内部站点状态修改会实时同步到csv文件里 |
| `5b76f59` | feat: 网页恢复初始功能 + 补全web页面功能 + 主题风格 |
| `f1d4289` | fix: 变量初始化 / Dijkstra方向填充去重 / readInt EOF处理 / batchUpdate返回值区分 / saveCurrentToCSV简化写法 |
| `c2b6f73` | fix: 修复一些前后端问题 |
| 待提交 | docs: 代码注释重构 + README/WORK_LOG 重写 |

---

> **备注**：本日志按实际工作节奏整理，记录关键决策、问题、结果。实验报告可直接引用本日志的"完成项"和"问题与解决"部分。
