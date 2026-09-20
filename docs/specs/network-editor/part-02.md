## 7. Interaction Model และ Tools

### 7.1 Tool System

```cpp
class Tool {
public:
    virtual void onActivate(EditorContext&) = 0;
    virtual void onDeactivate(EditorContext&) = 0;
    virtual void onMouseDown(MouseEvent&) = 0;
    virtual void onMouseMove(MouseEvent&) = 0;
    virtual void onMouseUp(MouseEvent&) = 0;
    virtual void onKeyDown(KeyEvent&) = 0;
    virtual void onKeyUp(KeyEvent&) = 0;
    virtual void drawPreview(Renderer&) = 0;
    virtual Cursor cursor() const = 0;
};
```

### 7.2 รายการ Tools

| Tool | Shortcut | หน้าที่ |
| :--- | :--- | :--- |
| **Select** | `S` | เลือก/ย้าย/แก้ไข |
| **Link** | `L` | สร้าง Link |
| **Connector** | `C` | สร้าง Connector |
| **Node** | `N` | สร้าง Node |
| **Signal** | `G` | วาง Signal head |
| **Detector** | `D` | วาง Detector |
| **Stop Line** | `B` | วาง Stop line |
| **Parking** | `P` | สร้าง Parking area |
| **Crosswalk** | `X` | วางทางม้าลาย |
| **Measure** | `M` | วัดระยะ |
| **Pan** | `H` | เลื่อน view |

### 7.3 Tool State Machine

```
[Idle] ──mouseDown──> [Pressing] ──mouseUp──> [Idle]
   │                       │
   │                       └──mouseMove──> [Dragging]
   │                                          │
   └──keyDown(Esc)──> [Cancelled] ──> [Idle]
```

### 7.4 Tool Preview

ทุก tool แสดง preview ขณะใช้งาน:
- **Link tool**: เส้น ghost ตาม pointer
- **Connector tool**: เส้นโค้ง ghost + ไฮไลต์ lane ที่ snap
- **Signal tool**: ไอคอนสัญญาณ + ตำแหน่ง station
- **Detector tool**: กรอบ detection zone

### 7.5 Modal Behavior

| Tool | Modal? | ยกเลิกด้วย |
| :--- | :--- | :--- |
| Select | ไม่ | — |
| Link | ใช่ (จนจบเส้น) | Esc / double-click |
| Connector | ใช่ (2 clicks) | Esc |
| Signal | ไม่ (วางซ้ำได้) | Esc |
| Detector | ไม่ | Esc |

---

## 8. Selection System

### 8.1 Selection Model

```cpp
class Selection {
    std::set<ObjectId> objects;
    ObjectId primary;      // primary selection (inspector)
    Rect2 boundingBox;

    void add(ObjectId);
    void remove(ObjectId);
    void toggle(ObjectId);
    void clear();
    void setPrimary(ObjectId);
};
```

### 8.2 Selection Operations

| การดำเนินการ | Mouse | Keyboard |
| :--- | :--- | :--- |
| เลือกเดี่ยว | Click | — |
| เพิ่ม | Shift+Click | Shift+Arrow |
| toggle | Ctrl+Click | Ctrl+Space |
| เลือกทั้งหมด | — | Ctrl+A |
| ยกเลิก | Click พื้น | Esc |
| Box select | Drag บนพื้น | — |
| เลือกตาม type | — | Ctrl+Alt+A |
| Invert | — | Ctrl+I |

### 8.3 Selection Feedback

- **Outline**: เส้นรอบ object (สีฟ้า 2 px)
- **Handles**: จุด/แท็บปรากฏ
- **Inspector**: อัปเดตทันที
- **Status bar**: แสดงจำนวนที่เลือก

### 8.4 Selection Filter

ผู้ใช้กำหนดได้ว่าจะเลือก object ประเภทไหน:

```
☑ Links
☑ Connectors
☑ Nodes
☐ Signals
☐ Detectors
```

### 8.5 Cascading Selection

- เลือก Link → เลือก Connector ที่เชื่อมได้ (ถ้าเปิด cascade)
- เลือก Node → เลือก Link ที่เชื่อม

---

## 9. Snapping และ Constraint System

### 9.1 Snap Types

| ประเภท | คำอธิบาย | Tolerance |
| :--- | :--- | :--- |
| **Endpoint** | ปลาย Link/Connector | 10 px |
| **Node** | Node | 10 px |
| **Lane center** | จุดศูนย์กลางเลน | 8 px |
| **Lane boundary** | ขอบเลน | 8 px |
| **Grid** | จุด grid | 5 px |
| **Perpendicular** | ตั้งฉากกับ object | 10 px |
| **Parallel** | ขนานกับ object | 10 px |
| **Tangent** | สัมผัสกับ curve | 10 px |
| **Intersection** | จุดตัดของ object | 8 px |
| **Midpoint** | กึ่งกลาง segment | 8 px |
| **Station** | station บน Link | 8 px |
| **Angle** | มุม 0/45/90/135° | 5° |

### 9.2 Snap Priority

เมื่อมีหลาย snap ใกล้กัน เรียงตามลำดับ:

```
1. Node / Endpoint (สูงสุด)
2. Existing attachment
3. Lane center
4. Intersection
5. Grid
6. Free
```

### 9.3 Snap Engine

```cpp
class SnapEngine {
    struct SnapResult {
        Point2 position;
        SnapType type;
        ObjectId target;
        double distance;
        bool valid;
    };

    SnapResult snap(Point2 cursor, SnapContext ctx);

    // Individual snappers
    SnapResult snapToNode(Point2);
    SnapResult snapToEndpoint(Point2);
    SnapResult snapToLane(Point2);
    SnapResult snapToGrid(Point2);
    SnapResult snapToGuide(Point2);
};
```

### 9.4 Visual Feedback

| Snap type | สี | สัญลักษณ์ |
| :--- | :--- | :--- |
| Node | เขียว | สี่เหลี่ยม |
| Endpoint | เขียว | วงกลม |
| Lane center | ฟ้า | เส้นประ |
| Grid | เทา | จุด |
| Intersection | เหลือง | กากบาท |
| Angle | ส้ม | เส้นรัศมี |

### 9.5 Constraint Modes

| Mode | คำอธิบาย |
| :--- | :--- |
| **Free** | ไม่มี constraint |
| **Orthogonal** | บังคับ 0/90° |
| **Angle (45°)** | บังคับ 0/45/90/135° |
| **Along object** | ตามแนว Link/Connector |
| **Perpendicular** | ตั้งฉาก |

### 9.6 Guide Lines

- เส้น guide ชั่วคราวขณะลาก
- แสดงระยะ/มุม
- หายไปเมื่อปล่อยเมาส์

---

## 10. Drag & Drop Operations

### 10.1 Drag Types

| Object | Drag | ผลลัพธ์ |
| :--- | :--- | :--- |
| Link endpoint | ย้ายปลาย | เปลี่ยน geometry, re-anchor |
| Link interior point | ย้ายจุด | แก้ polyline |
| Link body | ย้ายทั้งเส้น | เคลื่อนที่ทั้ง Link |
| Connector endpoint | ย้ายปลาย | retarget |
| Connector body | ย้าย | เคลื่อนที่ |
| Lane tab | ลาก | เปลี่ยน lane count |
| Range handle | ลาก | เปลี่ยน range |
| Signal | ลาก | เปลี่ยน station |
| Node | ลาก | ย้าย node |

### 10.2 Drag Lifecycle

```
1. onMouseDown → ตรวจสอบ hit → เริ่ม drag session
2. onMouseMove → อัปเดต ghost/preview + snap
3. onMouseUp → commit เป็น command (หรือ rollback ถ้า Esc)
```

### 10.3 Drag Preview

- **Ghost object**: วาด object ใหม่ที่ตำแหน่งปัจจุบัน
- **Original object**: จางลง (opacity 0.3)
- **Snap guides**: แสดงเส้น snap
- **Dimension**: แสดงระยะ/มุม

### 10.4 Multi-Drag

- ลาก selection ทั้งหมดพร้อมกัน
- Snap ทำงานกับ object ที่เป็น primary

### 10.5 Drag Constraints

- **Link endpoint**: ต้องอยู่บนเลนของ Link
- **Connector endpoint**: ต้องอยู่บนเลนของ Link ที่อ้างอิง
- **Lane tab**: ต้องอยู่ภายในช่วง contiguous ที่เหลือ
- **Signal**: ต้องอยู่บน Link/Connector

### 10.6 Cancel

- `Esc` ระหว่าง drag → rollback ทั้งหมด
- คลิกขวาระหว่าง drag → ยกเลิก

---

## 11. Handle System (Lane Tabs, Endpoints, Range Handles)

### 11.1 Handle Types

| Handle | ตำแหน่ง | หน้าที่ |
| :--- | :--- | :--- |
| **Endpoint** | ปลาย Link/Connector | ย้ายปลาย |
| **Interior point** | จุดกลาง | แก้ polyline |
| **Lane tab (leading)** | ต้น Link | เปลี่ยน lane count ต้น |
| **Lane tab (trailing)** | ปลาย Link | เปลี่ยน lane count ปลาย |
| **Range handle (source)** | ต้น Connector | เปลี่ยน source lane range |
| **Range handle (target)** | ปลาย Connector | เปลี่ยน target lane range |
| **Rotation handle** | ข้าง object | หมุน |

### 11.2 Handle Appearance

| Handle | รูปร่าง | สี | ขนาด |
| :--- | :--- | :--- | :--- |
| Endpoint | วงกลม | ขาว | 8 px |
| Interior | สี่เหลี่ยม | ขาว | 6 px |
| Lane tab | แท็บ | ส้ม | 12×8 px |
| Range handle | ลูกศร | ส้ม | 10 px |
| Rotation | วงกลม + ลูกศร | ฟ้า | 10 px |

### 11.3 Handle Visibility

- แสดงเฉพาะเมื่อ:
  - object ถูกเลือก
  - zoom เพียงพอ (> 0.5×)
  - tool เป็น Select
- ซ่อนเมื่อ zoom out มาก

### 11.4 Handle Snapping

Handle snap เข้าหา:
- จุดที่ valid
- grid
- object อื่น

### 11.5 Handle Keyboard

| ปุ่ม | ผลลัพธ์ |
| :--- | :--- |
| Tab | เลือก handle ถัดไป |
| Shift+Tab | ย้อนกลับ |
| Enter | Activate handle |
| Esc | ยกเลิก |

### 11.6 Lane Tab Behavior

```
กดที่ lane tab → ลากซ้าย/ขวา
  ├── ลากขวา → เพิ่มจำนวนเลน
  └── ลากซ้าย → ลดจำนวนเลน
ปล่อย → commit เป็น changeConnectorRange command
```

**ข้อจำกัด**:
- สูงสุด 12 เลน
- ต้อง contiguous
- ถ้า object ถูกอ้างอิง → ห้ามเปลี่ยน (แสดงไอคอนล็อก)

---

## 12. Command System และ Undo/Redo

### 12.1 Command Interface

```cpp
class Command {
public:
    virtual ~Command() = default;
    virtual bool validate(Document&) const = 0;
    virtual void execute(Document&) = 0;
    virtual void undo(Document&) = 0;
    virtual std::string label() const = 0;
};
```

### 12.2 History Stack

```cpp
class History {
    std::vector<std::unique_ptr<Command>> undoStack;
    std::vector<std::unique_ptr<Command>> redoStack;
    size_t maxDepth = 1000;

    bool execute(std::unique_ptr<Command>);
    bool undo();
    bool redo();
    void clear();
    bool canUndo() const;
    bool canRedo() const;
};
```

### 12.3 Transaction

ทุก command รันใน transaction:

```
1. เริ่ม transaction
2. validate (draft) → ถ้า fail → rollback + แสดง error
3. execute (mutate model)
4. ตรวจสอบ invariant
5. ถ้า fail → rollback
6. commit → push ลง undo stack
7. clear redo stack
8. notify observers
```

### 12.4 Composite Command

หลาย command รวมเป็น 1 undo unit:

```cpp
class CompositeCommand : public Command {
    std::vector<std::unique_ptr<Command>> commands;
    // execute ทั้งหมด, undo ย้อนกลับ
};
```

### 12.5 Undo/Redo UI

| การดำเนินการ | Shortcut |
| :--- | :--- |
| Undo | `Ctrl+Z` |
| Redo | `Ctrl+Y` / `Ctrl+Shift+Z` |
| History panel | `Ctrl+H` |

### 12.6 History Panel

แสดงรายการ action:

```
▶ Add link "east"
▶ Add connector "turn-east"
▶ Change connector range (2→1)
▼ Delete detector "det-1"
   ▶ Remove from link
   ▶ Remove from simulation
```

### 12.7 Auto-Save

- ทุก 60 วินาที (ถ้ามีการเปลี่ยนแปลง)
- เก็บใน recovery file
- กู้คืนได้เมื่อเปิดโปรเจกต์ใหม่หลัง crash

---

