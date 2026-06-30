// =============================================================
//  graph.h
//  邻接表图结构：维护同线边 + 自动生成换乘边
//  - Edge: 表示一条 (from -> to, line, time)
//  - Graph: 装载 StationManager 与 Edge.csv，提供构建 / 查询接口
// =============================================================
#pragma once

#include "station.h"
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>


// 邻接表中的"边"
struct Edge {
  int to;           // 终点站 id
  std::string line; // 所属线路
  int time;         // 通行时间（分钟）
  // 便于按 (to, line) 唯一标识一条边：换乘和同线用同一结构存储
  bool operator==(const Edge &other) const {
    return to == other.to && line == other.line;
  }
};

// 换乘时间常量：题目要求 5 分钟
const int TRANSFER_TIME = 5;

class Graph {
public:
  // 构造时绑定站点管理器
  explicit Graph(StationManager &sm) : stationMgr_(sm) {}

  // ---- 构建 ----
  // 从 Edge.csv 加载同线边
  bool loadEdgesFromCSV(const std::string &csvPath);
  // 自动扫描所有同名站点，添加换乘边（双向、权重 = TRANSFER_TIME）
  void buildTransferEdges();
  // 一键完成：清空 + 加载同线边 + 建换乘边
  bool build(const std::string &edgeCsvPath);

  // ---- 查询 ----
  // 邻接表（id -> 出边列表）
  const std::vector<std::vector<Edge>> &adjList() const { return adj_; }
  // 取某站点所有出边
  const std::vector<Edge> &neighbors(int id) const { return adj_[id]; }
  // 同线相邻（同一条线路上的"前/后一站"）
  std::vector<int> sameLineNeighbors(int id) const;
  // 换乘相邻（通过换乘边直接相连的其他线路同名站）
  std::vector<int> transferNeighbors(int id) const;
  // 站点总数（含已关闭）
  int nodeCount() const { return (int)adj_.size(); }

  // ---- 工具 ----
  // 返回站点所在的所有线路（去重）
  std::vector<std::string> getLinesOfStation(int id) const;
  // 简单可视化：把邻接表写到 cout（调试用）
  void debugPrint() const;

private:
  StationManager &stationMgr_;
  std::vector<std::vector<Edge>> adj_; // 邻接表：外层下标 = 起点 id
  // 缓存：同名站两两换乘对，避免重复加边
  std::set<std::pair<int, int>> transferPairs_;
};
