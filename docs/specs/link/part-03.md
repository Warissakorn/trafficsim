## 15. โครงสร้าง Project JSON (Schema v7)

```json
{
  "schemaVersion": 7,
  "link": {
    "id": "main-east",
    "name": "Main Street Eastbound",
    "type": "primary",
    "drivingSide": "right",
    "level": 0,
    "displayType": "default",

    "geometry": [
      { "x": 0.0,  "y": 0.0, "z": 0.0 },
      { "x": 50.0, "y": 0.0, "z": 0.0 },
      { "x": 100.0,"y": 5.0, "z": 0.0 },
      { "x": 150.0,"y": 10.0,"z": 0.0 }
    ],

    "startNodeId": "node-A",
    "endNodeId": "node-B",
    "startStation": 0.0,

    "curve": {
      "curveType": "spline",
      "splineTension": 0.5,
      "smoothingAngle": 15.0,
      "minRadius": 0.0,
      "sampleDensity": 32,
      "elevationMode": "flat"
    },

    "lanes": [
      {
        "id": "main-east-1",
        "index": 0,
        "width": 3.5,
        "type": "car",
        "leftMarking": "solid",
        "rightMarking": "dashed",
        "behavior": {}
      },
      {
        "id": "main-east-2",
        "index": 1,
        "width": 3.5,
        "type": "car",
        "leftMarking": "dashed",
        "rightMarking": "solid",
        "behavior": {}
      }
    ],

    "crossSection": {
      "leftShoulderWidth": 0.5,
      "rightShoulderWidth": 1.0,
      "medianWidth": 0.0,
      "medianType": "none",
      "leftSidewalkWidth": 0.0,
      "rightSidewalkWidth": 2.0
    },

    "behavior": {
      "desiredSpeed": { "type": "normal", "mean": 13.9, "stdDev": 2.0,
                        "min": 6.0, "max": 25.0 },
      "speedFactor": 1.0,
      "maxAcceleration": 2.5,
      "normalDeceleration": 3.0,
      "maxDeceleration": 6.0,
      "minHeadway": 1.0,
      "reactionTime": 1.0,
      "lookAheadDistance": 100.0,
      "lookBackDistance": 50.0,
      "lateralBehavior": "normal",
      "cooperative": true,
      "yieldToPedestrians": true,
      "keepRight": true,
      "slope": 0.0,
      "friction": 1.0,
      "headwayDistribution": "normal",
      "speedVariation": 0.1
    },

    "laneChange": {
      "laneChangeDistance": 200.0,
      "emergencyStopDistance": 50.0,
      "cooperativeLaneChange": true,
      "aggressiveLaneChange": false,
      "mandatoryLaneChangeDistance": 500.0,
      "laneChangeDuration": 3.0,
      "minLateralGap": 0.5,
      "laneChangeModel": "discretionary"
    },

    "priority": {
      "priority": 5,
      "priorityOverride": false,
      "avoidanceFactor": 0.0
    },

    "detectors": [
      {
        "id": "det-1",
        "laneIds": ["main-east-1", "main-east-2"],
        "station": 50.0,
        "length": 2.0,
        "type": "inductiveLoop",
        "vehicleClasses": ["car", "truck", "bus"],
        "aggregationInterval": 60.0,
        "dataType": ["count", "speed", "occupancy"]
      }
    ],

    "signalHeads": [
      {
        "id": "sh-1",
        "laneIds": ["main-east-1", "main-east-2"],
        "station": 145.0,
        "type": "vehicle",
        "signalGroupId": "sg-east",
        "signalControllerId": "ctrl-1"
      }
    ],

    "routing": [
      {
        "id": "rd-1",
        "station": 50.0,
        "routeType": "static",
        "lookAheadDistance": 200.0,
        "destinations": [
          { "connectorId": "turn-east/lane-1", "relativeFlow": 0.7 },
          { "connectorId": "turn-east/lane-2", "relativeFlow": 0.3 }
        ]
      }
    ],

    "parking": [],

    "stopLines": [
      {
        "id": "sl-1",
        "laneIds": ["main-east-1", "main-east-2"],
        "station": 145.0,
        "type": "signal",
        "width": 0.3
      }
    ],

    "visual": {
      "color": "#4a4a4a",
      "lineStyle": "solid",
      "lineWidth": 3.0,
      "showFlowLabels": true,
      "showLaneNumbers": false,
      "showDetectors": true,
      "highlightOnHover": true
    }
  }
}
```

**Compatibility**:
- Schema 1–5: ฟิลด์ behavior/laneChange/etc. ใช้ค่าเริ่มต้น
- Schema 6: เพิ่ม laneWidths, laneMarkings (ผ่าน Connector)
- Schema 7: เพิ่มทุกอย่างในเอกสารนี้
- ฟิลด์ที่หายไป → ค่าเริ่มต้น (backward compatible)

---

## 16. Runtime Compilation และ Simulation Semantics

### 16.1 ขั้นตอน compile

```
1. Link → Section decomposition:
   - ตัด Link ตาม station ของ:
     * body attachment (Connector endpoints)
     * detector
     * signal head
     * stop line
     * lane add/drop
     * parking
   - แต่ละ section มี: laneIds, startStation, endStation, length

2. Section → Segment:
   - 1 section ต่อ 1 เลน ต่อ 1 Segment
   - Segment มี: length, successor, predecessor, lane

3. Connectivity:
   - ภายใน Link: section ต่อเนื่อง
   - ที่ปลาย Link: ผ่าน Connector ไปยัง Link อื่น

4. Runtime diagnostics:
   - section length ≥ kMinSectionLength (0.2 m)
   - priority rules valid
```

### 16.2 Derived Section IDs

- Format: `<linkId>/<laneId>/s<N>`
- ไม่ persist
- สร้างใหม่ทุกครั้ง

### 16.3 Vehicle Semantics

- รถเคลื่อนที่ตาม reference polyline + lateral offset
- ใช้ `behavior` ของ Link (override ด้วย lane)
- ที่ section boundary → ตรวจสอบ successor
- ที่ connector boundary → ใช้ behavior ของ Connector

### 16.4 Detector Semantics

- ตรวจสอบรถที่ผ่าน station ในช่วง `[station, station+length]`
- บันทึก: count, speed, occupancy, headway
- เขียน output ทุก `aggregationInterval` วินาที

### 16.5 Signal Semantics

- Signal head ควบคุมการหยุดที่ stop line
- ถ้าไม่มี stop line → ใช้ station ของ head
- Signal phase เปลี่ยนตาม controller

### 16.6 Constraint

- Section ต้องยาว ≥ 0.2 m
- ถ้าผิด → **Run blocked**
- `UNSUPPORTED_LINK_POSITION`

---

## 17. การตรวจสอบ (Validation) และรหัสวินิจฉัย

| Code | Severity / Context | ความหมาย |
| :--- | :--- | :--- |
| `INVALID_ID` / `DUPLICATE_ID` | Draft | ID ว่างหรือซ้ำ |
| `INVALID_GEOMETRY` | Draft | < 2 points, non-finite, zero length, duplicate |
| `INVALID_LANE_COUNT` | Draft | จำนวนเลน 0 หรือ > 12 |
| `INVALID_LANE_WIDTH` | Draft | ความกว้าง < 0.5 หรือ > 20 |
| `INVALID_LANE_ID` | Draft | lane ID ซ้ำหรือว่าง |
| `UNKNOWN_NODE` | Draft | node อ้างอิงไม่มี |
| `DISCONNECTED_GEOMETRY` | Draft | ปลายไม่ตรง node (> 0.01 m) |
| `EDIT_LINK_POSITION` | Edit | station นอกช่วง |
| `EDIT_LANE_CHANGE_DISTANCE` | Edit | `laneChangeDistance < emergencyStopDistance` |
| `EDIT_SPEED_DISTRIBUTION` | Edit | desired speed ไม่ถูกต้อง |
| `EDIT_REFERENCED_LINK` | Edit | ลบ Link ที่ถูกอ้างอิงโดยไม่ลบ dependency |
| `EDIT_SIGNAL_CONFLICT` | Edit | signal group ไม่มี |
| `EDIT_DETECTOR_POSITION` | Edit | detector นอกช่วง |
| `EDIT_ROUTING_DECISION` | Edit | destination ไม่มี |
| `EDIT_LINK_TYPE` | Edit | ประเภทไม่ถูกต้อง |
| `EDIT_LANE_TAPER` | Edit | taper ยาวเกิน Link |
| `EDIT_DRIVING_SIDE` | Edit | ขัดกับ project default โดยไม่ได้ตั้งใจ |
| `UNSUPPORTED_LINK_POSITION` | Runtime blocker | section < 0.2 m |
| `EDIT_NO_PRIORITY_DEFAULTS` | Runtime blocker | merge ต้องมีค่า priority |
| `WARN_TIGHT_CURVE` | Advisory | รัศมีโค้งน้อย |
| `WARN_SHORT_LINK` | Advisory | Link สั้นกว่า 5 m |
| `WARN_NARROW_LANE` | Advisory | เลน < 2.5 m |
| `WARN_HIGH_SLOPE` | Advisory | ความชัน > 10% |

---

## 18. การแสดงผล (Visualization)

### 18.1 พารามิเตอร์

| พารามิเตอร์ | ชนิด | ค่าเริ่มต้น | คำอธิบาย |
| :--- | :--- | :--- | :--- |
| `color` | string | `#4a4a4a` | สีเส้น |
| `lineStyle` | enum | `solid` | `solid`/`dashed`/`dotted` |
| `lineWidth` | double | 3.0 | ความหนา (px) |
| `showFlowLabels` | bool | true | ป้ายการไหล |
| `showLaneNumbers` | bool | false | หมายเลขเลน |
| `showDetectors` | bool | true | แสดง detector |
| `highlightOnHover` | bool | true | ไฮไลต์ |
| `showSignalMarkers` | bool | true | marker signal |
| `showStopLines` | bool | true | เส้นหยุด |

### 18.2 ระดับ Zoom

| Zoom | แสดง |
| :--- | :--- |
| < 0.25× | เฉพาะเส้น |
| 0.25×–1.0× | เส้น + ชื่อ |
| 1.0×–2.0× | เส้น + ป้าย + markers |
| > 2.0× | ทุกอย่าง + lane numbers |

### 18.3 Network Editor Integration

- คลิกเลือก → properties inspector
- Double-click → dialog แก้ไข
- Right-click → context menu (Edit/Delete/Show in List/Reverse)

### 18.4 Traffic Overlay (Runtime)

- แสดงระดับการจราจร (สีเขียว/เหลือง/แดง) ตามความหนาแน่น
- แสดง queue length
- แสดง speed

---

## 19. ข้อจำกัดและแผนงานในอนาคต

### 19.1 ข้อจำกัดปัจจุบัน

| # | ข้อจำกัด | ผลกระทบ |
| :--- | :--- | :--- |
| 1 | ไม่มี auto lane change ภายใน Link | ต้องพึ่ง Connector |
| 2 | ไม่มี autoritative taper length | taper ตามที่กำหนด |
| 3 | ไม่มี dynamic lane reversal | ต้อง define ใหม่ |
| 4 | ไม่มี autonomous vehicle behavior | ต้อง define model ใหม่ |
| 5 | ไม่มี emission model | roadmap v11 |
| 6 | ไม่มี multi-modal interaction ในตัว | roadmap v11 |
| 7 | ผลลัพธ์ยังไม่ผ่าน validation | ยังไม่ scientific |

### 19.2 แผนงานในอนาคต (Roadmap)

- **v8**: Autoritative taper length, swept-path
- **v9**: Dynamic routing, adaptive signal, lane reversal
- **v10**: Calibrated gap-acceptance, lane-change model (Wiedemann), AV behavior
- **v11**: Emission model, multi-modal interaction, EV charging
- **v12**: External controller (OCIT, NTCIP), cloud integration
- **v13**: Machine-learning-based calibration

---

## 20. Source Map

| Concern | Primary Source |
| :--- | :--- |
| Data types & public geometry/runtime helpers | `src/model/network/network.hpp` |
| Path IDs, lane pairing, range resizing | `src/model/network/link_paths.cpp` |
| Attachments, tangents, default curve | `src/model/network/link_geometry.cpp` |
| Widths, boundaries, mouths, markings | `src/model/network/road_boundaries.cpp` |
| Runtime lane cutting & merge rules | `src/model/network/sections.cpp` |
| Scenario compilation & runtime diagnostics | `src/model/network/compile.cpp` |
| Draft validation | `src/model/network/validate.cpp` |
| Mutating commands & dependency cleanup | `src/commands/link_commands.cpp` |
| Canvas picking, snapping, endpoint editing | `src/editor/canvas_links.cpp` |
| Range handles | `src/editor/canvas_lanes.cpp` |
| Properties inspector | `src/shell/editor_links.cpp` |
| **Behavior parameters** | `src/model/network/link_behavior.cpp` |
| **Lane change & emergency stop** | `src/model/network/link_lane_change.cpp` |
| **Detectors & DCP** | `src/model/network/detectors.cpp` |
| **Signal integration** | `src/model/network/signal.cpp` |
| **Routing decisions** | `src/model/network/routing.cpp` |
| **Parking & stops** | `src/model/network/parking.cpp` |
| **Nodes & intersections** | `src/model/network/nodes.cpp` |
| JSON read/write | `src/project/parse.cpp`, `src/project/document.cpp` |
| Thai/English UI & diagnostic text | `data/locales/th.json`, `data/locales/en.json` |

---

