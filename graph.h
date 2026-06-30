// 邻接表图结构，负责线路边与站内换乘边。
#pragma once

#include "station.h"
#include <iosfwd>
#include <set>
#include <string>
#include <vector>

struct Edge {
  int to;                    // 终点站 id
  std::string line;          // 所属线路
  std::string direction;     // 运行方向，例如 "内圈" / "往徐家汇"
  int time;                  // 通行时间（分钟）

  bool operator==(const Edge &other) const {
    return to == other.to && line == other.line &&
           direction == other.direction;
  }
};

constexpr int kTransferMinutes = 5;

class Graph {
public:
  explicit Graph(StationManager &stationManager)
      : stationManager_(stationManager) {}

  bool loadEdgesFromCSV(const std::string &csvPath);
  void buildTransferEdges();
  bool build(const std::string &edgeCsvPath);
  void rebuild(const std::string &edgeCsvPath);

  bool addEdge(int fromId, int toId, const std::string &line, int timeMin,
               const std::string &direction = "");
  void clear();

  const std::vector<std::vector<Edge>> &adjList() const { return adjacencyList_; }
  const std::vector<Edge> &neighbors(int id) const { return adjacencyList_[id]; }
  std::vector<int> sameLineNeighbors(int id) const;
  std::vector<int> transferNeighbors(int id) const;
  int nodeCount() const { return (int)adjacencyList_.size(); }

  std::vector<std::string> getLinesOfStation(int id) const;
  void debugPrint(std::ostream &output) const;
  const StationManager &stationManager() const { return stationManager_; }

private:
  StationManager &stationManager_;
  std::vector<std::vector<Edge>> adjacencyList_;
  std::set<std::pair<int, int>> transferPairs_;
};
