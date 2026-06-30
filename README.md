# DHU-Subway-Pathfinding-System

东华大学数据结构课程设计项目：上海地铁路径规划与管理系统。

## 当前项目结构

```text
.
├── main.cpp
├── station.h / station.cpp
├── graph.h / graph.cpp
├── pathfinder.h / pathfinder.cpp
├── menu.h / menu.cpp
├── data/
│   ├── Station.csv
│   ├── Station_init.csv
│   ├── Edge.csv
│   └── update_station_status.csv
├── web/
│   └── index.html
├── 小组任务指导书.md
├── 小组任务测试文档.md
└── WORK_LOG.md
```

## 当前能力

- 站点数据加载与状态管理
- 单条最短时间路径规划
- 单条最少换乘路径规划
- 前 3 条候选路径查询
- 关闭站点后的路径规避
- 4 号线内圈 / 外圈显示
- 受关闭站点影响分析
- 静态网页展示入口

## 编译

Unity Build:

```bash
g++ -std=c++17 -Wall -Wextra -O2 main.cpp -o metro.exe
```

传统多文件编译:

```bash
g++ -std=c++17 -Wall -Wextra -O2 -DNO_UNITY_BUILD main.cpp station.cpp graph.cpp pathfinder.cpp menu.cpp -o metro.exe
```

## 运行

交互菜单:

```bash
.\metro.exe
```

演示模式:

```bash
.\metro.exe --demo
```

静态网页:

- 直接用浏览器打开 [web/index.html](web/index.html)
- 或在项目根目录启动静态服务器后访问 `/web/index.html`

## 当前已知情况

- 控制台主流程可运行，但命名、注释和模块边界仍在持续清理中。
- `web/index.html` 当前是静态演示页面，不直接调用本地 C++ 可执行文件。
- README 只描述当前工作区真实存在的内容，不再沿用外部参考项目的旧目录说明。
