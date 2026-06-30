// 路径规划算法与路径格式化输出。
#pragma once

#include "graph.h"
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

struct Path {
  std::vector<int> nodes;               // 路径中的站点 id
  std::vector<std::string> lines;       // nodes[i] -> nodes[i+1] 所在线路
  std::vector<std::string> directions;  // nodes[i] -> nodes[i+1] 的方向
  int totalTime = 0;                    // 总耗时（分钟）
  int transferCnt = 0;                  // 换乘次数
  int stopCnt = 0;                      // 途经站数（包含换乘拆分节点）
  int actualStopCnt = 0;                // 实际经过站数（去掉重复换乘节点）
  bool valid = false;

  std::string signature() const;
  std::string toPrettyString(const StationManager &sm) const;
};

enum class OptGoal { TIME, TRANSFER };

class PathFinder {
public:
  PathFinder(Graph &graph, StationManager &stationManager)
      : graph_(graph), stationManager_(stationManager) {}

  Path findBestByTime(int startId, int endId);
  Path findBestByTransfer(int startId, int endId);
  std::vector<Path> findKShortestByTime(int startId, int endId, int K = 3);
  std::vector<Path> findKShortestByTransfer(int startId, int endId, int K = 3);

  struct ImpactInfo {
    std::string name;                       // 被关闭站点名称
    std::string line;                       // 被关闭站点所在线路
    std::vector<int> sameLineAdj;           // 同线直接受影响站点
    std::vector<int> noAdj;                 // 关闭后会失去同线相邻站的站点
    std::vector<std::string> affectedLines; // 受影响线路
    std::string level;                      // 影响等级
  };
  ImpactInfo analyzeImpact(const std::string &name, const std::string &line);

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
