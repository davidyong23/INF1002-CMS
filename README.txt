# INF1002 CMS — v2 (P10-09)

Upgrades in this version:
- **SET AUTOSAVE ON|OFF** (OFF by default; saves after INSERT/UPDATE/DELETE/UNDO)
- **FIND NAME="<keyword>"** (case-insensitive substring)
- **SHOW SUMMARY** adds grade bands: A≥80, B≥70, C≥60, D≥50, F<50
- **Validation:** ID>0, 0≤Mark≤100
- **Formatting:** marks printed with 2 d.p.; clearer OPEN/new-file messages
- Declaration pre-filled with **Group P10-09** and member names.

## Files
- `cms.c` — full source
- `P10-09-CMS.txt` — sample database for your group

## Build
Windows (GCC):
```bash
gcc -std=c99 -O2 cms.c -o cms.exe
.\cms.exe
```
macOS/Linux:
```bash
gcc -std=c99 -O2 cms.c -o cms
./cms
```

## Quick demo
```
OPEN P10-09
SET AUTOSAVE ON
SHOW ALL
FIND NAME="isaac"
INSERT ID=2401234 Name="Michelle Lee" Programme="Information Security" Mark=73.2
UPDATE ID=2401234 Programme="Applied AI" Mark=69.8
SHOW SUMMARY
SAVE
EXIT
```
