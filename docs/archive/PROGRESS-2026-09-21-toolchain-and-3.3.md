# PROGRESS entry moved out of docs/PROGRESS.md on 2026-09-22

## 2026-09-21 — A toolchain, and the audit's §3.3 answered with a measurement

**Request:** the owner asked whether the toolchain was hard to install, whether VS Code could be
used, then *"do as recommended"* — install it and verify the build.

**The toolchain went into the WSL2 Ubuntu that was already on the machine.** No admin rights and
no reboot were needed, and the tight `C:` drive is irrelevant because WSL has 955 GB free:
`apt-get install g++ cmake ninja-build nlohmann-json3-dev qt6-base-dev` gives `g++ 15.2.0`,
`cmake 4.2.3`, `ninja 1.13.2`, Qt 6 Widgets and Qt 6 Test. This is the first time in this
workstream that anything was built at all.

| Preset | Build | Tests |
|---|---|---|
| `headless` | 65/65, exit 0 | **21/21 passed**, 4.30 s |
| `desktop` (Qt 6 Widgets) | 93/93, exit 0 | **30/30 passed**, 9.47 s |

`QT_QPA_PLATFORM=offscreen` is needed for the desktop suite on a displayless machine — the
checked-in `desktop` preset does not set it. **And ninja's default parallelism OOMs the Qt build
here**: the host has 7.66 GB and WSL is given 3.74 GB, so the first desktop build died with exit
code 15. Build with `-- -j 3`. That is this machine, not the project. The four gates that matter
for the audit all passed — `file-sizes` (so the `PROGRESS.md` archive move was required, not
cosmetic), `all-model-tests`, `architecture` and `reference`. **Linux only**, so this is not
cross-platform evidence; the Windows MSVC path is still unexercised.

**Finding 1 is answered, and the audit's first reading of it was wrong.** Two Connectors arriving
at the same station on one lane do **not** compile and then overlap — the second is **refused**.
`runtimeSections` (`sections.cpp:68`) tests each cut against `boundaries.back() + kMinSectionLength`,
and the boundary the *first* arrival just made is at that very station, so the second arrival is
measured against itself and lands in `table.unsectionable` → `UNSUPPORTED_CONNECTOR_POSITION`,
which blocks Run. Nothing is physically wrong with the pair: the cut the second needs is the one
the first already made. So the defect is the **refusal**, and "they do not yield to each other" is
a second question sitting behind it that this session never reaches.

**One test added**, `connectors.two_connectors_arriving_at_one_station_are_refused_though_one_cut_would_serve`
in `tests/connector_tests.cpp`. It asserts the behaviour **as it is**, not as it should be, and its
comment carries what it should assert once the refusal is fixed — so the fix turns the test into
the specification rather than deleting it. It follows the standing rule: the forcing comes first
(one interior arrival is clean, the cut at the drawn station survives, both ends really are inside
the body at the same metre), then the consequence.

**No production C++ was changed, and no milestone is closed.** The fix is a behaviour change to the
section table, which every other surface reads; it is one system and it is booked as M3.2 work
beside conflict areas. `CONNECTOR_PARITY_AUDIT.md` §3.3 is rewritten to the measurement and gains a
new §7 recording the verification above.

### Next

Two independent things, neither started. First, **the §3.3 fix**: make `runtimeSections` reuse an
existing cut when a second arrival lands on the same station within `kMinSectionLength`, instead of
rejecting it — the test above then flips to its commented-out expectation. That is a section-table
change, so it wants its own session and a check that nothing downstream regressed. Second, resume
the separately booked M1.22 features and the owner's M1.21.1 recheck against their original
`.traffic.json`. Do **not** start §3.2 curve parameters or §16 visual overrides. Keep M3.2
(conflict areas, lane changing) as the home for the §3.3 second half, §3.4 and §3.6.

---


---
