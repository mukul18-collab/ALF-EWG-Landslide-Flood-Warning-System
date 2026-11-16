# PROJECT REPORT
## Autonomous Landslide & Flash-Flood Early Warning Grid (ALF-EWG)

**Submitted by:** Mukul  
**Roll Number:** 15514802725  
**Section:** C789  
**Department:** Computer Science and Engineering  
**Institution:** Maharaja Agrasen Institute of Technology  
**Subject:** Object-Oriented Programming Using C++ (CIC-211)  
**Submission date:** November 2026

---

## 1. Abstract
ALF-EWG is a C++17 educational simulation of a disaster-monitoring backend for landslide and flash-flood-prone regions. The system models a hybrid sensor probe that receives rainfall, slope, soil-saturation and vibration readings. It calculates a hazard index, assigns an alert level, activates a regional siren and stores incident information in a binary file. The implementation demonstrates the four units of the CIC-211 syllabus through namespaces, function overloading, references, friend classes, dynamic memory, operator overloading, virtual inheritance, composition, runtime polymorphism, exceptions, file streams and STL algorithms.

## 2. Problem statement
Mountainous regions can experience intense rainfall, slope instability and sudden changes in river or soil conditions. A monitoring backend must process telemetry, identify elevated risk and preserve an incident history for review. This project demonstrates the software-side workflow using simulated telemetry rather than physical IoT hardware.

## 3. Objectives
- Model an autonomous hybrid telemetry probe.
- Compute a hazard index from multiple sensor parameter combinations.
- Classify risk and activate a regional siren.
- Demonstrate encapsulation, inheritance, polymorphism and operator overloading.
- Store incident records using binary file streams.
- Handle sensor and threshold failures through custom exceptions.

## 4. System design
The main flow is:

1. Normalize the monitoring coordinate.
2. Create an autonomous hybrid probe with simulated sensor readings.
3. Audit private readings through a friend class.
4. Read telemetry through a base-class pointer.
5. Calculate the hazard index and determine alert level.
6. Activate the siren and append an incident record.
7. Sort stations by hazard and display a formatted report.
8. Read saved incident records back from the binary log.

## 5. OOP and syllabus mapping
### Unit I — Core foundations and call semantics
The program uses `DisasterCore::Sensors` and `DisasterCore::Alerts` namespaces. `computeHazardIndex()` is overloaded for rainfall/slope and soil-saturation/vibration inputs. `normalizeCoordinate()` accepts and returns a reference. `TelemetryAuditor` is a friend class of `AutonomousHybridProbe`.

### Unit II — Encapsulation, dynamic memory and operator overloading
`SensorStation`, `EarlyWarningSiren`, `PowerSubsystem` and `TelemetryBuffer` encapsulate their state. Static members track active station instances and the global alert level. `TelemetryBuffer` owns a dynamically allocated array and implements deep copying and destruction. `SensorStation` overloads `+`, `>`, `<<` and `>>`.

### Unit III — Inheritance, containership and polymorphism
`BaseSensorNode` is abstract and declares `readTelemetry()` and `evaluateThreshold()` as pure virtual functions. `SolarPoweredNode` and `WirelessLoRaTransceiver` virtually inherit `DisasterTelemetryDevice`; `AutonomousHybridProbe` derives from both. `BaseSensorNode` contains a `PowerSubsystem` object. A vector of `BaseSensorNode*` invokes the derived implementation at runtime.

### Unit IV — Streams, persistence, exception safety and STL
Custom exception classes derive from `std::exception`. `IncidentLogger` writes fixed-size incident records in binary mode and uses stream positioning for rollback/read positioning. The program uses `vector`, `map`, `sort`, `for_each`, a custom comparator/functor and a custom output manipulator.

## 6. Hazard calculation
The demonstration uses two teaching formulas:

- Rainfall/slope: `min(100, rainfall × 0.65 + slope × 0.55)`
- Soil/vibration: `min(100, saturation × 0.55 + vibration × 4.0)`

The final displayed hazard is the average of these two values. These coefficients are illustrative only and are not validated for operational disaster prediction.

## 7. Alert classification
- Level 0: below 30 — Normal
- Level 1: 30 to below 50 — Advisory
- Level 2: 50 to below 75 — Warning
- Level 3: 75 and above — Evacuation alert

## 8. Testing plan
| Test | Input / action | Expected result |
|---|---|---|
| Normalization | Latitude 91.2, longitude 181.4 | Values normalized into valid ranges |
| Hazard calculation | Rainfall/slope and soil/vibration values | Hazard returned within 0–100 |
| Threshold validation | Hazard outside 0–100 | `ThresholdOverflowException` |
| Friend audit | Inspect hybrid probe | Private raw readings displayed |
| Deep copy | Copy buffer, modify copied element | Original remains unchanged |
| Polymorphism | Call through `BaseSensorNode*` | Derived probe method executes |
| Persistence | Run program twice | Incident records are appended and read |
| Sorting | Multiple station records | Highest hazard appears first |

## 9. Sample execution
The program prints the project title, normalized coordinates, audited raw telemetry, calculated hazard, siren activation, a sorted station table, deep-copy result and persisted incident count. Exact output may vary with the values configured in `src/main.cpp` and with previous contents of `incident_log.bin`.

## 10. Limitations and future scope
This is a classroom simulation with one hybrid probe and fixed demonstration readings. Future work could add real sensor drivers, network ingestion, authentication, calibrated risk models, a database, a map dashboard, multiple regions and alert delivery integrations. The current program must not be used as a real emergency warning system.

## 11. Conclusion
ALF-EWG demonstrates how C++ object-oriented programming and standard-library features can be combined to model a modular telemetry and alerting workflow. It covers the major technical mechanisms specified in the CIC-211 project assignment while keeping the simulation reproducible and understandable.

## 12. References
- CIC-211 Project Assignment, Maharaja Agrasen Institute of Technology, Department of CSE.
- ISO C++ standard library documentation for streams, containers, algorithms and exceptions.
