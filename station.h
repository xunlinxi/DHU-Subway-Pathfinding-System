// =============================================================
//  station.h
//  站点信息建模与状态管理
//  - 维护 Station.csv / Station_init.csv / update_station_status.csv
//  - 提供模糊搜索、状态切换、受影响区域分析等接口
//  作者：俞道松 (课程设计)
// =============================================================
#pragma once

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


// 单个站点的数据结构（邻接表中的"顶点"）
struct Station {
  int id;             // 站点唯一编号（同名换乘站分属不同线路，id 不同）
  std::string name;   // 站点名（中文）
  std::string line;   // 所属线路，如 "1号线"
  std::string status; // "开启" / "关闭"
};

// 站点管理器：负责加载 CSV、查询、状态修改
class StationManager {
public:
  // ---- 加载类 ----
  // 从 Station.csv 加载站点基础信息
  bool loadFromCSV(const std::string &csvPath);
  // 加载一份"初始状态"快照（用于一键恢复）
  bool loadInitFromCSV(const std::string &csvPath);
  // 从 update_station_status.csv 批量更新（按 name+line 匹配）
  bool batchUpdateFromCSV(const std::string &csvPath);

  // ---- 查询类 ----
  // 模糊搜索：返回所有 name 包含关键字的站点
  std::vector<Station> fuzzySearch(const std::string &keyword) const;
  // 全名匹配：同名可能有多条线路的换乘站，返回所有候选
  std::vector<Station> getStationsByName(const std::string &name) const;
  // 按 id 精确查询
  const Station *findById(int id) const;
  // 获取所有当前处于"关闭"状态的站点
  std::vector<Station> getClosedStations() const;
  // 获取指定线路上的所有站点（按 id 升序）
  std::vector<Station> getStationsByLine(const std::string &line) const;

  // ---- 状态修改类 ----
  // 关闭 / 开启 指定 (name, line) 的站点
  bool setStationStatus(const std::string &name, const std::string &line,
                        const std::string &newStatus);
  // 恢复全部站点为初始快照中的状态
  void restoreInitialStatus();
  // 把当前 status 同步回 Station.csv（便于人工查看最新结果）
  bool saveCurrentToCSV(const std::string &csvPath) const;

  // ---- §3.3 路径状态信息管理与可视化模块 ----
  // 规范名: updateStationStatus / saveStationStatus / showClosedStations / showStationsByLine
  // （与上方方法语义一致，仅为对齐规范命名）
  bool   updateStationStatus(const std::string &csvPath) { return batchUpdateFromCSV(csvPath); }
  bool   saveStationStatus  (const std::string &csvPath) const { return saveCurrentToCSV(csvPath); }
  void   showClosedStations(std::ostream &os = std::cout) const;
  void   showStationsByLine(const std::string &line, std::ostream &os = std::cout) const;

  // ---- §3.3 建站管理（可选加分）----
  // 新增站点：返回新站 id；若同名+线路已存在返回 -1
  int    addStation(const std::string &name, const std::string &line,
                    const std::string &status = "开启");
  // 删除站点：返回是否成功
  bool   removeStation(const std::string &name, const std::string &line);
  // 当前最大 id（用于自动分配新 id）
  int    nextStationId() const;

  // ---- 辅助 ----
  // 解析一行 CSV（处理引号、逗号），返回字段数组
  static std::vector<std::string> parseCSVLine(const std::string &line);
  // 去掉 UTF-8 字符串首尾空白
  static std::string trim(const std::string &s);

  // 数据访问
  const std::vector<Station> &allStations() const { return stations_; }
  bool isClosed(int id) const;

private:
  std::vector<Station> stations_;          // 当前状态
  std::unordered_map<int, size_t> id2idx_; // id -> stations_ 下标
  std::unordered_map<std::string, std::vector<size_t>>
      name2idx_;                      // name -> 下标集合
  std::vector<Station> initSnapshot_; // 初始状态快照
};
