# เอกสารข้อกำหนด Network Editor (ฉบับสมบูรณ์ — สำหรับระบบ Traffic Simulation ระดับ VISSIM)

> เอกสารนี้อธิบายการออกแบบและสร้าง **Network Editor** ซึ่งเป็นเครื่องมือหลักที่ผู้ใช้ใช้สร้าง/แก้ไขเครือข่ายถนน (Links, Connectors, Nodes, Signals, Detectors ฯลฯ) ครอบคลุมสถาปัตยกรรม, interaction model, rendering, data flow, undo/redo, snapping, validation, ไปจนถึง performance และ testing โดยสอดคล้องกับ Link spec และ Connector spec (Schema v7) ที่กล่าวมาแล้ว

---

## สารบัญ

1. ภาพรวมและปรัชญาการออกแบบ
2. สถาปัตยกรรมระบบ (System Architecture)
3. โครงสร้างโมดูล (Module Structure)
4. Canvas และ Rendering Pipeline
5. ระบบพิกัดและการแปลง (Coordinate Systems)
6. Camera และ Viewport
7. Interaction Model และ Tools
8. Selection System
9. Snapping และ Constraint System
10. Drag & Drop Operations
11. Handle System (Lane Tabs, Endpoints, Range Handles)
12. Command System และ Undo/Redo
13. Properties Inspector
14. Layer System และ Z-Ordering
15. Hit Testing และ Picking
16. Validation Feedback แบบ Real-time
17. Context Menu และ Keyboard Shortcuts
18. Import/Export และ Interop
19. Multi-Document และ Project Management
20. Performance และ Optimization
21. Accessibility และ Localization
22. Testing Strategy
23. Source Map
24. ภาคผนวก: Use Case Walkthrough

---

## 1. ภาพรวมและปรัชญาการออกแบบ

### 1.1 Network Editor คืออะไร

**Network Editor** คือ GUI component ที่ให้ผู้ใช้:

- **สร้าง** Links, Connectors, Nodes, Signals, Detectors, Parking ฯลฯ
- **แก้ไข** geometry, lanes, parameters ของ object เหล่านั้น
- **ตรวจสอบ** ความถูกต้อง (validation) แบบ real-time
- **บันทึก/โหลด** โครงข่ายเป็น project JSON
- **Simulate** โครงข่ายเพื่อดูผลลัพธ์

### 1.2 ปรัชญาการออกแบบ

| หลักการ | คำอธิบาย |
| :--- | :--- |
| **Authoring-first** | Editor คือที่ที่ผู้ใช้ "วาด" โครงข่าย ไม่ใช่แค่แก้ JSON |
| **Direct manipulation** | คลิก/ลากบน object โดยตรง ไม่ต้องผ่าน dialog |
| **Immediate feedback** | ทุก action แสดงผลทันที (validation, preview) |
| **Non-destructive** | ใช้ command pattern + undo/redo ทุกอย่าง |
| **Modal clarity** | Tool แต่ละอันมี mode ชัดเจน ไม่สับสน |
| **Snap intelligently** | Snap ช่วยสร้างโครงข่ายที่ถูกต้อง |
| **Layer separation** | Model / View / Controller แยกชัดเจน |
| **Performance-aware** | รองรับโครงข่ายขนาดใหญ่ (10,000+ objects) |
| **Extensible** | เพิ่ม object type ใหม่ได้โดยไม่ต้องแก้ core |
| **Consistent UX** | ทุก object ใช้ interaction pattern เดียวกัน |

### 1.3 ขอบเขตของ Editor

| ทำได้ | ไม่ทำ |
| :--- | :--- |
| สร้าง/แก้ไข geometry | คำนวณ simulation physics |
| จัดการ topology | รัน simulation |
| ตั้งค่า parameters | วิเคราะห์ผลลัพธ์ (ดูใน Analyzer) |
| Validate แบบ real-time | Calibrate model |
| Undo/redo | Multi-user collaboration (roadmap) |
| Import/export | Cloud sync (roadmap) |

---

## 2. สถาปัตยกรรมระบบ (System Architecture)

### 2.1 Layered Architecture

```
┌─────────────────────────────────────────────────────┐
│                   Application Shell                  │
│  (Menus, Toolbar, Status Bar, Dialogs, Shortcuts)   │
└───────────────────────┬─────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────┐
│                    Editor Shell                      │
│  (Editor Window, Panels, Docks, Layout Manager)     │
└───────────────────────┬─────────────────────────────┘
                        │
        ┌───────────────┼───────────────┐
        │               │               │
┌───────▼──────┐ ┌──────▼──────┐ ┌──────▼──────┐
│   Canvas     │ │ Properties  │ │   Toolbox   │
│   (View)     │ │  Inspector  │ │   (Tools)   │
└───────┬──────┘ └──────┬──────┘ └──────┬──────┘
        │               │               │
        └───────────────┼───────────────┘
                        │
┌───────────────────────▼─────────────────────────────┐
│                  Editor Controller                   │
│  (Interaction, Selection, Snapping, Hit Testing)    │
└───────────────────────┬─────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────┐
│                  Command System                      │
│     (History, Transactions, Undo/Redo Stack)        │
└───────────────────────┬─────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────┐
│                   Network Model                      │
│  (Link, Connector, Node, Signal, Detector, ...)     │
└───────────────────────┬─────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────┐
│              Persistence & Validation                │
│   (JSON parse/write, Draft Validation, Diagnostics) │
└─────────────────────────────────────────────────────┘
```

### 2.2 Data Flow

```
User Input (mouse/keyboard)
        │
        ▼
Input Handler → Tool → Interaction State
        │
        ▼
Controller → Command (with parameters)
        │
        ▼
History::execute(command)
        │
        ├──> Validation (draft) ──[fail]──> rollback + show error
        │
        ▼
Model Mutation (atomic)
        │
        ▼
Notify Observers (Observer Pattern)
        │
        ├──> Canvas.markDirty() → repaint
        ├──> Inspector.refresh()
        └──> StatusBar.update()
```

### 2.3 Design Patterns ที่ใช้

| Pattern | ใช้ที่ไหน | เหตุผล |
| :--- | :--- | :--- |
| **MVC/MVVM** | ทั้งระบบ | แยก model/view/controller |
| **Command** | ทุก mutation | undo/redo, transaction |
| **Observer** | Model → View | แจ้งเตือนการเปลี่ยนแปลง |
| **State** | Tool interaction | จัดการ state ของแต่ละ tool |
| **Strategy** | Snapping, hit testing | เปลี่ยน algorithm ได้ |
| **Factory** | Object creation | สร้าง object จาก type |
| **Composite** | Selection, layers | จัดการ group |
| **Visitor** | Validation, rendering | traverse object tree |
| **Flyweight** | Rendering cache | ประหยัด memory |
| **Memento** | Undo/redo | เก็บ state |

---

## 3. โครงสร้างโมดูล (Module Structure)

```
src/
├── model/
│   └── network/
│       ├── network.hpp              # Data types
│       ├── link_geometry.cpp        # Link geometry
│       ├── connector_paths.cpp      # Connector paths
│       ├── road_boundaries.cpp      # Widths, boundaries
│       ├── sections.cpp             # Runtime cutting
│       ├── compile.cpp              # Scenario compilation
│       └── validate.cpp             # Draft validation
│
├── commands/
│   ├── command_base.hpp             # Command interface
│   ├── history.cpp                  # Undo/redo stack
│   ├── link_commands.cpp            # Link mutations
│   ├── connector_commands.cpp       # Connector mutations
│   ├── node_commands.cpp            # Node mutations
│   └── signal_commands.cpp          # Signal mutations
│
├── editor/
│   ├── editor_window.cpp            # Main window
│   ├── canvas_base.cpp              # Canvas abstraction
│   ├── canvas_links.cpp             # Link interaction
│   ├── canvas_connectors.cpp        # Connector interaction
│   ├── canvas_nodes.cpp             # Node interaction
│   ├── canvas_lanes.cpp             # Lane handles
│   ├── canvas_signals.cpp           # Signal placement
│   ├── canvas_detectors.cpp         # Detector placement
│   ├── selection.cpp                # Selection set
│   ├── snapping.cpp                 # Snap engine
│   ├── hit_test.cpp                 # Hit testing
│   ├── camera.cpp                   # Camera/viewport
│   ├── renderer.cpp                 # Rendering pipeline
│   └── tools/
│       ├── tool_base.hpp            # Tool interface
│       ├── select_tool.cpp          # Select/move
│       ├── link_tool.cpp            # Create link
│       ├── connector_tool.cpp       # Create connector
│       ├── node_tool.cpp            # Create node
│       ├── signal_tool.cpp          # Place signal
│       └── detector_tool.cpp        # Place detector
│
├── shell/
│   ├── editor_links.cpp             # Link inspector
│   ├── editor_connectors.cpp        # Connector inspector
│   ├── editor_nodes.cpp             # Node inspector
│   ├── editor_signals.cpp           # Signal inspector
│   ├── editor_detectors.cpp         # Detector inspector
│   ├── toolbar.cpp                  # Toolbar
│   ├── status_bar.cpp               # Status bar
│   └── context_menu.cpp             # Right-click menu
│
├── project/
│   ├── parse.cpp                    # JSON read
│   ├── document.cpp                 # JSON write
│   ├── project.cpp                  # Project management
│   └── migration.cpp                # Schema migration
│
├── ui/
│   ├── widgets/                     # Custom widgets
│   ├── dialogs/                     # Dialogs
│   └── theme/                       # Colors, icons
│
└── data/
    └── locales/
        ├── th.json                  # Thai UI
        └── en.json                  # English UI
```

---

## 4. Canvas และ Rendering Pipeline

### 4.1 Canvas Architecture

```
Canvas
├── Viewport              # Visible area in world coordinates
├── Camera                # Pan, zoom, rotation
├── Renderer              # Draw objects
├── LayerStack            # Z-ordering
├── Grid                  # Background grid
├── Overlays              # Tooltips, handles, guides
└── InputHandler          # Mouse/keyboard events
```

### 4.2 Rendering Pipeline

```
1. Clear back buffer
2. Apply camera transform (pan, zoom)
3. Draw background (grid, world bounds)
4. For each layer (bottom → top):
   a. Draw layer content (culled by viewport)
   b. Draw layer overlays
5. Draw selection highlights
6. Draw handles (endpoints, lane tabs, range handles)
7. Draw snap guides
8. Draw tool preview (rubber band, ghost)
9. Draw tooltips
10. Draw HUD (coordinates, zoom level)
11. Swap buffers
```

### 4.3 Layer Structure (Z-Order)

| Z | Layer | ตัวอย่าง |
| :--- | :--- | :--- |
| -100 | Grid | เส้น grid |
| -50 | Background | พื้นที่ |
| 0 | Road surface | Links, Connectors (พื้น) |
| 10 | Markings | เส้นแบ่งเลน |
| 20 | Boundaries | ขอบถนน |
| 30 | Objects | Signals, Detectors, Parking |
| 40 | Labels | ชื่อ, หมายเลข |
| 50 | Selection | ไฮไลต์ |
| 60 | Handles | endpoints, lane tabs |
| 70 | Guides | เส้น snap, guide |
| 80 | Preview | ghost, rubber band |
| 90 | Tooltips | ข้อความ |
| 100 | HUD | พิกัด, zoom |

### 4.4 Rendering Modes

| Mode | ความเร็ว | คุณภาพ | ใช้เมื่อ |
| :--- | :--- | :--- | :--- |
| **Full** | ช้า | สูง | zoom > 1.0× |
| **Simplified** | ปานกลาง | กลาง | 0.5×–1.0× |
| **Schematic** | เร็ว | ต่ำ | 0.25×–0.5× |
| **Abstract** | เร็วมาก | ต่ำสุด | < 0.25× |

### 4.5 Culling

- **Frustum culling**: ไม่วาด object นอก viewport
- **LOD (Level of Detail)**: ลดรายละเอียดเมื่อ zoom ออก
- **Dirty rectangle**: วาดเฉพาะพื้นที่ที่เปลี่ยน
- **Spatial index**: R-tree / Quadtree สำหรับ query เร็ว

### 4.6 Anti-aliasing

- เปิด MSAA (Multisample Anti-Aliasing) 4× เป็นค่าเริ่มต้น
- ปรับได้ 0×, 2×, 4×, 8×
- ใช้ FXAA เมื่อ MSAA ไม่พอ

### 4.7 Color Palette

```cpp
struct Theme {
    Color background     = #1e1e1e;   // dark mode
    Color grid           = #2a2a2a;
    Color gridMajor      = #3a3a3a;
    Color roadSurface    = #4a4a4a;
    Color laneMarking    = #ffffff;
    Color boundary       = #e0e0e0;
    Color selection      = #3f8efc;
    Color hover          = #5a9efc;
    Color handleDefault  = #ffffff;
    Color handleActive   = #ffcc00;
    Color snapGuide      = #00ff88;
    Color errorHighlight = #ff4444;
    Color warningHighlight = #ffaa00;
    Color text           = #e0e0e0;
};
```

---

## 5. ระบบพิกัดและการแปลง (Coordinate Systems)

### 5.1 Coordinate Systems

| ระบบ | หน่วย | ใช้ที่ไหน |
| :--- | :--- | :--- |
| **World** | เมตร | เก็บข้อมูล (model) |
| **Screen** | pixel | แสดงผล |
| **Viewport** | pixel (relative to canvas) | clip |
| **NDC** | -1..1 | GPU pipeline |
| **Station** | เมตร (ตาม polyline) | Link/Connector positions |
| **Lane-relative** | เมตร (offset จาก centerline) | lateral position |

### 5.2 การแปลง (Transforms)

```cpp
// World → Screen
Point2 screen = camera.worldToScreen(world);

// Screen → World
Point2 world = camera.screenToWorld(screen);

// Station → World (บน Link)
Point2 world = link.pointAtStation(station);

// World → Station (บน Link)
double station = link.stationAtPoint(world);

// World → Lane-relative
double lateral = link.lateralOffset(world);
```

### 5.3 Camera Transform

```cpp
struct Camera {
    Point2 center;       // world coordinate at screen center
    double zoom;         // pixels per meter
    double rotation;     // radians (0 = north up)

    Matrix3x3 worldToScreen() const;
    Matrix3x3 screenToWorld() const;
};
```

### 5.4 Grid System

| ประเภท | ค่าเริ่มต้น | คำอธิบาย |
| :--- | :--- | :--- |
| Minor grid | 1 m | เส้นบาง |
| Major grid | 10 m | เส้นหนา |
| Snap grid | 5 m | จุด snap (ถ้าเปิด) |
| Adaptive | อัตโนมัติ | ปรับตาม zoom |

**Adaptive Grid Algorithm**:
```cpp
double spacing = 1.0;
while (spacing * zoom < 20) spacing *= 10;  // minimum 20 px
while (spacing * zoom > 200) spacing /= 10; // maximum 200 px
```

---

## 6. Camera และ Viewport

### 6.1 Camera Operations

| การดำเนินการ | Mouse | Keyboard |
| :--- | :--- | :--- |
| Pan | Middle-drag / Space+drag | Arrow keys |
| Zoom | Scroll wheel | `+` / `-` |
| Zoom to fit | `Ctrl+0` | — |
| Zoom to selection | `Ctrl+Shift+0` | — |
| Zoom to 100% | `Ctrl+1` | — |
| Rotate | `Alt+Middle-drag` | — |
| Reset view | `Ctrl+Home` | — |

### 6.2 Viewport Bounds

```cpp
struct Viewport {
    Rect2 worldBounds;   // visible area
    Rect2 screenBounds;  // canvas rect
    double zoom;

    bool contains(Point2 world) const;
    bool intersects(Rect2 world) const;
};
```

### 6.3 Zoom Limits

| พารามิเตอร์ | ค่า |
| :--- | :--- |
| Min zoom | 0.01 px/m (มองเห็น 100 km) |
| Max zoom | 100 px/m (มองเห็น 10 cm) |
| Default zoom | 1 px/m |
| Zoom step | 1.2× ต่อ scroll |

### 6.4 Camera Animation

- **Smooth pan**: interpolate 200 ms
- **Smooth zoom**: interpolate 150 ms, zoom ที่ pointer
- **Zoom to fit**: interpolate 300 ms

---

