# INF1002 CMS — v2 (Group P10-09)

**Team:** P10-09 — David Yong Jing Xiang, Amal Nadiy, Lim Kai Hin, Liaw Jun De

Single-file C99 CMS with enhancements and a unique UNDO feature.

## New in v2
- `SET AUTOSAVE ON|OFF` (default OFF)
- `FIND NAME="keyword"` search (case-insensitive substring)
- Grade bands in `SHOW SUMMARY` (A/B/C/D/F)
- Numeric validation: ID>0, Mark ∈ [0,100]
- Marks shown with **2 d.p.** and clearer OPEN/SAVE messages

## Build
```bash
# macOS/Linux
gcc -std=c99 -O2 cms.c -o cms      
# Windows
gcc -std=c99 -O2 cms.c -o cms.exe  
```

## Run
```bash
# macOS/Linux:
./cms
# Windows:
./cms.exe
```

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
SAVE
UNDO
HELP
EXIT
```

## File Format
`<TeamName>-CMS.txt` with pipe separators: `ID|Name|Programme|Mark`

## Notes for your report
- Data structure: static array of `struct Student` (10,000 cap)
- Parsing: tolerant key-value, quoted strings allow spaces
- Sorting: `qsort` comparators; deterministic tie-break by ID
- Unique: `UNDO` stack (depth 128)
- Input validation for IDs/Marks; friendly errors
