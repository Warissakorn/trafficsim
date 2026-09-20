# เอกสารข้อกำหนด Connector (ฉบับปรับปรุง — ระดับรายละเอียดเทียบเท่า VISSIM)

> เอกสารนี้เป็นฉบับขยายของสเปก Connector เดิม โดยเพิ่มรายละเอียดด้านพารามิเตอร์พฤติกรรม, การเชื่อมโยงกับสัญญาณไฟ, conflict area, priority rule, routing decision, และการตรวจสอบแบบ VISSIM เพื่อให้สามารถนำไปใช้ออกแบบระบบจำลองการจราจรระดับจุลภาค (microsimulation) ได้จริง

---

## สารบัญ

1. ภาพรวมและปรัชญาการออกแบบ
2. โครงสร้างข้อมูล (Authored Object)
3. เรขาคณิตและการสร้างเส้นโค้ง (Geometry & Spline)
4. การแมปเลนและรูปแบบทอพอโลยี
5. พารามิเตอร์พฤติกรรมของ Connector (Behavior Parameters)
6. Lane Change Distance และ Emergency Stop
7. Conflict Areas และ Priority Rules
8. Signal Heads และการควบคุมสัญญาณ
9. Routing Decisions และ Static Routes
10. ความกว้าง ขอบเขต เครื่องหมาย และปากทาง
11. การสร้างและแก้ไขผ่าน Editor
12. Command Contract และการอ้างอิง
13. โครงสร้าง Project JSON (Schema v7)
14. Runtime Compilation และ Simulation Semantics
15. การตรวจสอบ (Validation) และรหัสวินิจฉัย
16. การแสดงผล (Visualization) และระดับรายละเอียด
17. ข้อจำกัดและแผนงานในอนาคต
18. Source Map

---

## 1. ภาพรวมและปรัชญาการออกแบบ

Connector คือ **การเคลื่อนที่แบบมีทิศทาง (directed movement)** ระหว่างช่วงเลนที่ต่อเนื่องกันของ Link ต้นทาง และช่วงเลนที่ต่อเนื่องกันของ Link ปลายทาง (หรือตำแหน่งอื่นของ Link เดียวกัน) Connector ไม่ใช่การเชื่อมต่อกับระบบภายนอก แต่เป็น **องค์ประกอบเชิงโครงสร้างของเครือข่ายถนน** ที่ทำหน้าที่:

- กำหนดความเป็นไปได้ในการเคลื่อนที่ระหว่างเลน
- กำหนดพฤติกรรมของยานพาหนะขณะเปลี่ยนเลนหรือเลี้ยว
- เป็นจุดเชื่อมโยงกับ **Conflict Area**, **Priority Rule**, **Signal Head** และ **Routing Decision**
- เป็นหน่วยที่เก็บพารามิเตอร์ระดับการจำลอง (simulation-level parameters) เช่น ความเร็วเป้าหมาย, lane change distance, emergency stop distance

**ปรัชญาการออกแบบ (Design Philosophy)**

| หลักการ | คำอธิบาย |
| :--- | :--- |
| Explicit over Implicit | ทุกการเคลื่อนที่ต้องถูกประกาศอย่างชัดเจน ไม่มี auto-routing ที่ซ่อนอยู่ |
| Authored base, Derived detail | เก็บเฉพาะ Connector ฐาน ส่วน path รายเลนและ section ถูกคำนวณ |
| Deterministic compilation | Runtime compile แบบ deterministic ทุกครั้งจากข้อมูล authored เดียวกัน |
| Reference-safe editing | Connector ที่ถูกอ้างอิงแก้ไขได้อย่างจำกัด เพื่อรักษาความถูกต้องของ topology |

---

## 2. โครงสร้างข้อมูล (Authored Object)

```cpp
struct Connector {
    std::string          id;               // globally unique, also first path ID
    LaneReference        from;             // linkId, laneId, optional station
    LaneReference        to;               // linkId, laneId, optional station
    std::vector<Point>   geometry;         // polyline, 2..42 points
    int                  fromLaneCount;    // 1..12
    int                  toLaneCount;      // 1..12
    int                  level;            // -1000..1000
    std::string          displayType;      // display catalog ID
    std::vector<double>  laneBlend;        // optional frozen weights
    std::string          name;             // free-text label
    std::vector<double>  laneWidths;       // optional per-path widths (m)
    std::vector<Marking> laneMarkings;     // optional interior dividers
    // --- ส่วนขยายใหม่ (VISSIM-compatible) ---
    BehaviorParams       behavior;         // §5
    LaneChangeParams     laneChange;       // §6
    PriorityParams       priority;         // §7
    std::string          signalGroupId;    // §8
    std::string          routingDecisionId;// §9
    VisualizationParams  visual;           // §16
};
```

**LaneReference** ประกอบด้วย `linkId`, `laneId` และ `optional station` (หน่วยเมตรตาม reference polyline ของ Link ไม่ใช่ offset lane) — station หนึ่งค่าหมายถึงหน้าตัดเดียวกันสำหรับทุกเลนในช่วง

**Station semantics**

| กรณี | ความหมาย |
| :--- | :--- |
| Source ไม่มี station | ใช้ปลายทาง (end) ของ Link ต้นทาง |
| Target ไม่มี station | ใช้จุดเริ่มต้น (start) ของ Link ปลายทาง |
| Body attachment | ต้องระบุ station ชัดเจน |

---

## 3. เรขาคณิตและการสร้างเส้นโค้ง (Geometry & Spline)

### 3.1 รูปแบบเส้น

Connector เก็บเป็น **polyline** (ไม่ใช่ spline ที่ persist) ประกอบด้วย:
- 2 จุด attachment (ต้นทาง/ปลายทาง)
- 0–40 จุดกลางที่แก้ไขได้ (intermediate points)

แต่ใน **ขั้นตอนการสร้าง** ระบบจะสร้าง **cubic spline** ชั่วคราวให้สอดคล้องกับทิศทางของเลนต้นทางและปลายทาง แล้ว sample ออกมาเป็น polyline

### 3.2 พารามิเตอร์เส้นโค้ง (Curve Parameters — VISSIM-compatible)

| พารามิเตอร์ | ช่วงค่า | ค่าเริ่มต้น | คำอธิบาย |
| :--- | :--- | :--- | :--- |
| `curveType` | `spline`, `polyline`, `arc` | `spline` | รูปแบบเส้นที่ใช้ sample |
| `splineTension` | 0.0–1.0 | 0.5 | ความตึงของ spline (0 = ตรง, 1 = โค้งมาก) |
| `controlReach` | 0.0–∞ (m) | อัตโนมัติ | ระยะควบคุมของ cubic |
| `smoothingAngle` | 0–180° | 15° | มุมขั้นต่ำก่อนจัดว่าเป็น "โค้ง" |
| `minRadius` | 0.0–∞ (m) | 0.0 | รัศมีขั้นต่ำ (0 = ไม่จำกัด) |
| `sampleDensity` | 2–200 | 32 | จำนวน sample ต่อความยาวหน่วย |

### 3.3 การดำเนินการแก้ไข

| การดำเนินการ | ผลลัพธ์ |
| :--- | :--- |
| **Reset curve** | สร้าง cubic ใหม่ + sample 3 จุดกลาง |
| **Straight connector** | เหลือเฉพาะ 2 attachments |
| **Increase intermediate points** | แยกส่วนที่ยาวที่สุดซ้ำๆ ไม่ขยับจุดเดิม |
| **Decrease intermediate points** | กระจายจุดที่เหลือให้เท่ากันตามระยะ |
| **Drag interior point** | แก้ polyline โดยตรง |
| **Drag endpoint** | retarget ปลายทางไปยังเลนใต้ pointer |
| **Drag body** | ย้าย geometry ทั้งหมด ถ้าปลายออกจากทุกเลน → ลบ Connector |
| **Smooth** (ใหม่) | ประมวลผลใหม่ด้วย spline ตาม `splineTension` |

**ข้อกำหนด**: ปลายต้องอยู่ห่างจากจุดศูนย์กลางเลนอ้างอิง ≤ **0.01 ม.**

### 3.4 ฟังก์ชันที่เกี่ยวข้อง

- `connectorCurve` — สร้าง cubic ชั่วคราวและ sample
- `anchorConnectorEnds` — ย้ายปลายไปยังเลนที่ระบุชื่อ
- `reanchorConnector` — คงตำแหน่งโลก, re-read reference หลัง Link เคลื่อนที่
- `connectorMouthFit` — คำนวณการ shift, transition zone ที่ปากทาง

---

## 4. การแมปเลนและรูปแบบทอพอโลยี

### 4.1 จำนวน path

```text
pathCount = max(fromLaneCount, toLaneCount)
```

### 4.2 สูตรการจับคู่ (integer division โดยเจตนา)

```text
source = (pathCount == 1) ? 0 : i * (fromLaneCount - 1) / (pathCount - 1)
target = (pathCount == 1) ? 0 : i * (toLaneCount - 1) / (pathCount - 1)
```

### 4.3 รูปแบบที่รองรับ

| Range | Derived mapping | ความหมาย |
| :--- | :--- | :--- |
| 1→1 | 0→0 | ต่อตรง |
| 1→N | 0→0…0→N-1 | Fan-out |
| N→1 | 0→0…N-1→0 | Merge |
| N→N | 0→0…N-1→N-1 | Parallel (order-preserving) |
| 2→3 | 0→0, 0→1, 1→2 | Unequal fan-out |
| 3→2 | 0→0, 1→0, 2→1 | Unequal merge |

**ข้อจำกัด**: ไม่ใช่ routing matrix อิสระ — การแมปแบบไม่ monotonic ต้องใช้ Connector หลายตัว

### 4.4 Path IDs

| Path index | ID |
| :--- | :--- |
| 0 | `<connector-id>` |
| 1 | `<connector-id>/lane-2` |
| i | `<connector-id>/lane-(i+1)` |

---

## 5. พารามิเตอร์พฤติกรรมของ Connector (Behavior Parameters)

> ส่วนขยายใหม่เพื่อความเข้ากันได้กับ VISSIM — พารามิเตอร์เหล่านี้มีผลต่อการจำลองระดับจุลภาค

### 5.1 ตารางพารามิเตอร์

| พารามิเตอร์ | ชนิด | ค่าเริ่มต้น | ช่วง | คำอธิบาย |
| :--- | :--- | :--- | :--- | :--- |
| `desiredSpeed` | `SpeedDistribution` | inherit from Link | 0–∞ | ความเร็วเป้าหมายบน Connector |
| `speedFactor` | double | 1.0 | 0.1–2.0 | ตัวคูณความเร็ว |
| `acceleration` | double | 2.5 m/s² | 0.1–8.0 | ความเร่งสูงสุด |
| `deceleration` | double | 3.0 m/s² | 0.1–8.0 | ความหน่วงปกติ |
| `maxDeceleration` | double | 6.0 m/s² | 0.1–12.0 | ความหน่วงสูงสุด (emergency) |
| `minHeadway` | double | 1.0 m | 0.1–10.0 | ระยะห่างขั้นต่ำ |
| `reactionTime` | double | 1.0 s | 0.1–3.0 | เวลาตอบสนอง |
| `lookAheadDistance` | double | 50.0 m | 0.0–500.0 | ระยะมองล่วงหน้า |
| `lateralBehavior` | enum | `normal` | `conservative`/`normal`/`aggressive` | ลักษณะการขับ |
| `cooperative` | bool | true | — | ยอมให้รถอื่นแทรก |
| `yieldToPedestrians` | bool | true | — | ให้ทางคนเดินเท้าที่ทางข้าม |

### 5.2 Speed Distribution (VISSIM-style)

```json
{
  "desiredSpeed": {
    "type": "normal",
    "mean": 13.9,
    "stdDev": 1.5,
    "min": 8.0,
    "max": 22.0
  }
}
```

รองรับการแจกแจง: `normal`, `uniform`, `empirical`, `constant`

### 5.3 ยานพาหนะที่ใช้พารามิเตอร์

ค่าใน Connector จะ **override** ค่าจาก Link ต้นทาง เฉพาะช่วงที่รถอยู่บน Connector และจะกลับไปใช้ค่าของ Link ปลายทางเมื่อออกจาก Connector

---

## 6. Lane Change Distance และ Emergency Stop

### 6.1 Lane Change Distance (LCD)

ระยะที่รถต้องเริ่มพิจารณาเปลี่ยนเลนก่อนถึง Connector

| พารามิเตอร์ | ชนิด | ค่าเริ่มต้น | คำอธิบาย |
| :--- | :--- | :--- | :--- |
| `laneChangeDistance` | double (m) | 200.0 | ระยะเริ่มพิจารณาเปลี่ยนเลน |
| `emergencyStopDistance` | double (m) | 50.0 | ระยะที่ต้องหยุดถ้าไม่สามารถเปลี่ยนเลนได้ |
| `cooperativeLaneChange` | bool | true | ให้รถในเลนเป้าหมายชะลอให้ |
| `aggressiveLaneChange` | bool | false | ยอมให้เปลี่ยนเลนในระยะกระชั้นชิด |

### 6.2 Emergency Stop Behavior

เมื่อรถเข้าใกล้ Connector ที่ไม่สามารถเข้าถึงได้ (เช่น เลนปลายทางเต็ม) จะเกิดขั้นตอน:

1. **Warning zone** — ระยะ `laneChangeDistance` → ลดความเร็วเล็กน้อย
2. **Critical zone** — ระยะ `emergencyStopDistance` → ลดความเร็วจนหยุด
3. **Blocking** — ถ้าหยุดนานเกิน `blockingTimeout` (ค่าเริ่มต้น 30 วินาที) จะลบรถออกจาก simulation

### 6.3 การตรวจสอบ

- `laneChangeDistance` ต้อง ≥ `emergencyStopDistance`
- `emergencyStopDistance` ต้อง ≥ ความยาวยานพาหนะสูงสุดในชนิดที่อนุญาต
- หากไม่เป็นไปตามนี้ → `EDIT_LANE_CHANGE_DISTANCE`

---

## 7. Conflict Areas และ Priority Rules

### 7.1 Conflict Areas

Connector สองตัวที่ตัดกันทางเรขาคณิตจะสร้าง **Conflict Area** อัตโนมัติ (เว้นแต่จะปิดด้วย `autoConflictArea = false`)

| พารามิเตอร์ | ชนิด | ค่าเริ่มต้น | คำอธิบาย |
| :--- | :--- | :--- | :--- |
| `autoConflictArea` | bool | true | สร้าง conflict area อัตโนมัติ |
| `conflictPriority` | enum | `undefined` | `undefined`/`connector1`/`connector2`/`both` |
| `conflictFrontGap` | double (m) | 3.0 | ระยะห่างด้านหน้าขั้นต่ำ |
| `conflictRearGap` | double (m) | 2.0 | ระยะห่างด้านหลังขั้นต่ำ |
| `conflictVisibility` | double (m) | 100.0 | ระยะมองเห็น |

### 7.2 Priority Rules (VISSIM-style)

สำหรับ **Interior Arrival** ที่ปากทางแยก:

| พารามิเตอร์ | ชนิด | ค่าเริ่มต้น | คำอธิบาย |
| :--- | :--- | :--- | :--- |
| `priorityRuleType` | enum | `gapTime` | `gapTime`/`headway`/`stop` |
| `gapTime` | double (s) | 3.0 | เวลาช่องว่างขั้นต่ำ |
| `headway` | double (m) | 7.0 | ระยะห่างขั้นต่ำ |
| `minGap` | double (m) | 0.5 | ระยะห่างต่ำสุด |
| `maxWaitTime` | double (s) | 60.0 | เวลารอสูงสุด |
| `stopLinePosition` | double (m) | 0.0 | ตำแหน่งเส้นหยุดจากปากทาง |
| `visibilityDistance` | double (m) | 100.0 | ระยะมองเห็นที่ให้ทาง |

### 7.3 ลำดับความสำคัญ

1. Signal Head (ถ้ามี) → override ทุกอย่าง
2. Priority Rule (ถ้ามี)
3. Conflict Area Priority
4. `EDIT_NO_PRIORITY_DEFAULTS` → Run refused หากไม่มีค่าเริ่มต้นที่เป็นบวก

---

## 8. Signal Heads และการควบคุมสัญญาณ

### 8.1 Signal Group

| พารามิเตอร์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `signalGroupId` | string | ผูก Connector กับ signal group |
| `signalControllerId` | string | ผูกกับ controller (fixed-time/actuated) |
| `stopLineOffset` | double (m) | ระยะจากปลาย Connector ถึงเส้นหยุด |
| `signalHeadType` | enum | `vehicle`/`pedestrian`/`bicycle` |
| `signalHeadPosition` | Point | ตำแหน่งบน Connector |

### 8.2 ประเภทสัญญาณ

| ประเภท | คำอธิบาย |
| :--- | :--- |
| **Fixed-time** | รอบเวลาคงที่ (cycle, green, yellow, red) |
| **Actuated** | ปรับตาม detector |
| **Adaptive** | ปรับตาม traffic state (ผ่าน external controller) |

### 8.3 การเชื่อมโยง

- 1 Connector → 1 signal head ได้ (หรือมากกว่า ถ้าเป็น multi-phase movement)
- Signal head ผูกกับ Connector path ID (path 0 โดยค่าเริ่มต้น)
- การลบ Connector → ลบ signal head ที่ผูกอยู่ + ลบ signal group ถ้าไม่มี Connector อื่นอ้างอิง

---

## 9. Routing Decisions และ Static Routes

### 9.1 Routing Decision (VISSIM-style)

กำหนดจุดที่รถตัดสินใจเลือกเส้นทาง

| พารามิเตอร์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `routingDecisionId` | string | ID ของ routing decision |
| `decisionPosition` | double (m) | ตำแหน่งบน Link |
| `destinations` | array | รายการ Connector ปลายทาง + สัดส่วน |
| `routeType` | enum | `static`/`dynamic`/`partial` |
| `lookAheadDistance` | double (m) | ระยะมองล่วงหน้า |

### 9.2 Static Route

| พารามิเตอร์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `routeId` | string | ID ของ route |
| `segmentIds` | array | ลำดับของ Link/Connector path IDs |
| `vehicleCompositionId` | string | ชนิดรถที่อนุญาต |
| `relativeFlow` | double | สัดส่วนการไหล |

### 9.3 ความสัมพันธ์กับ Connector

- Route อ้างอิง Connector ผ่าน **path ID** (เช่น `turn-east/lane-2`)
- Connector ที่ถูกอ้างอิง:
  - ✅ reshape ได้
  - ❌ retarget ไม่ได้
  - ❌ เปลี่ยน lane range ไม่ได้
- การละเมิด → `EDIT_REFERENCED_CONNECTOR`

---

## 10. ความกว้าง ขอบเขต เครื่องหมาย และปากทาง

### 10.1 การตัดสินใจความกว้าง

```
if (authored laneWidths[i])      → use authored
else if (link lane width)        → use link
else                             → 0 (taper)
```

- `connectorLaneWidths` เป็นฟังก์ชันเดียวที่ drawing และ shape diagnostics ใช้
- เลนส่วนเกินที่ปลายแคบกว่าจะมีความกว้าง 0 → taper ตลอดความยาว

### 10.2 ขอบเขต (Boundaries)

- ใช้ mitered offset เดียวกับ Link
- ที่ปากทาง: **Link lane widths ชนะ** ที่รอยต่อ
- ขอบเขตถูก shift ตามยาวให้ตรงกับหน้าตัดของ Link แล้วค่อย transition
- `connectorMouthFit` เปิดเผย shift, transition zone, residual

### 10.3 เครื่องหมาย (Markings)

| ประเภท | ค่าเริ่มต้น | คำอธิบาย |
| :--- | :--- | :--- |
| Outer edges | `solid` | เสมอ |
| Interior dividers | `dashed` | กำหนดได้ |
| Converging | หยุดวาด | เมื่อเลนรวมตัว |

### 10.4 ข้อกำหนด list

- `laneWidths`: ต้องมี **pathCount** ค่า, positive finite
- `laneMarkings`: ต้องมี **pathCount - 1** ค่า
- List ว่าง → ใช้ค่า derived
- Range change → **clear both lists**

### 10.5 Mouth Parameters (ใหม่)

| พารามิเตอร์ | ชนิด | ค่าเริ่มต้น | คำอธิบาย |
| :--- | :--- | :--- | :--- |
| `mouthTransitionLength` | double (m) | 5.0 | ความยาว transition zone |
| `mouthBlendMode` | enum | `linear` | `linear`/`smooth`/`step` |
| `mouthSharpness` | double | 1.0 | ความคมของ transition |

---

## 11. การสร้างและแก้ไขผ่าน Editor

### 11.1 วิธีการสร้าง

| วิธี | ขั้นตอน |
| :--- | :--- |
| **Connectors tool (C)** | เลือกเครื่องมือ → pick source → pick target |
| **Ctrl+right-drag** | ใน Select/Links mode → ลากจาก Link หนึ่งไปยังอีก Link |

### 11.2 การ Snap

ลำดับการ snap:
1. Link endpoint
2. Connector attachment ที่มีอยู่บนเลนนั้น
3. Link intermediate point (ภายใน screen-space tolerance)

- Exact endpoint pick → เก็บโดยไม่มี station

### 11.3 Properties Inspector

- Connector selection
- Source/target Link + lane
- Source/target station (m)
- Source/target lane count
- Intermediate points (0–40)
- Lane widths (comma-separated)
- Interior marking types
- **Behavior parameters** (§5)
- **Lane change distance** (§6)
- **Priority rule** (§7)
- **Signal group** (§8)
- **Routing decision** (§9)
- Reset / Straighten / Delete

### 11.4 Range Handles

- Orange lane tabs → เปลี่ยน leading/trailing edge
- Each release = 1 History transaction
- รองรับ Undo/Redo
- จำกัดด้วย contiguous lanes ที่เหลือ + max 12

### 11.5 การสืบทอดค่าเริ่มต้น

- `level` และ `displayType` สืบทอดจาก source Link
- ID: `connector-N` โดยหลีกเลี่ยงทุก authored + derived ID

---

