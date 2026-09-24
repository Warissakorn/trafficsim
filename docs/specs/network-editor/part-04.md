## 19. Multi-Document และ Project Management

### 19.1 Document Model

```
Project
├── Network (1)
│   ├── Links
│   ├── Connectors
│   ├── Nodes
│   └── ...
├── Scenarios (N)
│   ├── Demand
│   ├── Signals
│   └── ...
├── Results (N)
└── Settings
```

### 19.2 Multiple Documents

- เปิดได้หลายโปรเจกต์พร้อมกัน
- Tab bar สำหรับสลับ
- Drag tab ออก → หน้าต่างใหม่

### 19.3 Project File Structure

```
project.tsp/
├── project.json          # metadata
├── network.json          # โครงข่าย
├── scenarios/
│   ├── default.json
│   └── peak.json
├── results/
│   └── run-2024-01-15.h5
└── assets/
    ├── icons/
    └── textures/
```

### 19.4 Project Settings

- Default behavior parameters
- Default display theme
- Units (metric / imperial)
- Driving side
- Locale

### 19.5 Recent Files

- แสดง 10 ไฟล์ล่าสุด
- Pin ได้
- Clear ได้

### 19.6 Auto-Recovery

- บันทึก recovery ทุก 60 วินาที
- เก็บใน `%TEMP%` หรือ user config
- กู้คืนอัตโนมัติเมื่อเปิดใหม่

---

## 20. Performance และ Optimization

### 20.1 เป้าหมาย Performance

| ขนาดโครงข่าย | เป้าหมาย FPS | Memory |
| :--- | :--- | :--- |
| < 100 objects | 60 | < 100 MB |
| 100–1,000 | 60 | < 500 MB |
| 1,000–10,000 | 30 | < 2 GB |
| 10,000–100,000 | 15 | < 8 GB |

### 20.2 Optimization Techniques

| เทคนิค | ใช้ที่ไหน |
| :--- | :--- |
| **Spatial index** | Hit test, culling |
| **Frustum culling** | Rendering |
| **LOD** | Rendering |
| **Dirty rectangle** | Rendering |
| **Batch rendering** | วาด object คล้ายกันพร้อมกัน |
| **Cached geometry** | Offset polyline, boundary |
| **Lazy validation** | Validate เฉพาะที่เปลี่ยน |
| **Object pooling** | ลด allocation |
| **Multi-threading** | Geometry calc, validation |
| **GPU acceleration** | Rendering (OpenGL/Vulkan) |

### 20.3 Profiling

- ในตัว: frame time, memory, object count
- External: Tracy, VTune, RenderDoc

### 20.4 Memory Management

- ใช้ `std::unique_ptr` / `shared_ptr`
- ลด copy
- ใช้ `string_view` สำหรับ string
- ใช้ arena allocator สำหรับ geometry ชั่วคราว

### 20.5 Rendering Optimization

- วาดเฉพาะ visible objects
- ใช้ instancing สำหรับ object ที่ซ้ำ
- เก็บ geometry ใน GPU buffer
- ใช้ shader สำหรับ markings

---

## 21. Accessibility และ Localization

### 21.1 Accessibility

- รองรับ screen reader (ผ่าน accessibility API)
- Keyboard navigation ครบถ้วน
- High contrast mode
- ปรับขนาด font ได้
- ปรับขนาด handle ได้

### 21.2 Localization

- ใช้ไฟล์ JSON ใน `data/locales/`
- รองรับ: Thai, English, (เพิ่มได้)
- เปลี่ยนภาษาได้ runtime
- รองรับ RTL (roadmap)

### 21.3 Number/Date Format

- ตาม locale
- Metric / Imperial
- ทศนิยม 2–3 ตำแหน่ง

---

## 22. Testing Strategy

### 22.1 Test Levels

| ระดับ | ขอบเขต | เครื่องมือ |
| :--- | :--- | :--- |
| **Unit** | ฟังก์ชันเดี่ยว | Catch2, GoogleTest |
| **Integration** | Module ร่วมกัน | Catch2 |
| **UI** | Interaction | Qt Test, custom |
| **Visual** | Rendering | Image diff |
| **Performance** | FPS, memory | Benchmark |
| **End-to-end** | Scenario เต็ม | Custom |

### 22.2 Test Cases ที่สำคัญ

- สร้าง Link/Connector ผ่าน UI
- Undo/redo ทุก action
- Snap ทำงานถูกต้อง
- Hit test ถูกต้อง
- Validation ทำงาน real-time
- Save/load project
- Import/export
- Performance กับ 10,000 objects

### 22.3 Visual Regression

- เก็บ reference image
- เปรียบเทียบเมื่อ render
- Tolerance 0.1%

### 22.4 Fuzzing

- Random input
- Random mouse movements
- ตรวจสอบ crash / hang

### 22.5 CI/CD

- รัน test ทุก commit
- รัน benchmark ทุกคืน
- รัน visual test ทุก release

---

## 23. Source Map

| Concern | Primary Source |
| :--- | :--- |
| Main window | `src/editor/editor_window.cpp` |
| Canvas abstraction | `src/editor/canvas_base.cpp` |
| Link interaction | `src/editor/canvas_links.cpp` |
| Connector interaction | `src/editor/canvas_connectors.cpp` |
| Lane handles | `src/editor/canvas_lanes.cpp` |
| Selection | `src/editor/selection.cpp` |
| Snapping | `src/editor/snapping.cpp` |
| Hit testing | `src/editor/hit_test.cpp` |
| Camera | `src/editor/camera.cpp` |
| Renderer | `src/editor/renderer.cpp` |
| Tools | `src/editor/tools/*.cpp` |
| Command system | `src/commands/*.cpp` |
| History | `src/commands/history.cpp` |
| Link inspector | `src/shell/editor_links.cpp` |
| Connector inspector | `src/shell/editor_connectors.cpp` |
| Toolbar | `src/shell/toolbar.cpp` |
| Status bar | `src/shell/status_bar.cpp` |
| Context menu | `src/shell/context_menu.cpp` |
| Project management | `src/project/project.cpp` |
| Import/export | `src/project/io.cpp` |
| Localization | `data/locales/*.json` |

---

## 24. ภาคผนวก: Use Case Walkthrough

### 24.1 สร้าง 4-Way Intersection

**ขั้นตอน**:

1. เปิดโปรเจกต์ใหม่ (`Ctrl+N`)
2. เลือก Link tool (`L`)
3. ลาก Link `west` (2 เลน, 200 m)
4. ลาก Link `east` (2 เลน, 200 m) — ต่อจาก `west`
5. ลาก Link `north` (2 เลน, 150 m) — ตั้งฉาก
6. ลาก Link `south` (2 เลน, 150 m) — ตั้งฉาก
7. ทั้ง 4 Link ควรมี endpoint ร่วมกัน → snap เป็น Node อัตโนมัติ
8. เลือก Connector tool (`C`)
9. สร้าง Connector:
   - `west → east` (through) ×2
   - `west → south` (left turn)
   - `west → north` (right turn)
   - ... (ทำนองเดียวกับทุก approach)
10. เลือก Signal tool (`G`)
11. วาง Signal head ที่ stop line ของแต่ละ approach
12. เลือก Detector tool (`D`)
13. วาง Detector 50 m upstream ของ stop line
14. ตั้งค่า Signal Controller ใน Inspector
15. รัน validation (`Ctrl+Shift+V`) → ต้องไม่มี error
16. Save (`Ctrl+S`)

### 24.2 สร้าง Freeway Merge

1. สร้าง `main` Link (3 เลน, 500 m, motorway)
2. สร้าง `onramp` Link (1 เลน, 200 m, trunk)
3. สร้าง Connector `onramp → main` (1→1)
4. ตั้ง Priority Rule: `onramp` ให้ทาง `main`
5. วาง Detector ที่ upstream และ downstream
6. ตั้ง Vehicle Inputs
7. รัน simulation (`F5`)
8. ดูผลลัพธ์ใน Analyzer

### 24.3 แก้ไข Connector ที่มีอยู่

1. เลือก Connector
2. Inspector แสดง properties
3. ลาก interior point → เปลี่ยนรูปทรง
4. หรือกด "Reset curve" → คืนค่า default
5. ถ้า Connector ถูกอ้างอิง → fields retarget/range จะ disable
6. Undo (`Ctrl+Z`) ถ้าต้องการย้อน

### 24.4 Import จาก OSM

1. `File → Import → OSM`
2. เลือกพื้นที่ (bounding box)
3. เลือก layers (roads, intersections)
4. รอ import
5. ตรวจสอบโครงข่ายที่ได้
6. Snap endpoints เข้าหากัน
7. สร้าง Connectors ที่ทางแยก
8. Save

---

## สรุป

Network Editor ที่ออกแบบตามเอกสารนี้จะเป็นเครื่องมือที่:

- **ทรงพลัง**: รองรับโครงข่ายขนาดใหญ่ 10,000+ objects
- **ใช้งานง่าย**: direct manipulation + snap + preview
- **ปลอดภัย**: undo/redo + validation + transaction
- **ขยายได้**: เพิ่ม object type / tool ใหม่ได้ง่าย
- **มืออาชีพ**: เทียบเท่า VISSIM ในด้าน UX

โดยยึดหลักการ **"Authoring-first, Direct manipulation, Immediate feedback, Non-destructive"** ตามปรัชญาที่วางไว้

พร้อมสำหรับการ implement ต่อในระดับ production

---

**ภาคผนวก B: เปรียบเทียบ Network Editor กับ VISSIM**

| มิติ | Editor ของคุณ | VISSIM |
| :--- | :--- | :--- |
| Canvas | ✅ GPU-accelerated | ✅ |
| Tools | 11 tools | ~15 tools |
| Snap | 12 ประเภท | 8 ประเภท |
| Selection | ✅ multi, filter, cascade | ✅ |
| Undo/Redo | ✅ transaction | ✅ |
| Inspector | ✅ live edit | ✅ |
| Layers | ✅ toggle/lock/reorder | ✅ |
| Validation | ✅ real-time | ✅ on-demand |
| Import | GeoJSON, OSM | หลายรูปแบบ |
| Export | GeoJSON, CSV, PNG | หลายรูปแบบ |
| Multi-doc | ✅ tab | ✅ |
| Auto-save | ✅ | ✅ |
| Localization | Thai/English | หลายภาษา |
| Accessibility | ✅ | จำกัด |
| Performance | 100k objects | ~50k objects |
| **ราคา** | ฟรี / open | แพงมาก |

---

ถ้าต้องการให้ขยายหัวข้อใดเป็นพิเศษ (เช่น การ implement renderer ด้วย OpenGL/Vulkan, การออกแบบ UI ด้วย Qt, หรือตัวอย่าง code สำหรับ tool ใด tool หนึ่ง) แจ้งได้เลยครับ