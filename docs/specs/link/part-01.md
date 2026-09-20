# เอกสารข้อกำหนด Link (ฉบับสมบูรณ์ — ระดับรายละเอียดเทียบเท่า VISSIM)

> เอกสารนี้เป็นข้อกำหนดฉบับเต็มของ **Link** ซึ่งเป็นองค์ประกอบโครงสร้างพื้นฐานที่สุดของเครือข่ายถนน ครอบคลุมตั้งแต่โครงสร้างข้อมูล เรขาคณิต เลน พารามิเตอร์พฤติกรรม การเชื่อมโยงกับ Connector/Signal/Detector ไปจนถึงการคอมไพล์ในระดับ runtime โดยออกแบบให้สอดคล้องกับ Connector spec (Schema v7) และเทียบเท่าระดับรายละเอียดของ VISSIM

---

## สารบัญ

1. ภาพรวมและปรัชญาการออกแบบ
2. โครงสร้างข้อมูล (Authored Object)
3. เรขาคณิตและ Reference Polyline
4. เลน (Lanes) และการจัดเรียง
5. พารามิเตอร์พฤติกรรมของ Link
6. พารามิเตอร์การเปลี่ยนเลนและ Driving Side
7. ลำดับชั้นของ Link (Hierarchy & Priority)
8. Detectors และ Data Collection Points
9. Signal Heads และ Signal Controllers
10. Routing Decisions และ Parking
11. Nodes, Intersections และ Stop Lines
12. ความกว้าง ขอบเขต และเครื่องหมาย
13. การสร้างและแก้ไขผ่าน Editor
14. Command Contract และการอ้างอิง
15. โครงสร้าง Project JSON (Schema v7)
16. Runtime Compilation และ Simulation Semantics
17. การตรวจสอบ (Validation) และรหัสวินิจฉัย
18. การแสดงผล (Visualization)
19. ข้อจำกัดและแผนงานในอนาคต
20. Source Map
21. ภาคผนวก: ตัวอย่าง Link ใน scenario จริง

---

## 1. ภาพรวมและปรัชญาการออกแบบ

**Link** คือ **ท่อนถนนแบบมีทิศทาง (directed road segment)** ที่เป็นหน่วยพื้นฐานของเครือข่าย ประกอบด้วย:

- เส้น centerline (reference polyline) ที่กำหนดรูปทรง
- หน้าตัด (cross-section) ที่แบ่งเป็นเลน
- ข้อมูลการเชื่อมต่อ (เริ่มต้น/สิ้นสุด ที่ Node)
- พารามิเตอร์พฤติกรรม, สัญญาณ, detector และอื่นๆ

Connector ใช้เชื่อม Link เข้าด้วยกัน ส่วน Link เองใช้แทน **ถนนจริง** ที่รถวิ่งไปตามความยาว

### 1.1 ปรัชญาการออกแบบ (Design Philosophy)

| หลักการ | คำอธิบาย |
| :--- | :--- |
| Reference-based geometry | ทุกอย่างอ้างอิงจาก reference polyline เดียว |
| Cross-section consistency | ทุกตำแหน่งตามยาวมีหน้าตัดที่ define ได้ |
| Station = 1D coordinate | ใช้ station (ระยะตาม reference) เป็นพิกัดหลัก |
| Lane as first-class entity | เลนเป็น object ที่มี ID, width, marking, connectivity |
| Derived-from-base | เก็บ base, ที่เหลือ derive (เช่น section, offset geometry) |
| Deterministic compilation | Compile จาก authored data → runtime sections แบบ deterministic |
| Directional | Link มีทิศทางชัดเจน ต้องมีคู่ (หรือคู่เสมือน) สำหรับสองทิศทาง |

### 1.2 ความสัมพันธ์กับ Connector

```
Node A ──[Link 1: eastbound]──> Node B ──[Connector]──> Node B ──[Link 2]──> ...
                    ▲                                            ▲
                    │                                            │
              Authoring unit                              Authoring unit
```

- Link = ถนนจริง (ยาว)
- Connector = จุดเชื่อม (สั้น, ที่ทางแยก)
- ทั้งสองมี reference polyline, lane, width, station
- ต่างกัน: Link ไม่มี priority rule (ปกติ) แต่มี detector, parking, signal
- Connector อ้าง Link ผ่าน `linkId` + `laneId` + `station`

---

## 2. โครงสร้างข้อมูล (Authored Object)

```cpp
struct Link {
    std::string              id;            // globally unique
    std::string              name;          // free-text label
    LinkType                 type;          // road class (§7)
    std::vector<Point>       geometry;      // reference polyline, 2..1000 points
    std::vector<double>      elevations;    // optional z per point
    std::vector<Lane>        lanes;         // lane definitions (order matters)
    DrivingSide              drivingSide;   // left/right (§6)
    int                      level;         // -1000..1000
    std::string              displayType;   // display catalog ID

    // Endpoints
    std::string              startNodeId;   // optional
    std::string              endNodeId;     // optional
    double                   startStation;  // default 0
    double                   length;        // derived from geometry

    // Behavior
    LinkBehaviorParams       behavior;      // §5

    // Attachments
    std::vector<Detector>    detectors;     // §8
    std::vector<SignalHead>  signalHeads;   // §9
    std::vector<RoutingDecision> routing;   // §10
    std::vector<ParkingArea> parking;       // §10

    // Visualization
    VisualizationParams      visual;        // §18
};

struct Lane {
    std::string              id;            // e.g. "east-1"
    int                      index;         // 0-based, left to right (or per driving side)
    double                   width;         // metres
    LaneType                 type;          // car/bus/bike/pedestrian/shoulder
    Marking                  leftMarking;   // solid/dashed/none
    Marking                  rightMarking;  // solid/dashed/none
    LaneBehaviorParams       behavior;      // per-lane override
    std::vector<LaneAttachment> attachments; // det/stop/parking references
};
```

### 2.1 ข้อกำหนดสำคัญ

| ฟิลด์ | ข้อจำกัด |
| :--- | :--- |
| `id` | ไม่ซ้ำกันทั้งเอกสาร (authored + derived) |
| `geometry` | ≥ 2 points, finite, ไม่มี duplicate ติดกัน, length > 0 |
| `lanes` | 1–12 เลน |
| `lane.id` | ไม่ซ้ำกันภายใน Link |
| `lane.width` | 0.5–20.0 m |
| `level` | -1000..1000 |
| `length` | derived; ใช้ตรวจสอบ station validity |

---

## 3. เรขาคณิตและ Reference Polyline

### 3.1 Reference Polyline

Link เก็บเรขาคณิตเป็น **polyline** ที่ประกอบด้วยจุด 2–1000 จุด แต่ละจุดมี:

- `x`, `y` (พิกัดระนาบ)
- `z` (optional, elevation; ถ้าไม่มี → ใช้ `level`)
- `m` (optional, station ที่ระบุเอง; ถ้าไม่มี → derive จาก cumulative distance)

### 3.2 Curve Construction (VISSIM-compatible)

เช่นเดียวกับ Connector, Link มี **cubic spline** ในขั้นตอนการสร้าง:

| พารามิเตอร์ | ชนิด | ค่าเริ่มต้น | คำอธิบาย |
| :--- | :--- | :--- | :--- |
| `curveType` | enum | `spline` | `spline`/`polyline`/`arc` |
| `splineTension` | double | 0.5 | 0.0–1.0 |
| `smoothingAngle` | double | 15.0° | มุมขั้นต่ำ |
| `minRadius` | double | 0.0 | รัศมีขั้นต่ำ (0 = ไม่จำกัด) |
| `sampleDensity` | int | 32 | จำนวน sample |
| `elevationMode` | enum | `flat` | `flat`/`linear`/`spline` |

### 3.3 การคำนวณ Station

```text
station(p0) = startStation
station(p_i) = station(p_{i-1}) + distance(p_{i-1}, p_i)
```

- `distance` = Euclidean (2D) หรือ 3D ตาม `elevationMode`
- `length` = station(p_last) − station(p_first)

### 3.4 การดำเนินการแก้ไข

| การดำเนินการ | ผลลัพธ์ |
| :--- | :--- |
| **Reset curve** | สร้าง cubic ใหม่ |
| **Straighten** | ลบ intermediate points ทั้งหมด |
| **Add intermediate point** | แทรกที่กึ่งกลางส่วนที่ยาวที่สุด |
| **Insert point at station** | แทรกที่ station ที่ระบุ |
| **Delete point** | ลบ intermediate point |
| **Drag point** | ขยับ geometry โดยตรง |
| **Smooth** | Reprocess ด้วย spline |
| **Reverse direction** | กลับทิศทาง (lane index เรียงใหม่) |
| **Extend/split** | ยืดออก / แบ่งเป็นสอง Link |

### 3.5 ฟังก์ชันที่เกี่ยวข้อง

- `linkCurve` — สร้าง cubic และ sample
- `linkStationAt(point)` — หา station จากพิกัด
- `linkPointAt(station)` — หาพิกัดจาก station
- `linkTangentAt(station)` — หาทิศทางที่ station
- `linkOffsetPolyline(side, distance)` — สร้างเส้น offset
- `linkSplitAt(station)` — แบ่ง Link

---

## 4. เลน (Lanes) และการจัดเรียง

### 4.1 โครงสร้างเลน

แต่ละเลนมี:

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `id` | string | ID เฉพาะ เช่น `east-1`, `east-2` |
| `index` | int | 0-based จากซ้ายไปขวา (หรือตาม driving side) |
| `width` | double | ความกว้าง (m) |
| `type` | enum | §4.2 |
| `leftMarking` | enum | `solid`/`dashed`/`none`/`double` |
| `rightMarking` | enum | เช่นเดียวกัน |
| `behavior` | struct | per-lane override |
| `attachments` | array | det/stop/parking |

### 4.2 ประเภทเลน (Lane Types — VISSIM-compatible)

| ประเภท | คำอธิบาย | ยานพาหนะ |
| :--- | :--- | :--- |
| `car` | เลนรถยนต์ทั่วไป | ทุกชนิด |
| `bus` | เลนรถเมล์ | bus เท่านั้น (หรือ bus + car ถ้า allow) |
| `bike` | เลนจักรยาน | bike |
| `pedestrian` | ทางเดินเท้า | pedestrian |
| `shoulder` | ไหล่ทาง | ห้ามใช้ |
| `parking` | เลนจอด | จอดเท่านั้น |
| `tram` | รางรถราง | tram |
| `emergency` | เลนฉุกเฉิน | emergency เท่านั้น |
| `toll` | เลนเก็บค่าผ่านทาง | ทุกชนิด + toll booth |
| `hov` | เลน HOV | ตามข้อกำหนด |

### 4.3 การจัดเรียง (Ordering)

- `drivingSide = right` → เลนที่ 0 อยู่ **ซ้ายสุด** (เร็วสุด), เลนสุดท้ายอยู่ขวาสุด (ช้าสุด)
- `drivingSide = left` → กลับกัน
- **Lane 0** ใช้เป็น reference lane (มักอยู่ตรงกลาง)

### 4.4 การเพิ่ม/ลบเลน (Lane Add/Drop)

| พารามิเตอร์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `laneAddStation` | double | ตำแหน่งที่เพิ่มเลน |
| `laneDropStation` | double | ตำแหน่งที่ลดเลน |
| `taperLength` | double | ความยาว taper |
| `taperSide` | enum | `left`/`right` |
| `laneAddMethod` | enum | `merge`/`new`/`split` |

### 4.5 Lane Connectivity

ภายใน Link, เลนแต่ละเลนเชื่อมต่อกับเลนที่ index เดียวกันตามยาว (ไม่มี auto lane change ภายใน Link)

---

## 5. พารามิเตอร์พฤติกรรมของ Link

### 5.1 ตารางพารามิเตอร์

| พารามิเตอร์ | ชนิด | ค่าเริ่มต้น | ช่วง | คำอธิบาย |
| :--- | :--- | :--- | :--- | :--- |
| `desiredSpeed` | SpeedDistribution | 50 km/h | 0–∞ | ความเร็วเป้าหมาย |
| `speedFactor` | double | 1.0 | 0.1–2.0 | ตัวคูณความเร็ว |
| `maxAcceleration` | double | 2.5 m/s² | 0.1–8.0 | ความเร่งสูงสุด |
| `normalDeceleration` | double | 3.0 m/s² | 0.1–8.0 | ความหน่วงปกติ |
| `maxDeceleration` | double | 6.0 m/s² | 0.1–12.0 | ความหน่วงสูงสุด |
| `minHeadway` | double | 1.0 m | 0.1–10.0 | ระยะห่างขั้นต่ำ |
| `reactionTime` | double | 1.0 s | 0.1–3.0 | เวลาตอบสนอง |
| `lookAheadDistance` | double | 100.0 m | 0.0–500.0 | ระยะมองล่วงหน้า |
| `lookBackDistance` | double | 50.0 m | 0.0–200.0 | ระยะมองย้อนหลัง |
| `lateralBehavior` | enum | `normal` | `conservative`/`normal`/`aggressive` | ลักษณะการขับ |
| `cooperative` | bool | true | — | ยอมให้รถอื่นแทรก |
| `yieldToPedestrians` | bool | true | — | ให้ทางคนเดินเท้า |
| `keepRight` | bool | true | — | ชิดขวา |
| `slope` | double | 0.0 | -0.2–0.2 | ความชัน (จาก elevation) |
| `friction` | double | 1.0 | 0.1–2.0 | ค่าสัมประสิทธิ์แรงเสียดทาน |
| `headwayDistribution` | enum | `normal` | — | การแจกแจง headway |
| `speedVariation` | double | 0.1 | 0.0–1.0 | การแปรผันความเร็ว |

### 5.2 Speed Distribution

เช่นเดียวกับ Connector — รองรับ `normal`, `uniform`, `empirical`, `constant`

```json
{
  "desiredSpeed": {
    "type": "normal",
    "mean": 13.9,
    "stdDev": 2.0,
    "min": 6.0,
    "max": 25.0
  }
}
```

### 5.3 การสืบทอดค่า (Inheritance)

- Link → Lane: ถ้า lane ไม่มี `behavior` → ใช้ของ Link
- Link → Connector: Connector สืบทอดค่าเริ่มต้นจาก Link ต้นทาง แล้ว override ได้

---

## 6. พารามิเตอร์การเปลี่ยนเลนและ Driving Side

### 6.1 Driving Side

| ค่า | ความหมาย | ตัวอย่างประเทศ |
| :--- | :--- | :--- |
| `right` | ขับชิดขวา | US, EU, CN, TH (โดยทั่วไป) |
| `left` | ขับชิดซ้าย | UK, JP, AU, IN, TH (กรณีพิเศษ) |

- กำหนดในระดับ **project** (default) และ override ได้ระดับ **Link**
- มีผลต่อการจัดเรียง lane index, การจัดป้าย, และตำแหน่ง signal head

### 6.2 Lane Change Parameters

| พารามิเตอร์ | ชนิด | ค่าเริ่มต้น | คำอธิบาย |
| :--- | :--- | :--- | :--- |
| `laneChangeDistance` | double (m) | 200.0 | ระยะเริ่มพิจารณาเปลี่ยนเลน |
| `emergencyStopDistance` | double (m) | 50.0 | ระยะที่ต้องหยุดถ้าเปลี่ยนไม่ได้ |
| `cooperativeLaneChange` | bool | true | ให้รถในเลนเป้าหมายชะลอ |
| `aggressiveLaneChange` | bool | false | เปลี่ยนเลนระยะกระชั้นชิด |
| `mandatoryLaneChangeDistance` | double (m) | 500.0 | ระยะบังคับเปลี่ยนเลน |
| `laneChangeDuration` | double (s) | 3.0 | ระยะเวลาเปลี่ยนเลน |
| `minLateralGap` | double (m) | 0.5 | ระยะข้างขั้นต่ำ |
| `laneChangeModel` | enum | `discretionary` | `discretionary`/`mandatory`/`aggressive` |

### 6.3 Lane Change Model (VISSIM-compatible)

| Model | คำอธิบาย |
| :--- | :--- |
| `discretionary` | เปลี่ยนตามความต้องการ (แซง) |
| `mandatory` | เปลี่ยนตามเส้นทาง (จำเป็น) |
| `aggressive` | เปลี่ยนระยะกระชั้นชิด |
| `cooperative` | ยอมให้รถอื่นแทรก |

---

## 7. ลำดับชั้นของ Link (Hierarchy & Priority)

### 7.1 ประเภทถนน

| ประเภท | ความเร็วสูงสุด | จำนวนเลน | ลำดับความสำคัญ |
| :--- | :--- | :--- | :--- |
| `motorway` | 120 km/h | 2–6 | สูงสุด |
| `trunk` | 100 km/h | 2–4 | สูง |
| `primary` | 80 km/h | 2–4 | สูง |
| `secondary` | 60 km/h | 2 | กลาง |
| `tertiary` | 50 km/h | 1–2 | กลาง |
| `residential` | 30 km/h | 1–2 | ต่ำ |
| `service` | 20 km/h | 1 | ต่ำสุด |
| `pedestrian` | — | — | เฉพาะคนเดิน |

### 7.2 การใช้ลำดับชั้น

- ใช้กำหนด priority rule อัตโนมัติที่จุดตัด
- ใช้กำหนดค่าเริ่มต้นของ `desiredSpeed`
- ใช้กำหนดสีเริ่มต้นในการแสดงผล
- ใช้ใน routing decision (`avoid motorway`, `prefer primary`, ฯลฯ)

### 7.3 Link Priority

| พารามิเตอร์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `priority` | int | 0–10 (สูง = สำคัญ) |
| `priorityOverride` | bool | override ค่าจาก type |
| `avoidanceFactor` | double | 0.0–1.0 (ใช้ในการ routing) |

---

## 8. Detectors และ Data Collection Points

### 8.1 Detector

เก็บข้อมูลการจราจร ณ จุดหนึ่งบน Link

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `id` | string | ID เฉพาะ |
| `linkId` | string | Link ที่ติดตั้ง |
| `laneIds` | array | เลนที่ตรวจจับ |
| `station` | double | ตำแหน่ง (m) |
| `length` | double | ความยาว detection zone (m) |
| `type` | enum | §8.2 |
| `vehicleClasses` | array | ชนิดรถที่ตรวจจับ |
| `aggregationInterval` | double (s) | ช่วงเวลารวบรวมข้อมูล |
| `dataType` | enum | `count`/`speed`/`occupancy`/`headway`/`queue` |

### 8.2 ประเภท Detector

| ประเภท | คำอธิบาย |
| :--- | :--- |
| `inductiveLoop` | ลูปเหนี่ยวนำ |
| `pointDetector` | จุดเดียว |
| `areaDetector` | พื้นที่ |
| `videoDetector` | กล้อง |
| `radarDetector` | เรดาร์ |
| `pressureDetector` | แผ่นความดัน |

### 8.3 Data Collection Point (DCP)

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `id` | string | ID |
| `linkId` | string | Link |
| `laneIds` | array | เลน |
| `station` | double | ตำแหน่ง |
| `dataType` | enum | `travel time`/`delay`/`queue length`/`stops` |

### 8.4 Output

- เขียนเป็นไฟล์ CSV / database
- ใช้เปรียบเทียบกับข้อมูลภาคสนาม (calibration)

---

