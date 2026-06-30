// 站点数据与运营状态管理。
#pragma once

#include <iosfwd>
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
  bool batchUpdateFromCSV(const std::string &csvPath);

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

  void showClosedStations(std::ostream &output) const;
  void showStationsByLine(const std::string &line, std::ostream &output) const;

  int addStation(const std::string &name, const std::string &line,
                 const std::string &status = "开启");
  bool removeStation(const std::string &name, const std::string &line);
  int nextStationId() const;

  static std::vector<std::string> parseCSVLine(const std::string &line);
  static std::string trim(const std::string &s);

  const std::vector<Station> &allStations() const { return stations_; }
  bool isClosed(int id) const;

private:
  std::vector<Station> stations_; // 当前站点状态
  std::unordered_map<int, size_t> indexById_;
  std::unordered_map<std::string, std::vector<size_t>>
      indexesByName_;
  std::vector<Station> initialSnapshot_;
};
