# DHU-Subway-Pathfinding-System

DHU Data Structures Course Project: Shanghai Subway Pathfinding and Management System.
东华大学数据结构课程设计：上海地铁路径规划与管理系统。

## 项目简介

基于 **现代 C++ (C++11)** 实现的上海地铁网络路径规划系统，使用**邻接表**建模图结构，结合 **优先队列 + Dijkstra 变种 + Yen K 短路算法**，支持：

- 单条最优路径（时间最短 / 换乘最少）
- K 条候选路径（时间最短前 3 条 / 换乘最少前 3 条）
- 动态站点状态管理（开启 / 关闭 / 批量更新 / 一键恢复）
- 受影响区域分析（自动评估影响等级）
- 模糊搜索（部分汉字匹配 + 换乘站 ID 二次选择）

## 数据源

- `Station.csv`：525 个站点（含同名换乘站，按线路拆分 ID）
- `Edge.csv`：同线边 + 数据中显式给出的换乘边
- `Station_init.csv`：初始状态快照（一键恢复用）
- `update_station_status.csv`：批量更新示例

## 项目结构

```
├── main.cpp            主程序入口
├── station.h / .cpp    站点管理
├── graph.h / .cpp      邻接表图结构
├── pathfinder.h / .cpp 核心算法模块
├── menu.h / .cpp       控制台菜单交互
├── Station.csv         站点数据
├── Edge.csv            边数据
├── Station_init.csv    初始状态
├── update_station_status.csv  批量更新示例
└── .gitignore
```

## 编译

```powershell
g++ -std=c++11 -O2 -Wall -Wextra -o metro.exe `
    main.cpp station.cpp graph.cpp pathfinder.cpp menu.cpp
```

## 运行

```powershell
.\metro.exe
```

## 技术栈

| 类别 | 实现 |
|---|---|
| 图结构 | `vector<vector<Edge>>` 邻接表 |
| 最短路径 | 状态扩展 Dijkstra（`StateKey = (id, line)`） |
| K 短路 | Yen 算法（偏离点 + 受限 Dijkstra） |
| 排序策略 | 双键优先队列（time+transfer） |
| 路径去重 | `signature()` 字符串哈希 |
| 剪枝策略 | 关闭站点 / 过期状态 / blocked 节点 / 禁用边 |

## 作者

数据科学 / 计算机科学 课程设计
