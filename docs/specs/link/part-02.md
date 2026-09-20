## 9. Signal Heads และ Signal Controllers

### 9.1 Signal Head

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `id` | string | ID |
| `linkId` | string | Link (หรือ Connector) |
| `laneIds` | array | เลนที่ควบคุม |
| `station` | double | ตำแหน่งบน Link |
| `type` | enum | `vehicle`/`pedestrian`/`bicycle` |
| `signalGroupId` | string | กลุ่มสัญญาณ |
| `signalControllerId` | string | controller |

### 9.2 Signal Controller

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `id` | string | ID |
| `type` | enum | `fixedTime`/`actuated`/`adaptive` |
| `cycleTime` | double (s) | รอบเวลา |
| `offset` | double (s) | offset |
| `phases` | array | ระยะ (green/yellow/red) |
| `coordination` | enum | `none`/`greenWave`/`adaptive` |

### 9.3 Signal Phase

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `phaseId` | string | ID |
| `signalGroupIds` | array | กลุ่มที่ active |
| `duration` | double (s) | ระยะเวลา |
| `minDuration` | double (s) | ขั้นต่ำ (actuated) |
| `maxDuration` | double (s) | สูงสุด |
| `yellowTime` | double (s) | เวลาเหลือง |
| `redClearance` | double (s) | all-red |
| `pedestrian` | bool | มีคนเดินเท้า |

### 9.4 แหล่งสัญญาณ

- **Fixed-time**: ลำดับคงที่
- **Actuated**: ปรับตาม detector
- **Adaptive**: ปรับตาม state (ต้อง external controller)
- **Manual**: ควบคุมจากภายนอก

---

## 10. Routing Decisions และ Parking

### 10.1 Routing Decision

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `id` | string | ID |
| `linkId` | string | Link ที่ติดตั้ง |
| `station` | double | ตำแหน่ง |
| `destinations` | array | Connector ปลายทาง + สัดส่วน |
| `routeType` | enum | `static`/`dynamic`/`partial` |
| `lookAheadDistance` | double | ระยะมองล่วงหน้า |
| `vehicleClasses` | array | ชนิดรถที่ใช้ |

### 10.2 Static Route

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `routeId` | string | ID |
| `segmentIds` | array | Link + Connector path IDs |
| `relativeFlow` | double | สัดส่วน |
| `vehicleComposition` | string | ชนิดรถ |

### 10.3 Parking Area

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `id` | string | ID |
| `linkId` | string | Link |
| `laneId` | string | เลนที่จอด |
| `startStation` | double | จุดเริ่มต้น |
| `endStation` | double | จุดสิ้นสุด |
| `capacity` | int | จำนวนช่อง |
| `type` | enum | `parallel`/`perpendicular`/`angled` |
| `dwellTime` | double (s) | เวลาจอดเฉลี่ย |
| `occupancy` | double | 0.0–1.0 |

### 10.4 Public Transport Stop

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `id` | string | ID |
| `linkId` | string | Link |
| `laneId` | string | เลน |
| `station` | double | ตำแหน่ง |
| `type` | enum | `bus`/`tram`/`bike` |
| `dwellTime` | double (s) | เวลาจอด |
| `routes` | array | เส้นทางที่จอด |

---

## 11. Nodes, Intersections และ Stop Lines

### 11.1 Node

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `id` | string | ID |
| `type` | enum | `priority`/`signalized`/`roundabout`/`merge`/`diverge` |
| `position` | Point | พิกัด |
| `links` | array | Link IDs ที่เชื่อม |
| `connectors` | array | Connector IDs ภายใน |
| `controlType` | enum | `none`/`priority`/`signal`/`allWayStop` |

### 11.2 Stop Line

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `id` | string | ID |
| `linkId` | string | Link |
| `laneIds` | array | เลน |
| `station` | double | ตำแหน่ง |
| `type` | enum | `signal`/`stop`/`yield`/`crosswalk` |
| `width` | double | ความกว้างเส้น |

### 11.3 Crosswalk

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `id` | string | ID |
| `linkId` | string | Link |
| `station` | double | ตำแหน่ง |
| `type` | enum | `zebra`/`signal`/`raised` |
| `width` | double | ความกว้าง |
| `pedestrianFlow` | double | อัตราการมาถึง |

---

## 12. ความกว้าง ขอบเขต และเครื่องหมาย

### 12.1 Cross-Section

Cross-section ที่ station ใดๆ ประกอบด้วย:

- lane ทุกเลนเรียงตาม index
- shoulder (ถ้ามี)
- median (ถ้ามี)
- sidewalk (ถ้ามี)

### 12.2 การคำนวณขอบเขต

```text
offset(0) = 0
offset(i+1) = offset(i) + lane[i].width
width = offset(n) = total
```

- ขอบเขตใช้ mitered offset geometry
- ที่มุมแหลม (< 30°) จะถูกตัด (trim) เพื่อหลีกเลี่ยง self-intersection

### 12.3 เครื่องหมาย

| ประเภท | ค่าเริ่มต้น | คำอธิบาย |
| :--- | :--- | :--- |
| Outer left | `solid` | เสมอ |
| Outer right | `solid` | เสมอ |
| Between lanes | `dashed` | ปรับได้ |
| Center (median) | `double solid` | ถ้ามี median |
| Lane drop | `solid` | ตลอด taper |
| HOV | `dashed` + สี | ตามข้อกำหนด |

### 12.4 Median

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `medianWidth` | double | ความกว้าง |
| `medianType` | enum | `none`/`painted`/`raised`/`grass` |
| `medianHeight` | double | ความสูง |

### 12.5 Shoulder

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `leftShoulderWidth` | double | ไหล่ซ้าย |
| `rightShoulderWidth` | double | ไหล่ขวา |
| `shoulderType` | enum | `paved`/`unpaved`/`none` |

### 12.6 Sidewalk

| ฟิลด์ | ชนิด | คำอธิบาย |
| :--- | :--- | :--- |
| `leftSidewalkWidth` | double | ทางเท้าซ้าย |
| `rightSidewalkWidth` | double | ทางเท้าขวา |
| `sidewalkType` | enum | `paved`/`unpaved` |

---

## 13. การสร้างและแก้ไขผ่าน Editor

### 13.1 วิธีการสร้าง

| วิธี | ขั้นตอน |
| :--- | :--- |
| **Links tool (L)** | เลือกเครื่องมือ → คลิกจุดเริ่มต้น → คลิกจุดถัดไป → double-click สิ้นสุด |
| **Ctrl+drag** | ลากจาก Node หนึ่งไปยังอีก Node |
| **Import** | นำเข้าจากไฟล์ (Shapefile, GeoJSON, OSM) |
| **Split** | แบ่ง Link ที่ station ที่เลือก |

### 13.2 การ Snap

1. Node endpoint
2. Link endpoint
3. Grid (ถ้าเปิด)
4. อิสระ

### 13.3 Properties Inspector

- Link ID, name, type
- Geometry (points)
- Lanes (เพิ่ม/ลบ/แก้ไข)
- Driving side
- Level, display type
- Behavior parameters (§5)
- Lane change parameters (§6)
- Endpoints (start/end node)
- Start station, length
- Visual (§18)
- Actions: Reset curve, Straighten, Reverse, Split, Extend

### 13.4 Lane Operations

| การดำเนินการ | คำอธิบาย |
| :--- | :--- |
| Add lane | เพิ่มเลนที่ตำแหน่งระบุ |
| Remove lane | ลบเลน |
| Change width | แก้ความกว้าง |
| Change type | เปลี่ยนชนิดเลน |
| Add taper | เพิ่ม taper (lane add/drop) |
| Reverse order | กลับลำดับเลน |

### 13.5 การสืบทอดค่าเริ่มต้น

- `level` และ `displayType` สืบทอดจาก project default
- `behavior` สืบทอดจาก project default
- ID: `link-N` โดยหลีกเลี่ยงทุก ID

---

## 14. Command Contract และการอ้างอิง

### 14.1 ตาราง Command

| Command | ผลลัพธ์ |
| :--- | :--- |
| `addLink` | สร้าง Link ใหม่ |
| `addLinkRange` | สร้าง Link หลายช่วงต่อเนื่อง |
| `changeLinkGeometry` | แทน polyline + re-read endpoints |
| `changeLinkEndpoints` | retarget start/end node |
| `changeLinkLanes` | แก้ lane definitions |
| `changeLinkBehavior` | แก้ §5 |
| `changeLinkLaneChange` | แก้ §6 |
| `changeLinkType` | เปลี่ยนประเภท |
| `changeLinkDirection` | กลับทิศ |
| `splitLink` | แบ่ง Link |
| `mergeLinks` | รวม Link |
| `deleteLink` | ลบ + dependent objects |
| `addDetector` / `changeDetector` / `deleteDetector` | §8 |
| `addSignalHead` / `changeSignalHead` / `deleteSignalHead` | §9 |
| `addSignalController` / `changeSignalController` | §9 |
| `addRoutingDecision` / `changeRoutingDecision` | §10 |
| `addParking` / `changeParking` | §10 |
| `addStopLine` / `changeStopLine` | §11 |
| `anchorLinks` | ย้าย endpoints ไปยัง node |
| `reanchorLinks` | คงตำแหน่ง + re-read references |

### 14.2 การอ้างอิง

Link ถือว่า **referenced** เมื่อ:
- Connector ใดๆ ใช้ `linkId`
- Route ใดๆ ใช้ Link ID
- Signal head / detector / parking ติดตั้งอยู่
- Lane ID ถูกใช้ใน route / connector

**ผลกระทบ**:
- ✅ reshape ได้ (ถ้าไม่เปลี่ยน topology)
- ⚠️ เปลี่ยน lane count ได้ แต่ต้องอัปเดต Connector ที่อ้างอิง
- ❌ ลบได้ แต่ต้องลบ dependent objects ทั้งหมด
- ❌ เปลี่ยนทิศทางได้ แต่ต้อง re-anchor Connector ทั้งหมด

### 14.3 Dependency Cleanup

เมื่อลบ Link:
1. ลบ Connector ทั้งหมดที่อ้าง Link นี้
2. ลบ Signal head, detector, parking, routing decision
3. ลบ Route ที่ใช้ Link นี้
4. ลบ Vehicle input ที่ผูกกับ Route เหล่านั้น
5. ทั้งหมดใน **transaction เดียว**

---

