# INF1002 CMS — Single-file C Project

This is a portable, single-file C implementation of the Class Management System (CMS) described in your project spec. It compiles with any C99 compiler (GCC/Clang/MSVC) and runs in a terminal.

## Files
- `cms.c` — the full source code (single file)
- `P1_1-CMS.txt` — sample database (pipe-separated, safe with spaces)

## Build & Run

### Windows (MSYS2/MinGW or TDM-GCC)
1. Install GCC (e.g., MSYS2). Open **MSYS2 MinGW64** shell.
2. Compile:
   ```bash
   gcc -std=c99 -O2 cms.c -o cms.exe
   ```
3. Run:
   ```bash
   ./cms.exe
   ```

> If you're using plain **Command Prompt** and not MSYS2/MinGW, install **TDM-GCC** or **MinGW-w64** first. CMake/`make` is **not required** for this project.

### macOS / Linux
```bash
gcc -std=c99 -O2 cms.c -o cms
./cms
```

## Database Filename Convention
Use:
```
OPEN P1_1
```
This opens/creates `P1_1-CMS.txt`. You can replace `P1_1` with your actual team name.

## Commands
```
OPEN <TeamName>
SHOW ALL [SORT BY ID|MARK [ASC|DESC]]
SHOW SUMMARY
INSERT ID=<int> Name="<str>" Programme="<str>" Mark=<float>
QUERY  ID=<int>
UPDATE ID=<int> [Name="<str>"] [Programme="<str>"] [Mark=<float>]
DELETE ID=<int>
SAVE
UNDO
HELP
EXIT
```

Examples:
```
OPEN P1_1
SHOW ALL
INSERT ID=2401234 Name="Michelle Lee" Programme="Information Security" Mark=73.2
UPDATE ID=2401234 Programme="Applied AI" Mark=69.8
QUERY  ID=2401234
SHOW ALL SORT BY MARK DESC
SHOW SUMMARY
DELETE ID=2401234
UNDO
SAVE
EXIT
```

## File Format
The app writes **pipe-separated** lines so names/programmes may contain spaces safely:
```
ID|Name|Programme|Mark
2301234|Joshua Chen|Software Engineering|70.5
2201234|Isaac Teo|Computer Science|63.40
2304567|John Levoy|Digital Supply Chain|85.90
```
You can edit this file in any text editor.

## Notes for the Report
- Data structure: dynamic array (bounded to 10,000) of `struct Student`.
- Parsing: tolerant key-value reader with quoted strings for values with spaces.
- Sorting: `qsort` comparators for ID/Mark with ASC/DESC.
- Summary: count, average, highest/lowest with names.
- **Unique feature**: `UNDO` (last INSERT/UPDATE/DELETE) backed by a small ring-stack.
- Reliability: input trimming, missing-field checks, and friendly error messages.
- No external libraries, no GUI dependencies.

## VS Code
Open the folder, then either:
- Use the integrated terminal and run the build commands above, or
- Create a simple `tasks.json` that runs `gcc -std=c99 -O2 cms.c -o cms`.

## Sample Session
```
OPEN P1_1
SHOW ALL
INSERT ID=2401234 Name="Michelle Lee" Programme="Information Security" Mark=73.2
SHOW ALL
QUERY ID=2401234
UPDATE ID=2401234 Programme="Applied AI" Mark=69.8
SHOW ALL SORT BY ID ASC
SHOW SUMMARY
SAVE
EXIT
```
