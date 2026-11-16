# ALF-EWG — Autonomous Landslide & Flash-Flood Early Warning Grid

**Course:** CIC-211 — Object-Oriented Programming Using C++  
**Institution:** Maharaja Agrasen Institute of Technology  
**Language:** Standard C++17

## Submission details
- **Student:** Mukul
- **Roll number:** 15514802725
- **Section:** C789

## Project overview
ALF-EWG simulates a monitoring backend for vulnerable Himalayan regions. A hybrid telemetry probe reads rainfall, slope, soil saturation and seismic vibration values, computes a hazard index, classifies the alert level, activates a regional siren, and stores incident records in a binary log.

## Build and run
From this folder:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic src/main.cpp -o alf_ewg
./alf_ewg
```

On Windows with MinGW:
```powershell
g++ -std=c++17 -Wall -Wextra -pedantic src/main.cpp -o alf_ewg.exe
.lf_ewg.exe
```

The program creates `incident_log.bin` in the current working directory. Delete this file before a fresh demonstration if you want an empty incident history.

## Features mapped to the assignment
### Unit I — Core foundations and call semantics
- Namespaces: `DisasterCore::Sensors` and `DisasterCore::Alerts`.
- Inline hazard utility and overloaded `computeHazardIndex()` functions.
- Call-by-reference and return-by-reference coordinate normalization.
- `TelemetryAuditor` friend class reads private probe metrics.

### Unit II — Encapsulation, dynamic memory and operator overloading
- Encapsulated `SensorStation`, `EarlyWarningSiren`, `PowerSubsystem`, and `TelemetryBuffer`.
- Static station count and global alert level.
- Dynamic buffer, parameterized construction, deep copy constructor, assignment operator and destructor.
- `SensorStation` overloads `+`, `>`, `<<` and `>>`.

### Unit III — Inheritance, containership and polymorphism
- Abstract `BaseSensorNode` has pure virtual `readTelemetry()` and `evaluateThreshold()`.
- `DisasterTelemetryDevice` is the virtual base of `SolarPoweredNode` and `WirelessLoRaTransceiver`; `AutonomousHybridProbe` forms the diamond leaf.
- `PowerSubsystem` is composed inside `BaseSensorNode`.
- `std::vector<BaseSensorNode*>` demonstrates runtime dispatch.

### Unit IV — Streams, persistence, exception safety and STL
- Custom exceptions derive from `std::exception`.
- Binary `std::fstream` incident persistence with `seekp()` rollback positioning and `seekg()` positioning.
- `std::vector`, `std::map`, `std::sort`, `std::for_each`, custom priority functor.
- Custom `alertTable` stream manipulator.

## Demo data and limitations
The values are simulated, not live IoT readings. The timestamp in the demonstration record is illustrative. The hazard formula is a teaching/demo heuristic and must not be used as a real-world warning model. Production deployment would require calibrated sensors, validated geotechnical models, secure communications, redundant power, and authority-approved alert thresholds.

## Before submission
1. Replace student placeholders in `docs/PROJECT_REPORT.md`.
2. Compile and run on the lab/college compiler.
3. Capture your own screenshots of the successful build and output.
4. Verify `incident_log.bin` is created and records can be read back.
5. Push the final source and report to your GitHub repository.
