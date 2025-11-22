# INF1002 CMS — v2 (Group P10-09)

**Team:** P10-09 — David Yong Jing Xiang, Amal Nadiy, Lim Kai Hin, Liaw Jun De

Single-file C99 CMS with enhancements and a unique UNDO feature.

---

## New in v2
- `SET AUTOSAVE ON|OFF` (default OFF) — automatically saves after INSERT/UPDATE/DELETE  
- `FIND NAME="keyword"` — case-insensitive substring search on names  
- Grade bands in `SHOW SUMMARY` (A/B/C/D/F)  
- Numeric validation:
  - ID must be a **7-digit positive integer**
  - Mark must be in `[0, 100]`  
- Marks shown with 2 decimal places and clearer OPEN/SAVE messages  
- Name and Programme are automatically normalised to title case  
  - e.g. `digital supply chain` → `Digital Supply Chain`  
- Additional utilities:
  - Command history: `HISTORY`  
  - Programme-based summary: `SHOW PROGRAMME SUMMARY`
  - Programme filter: `SHOW PROGRAMME PROGRAMME="Digital Supply Chain"`  
  - CSV export: `EXPORT CSV="students.csv"`  

---

## Build
```bash
# macOS/Linux
gcc -std=c99 -O2 cms.c -o cms      
# Windows
gcc -std=c99 -O2 cms.c -o cms.exe  
```

---

## Run
```bash
# macOS/Linux:
./cms
# Windows:
./cms.exe
```

---

## Quick Start
```
OPEN P10-09
SET AUTOSAVE ON
SHOW ALL
INSERT ID=250001 Name="Alice Tan" Programme="SE" Mark=88
FIND NAME="Alice"
FIND PROGRAMME="SE"
SHOW SUMMARY
SAVE
EXIT
```

---

## Commands
```
OPEN <TeamName>
SHOW ALL [SORT BY ID|MARK|PROGRAMME [ASC|DESC]]
SHOW SUMMARY
SHOW PROGRAMME SUMMARY
INSERT ID=<int> Name="<str>" Programme="<str>" Mark=<float>
QUERY  ID=<int>
UPDATE ID=<int> [Name="<str>"] [Programme="<str>"] [Mark=<float> (0..100)]
DELETE ID=<int>
FIND NAME="<keyword>"
FIND PROGRAMME="<keyword>"
SET AUTOSAVE ON|OFF
EXPORT CSV="<filename.csv>"
SAVE
UNDO
HELP
EXIT
```
---

### Command notes

- **ID rules**
  - Exactly 7 digits (e.g. `2301234`); non‑numeric or wrong length is rejected.
  - IDs must be unique; duplicate inserts are rejected.  
- **Mark rules**
  - Must be between 0 and 100; values outside this range are rejected.  
- **Name/Programme formatting**
  - Input may be any case; CMS normalises to title case internally.  
- **SHOW ALL sorting**
  - Optional `SORT BY` lets you order by `ID`, `MARK`, `PROGRAMME`, or `NAME`, each `ASC` or `DESC`.  

---

## Programme-based Features

### SHOW PROGRAMME SUMMARY

Summarises records per programme:

    SHOW PROGRAMME SUMMARY

Displays, for each programme:

- Programme name  
- Student count  
- Average mark  

### SHOW PROGRAMME

Lists only students in a specific programme (exact match after normalisation):

    SHOW PROGRAMME PROGRAMME="Digital Supply Chain"

---

## CSV Export

Export all current records to a CSV file compatible with Excel/Sheets:

    EXPORT CSV="students.csv"

The CSV format matches:

    ID,Name,Programme,Mark
    2301234,"Joshua Chen","Software Engineering",70.50
    2201234,"Isaac Teo","Computer Science",63.40
    2304567,"John Levoy","Digital Supply Chain",85.90

---

## File Format

The main database file is:

    <TeamName>-CMS.txt

Each line uses pipe separators:

    ID|Name|Programme|Mark
    2301234|Joshua Chen|Software Engineering|70.50
    2201234|Isaac Teo|Computer Science|63.40
    2304567|John Levoy|Digital Supply Chain|85.90

- IDs are stored as 7-digit integers.  
- Marks are stored with 2 decimal places.  

---

## Notes for your report
- Data structure: static array of `struct Student` (10,000 cap)
- Parsing: tolerant key-value, quoted strings allow spaces
- Sorting: `qsort` comparators; deterministic tie-break by ID
- Unique: `UNDO` stack (depth 128)
- Input validation for IDs/Marks; friendly errors
