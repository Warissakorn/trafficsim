## 21. ภาคผนวก: ตัวอย่าง Link ใน Scenario จริง

### 21.1 Intersection 4 แยก แบบ Fixed-Time

```
                North
                  │
                  │  north-1 (2 เลน)
                  │
West ─── west-1 ──┼── east-1 ─── East
       west-2     │   east-2
                  │
                  │  south-1 (2 เลน)
                  │
                South
```

**Links**:
- `west` : 2 เลน, ความยาว 200 m, type=primary
- `east` : 2 เลน, ความยาว 200 m, type=primary
- `north`: 2 เลน, ความยาว 150 m, type=secondary
- `south`: 2 เลน, ความยาว 150 m, type=secondary

**Connectors**:
- `west-east/lane-1`, `west-east/lane-2` (through)
- `west-south/lane-1` (left turn)
- `west-north/lane-1` (right turn)
- ... (ทำนองเดียวกันสำหรับทุกทิศ)

**Signal Controller**:
- `ctrl-1` : 4-phase fixed-time
  - Phase A: E-W through (green 30s, yellow 3s, all-red 2s)
  - Phase B: E-W left turn (green 15s, yellow 3s, all-red 2s)
  - Phase C: N-S through (green 30s, yellow 3s, all-red 2s)
  - Phase D: N-S left turn (green 15s, yellow 3s, all-red 2s)

**Detectors**:
- `det-w-1`, `det-w-2`: 50 m upstream ของ stop line
- `det-e-1`, `det-e-2`: 50 m upstream
- `det-n-1`, `det-n-2`: 50 m upstream
- `det-s-1`, `det-s-2`: 50 m upstream

**Routing**:
- 70% through, 15% left, 15% right (ที่แต่ละ approach)

**Vehicle Inputs**:
- West: 800 veh/h
- East: 700 veh/h
- North: 500 veh/h
- South: 400 veh/h

### 21.2 Freeway Merge

```
       ┌──── main-1 (3 เลน, 100 km/h)
       │
main ──┤
       │
       └──── onramp-1 (1 เลน, 60 km/h)
              │
              └── merge ──> main-2 (3 เลน)
```

**Links**:
- `main-1`: 3 เลน, type=motorway
- `onramp-1`: 1 เลน, type=trunk
- `main-2`: 3 เลน, type=motorway

**Connectors**:
- `onramp-merge`: 1→1 (merge เข้าเลนขวาสุด)

**Priority Rule**:
- `onramp` ให้ทาง `main-1`

**Detectors**:
- `det-main-up`: 100 m upstream
- `det-onramp-up`: 100 m upstream
- `det-merge`: 10 m downstream ของ merge point

### 21.3 Roundabout

```
         north
           │
     ┌─────┴─────┐
     │           │
west─┤  round-1  ├─east
     │  (circular)│
     └─────┬─────┘
           │
         south
```

**Links**:
- `north`, `east`, `south`, `west`: 1 เลน, type=secondary
- `round-1`: 1 เลน (วงกลม), type=secondary

**Connectors**:
- entry: 1→1 (เข้า round)
- exit: 1→1 (ออก round)
- circulations: 1→1 (วนใน round)

**Priority Rule**:
- รถที่เข้า round ให้ทางรถใน round

**Detectors**:
- entry detector, exit detector, circulation detector

---

## สรุป

เอกสารนี้ยกระดับ Link spec ให้เป็น **ระบบ Link ระดับ microsimulation** ที่เทียบเท่า VISSIM ในแง่ของ:

- พารามิเตอร์พฤติกรรม (behavior)
- การเปลี่ยนเลน (lane change)
- ลำดับชั้น (hierarchy)
- Detector และ DCP
- Signal และ controller
- Routing และ parking
- Node และ intersection
- Cross-section และ marking

โดยยังคงหลักการ **"Reference-based geometry"**, **"Cross-section consistency"**, **"Deterministic compilation"** และ **"Authored base, Derived detail"** ของระบบเดิมไว้

พร้อมสำหรับการนำไป implement, ทดสอบ, และ calibrate กับข้อมูลจริงต่อไป

---

**ภาคผนวก B: เปรียบเทียบ Link ของคุณกับ VISSIM**

| มิติ | Link ของคุณ (v7) | VISSIM |
| :--- | :--- | :--- |
| แนวคิด | Directed road segment | Directed road segment |
| เรขาคณิต | Polyline + spline sampling | Polyline + spline |
| Cross-section | Lane array + shoulder + median + sidewalk | Lane array + shoulder |
| Lane types | 10 ชนิด | 5 ชนิด |
| Driving side | ✅ | ✅ |
| Behavior params | ✅ | ✅ |
| Lane change | ✅ | ✅ |
| Hierarchy | ✅ | ✅ |
| Detectors | ✅ | ✅ |
| Signal | ✅ | ✅ |
| Routing | ✅ | ✅ |
| Parking | ✅ | ✅ |
| Multi-modal | จำกัด | ✅ |
| Emission | ❌ (roadmap) | ✅ |
| AV behavior | ❌ (roadmap) | ✅ |
| External controller | ❌ (roadmap) | ✅ |

---

ถ้าต้องการให้ขยายหัวข้อใดเป็นพิเศษ (เช่น พารามิเตอร์ calibration, ตัวอย่าง scenario เต็มรูปแบบ, หรือ schema migration) แจ้งได้เลยครับ