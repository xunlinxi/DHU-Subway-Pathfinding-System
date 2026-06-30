// 路径规划算法与路径格式化输出。
#pragma once

#include "graph.h"
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

struct Path {
  // 节点序列：每个元素是 Station.id（同名换乘站 id 不同）
  std::vector<int> nodes;
  // 段线路：lines[i] 表示从 nodes[i] -> nodes[i+1] 所乘坐的线路
  std::vector<std::string> lines;
  // 段方向：directions[i] 对应 lines[i] 的 4 号线内/外圈信息（其余为空）
  std::vector<std::string> directions;
  // 统计
  int totalTime = 0;   // 总耗时（分钟）
  int transferCnt = 0; // 换乘次数
  int stopCnt = 0;     // 途经站数（含起终点 + 同名换乘拆分后的所有节点）
  int actualStopCnt =
      0; // 实际经过站数（不含换乘拆分冗余）= stopCnt - transferCnt
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

  // 6) 网络连通性分析：使用 DFS 统计连通分量
  struct NetworkInfo {
    int componentCount;                       // 连通分量数
    int totalOpenStations;                    // 当前开放站点数
    int totalClosedStations;                  // 当前关闭站点数
    std::vector<std::vector<int>> components; // 各连通分量的站点 id 列表
    bool isConnected; // 全网是否连通（componentCount == 1）
  };
  NetworkInfo analyzeNetworkConnectivity();

  // 由 Path 中 nodes/lines 重新计算 totalTime / transferCnt / stopCnt
  // 暴露为 public 供 K 短路辅助函数使用
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
