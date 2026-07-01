// station.h - 站点数据与运营状态管理
#pragma once

#include <iosfwd>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// 站点基本信息
struct Station {
  int id;
  std::string name;
  std::string line;
  std::string status;
};

// 站点管理器：加载、查询、更新站点状态
class StationManager {
public:
  bool loadFromCSV(const std::string &csvPath);
  bool loadInitFromCSV(const std::string &csvPath);
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
  bool restoreInitialStatusSilent();
  bool saveCurrentToCSV(const std::string &csvPath) const;

  void showClosedStations(std::ostream &os = std::cout) const;
  void showStationsByLine(const std::string &line,
                          std::ostream &os = std::cout) const;
  int addStation(const std::string &name, const std::string &line,
                 const std::string &status = "开启");
  bool removeStation(const std::string &name, const std::string &line);
  int nextStationId() const;

  // 运营管理扩展：换乘站整体关闭、线路停运、全网停运
  int closeTransferStation(const std::string &name);
  int closeLineStations(const std::string &line);
  int openLineStations(const std::string &line);
  int closeAllStations();
  int openAllStations();
  bool isTransferStation(const std::string &name) const;

  // 静默版本：不打印中文提示，适合子进程调用
  int closeTransferStationSilent(const std::string &name);
  int closeLineStationsSilent(const std::string &line);
  int openLineStationsSilent(const std::string &line);
  int closeAllStationsSilent();
  int openAllStationsSilent();

  static std::vector<std::string> parseCSVLine(const std::string &line);
  static std::string trim(const std::string &s);
  // 将 UTF-8 路径转为系统原生编码（Windows 下转 ANSI，解决中文路径问题）
  static std::string toNativePath(const std::string &utf8Path);

  const std::vector<Station> &allStations() const { return stations_; }
  bool isClosed(int id) const;

private:
  std::vector<Station> stations_;
  std::unordered_map<int, size_t> indexById_;
  std::unordered_map<std::string, std::vector<size_t>> indexesByName_;
  std::vector<Station> initialSnapshot_;
};
