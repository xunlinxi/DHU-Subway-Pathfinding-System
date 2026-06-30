// 站点数据与运营状态管理。
#pragma once

#include <iosfwd>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct Station {
  int id;             // 站点唯一编号
  std::string name;   // 站点名称
  std::string line;   // 所属线路，例如 "1号线"
  std::string status; // 运营状态："开启" / "关闭"
};

class StationManager {
public:
  bool loadFromCSV(const std::string &csvPath);
  bool loadInitFromCSV(const std::string &csvPath);
  // 从 update_station_status.csv 批量更新
  // 支持两种格式：id,status 或 name,line,status
  // 返回成功更新的站点数量（-1 表示文件打开失败）
  int batchUpdateFromCSV(const std::string &csvPath);
  std::vector<Station> fuzzySearch(const std::string &keyword) const;
  std::vector<Station> getStationsByName(const std::string &name) const;
  const Station *findById(int id) const;
  std::vector<Station> getClosedStations() const;
  std::vector<Station> getStationsByLine(const std::string &line) const;
  std::vector<std::string> getAllLines() const;

  bool setStationStatus(const std::string &name, const std::string &line,
                        const std::string &newStatus);
  void restoreInitialStatus();
  bool saveCurrentToCSV(const std::string &csvPath) const;

  // ---- §3.3 路径状态信息管理与可视化模块 ----
  // 规范名: updateStationStatus / saveStationStatus / showClosedStations /
  // showStationsByLine （与上方方法语义一致，仅为对齐规范命名）
  bool updateStationStatus(const std::string &csvPath) {
    return batchUpdateFromCSV(csvPath) >= 0;
  }
  bool saveStationStatus(const std::string &csvPath) const {
    return saveCurrentToCSV(csvPath);
  }
  void showClosedStations(std::ostream &os = std::cout) const;
  void showStationsByLine(const std::string &line,
                          std::ostream &os = std::cout) const;
  int addStation(const std::string &name, const std::string &line,
                 const std::string &status = "开启");
  bool removeStation(const std::string &name, const std::string &line);
  int nextStationId() const;

  // ---- 运营管理扩展 ----
  // 换乘站整体关闭：关闭所有同名站点（跨线路），返回关闭数量
  int closeTransferStation(const std::string &name);
  // 线路停运/恢复：关闭或开启指定线路上的所有站点
  int closeLineStations(const std::string &line);
  int openLineStations(const std::string &line);
  // 全网停运/恢复
  int closeAllStations();
  int openAllStations();
  // 判断站点是否为换乘站（同名出现在多条线路）
  bool isTransferStation(const std::string &name) const;

  // ---- 辅助 ----
  // 解析一行 CSV（处理引号、逗号），返回字段数组
  static std::vector<std::string> parseCSVLine(const std::string &line);
  static std::string trim(const std::string &s);

  const std::vector<Station> &allStations() const { return stations_; }
  bool isClosed(int id) const;

private:
  std::vector<Station> stations_; // 当前站点状态
  std::unordered_map<int, size_t> indexById_;
  std::unordered_map<std::string, std::vector<size_t>> indexesByName_;
  std::vector<Station> initialSnapshot_;
};
