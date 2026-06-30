// =============================================================
//  pathfinder.h
//  核心算法模块：单源最短 Dijkstra + Yen's K 短路 + 剪枝
//  - 支持按"时间最短 / 换乘最少"两种目标查询
//  - 支持输出前 K 条候选路径
//  - 自动跳过关闭站点 / 原地换乘循环
// =============================================================
#pragma once

#include "graph.h"
#include <functional>
#include <queue>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

// 一条完整的乘车路径
struct Path {
  // 节点序列：每个元素是 Station.id（同名换乘站 id 不同）
  std::vector<int> nodes;
  // 段线路：lines[i] 表示从 nodes[i] -> nodes[i+1] 所乘坐的线路
  std::vector<std::string> lines;
  // 统计
  int totalTime = 0;   // 总耗时（分钟）
  int transferCnt = 0; // 换乘次数
  int stopCnt = 0;     // 途经站数（含起终点）
  bool valid = false;

  // 路径签名：站点名序列 + 线路段序列，用于去重
  std::string signature() const;
  // 可视化输出：符合实验报告要求的中文格式
  std::string toPrettyString(const StationManager &sm) const;
};

// 优化目标
enum class OptGoal { TIME, TRANSFER };

class PathFinder {
public:
  PathFinder(Graph &g, StationManager &sm) : graph_(g), sm_(sm) {}

  // 1) 单条最优：时间最短，时间相同时换乘更少优先
  Path findBestByTime(int startId, int endId);
  // 2) 单条最优：换乘最少，换乘相同时时间更短优先
  Path findBestByTransfer(int startId, int endId);

  // 3) 时间最短的前 K 条路径（K 短路）
  std::vector<Path> findKShortestByTime(int startId, int endId, int K = 3);
  // 4) 换乘最少的前 K 条路径
  std::vector<Path> findKShortestByTransfer(int startId, int endId, int K = 3);

  // 5) 受影响区域分析：关闭某 (name,line) 站点后，统计影响
  struct ImpactInfo {
    std::string name;                       // 被关闭站点名
    std::string line;                       // 被关闭站点线路
    std::vector<int> sameLineAdj;           // 同线相邻（可能断链）
    std::vector<int> noAdj;                 // 无相邻（孤立点）
    std::vector<std::string> affectedLines; // 受影响线路
    std::string level;                      // "高/中/低"
  };
  ImpactInfo analyzeImpact(const std::string &name, const std::string &line);

  // 由 Path 中 nodes/lines 重新计算 totalTime / transferCnt / stopCnt
  // 暴露为 public 供 K 短路辅助函数使用
  static void finalizeStats(Path &p, const Graph &g);

  // 暴露 graph 引用供辅助函数访问邻接表
  Graph &graph() { return graph_; }

  // 暴露给 K 短路辅助函数使用
  Path dijkstraFull(int startId, int endId, OptGoal goal);
  Path dijkstraConstrained(int startId, int endId, OptGoal goal,
                           const std::unordered_set<int> &blockedNodes,
                           int bannedFrom, int bannedTo,
                           const std::string &bannedLine);

private:
  Graph &graph_;
  // 简易引用：用 graph_ 内部的 stationMgr_ (暴露给 PathFinder)
  StationManager &sm_;
};
