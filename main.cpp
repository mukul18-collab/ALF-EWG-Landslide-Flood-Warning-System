#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace DisasterCore {
namespace Sensors {

class SensorFailureException : public std::exception {
public:
    const char* what() const noexcept override { return "Sensor failure: telemetry could not be read."; }
};
class TelemetryChecksumMismatch : public std::exception {
public:
    const char* what() const noexcept override { return "Telemetry checksum mismatch detected."; }
};
class ThresholdOverflowException : public std::exception {
public:
    const char* what() const noexcept override { return "Hazard threshold value is outside the supported range."; }
};

struct GeoCoordinate {
    double latitude{0.0}, longitude{0.0};
};

// Call-by-reference update and return-by-reference for repeated coordinate work.
GeoCoordinate& normalizeCoordinate(GeoCoordinate& point) {
    while (point.latitude > 90.0) point.latitude -= 180.0;
    while (point.latitude < -90.0) point.latitude += 180.0;
    while (point.longitude > 180.0) point.longitude -= 360.0;
    while (point.longitude < -180.0) point.longitude += 360.0;
    return point;
}

inline double clampRisk(double value) {
    if (value < 0.0 || value > 100.0) throw ThresholdOverflowException();
    return value;
}

// Overloaded hazard-index calculations for two telemetry combinations.
inline double computeHazardIndex(double rainfallMmPerHour, double slopeAngleDegrees) {
    return clampRisk(std::min(100.0, rainfallMmPerHour * 0.65 + slopeAngleDegrees * 0.55));
}
inline double computeHazardIndex(double soilSaturationPercent, double vibrationHz, bool /*soilVibrationMode*/) {
    return clampRisk(std::min(100.0, soilSaturationPercent * 0.55 + vibrationHz * 4.0));
}

class PowerSubsystem {
    double batteryPercent_;
public:
    explicit PowerSubsystem(double battery = 100.0) : batteryPercent_(battery) {}
    bool hasPower() const { return batteryPercent_ > 5.0; }
    double battery() const { return batteryPercent_; }
    void consume(double amount) { batteryPercent_ = std::max(0.0, batteryPercent_ - amount); }
};

// Root of the virtual-inheritance diamond.
class DisasterTelemetryDevice {
protected:
    std::string deviceId_;
public:
    explicit DisasterTelemetryDevice(std::string id = "NODE-0") : deviceId_(std::move(id)) {}
    virtual ~DisasterTelemetryDevice() = default;
    const std::string& deviceId() const { return deviceId_; }
};
class SolarPoweredNode : virtual public DisasterTelemetryDevice {
protected:
    double solarChargeRate_{12.0};
public:
    void harvestSolarEnergy() { solarChargeRate_ += 0.5; }
};
class WirelessLoRaTransceiver : virtual public DisasterTelemetryDevice {
protected:
    int signalStrength_{4};
public:
    bool transmit() const { return signalStrength_ > 0; }
};

class BaseSensorNode {
protected:
    PowerSubsystem power_; // containership: independent object inside the sensor node
public:
    explicit BaseSensorNode(double battery = 100.0) : power_(battery) {}
    virtual ~BaseSensorNode() = default;
    virtual double readTelemetry() = 0;
    virtual bool evaluateThreshold(double threshold) const = 0;
    virtual std::string sensorType() const = 0;
    double batteryLevel() const { return power_.battery(); }
};

class TelemetryAuditor; // friend declaration

class AutonomousHybridProbe : public BaseSensorNode,
                              public SolarPoweredNode,
                              public WirelessLoRaTransceiver {
    double rainfall_, slope_, saturation_, vibration_;
    bool simulatedFailure_;
    friend class TelemetryAuditor;
public:
    AutonomousHybridProbe(const std::string& id, double rainfall, double slope,
                          double saturation, double vibration, bool fail = false)
        : DisasterTelemetryDevice(id), BaseSensorNode(95.0),
          rainfall_(rainfall), slope_(slope), saturation_(saturation),
          vibration_(vibration), simulatedFailure_(fail) {}

    double readTelemetry() override {
        if (simulatedFailure_ || !power_.hasPower() || !transmit()) throw SensorFailureException();
        power_.consume(0.25);
        return computeHazardIndex(rainfall_, slope_) * 0.5 +
               computeHazardIndex(saturation_, vibration_, true) * 0.5;
    }
    bool evaluateThreshold(double threshold) const override {
        return (rainfall_ > 0 && (rainfall_ * 0.65 + slope_ * 0.55) >= threshold) ||
               (saturation_ * 0.55 + vibration_ * 4.0) >= threshold;
    }
    std::string sensorType() const override { return "Autonomous Hybrid Probe"; }
};

class TelemetryAuditor {
public:
    static void inspect(const AutonomousHybridProbe& probe) {
        std::cout << "[AUDIT] " << probe.deviceId_
                  << " raw rainfall=" << probe.rainfall_
                  << " mm/h, slope=" << probe.slope_
                  << " deg, saturation=" << probe.saturation_
                  << "%, vibration=" << probe.vibration_ << " Hz\n";
    }
};

// Dynamic memory buffer with parameterized/dynamic construction, deep copy and destructor.
class TelemetryBuffer {
    double* values_;
    std::size_t size_;
public:
    explicit TelemetryBuffer(std::size_t size)
        : values_(new double[size]()), size_(size) {}
    TelemetryBuffer(const double* source, std::size_t size)
        : values_(new double[size]), size_(size) {
        std::copy(source, source + size, values_);
    }
    TelemetryBuffer(const TelemetryBuffer& other)
        : values_(new double[other.size_]), size_(other.size_) {
        std::copy(other.values_, other.values_ + size_, values_);
    }
    TelemetryBuffer& operator=(const TelemetryBuffer& other) {
        if (this != &other) {
            double* fresh = new double[other.size_];
            std::copy(other.values_, other.values_ + other.size_, fresh);
            delete[] values_;
            values_ = fresh;
            size_ = other.size_;
        }
        return *this;
    }
    ~TelemetryBuffer() { delete[] values_; }
    double& operator[](std::size_t index) { return values_[index]; }
    double operator[](std::size_t index) const { return values_[index]; }
    std::size_t size() const { return size_; }
};

class SensorStation {
    std::string stationId_;
    GeoCoordinate location_;
    double hazard_;
    static int activeStations_;
public:
    SensorStation(std::string id = "ST-0", GeoCoordinate location = {}, double hazard = 0.0)
        : stationId_(std::move(id)), location_(location), hazard_(clampRisk(hazard)) { ++activeStations_; }
    SensorStation(const SensorStation& other)
        : stationId_(other.stationId_), location_(other.location_), hazard_(other.hazard_) { ++activeStations_; }
    ~SensorStation() { --activeStations_; }
    const std::string& id() const { return stationId_; }
    double hazard() const { return hazard_; }
    void setHazard(double value) { hazard_ = clampRisk(value); }
    static int activeCount() { return activeStations_; }

    SensorStation operator+(const SensorStation& rhs) const {
        return SensorStation(stationId_ + "+" + rhs.stationId_, location_,
                             std::min(100.0, (hazard_ + rhs.hazard_) / 2.0));
    }
    bool operator>(const SensorStation& rhs) const { return hazard_ > rhs.hazard_; }
    friend std::ostream& operator<<(std::ostream& out, const SensorStation& station) {
        return out << station.stationId_ << ' ' << station.location_.latitude << ' '
                   << station.location_.longitude << ' ' << station.hazard_;
    }
    friend std::istream& operator>>(std::istream& in, SensorStation& station) {
        in >> station.stationId_ >> station.location_.latitude
           >> station.location_.longitude >> station.hazard_;
        station.hazard_ = clampRisk(station.hazard_);
        return in;
    }
};
int SensorStation::activeStations_ = 0;

} // namespace Sensors

namespace Alerts {
using Sensors::SensorStation;
class EarlyWarningSiren {
    std::string region_;
    bool active_{false};
    static int globalAlertLevel_;
public:
    explicit EarlyWarningSiren(std::string region) : region_(std::move(region)) {}
    void activate(int level) {
        if (level < 0 || level > 3) throw Sensors::ThresholdOverflowException();
        globalAlertLevel_ = std::max(globalAlertLevel_, level);
        active_ = level > 0;
        if (active_) std::cout << "\n*** SIREN ALERT: " << region_
                               << " | LEVEL " << level << " ***\n";
    }
    bool isActive() const { return active_; }
    static int globalAlertLevel() { return globalAlertLevel_; }
};
int EarlyWarningSiren::globalAlertLevel_ = 0;
} // namespace Alerts
} // namespace DisasterCore

namespace DisasterCore {
namespace Sensors {
struct IncidentRecord {
    char stationId[32];
    double hazard;
    int alertLevel;
    std::int64_t timestamp;
};
}
}

class IncidentLogger {
    std::string fileName_;
public:
    explicit IncidentLogger(std::string fileName) : fileName_(std::move(fileName)) {}
    bool append(const DisasterCore::Sensors::IncidentRecord& record) {
        using namespace DisasterCore::Sensors;
        std::fstream file(fileName_, std::ios::binary | std::ios::in | std::ios::out);
        if (!file) {
            std::ofstream create(fileName_, std::ios::binary);
            create.close();
            file.open(fileName_, std::ios::binary | std::ios::in | std::ios::out);
        }
        if (!file) return false;
        file.seekp(0, std::ios::end);
        const std::streampos rollbackPosition = file.tellp();
        file.write(reinterpret_cast<const char*>(&record), sizeof(record));
        if (!file) { // transactional rollback to previous file end
            file.clear();
            file.seekp(rollbackPosition);
            file.flush();
            return false;
        }
        file.flush();
        file.seekg(rollbackPosition); // demonstrate seekg-based read positioning
        return static_cast<bool>(file);
    }
    std::vector<DisasterCore::Sensors::IncidentRecord> readAll() const {
        using namespace DisasterCore::Sensors;
        std::vector<IncidentRecord> records;
        std::ifstream file(fileName_, std::ios::binary);
        IncidentRecord item{};
        while (file.read(reinterpret_cast<char*>(&item), sizeof(item))) records.push_back(item);
        return records;
    }
};

struct AlertPriority {
    bool operator()(const DisasterCore::Sensors::SensorStation& a,
                    const DisasterCore::Sensors::SensorStation& b) const {
        return a.hazard() > b.hazard();
    }
};

std::ostream& alertTable(std::ostream& out) {
    out << std::left << std::setw(14) << "STATION"
        << std::setw(14) << "HAZARD(%)" << std::setw(14) << "STATUS";
    return out;
}

int main() {
    using namespace DisasterCore::Sensors;
    using namespace DisasterCore::Alerts;

    std::cout << "===============================================\n"
              << " ALF-EWG: AUTONOMOUS LANDSLIDE & FLASH-FLOOD GRID\n"
              << "===============================================\n";

    GeoCoordinate site{91.2, 181.4};
    GeoCoordinate& normalized = normalizeCoordinate(site);
    std::cout << "Normalized monitoring coordinate: "
              << normalized.latitude << ", " << normalized.longitude << "\n\n";

    AutonomousHybridProbe probe("HIMALAYA-01", 82.0, 38.0, 76.0, 8.0);
    TelemetryAuditor::inspect(probe); // friend access to private raw metrics

    std::vector<BaseSensorNode*> sensorNetwork;
    sensorNetwork.push_back(&probe);
    std::vector<SensorStation> stations;
    std::map<std::string, double> regionalRisk;

    EarlyWarningSiren northSiren("North Himalayan Zone");
    IncidentLogger logger("incident_log.bin");

    for (BaseSensorNode* node : sensorNetwork) {
        try {
            const double hazard = node->readTelemetry(); // runtime virtual dispatch
            std::cout << "\nSensor: " << node->sensorType()
                      << " | Battery: " << node->batteryLevel() << "%\n";
            SensorStation station("ST-ALPHA", {30.2, 78.1}, hazard);
            stations.push_back(station);
            regionalRisk["North-Himalaya"] = hazard;

            const int level = hazard >= 75.0 ? 3 : (hazard >= 50.0 ? 2 : (hazard >= 30.0 ? 1 : 0));
            northSiren.activate(level);

            IncidentRecord incident{};
            std::snprintf(incident.stationId, sizeof(incident.stationId), "%s", station.id().c_str());
            incident.hazard = hazard;
            incident.alertLevel = level;
            incident.timestamp = 1794787200; // illustrative fixed demo timestamp
            if (!logger.append(incident)) std::cerr << "Warning: incident could not be saved.\n";
        } catch (const std::exception& error) {
            std::cerr << "Sensor cycle error: " << error.what() << "\n";
        }
    }

    // STL algorithm + custom priority functor.
    std::sort(stations.begin(), stations.end(), AlertPriority{});
    std::cout << "\n" << alertTable << "\n";
    std::for_each(stations.begin(), stations.end(), [](const SensorStation& station) {
        std::cout << std::left << std::setw(14) << station.id()
                  << std::setw(14) << station.hazard()
                  << std::setw(14) << (station.hazard() >= 75 ? "EVACUATE" :
                                      station.hazard() >= 50 ? "WARNING" : "NORMAL") << "\n";
    });

    if (stations.size() >= 1) {
        SensorStation regional = stations[0] + stations[0]; // overloaded binary +
        std::cout << "\nAggregated regional cluster: " << regional << "\n";
        std::cout << "Highest-risk station check (>): "
                  << (regional > stations[0] ? "true" : "false") << "\n";
    }

    // Dynamic buffer and deep-copy demonstration.
    double rawValues[] = {82.0, 38.0, 76.0, 8.0};
    TelemetryBuffer original(rawValues, 4);
    TelemetryBuffer copied(original);
    copied[0] = 12.0;
    std::cout << "\nDynamic buffer deep copy: original[0]=" << original[0]
              << ", copied[0]=" << copied[0] << "\n";

    const auto saved = logger.readAll();
    std::cout << "\nPersisted incident records: " << saved.size() << "\n";
    for (const auto& record : saved) {
        std::cout << "  " << record.stationId << " | hazard=" << record.hazard
                  << " | level=" << record.alertLevel << "\n";
    }

    std::cout << "\nActive sensor stations (including temporary objects): "
              << SensorStation::activeCount()
              << " | Global alert level: " << EarlyWarningSiren::globalAlertLevel() << "\n";
    std::cout << "\nSimulation completed. Check incident_log.bin for persistent records.\n";
    return 0;
}
