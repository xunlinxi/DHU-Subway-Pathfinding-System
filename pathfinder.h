// pathfinder.h - 路径规划算法与路径格式化输出
#pragma once

#include "graph.h"
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

// 一条完整路径及其统计信息
struct Path {
  std::vector<int> nodes;
  std::vector<std::string> lines;
  std::vector<std::string> directions;
  int totalTime = 0;
  int transferCnt = 0;
  int stopCnt = 0;
  int actualStopCnt = 0;
  bool valid = false;

  std::string signature() const;
  std::string toPrettyString(const StationManager &sm) const;
};

enum class OptGoal { TIME, TRANSFER };

// 路径查找器：Dijkstra 变种与 Yen K 短路
class PathFinder {
public:
  PathFinder(Graph &graph, StationManager &stationManager)
      : graph_(graph), stationManager_(stationManager) {}

  Path findBestByTime(int startId, int endId);
  Path findBestByTransfer(int startId, int endId);
  std::vector<Path> findKShortestByTime(int startId, int endId, int K = 3);
  std::vector<Path> findKShortestByTransfer(int startId, int endId, int K = 3);

  // 受影响区域分析结果
  struct ImpactInfo {
    std::string name;
    std::string line;
    std::vector<int> sameLineAdj;
    std::vector<std::string> affectedLines;
  };
  ImpactInfo analyzeImpact(const std::string &name, const std::string &line);

  // 网络连通性分析结果（DFS 统计连通分量）
  struct NetworkInfo {
    int componentCount = 0;
    int totalOpenStations = 0;
    int totalClosedStations = 0;
    std::vector<std::vector<int>> components;
    bool isConnected = false;
  };
  NetworkInfo analyzeNetworkConnectivity();

  static void finalizeStats(Path &p, const Graph &g);
  Graph &graph() { return graph_; }
  Path dijkstraFull(int startId, int endId, OptGoal goal);
  Path dijkstraConstrained(int startId, int endId, OptGoal goal,
                           const std::unordered_set<int> &blockedNodes,
                           int bannedFrom, int bannedTo,
                           const std::string &bannedLine);

private:
  Graph &graph_;
  StationManager &stationManager_;
};
