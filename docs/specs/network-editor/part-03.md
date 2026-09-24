## 13. Properties Inspector

### 13.1 Layout

```
┌─────────────────────────────────┐
│ Properties                       │
├─────────────────────────────────┤
│ ▸ Identity                       │
│   ID:    [east-1         ]      │
│   Name:  [Main East      ]      │
│   Type:  [primary       ▼]      │
│                                 │
│ ▸ Geometry                       │
│   Length: 200.0 m                │
│   Points: 5                      │
│   [Edit...]                      │
│                                 │
│ ▸ Lanes                          │
│   Count: 2                       │
│   ▸ Lane 1 (east-1)              │
│     Width: [3.5] m               │
│     Type:  [car        ▼]       │
│     Marking: [dashed   ▼]       │
│   ▸ Lane 2 (east-2)              │
│     ...                          │
│   [+ Add Lane]                   │
│                                 │
│ ▸ Behavior                       │
│   Speed: [50.0] km/h            │
│   ...                            │
│                                 │
│ ▸ Visual                         │
│   Color: [■]                     │
│   ...                            │
└─────────────────────────────────┘
```

### 13.2 Widget Types

| ชนิดข้อมูล | Widget |
| :--- | :--- |
| String | Text field |
| Number | Spinbox / slider |
| Boolean | Checkbox |
| Enum | Dropdown |
| Color | Color picker |
| Point | X/Y fields |
| Array | List + add/remove |
| Object | Nested section |

### 13.3 Live Editing

- แก้ค่า → preview ทันที
- Commit เมื่อ blur / Enter
- Esc → ยกเลิก

### 13.4 Multi-Selection

- ถ้าเลือกหลาย object → แสดง field ที่เหมือนกัน
- ถ้าค่าต่างกัน → แสดง `<varies>`
- แก้ → apply กับทุก object

### 13.5 Validation Feedback

- Field ที่ผิด → ขอบแดง + tooltip
- Field ที่ควรระวัง → ขอบเหลือง + tooltip
- แสดง error message ใต้ field

### 13.6 Context Menu ใน Inspector

- Right-click บน field → menu (reset, copy, paste)

---

## 14. Layer System และ Z-Ordering

### 14.1 Layer Types

| Layer | คำอธิบาย | Toggle |
| :--- | :--- | :--- |
| **Road** | Links + Connectors | ☑ |
| **Markings** | เส้นแบ่งเลน | ☑ |
| **Signals** | Signal heads | ☑ |
| **Detectors** | Detectors | ☑ |
| **Parking** | Parking areas | ☑ |
| **Stop Lines** | เส้นหยุด | ☑ |
| **Crosswalks** | ทางม้าลาย | ☑ |
| **Nodes** | Nodes | ☑ |
| **Labels** | ชื่อ, หมายเลข | ☑ |
| **Grid** | Grid | ☑ |

### 14.2 Layer Operations

- Toggle visibility
- Lock (ป้องกันการแก้ไข)
- Reorder (Z-order)
- Opacity

### 14.3 Layer Panel

```
┌─────────────────────────┐
│ Layers                   │
├─────────────────────────┤
│ ☑ 👁 🔒 Road            │
│ ☑ 👁 🔒 Markings        │
│ ☑ 👁 🔓 Signals         │
│ ☐ 👁 🔒 Detectors       │
│ ☑ 👁 🔒 Labels          │
└─────────────────────────┘
```

### 14.4 Z-Order ภายใน Layer

- Object ที่สร้างทีหลัง → อยู่บน (default)
- ปรับได้ผ่าน context menu (Bring to front / Send to back)
- ใช้เฉพาะในกรณีที่ object ทับกัน

---

## 15. Hit Testing และ Picking

### 15.1 Hit Test Priority

เมื่อคลิกที่จุดหนึ่ง อาจมีหลาย object → เลือกตามลำดับ:

```
1. Handles (endpoints, tabs)
2. Selected objects (ไฮไลต์)
3. Small objects (signals, detectors)
4. Connectors
5. Links
6. Nodes
```

### 15.2 Hit Test Tolerance

| Object | Tolerance (px) |
| :--- | :--- |
| Endpoint | 10 |
| Lane tab | 12 |
| Signal | 10 |
| Detector | 8 |
| Connector | 6 |
| Link | 4 |
| Node | 10 |

### 15.3 Hit Test Algorithm

```cpp
HitResult hitTest(Point2 screenPoint, HitContext ctx) {
    Point2 world = camera.screenToWorld(screenPoint);
    double toleranceWorld = ctx.tolerancePx / camera.zoom;

    // 1. Check handles of selected objects
    for (auto& obj : selection.objects) {
        if (auto h = hitHandle(obj, world, toleranceWorld))
            return {obj, h};
    }

    // 2. Check all objects (sorted by priority)
    for (auto& layer : layersInOrder()) {
        if (!layer.visible || layer.locked) continue;
        for (auto& obj : layer.objectsInViewport(ctx.viewport)) {
            if (auto hit = obj.hitTest(world, toleranceWorld))
                return hit;
        }
    }

    return {};  // no hit
}
```

### 15.4 Spatial Index

ใช้ R-tree สำหรับ object ขนาดใหญ่:

```cpp
class SpatialIndex {
    RTree<ObjectId> tree;
    void insert(ObjectId, Rect2 bounds);
    void remove(ObjectId);
    std::vector<ObjectId> query(Rect2 bounds);
};
```

### 15.5 Hover Feedback

- เมื่อ hover object → ไฮไลต์ (สีสว่างขึ้น)
- แสดง tooltip ที่มีข้อมูลสรุป
- เปลี่ยน cursor (ลูกศร, มือ, ปากกา)

### 15.6 Multi-Layer Hit

- ถ้า hit object ใน layer ที่ lock → ไม่เลือก
- ถ้า hit object ใน layer ที่ซ่อน → ไม่เลือก
- ใช้ Alt+Click เพื่อ penetrate (เลือก object ที่อยู่ล่าง)

---

## 16. Validation Feedback แบบ Real-time

### 16.1 Real-time Validation

ทุกครั้งที่แก้ไข → validate ทันที:

```
1. รัน draft validation
2. รวบรวม diagnostics
3. อัปเดต UI:
   - Object ที่ error → ขอบแดง
   - Object ที่ warning → ขอบเหลือง
   - Status bar → แสดงจำนวน error/warning
4. แสดงใน Problems panel
```

### 16.2 Problems Panel

```
┌────────────────────────────────────────────┐
│ Problems                                    │
├────────────────────────────────────────────┤
│ 🔴 ERROR  turn-east                        │
│    DISCONNECTED_GEOMETRY: endpoint 0.05m   │
│    from lane center                         │
│                                             │
│ 🟡 WARNING west-east                       │
│    TIGHT_CONNECTOR_RADIUS: 4.2m < 5.0m     │
│                                             │
│ 🔵 INFO    new-link                        │
│    Link created successfully                │
└────────────────────────────────────────────┘
```

### 16.3 Canvas Highlighting

| ระดับ | สี | ผล |
| :--- | :--- | :--- |
| Error | แดง | block save |
| Warning | เหลือง | allow save |
| Info | ฟ้า | ไม่ block |
| Advisory | ส้ม | ไม่ block |

### 16.4 Click to Navigate

- คลิกที่ error ใน Problems → zoom ไปที่ object
- คลิกที่ error อีกครั้ง → zoom ไปที่จุดที่ผิด

### 16.5 Validation Dialog

- รัน full validation เมื่อ save
- ถ้ามี error → แสดง dialog + ไม่ save
- ถ้ามี warning → ถามผู้ใช้

### 16.6 Runtime Validation

- รันเมื่อกด Run
- ถ้ามี blocker → refuse + แสดง error
- เก็บผลลัพธ์ไว้ใน Run log

---

## 17. Context Menu และ Keyboard Shortcuts

### 17.1 Context Menu (Right-Click)

**บน Link**:
- Edit properties
- Add lane / Remove lane
- Split link
- Reverse direction
- Delete
- Copy / Paste

**บน Connector**:
- Edit properties
- Reset curve
- Straighten
- Retarget source/target
- Delete

**บน Selection (หลาย object)**:
- Group / Ungroup
- Align (left/center/right)
- Distribute
- Delete

**บน Empty Space**:
- Paste
- Select all
- Zoom to fit
- Add node here

### 17.2 Keyboard Shortcuts

#### Tools
| ปุ่ม | Tool |
| :--- | :--- |
| `S` | Select |
| `L` | Link |
| `C` | Connector |
| `N` | Node |
| `G` | Signal |
| `D` | Detector |
| `B` | Stop line |
| `P` | Parking |
| `X` | Crosswalk |
| `M` | Measure |
| `H` | Pan |

#### Edit
| ปุ่ม | Action |
| :--- | :--- |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+C` | Copy |
| `Ctrl+V` | Paste |
| `Ctrl+X` | Cut |
| `Delete` | Delete |
| `Ctrl+A` | Select all |
| `Esc` | Deselect / Cancel |

#### View
| ปุ่ม | Action |
| :--- | :--- |
| `Ctrl+0` | Zoom to fit |
| `Ctrl+1` | Zoom 100% |
| `Ctrl++` | Zoom in |
| `Ctrl+-` | Zoom out |
| `Ctrl+Home` | Reset view |
| `F11` | Fullscreen |

#### File
| ปุ่ม | Action |
| :--- | :--- |
| `Ctrl+N` | New project |
| `Ctrl+O` | Open |
| `Ctrl+S` | Save |
| `Ctrl+Shift+S` | Save as |
| `Ctrl+W` | Close |

#### Simulation
| ปุ่ม | Action |
| :--- | :--- |
| `F5` | Run |
| `Shift+F5` | Stop |
| `F6` | Pause |

### 17.3 Shortcut Customization

- Settings → Keyboard → ปรับ shortcut ได้
- รองรับ import/export keymap
- Profile: default, VISSIM-compatible, custom

---

## 18. Import/Export และ Interop

### 18.1 Import Formats

| รูปแบบ | ข้อมูล | หมายเหตุ |
| :--- | :--- | :--- |
| **Project JSON** | ทั้งหมด | Native format |
| **GeoJSON** | Geometry | สำหรับ Links |
| **Shapefile** | Geometry + attributes | ใช้ GDAL |
| **OSM** | Roads + lanes | ผ่าน Overpass API |
| **CSV** | Detector data | สำหรับ calibration |
| **VISSIM** | Network | ผ่าน .inpx (roadmap) |

### 18.2 Export Formats

| รูปแบบ | ข้อมูล |
| :--- | :--- |
| **Project JSON** | ทั้งหมด |
| **GeoJSON** | Geometry |
| **Shapefile** | Geometry |
| **CSV** | Detector output |
| **PNG/SVG** | ภาพ |
| **PDF** | รายงาน |

### 18.3 Import Wizard

```
1. เลือกไฟล์
2. ตรวจจับรูปแบบอัตโนมัติ
3. แสดง preview
4. Map fields (ถ้าจำเป็น)
5. เลือก options (merge / replace)
6. Import
```

### 18.4 Coordinate Systems

- รองรับ WGS84, UTM, local
- แปลงอัตโนมัติเมื่อ import
- เก็บ metadata ไว้ใน project

### 18.5 Merge Strategy

| Strategy | คำอธิบาย |
| :--- | :--- |
| **Replace** | ลบทั้งหมดแล้วแทนที่ |
| **Merge** | รวม object (ID ไม่ซ้ำ) |
| **Merge with offset** | Merge + เลื่อนตำแหน่ง |

---

