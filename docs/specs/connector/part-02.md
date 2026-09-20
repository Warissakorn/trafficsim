## 12. Command Contract และการอ้างอิง

### 12.1 ตาราง Command

| Command | ผลลัพธ์ |
| :--- | :--- |
| `addConnector` | สร้าง Connector 1 เลน + default curve |
| `addConnectorRange` | สร้าง ranged Connector + สืบทอด display |
| `changeConnectorRange` | ปรับขนาดช่วง contiguous |
| `changeConnectorLanes` | ตั้ง/ล้าง widths และ markings |
| `changeConnectorGeometry` | แทน polyline + re-read attachments |
| `changeConnectorEndpoints` | retarget source/target |
| `resetConnectorCurve` | คืนค่า curve เริ่มต้นหรือเส้นตรง |
| `resampleConnectorPoints` | เปลี่ยนจำนวน intermediate points |
| `deleteConnector` | ลบ + dependent objects |
| `anchorConnectors` | ย้ายปลายไปยังเลนที่ระบุชื่อ |
| `reanchorConnectors` | คงตำแหน่งโลก + ลบที่หลุด |
| **`changeConnectorBehavior`** (ใหม่) | แก้ §5 parameters |
| **`changeConnectorLaneChange`** (ใหม่) | แก้ §6 parameters |
| **`changeConnectorPriority`** (ใหม่) | แก้ §7 priority rule |
| **`changeConnectorSignal`** (ใหม่) | ผูก/ยกเลิก signal head |
| **`changeConnectorRouting`** (ใหม่) | ผูก/ยกเลิก routing decision |
| **`changeConnectorVisualization`** (ใหม่) | แก้ §16 |

### 12.2 การอ้างอิง

Connector ถือว่า **referenced** เมื่อ:
- Route ใดๆ ใช้ path ID ตัวใดตัวหนึ่ง
- Signal head ระบุ path ID ตัวใดตัวหนึ่ง

**ผลกระทบ**:
- ✅ reshape ได้
- ❌ retarget ไม่ได้
- ❌ เปลี่ยน lane range ไม่ได้
- ❌ ลบได้ แต่ต้องลบ heads + routes + vehicle inputs ใน transaction เดียวกัน

---

## 13. โครงสร้าง Project JSON (Schema v7)

```json
{
  "schemaVersion": 7,
  "connector": {
    "id": "turn-east",
    "from": { "linkId": "approach", "laneId": "approach-1", "station": 80.0 },
    "to":   { "linkId": "exit",     "laneId": "exit-1" },
    "geometry": [
      { "x": 80.0, "y": 1.75 },
      { "x": 85.0, "y": 3.0 },
      { "x": 90.0, "y": 8.0 }
    ],
    "fromLaneCount": 2,
    "toLaneCount": 1,
    "level": 0,
    "displayType": "default",
    "laneBlend": [],
    "name": "Eastbound turn",
    "laneWidths": [3.5, 3.25],
    "laneMarkings": ["dashed"],

    "curve": {
      "curveType": "spline",
      "splineTension": 0.5,
      "smoothingAngle": 15.0,
      "minRadius": 0.0,
      "sampleDensity": 32
    },

    "behavior": {
      "desiredSpeed": { "type": "normal", "mean": 13.9, "stdDev": 1.5,
                        "min": 8.0, "max": 22.0 },
      "speedFactor": 1.0,
      "acceleration": 2.5,
      "deceleration": 3.0,
      "maxDeceleration": 6.0,
      "minHeadway": 1.0,
      "reactionTime": 1.0,
      "lookAheadDistance": 50.0,
      "lateralBehavior": "normal",
      "cooperative": true,
      "yieldToPedestrians": true
    },

    "laneChange": {
      "laneChangeDistance": 200.0,
      "emergencyStopDistance": 50.0,
      "cooperativeLaneChange": true,
      "aggressiveLaneChange": false
    },

    "priority": {
      "priorityRuleType": "gapTime",
      "gapTime": 3.0,
      "headway": 7.0,
      "minGap": 0.5,
      "maxWaitTime": 60.0,
      "stopLinePosition": 0.0,
      "visibilityDistance": 100.0
    },

    "conflict": {
      "autoConflictArea": true,
      "conflictPriority": "undefined",
      "conflictFrontGap": 3.0,
      "conflictRearGap": 2.0,
      "conflictVisibility": 100.0
    },

    "signal": {
      "signalGroupId": "sg-north",
      "signalControllerId": "ctrl-1",
      "stopLineOffset": 3.0,
      "signalHeadType": "vehicle"
    },

    "routing": {
      "routingDecisionId": "rd-1",
      "decisionPosition": 50.0,
      "routeType": "static",
      "lookAheadDistance": 200.0,
      "destinations": [
        { "connectorId": "turn-east/lane-1", "relativeFlow": 0.7 },
        { "connectorId": "turn-east/lane-2", "relativeFlow": 0.3 }
      ]
    },

    "mouth": {
      "mouthTransitionLength": 5.0,
      "mouthBlendMode": "linear",
      "mouthSharpness": 1.0
    },

    "visual": {
      "color": "#3f8efc",
      "lineStyle": "solid",
      "lineWidth": 2.0,
      "showFlowLabels": true,
      "showPriorityMarkers": true,
      "showSignalMarkers": true
    }
  }
}
```

**Compatibility**:
- Schema 1–5: Connector ranges = 1 เลน, level 0, display default
- Schema 6: เพิ่ม `laneWidths`, `laneMarkings`
- Schema 7: เพิ่ม `curve`, `behavior`, `laneChange`, `priority`, `conflict`, `signal`, `routing`, `mouth`, `visual`
- Field ที่ไม่ปรากฏ → ใช้ค่าเริ่มต้น (backward compatible)

---

## 14. Runtime Compilation และ Simulation Semantics

### 14.1 ขั้นตอน compile

```
1. runtimeSections:
   - derive ทุก Connector path
   - ตัด Link lane เป็น sections ที่ body attachments
   - departure จาก lane body → diverge
   - arrival เข้า lane body → ต่อกับ downstream + merge

2. buildScenario:
   - compile แต่ละ path → core Segment
   - segment length = path polyline length
   - successor = target lane section
   - route อ้าง whole lane IDs + connector path IDs
   - lane IDs ขยายเป็น section chain

3. runtime diagnostics:
   - ตรวจ section length ≥ kMinSectionLength (0.2 m)
   - ตรวจ priority defaults
```

### 14.2 Derived Section IDs

- ไม่ persist
- สร้างใหม่ทุกครั้งที่โหลดโปรเจกต์

### 14.3 Priority Rule

- Interior arrival → derived PriorityRule
- ให้ทางกับ traffic บน upstream target-lane section
- gap time + headway จาก `data/priority-rules/`
- default: 3.0 s, 7.0 m
- ถ้าโหลด default ไม่ได้ → **Run refused**

### 14.4 Vehicle Semantics

- รถ carry residual travel ข้าม Connector
- ไม่ reset route distance
- ท้ายรถอาจยังอยู่บน upstream segment ขณะที่หัวเข้า Connector
- ใช้ `behavior` parameters ของ Connector ระหว่างอยู่บน Connector

### 14.5 Constraint

- Attachment ต้องเหลือ ≥ `kMinSectionLength` (0.2 m)
- ถ้าผิด → drawing save ได้, **Run blocked**
- `UNSUPPORTED_CONNECTOR_POSITION`

---

## 15. การตรวจสอบ (Validation) และรหัสวินิจฉัย

| Code | Severity / Context | ความหมาย |
| :--- | :--- | :--- |
| `INVALID_ID` / `DUPLICATE_ID` | Draft | ID ว่างหรือซ้ำ |
| `INVALID_GEOMETRY` | Draft | < 2 points, non-finite, zero length, duplicate, blend ไม่ถูกต้อง |
| `UNKNOWN_LANE` | Draft | Link/lane หาไม่เจอ |
| `EDIT_CONNECTOR_POSITION` | Draft | station ไม่ finite หรือนอก Link |
| `EDIT_LANE_RANGE` | Draft/Edit | count ไม่อยู่ใน 1–12 หรือ range ไม่ fit |
| `DUPLICATE_CONNECTION` | Draft/Edit | path อื่นเชื่อม lane pair เดียวกันที่ station เดียวกัน |
| `DISCONNECTED_GEOMETRY` | Draft | endpoint > 0.01 m จาก reference |
| `INVALID_WIDTH` / `INVALID_MARKING` | Draft/Edit | width/marking ไม่ถูกต้อง |
| `EDIT_LANES` | Edit | list length ไม่ครบ |
| `EDIT_REFERENCED_CONNECTOR` | Edit | retarget/resize Connector ที่ถูกอ้างอิง |
| `EDIT_CONNECTOR_GAP` | Edit | source-target attachments ทับกัน |
| `EDIT_CONNECTOR_POINTS` | Edit | intermediate point count ไม่อยู่ใน 0–40 |
| `UNSUPPORTED_CONNECTOR_POSITION` | Runtime blocker | body attachment เหลือ < 0.2 m |
| `EDIT_NO_PRIORITY_DEFAULTS` | Runtime blocker | merge ต้องมี gap/headway เป็นบวก |
| `TIGHT_CONNECTOR_RADIUS` | Advisory | รัศมี 3-point curvature < ครึ่งความกว้างรวม |
| **`EDIT_LANE_CHANGE_DISTANCE`** (ใหม่) | Edit | `laneChangeDistance < emergencyStopDistance` |
| **`EDIT_SPEED_DISTRIBUTION`** (ใหม่) | Edit | desired speed distribution ไม่ถูกต้อง |
| **`EDIT_PRIORITY_RULE`** (ใหม่) | Edit | priority rule ขัดกับ signal |
| **`EDIT_SIGNAL_CONFLICT`** (ใหม่) | Edit | signal group ไม่มีอยู่ |
| **`EDIT_ROUTING_DECISION`** (ใหม่) | Edit | routing decision อ้าง Connector ที่ไม่มี |
| **`EDIT_CONFLICT_PRIORITY`** (ใหม่) | Edit | conflict priority ขัดกันเอง |
| **`EDIT_CURVE_PARAMETERS`** (ใหม่) | Edit | spline tension / sample density ผิด |
| **`WARN_TIGHT_TURN`** (ใหม่) | Advisory | มุมเลี้ยวแหลม |
| **`WARN_SHORT_CONNECTOR`** (ใหม่) | Advisory | Connector สั้นกว่า 5 m |

**หมายเหตุ**: `TIGHT_CONNECTOR_RADIUS` และ `WARN_*` เป็น advisory ไม่ block save/simulation

---

## 16. การแสดงผล (Visualization) และระดับรายละเอียด

### 16.1 พารามิเตอร์

| พารามิเตอร์ | ชนิด | ค่าเริ่มต้น | คำอธิบาย |
| :--- | :--- | :--- | :--- |
| `color` | string | `#3f8efc` | สีเส้น |
| `lineStyle` | enum | `solid` | `solid`/`dashed`/`dotted` |
| `lineWidth` | double | 2.0 | ความหนาเส้น (px) |
| `showFlowLabels` | bool | true | แสดงป้ายการไหล |
| `showPriorityMarkers` | bool | true | แสดง marker priority |
| `showSignalMarkers` | bool | true | แสดง marker signal |
| `showLaneNumbers` | bool | false | แสดงหมายเลข path |
| `highlightOnHover` | bool | true | ไฮไลต์เมื่อ hover |

### 16.2 ระดับ Zoom

| ระดับ | แสดง |
| :--- | :--- |
| Zoom < 0.25× | เฉพาะเส้น |
| 0.25×–1.0× | เส้น + ป้ายชื่อ |
| 1.0×–2.0× | เส้น + ป้าย + markers |
| > 2.0× | ทุกอย่าง + path numbers |

### 16.3 Network Editor Integration

- คลิกเลือก Connector → properties แสดงใน inspector
- Double-click → เปิด dialog แก้ไข
- Right-click → context menu (Edit / Delete / Show in List)

---

## 17. ข้อจำกัดและแผนงานในอนาคต

### 17.1 ข้อจำกัดปัจจุบัน

| # | ข้อจำกัด | ผลกระทบ |
| :--- | :--- | :--- |
| 1 | Connector paths เป็น monotone contiguous ranges | ไม่มีการแมปเลนอิสระ |
| 2 | ไม่มี automatic lane changing | ต้องพึ่ง lane change distance ที่กำหนด |
| 3 | Geometric crossings ไม่สร้าง conflict area อัตโนมัติ | ต้องกำหนดเอง |
| 4 | Merge rule เป็น deterministic threshold | ไม่ใช่ gap-acceptance ที่ calibrated |
| 5 | Lane drop/gain taper ตลอดความยาว | ไม่มี authored taper length |
| 6 | Tight-turn เป็น geometric warning | ไม่ใช่ swept-path |
| 7 | ผลลัพธ์ยังไม่ผ่าน validation milestone | ยังไม่ scientific |

### 17.2 แผนงานในอนาคต (Roadmap)

- **v8**: Autoritative taper length, swept-path analysis
- **v9**: Dynamic routing decisions, adaptive signal control
- **v10**: Gap-acceptance model ที่ calibrated, lane-change model แบบ VISSIM Wiedemann
- **v11**: Multi-modal (bicycle/pedestrian) connectors
- **v12**: External controller integration (OCIT, NTCIP)

---

## 18. Source Map

| Concern | Primary Source |
| :--- | :--- |
| Data types & public geometry/runtime helpers | `src/model/network/network.hpp` |
| Path IDs, lane pairing, range resizing | `src/model/network/connector_paths.cpp` |
| Attachments, tangents, default curve | `src/model/network/connector_geometry.cpp` |
| Widths, boundaries, mouths, markings | `src/model/network/road_boundaries.cpp` |
| Runtime lane cutting & merge rules | `src/model/network/sections.cpp` |
| Scenario compilation & runtime diagnostics | `src/model/network/compile.cpp` |
| Draft validation | `src/model/network/validate.cpp` |
| Mutating commands & dependency cleanup | `src/commands/connector_commands.cpp` |
| Canvas picking, snapping, endpoint editing | `src/editor/canvas_connectors.cpp` |
| Range handles | `src/editor/canvas_lanes.cpp` |
| Properties inspector | `src/shell/editor_connectors.cpp` |
| **Behavior parameters** (ใหม่) | `src/model/network/connector_behavior.cpp` |
| **Lane change & emergency stop** (ใหม่) | `src/model/network/connector_lane_change.cpp` |
| **Priority rules & conflict areas** (ใหม่) | `src/model/network/connector_priority.cpp` |
| **Signal integration** (ใหม่) | `src/model/network/connector_signal.cpp` |
| **Routing decisions** (ใหม่) | `src/model/network/connector_routing.cpp` |
| JSON read/write | `src/project/parse.cpp`, `src/project/document.cpp` |
| Thai/English UI & diagnostic text | `data/locales/th.json`, `data/locales/en.json` |

---

## ภาคผนวก A: ตารางเทียบ Connector ของคุณกับ VISSIM (Summary)

| มิติ | Connector ของคุณ (v7) | VISSIM |
| :--- | :--- | :--- |
| แนวคิด | Directed movement ระหว่าง lane ranges | Directed movement ระหว่าง Links |
| การแมปเลน | Monotonic, ไม่เท่ากันได้ (2→3) | ต้องเท่ากัน, ห้ามตัดกัน |
| รูปทรง | Polyline + spline sampling | Spline + z-coordinate |
| ความกว้าง | Authored/Link/taper อัตโนมัติ | Manual drag points |
| Behavior params | ✅ (ใหม่ v7) | ✅ |
| Lane change distance | ✅ (ใหม่ v7) | ✅ |
| Priority rules | ✅ | ✅ |
| Conflict areas | ✅ | ✅ |
| Signal integration | ✅ | ✅ |
| Routing decisions | ✅ (ใหม่ v7) | ✅ |
| External controller | ❌ (roadmap v12) | ✅ |
| Auto lane change | ❌ | ✅ |
| Calibrated gap acceptance | ❌ (roadmap v10) | ✅ (Wiedemann) |
| Multi-modal | ❌ (roadmap v11) | ✅ |

---

**สรุป**: เอกสารฉบับนี้ยกระดับ Connector spec จากเดิมที่เป็นเพียง "authored object + derived path" ให้เป็น **ระบบ Connector ระดับ microsimulation** ที่เทียบเท่า VISSIM ในแง่ของพารามิเตอร์ พฤติกรรม และการเชื่อมโยงกับ signal/priority/routing โดยยังคงหลักการ "Authored base, Derived detail" และ "Deterministic compilation" ของระบบเดิมไว้

หากต้องการให้ขยายเฉพาะหัวข้อใด (เช่น เพียงแค่ behavior parameters, หรือเพิ่มตัวอย่าง scenario จริง) แจ้งได้เลยครับ