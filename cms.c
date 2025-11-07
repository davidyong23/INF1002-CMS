
/*
 * INF1002 Programming Fundamentals — Class Management System (CMS)
 * Single-file C program (portable C99) for Windows/Mac/Linux
 *
 * v2 Upgrades:
 *  - Autosave toggle: SET AUTOSAVE ON|OFF (OFF by default)
 *  - Name search: FIND NAME="<keyword>" (case-insensitive substring)
 *  - Summary adds grade bands: A≥80, B≥70, C≥60, D≥50, F<50
 *  - Numeric validation: ID>0, 0≤Mark≤100 (INSERT/UPDATE)
 *  - Improved formatting/messages: marks printed with 2 d.p., clearer OPEN messages
 *
 * Base Features:
 *  - File-backed "StudentRecords" table with: ID(int), Name(string), Programme(string), Mark(float)
 *  - Commands: OPEN, SHOW ALL, INSERT, QUERY, UPDATE, DELETE, SAVE
 *  - Sorting: SHOW ALL SORT BY ID [ASC|DESC], SHOW ALL SORT BY MARK [ASC|DESC]
 *  - Summary: SHOW SUMMARY (count, average, highest/lowest with names + grade bands)
 *  - Unique feature: UNDO (revert the most recent INSERT/UPDATE/DELETE)
 *  - HELP, EXIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _MSC_VER
#define strdup _strdup
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#define MAX_RECORDS 10000
#define MAX_NAME 128
#define MAX_PROG 128
#define MAX_LINE 1024

typedef struct {
    int id;
    char name[MAX_NAME];
    char programme[MAX_PROG];
    float mark;
} Student;

typedef enum { OP_NONE, OP_INSERT, OP_UPDATE, OP_DELETE } OpType;

typedef struct {
    OpType type;
    Student before;
    Student after;
    int had_before;
} UndoEntry;

static Student records[MAX_RECORDS];
static int n_records = 0;

static UndoEntry undo_stack[64];
static int undo_top = 0;

static char db_filename[260] = "";
static char team_name[128] = "";
static int autosave_enabled = 0;

static void print_declaration(void) {
    printf("\nDeclaration\n");
    printf("SIT’s policy on copying does not allow the students to copy source code as well as assessment solutions\n");
    printf("from another person AI or other places. It is the students’ responsibility to guarantee that their\n");
    printf("assessment solutions are their own work. Meanwhile, the students must also ensure that their work is\n");
    printf("not accessible by others. Where such plagiarism is detected, both of the assessments involved will\n");
    printf("receive ZERO mark.\n\n");
    printf("We hereby declare that:\n");
    printf("• We fully understand and agree to the abovementioned plagiarism policy.\n");
    printf("• We did not copy any code from others or from other places.\n");
    printf("• We did not share our codes with others or upload to any other places for public access and will not do that in the future.\n");
    printf("• We agree that our project will receive Zero mark if there is any plagiarism detected.\n");
    printf("• We agree that we will not disclose any information or material of the group project to others or upload to any other places for public access.\n");
    printf("• We agree that we did not copy any code directly from AI generated sources\n\n");
    printf("Declared by: Group Name P10-09\n");
    printf("Team members:\n");
    printf("1. David Yong Jing Xiang\n2. Amal Nadiy\n3. Lim Kai Hin\n4. Liaw Jun De\n");
    printf("Date: (please insert the date when you submit your group project)\n\n");
}

static void trim(char* s) {
    int start = 0; while (isspace((unsigned char)s[start])) start++;
    if (start) memmove(s, s + start, strlen(s + start) + 1);
    int end = (int)strlen(s) - 1;
    while (end >= 0 && isspace((unsigned char)s[end])) { s[end] = '\0'; end--; }
}

static int cmp_id_asc(const void* a, const void* b) {
    const Student* x = (const Student*)a; const Student* y = (const Student*)b;
    return (x->id > y->id) - (x->id < y->id);
}
static int cmp_id_desc(const void* a, const void* b) { return -cmp_id_asc(a,b); }

static int cmp_mark_asc(const void* a, const void* b) {
    const Student* x = (const Student*)a; const Student* y = (const Student*)b;
    if (x->mark < y->mark) return -1; if (x->mark > y->mark) return 1;
    return cmp_id_asc(a,b);
}
static int cmp_mark_desc(const void* a, const void* b) { return -cmp_mark_asc(a,b); }

static int find_index_by_id(int id) {
    for (int i = 0; i < n_records; ++i) if (records[i].id == id) return i;
    return -1;
}

static int parse_between(const char* line, const char* key, char* out, size_t outsz) {
    const char* p = line; size_t klen = strlen(key);
    while (*p) {
        if (strncasecmp(p, key, klen) == 0 && p[klen] == '=') {
            p += klen + 1;
            if (*p == '"') {
                p++; const char* q = strchr(p, '"'); if (!q) return 0;
                size_t len = (size_t)(q - p); if (len >= outsz) len = outsz - 1;
                memcpy(out, p, len); out[len] = 0; return 1;
            } else {
                const char* q = p;
                while (*q) {
                    if (strncasecmp(q, " ID=", 4) == 0 || strncasecmp(q, " NAME=", 6) == 0 ||
                        strncasecmp(q, " PROGRAMME=", 11) == 0 || strncasecmp(q, " MARK=", 6) == 0) break;
                    q++;
                }
                while (q > p && isspace((unsigned char)q[-1])) q--;
                size_t len = (size_t)(q - p); if (len >= outsz) len = outsz - 1;
                memcpy(out, p, len); out[len] = 0; return 1;
            }
        } p++;
    }
    return 0;
}

static int load_db(const char* filename) {
    FILE* f = fopen(filename, "r"); if (!f) return 0;
    char line[MAX_LINE]; n_records = 0;
    while (fgets(line, sizeof(line), f)) {
        trim(line); if (!line[0]) continue;
        char* p1 = strtok(line, "|"); char* p2 = strtok(NULL, "|");
        char* p3 = strtok(NULL, "|"); char* p4 = strtok(NULL, "|");
        if (!p1 || !p2 || !p3 || !p4) continue;
        Student s; s.id = atoi(p1);
        strncpy(s.name, p2, MAX_NAME - 1); s.name[MAX_NAME - 1] = 0;
        strncpy(s.programme, p3, MAX_PROG - 1); s.programme[MAX_PROG - 1] = 0;
        s.mark = (float)atof(p4);
        if (n_records < MAX_RECORDS) records[n_records++] = s;
    } fclose(f); return 1;
}

static int save_db(const char* filename) {
    FILE* f = fopen(filename, "w"); if (!f) return 0;
    for (int i = 0; i < n_records; ++i)
        fprintf(f, "%d|%s|%s|%.2f\n", records[i].id, records[i].name, records[i].programme, records[i].mark);
    fclose(f); return 1;
}

static void push_undo(UndoEntry e) {
    if (undo_top < (int)(sizeof(undo_stack)/sizeof(undo_stack[0]))) undo_stack[undo_top++] = e;
    else { memmove(undo_stack, undo_stack+1, (undo_top-1)*sizeof(UndoEntry)); undo_stack[undo_top-1] = e; }
}

static void maybe_autosave(void) {
    if (autosave_enabled && db_filename[0]) {
        if (save_db(db_filename)) printf("CMS: Autosaved to \"%s\".\n", db_filename);
        else printf("CMS: Autosave FAILED (check permissions/path).\n");
    }
}

static void print_header(void) { printf("ID\tName\tProgramme\tMark\n"); }
static void print_row(const Student* s) { printf("%d\t%s\t%s\t%.2f\n", s->id, s->name, s->programme, s->mark); }

static void cmd_show_all(const char* args) {
    Student* tmp = (Student*)malloc(sizeof(Student) * (size_t)n_records);
    if (!tmp) { printf("CMS: Memory error.\n"); return; }
    memcpy(tmp, records, sizeof(Student) * (size_t)n_records);
    int sort_by = 0, desc = 0;
    if (args && *args) {
        char up[MAX_LINE]; strncpy(up, args, sizeof(up)-1); up[sizeof(up)-1]=0;
        for (char* c = up; *c; ++c) *c = (char)toupper((unsigned char)*c);
        if (strstr(up, "SORT BY ID")) sort_by = 1; else if (strstr(up, "SORT BY MARK")) sort_by = 2;
        if (strstr(up, "DESC")) desc = 1;
        if (sort_by == 1) qsort(tmp, (size_t)n_records, sizeof(Student), desc ? cmp_id_desc : cmp_id_asc);
        else if (sort_by == 2) qsort(tmp, (size_t)n_records, sizeof(Student), desc ? cmp_mark_desc : cmp_mark_asc);
    }
    printf("CMS: Here are all the records found in the table \"StudentRecords\".\n");
    print_header(); for (int i = 0; i < n_records; ++i) print_row(&tmp[i]);
    free(tmp);
}

static void cmd_show_summary(void) {
    if (n_records == 0) { printf("CMS: No records loaded.\n"); return; }
    int total = n_records; float sum = 0.0f; int hi_idx = 0, lo_idx = 0;
    int cntA=0,cntB=0,cntC=0,cntD=0,cntF=0;
    for (int i = 0; i < n_records; ++i) {
        float m = records[i].mark; sum += m;
        if (m > records[hi_idx].mark) hi_idx = i; if (m < records[lo_idx].mark) lo_idx = i;
        if (m >= 80.0f) cntA++; else if (m >= 70.0f) cntB++; else if (m >= 60.0f) cntC++; else if (m >= 50.0f) cntD++; else cntF++;
    }
    float avg = sum / (float)total;
    printf("CMS: SUMMARY\n");
    printf("Total students: %d\n", total);
    printf("Average mark : %.2f\n", avg);
    printf("Highest mark : %.2f (%s)\n", records[hi_idx].mark, records[hi_idx].name);
    printf("Lowest mark  : %.2f (%s)\n", records[lo_idx].mark, records[lo_idx].name);
    printf("Grade bands  : A:%d  B:%d  C:%d  D:%d  F:%d\n", cntA,cntB,cntC,cntD,cntF);
}

static void cmd_query(const char* line) {
    char buf[64]; if (!parse_between(line, "ID", buf, sizeof(buf))) { printf("CMS: Please provide ID. e.g., QUERY ID=2401234\n"); return; }
    int id = atoi(buf); if (id <= 0) { printf("CMS: Invalid ID. Must be a positive integer.\n"); return; }
    int idx = find_index_by_id(id); if (idx < 0) { printf("CMS: The record with ID=%d does not exist.\n", id); return; }
    printf("CMS: The record with ID=%d is found in the data table.\n", id);
    print_header(); print_row(&records[idx]);
}

static int parse_mark(const char* s, float* out) {
    float val = (float)atof(s);
    if (val < 0.0f || val > 100.0f) return 0; *out = val; return 1;
}

static void cmd_insert(const char* line) {
    char s_id[64], s_name[MAX_NAME], s_prog[MAX_PROG], s_mark[64];
    if (!parse_between(line, "ID", s_id, sizeof(s_id))) { printf("CMS: Please provide ID. e.g., INSERT ID=2401234 Name=\"Michelle Lee\" Programme=\"Information Security\" Mark=73.2\n"); return; }
    int id = atoi(s_id); if (id <= 0) { printf("CMS: Invalid ID. Must be a positive integer.\n"); return; }
    if (find_index_by_id(id) >= 0) { printf("CMS: The record with ID=%d already exists.\n", id); return; }
    if (!parse_between(line, "NAME", s_name, sizeof(s_name)) || !parse_between(line, "PROGRAMME", s_prog, sizeof(s_prog)) || !parse_between(line, "MARK", s_mark, sizeof(s_mark))) {
        printf("CMS: Missing fields. Required: NAME, PROGRAMME, MARK.\n"); return;
    }
    float mark=0.0f; if (!parse_mark(s_mark, &mark)) { printf("CMS: Invalid Mark. Must be 0..100.\n"); return; }
    if (n_records >= MAX_RECORDS) { printf("CMS: Cannot insert, reached max records.\n"); return; }
    Student s; s.id = id;
    strncpy(s.name, s_name, MAX_NAME-1); s.name[MAX_NAME-1]=0;
    strncpy(s.programme, s_prog, MAX_PROG-1); s.programme[MAX_PROG-1]=0;
    s.mark = mark; records[n_records++] = s;
    printf("CMS: A new record with ID=%d is successfully inserted.\n", id);
    UndoEntry u = {0}; u.type = OP_INSERT; u.after = s; u.had_before = 0; push_undo(u);
    maybe_autosave();
}

static void cmd_update(const char* line) {
    char s_id[64], s_name[MAX_NAME], s_prog[MAX_PROG], s_mark[64];
    int has_name = 0, has_prog = 0, has_mark = 0;
    if (!parse_between(line, "ID", s_id, sizeof(s_id))) { printf("CMS: Please provide ID. e.g., UPDATE ID=2401234 Mark=69.8\n"); return; }
    int id = atoi(s_id); if (id <= 0) { printf("CMS: Invalid ID. Must be a positive integer.\n"); return; }
    int idx = find_index_by_id(id); if (idx < 0) { printf("CMS: The record with ID=%d does not exist.\n", id); return; }
    Student before = records[idx];
    if (parse_between(line, "NAME", s_name, sizeof(s_name))) has_name = 1;
    if (parse_between(line, "PROGRAMME", s_prog, sizeof(s_prog))) has_prog = 1;
    if (parse_between(line, "MARK", s_mark, sizeof(s_mark))) has_mark = 1;
    if (!has_name && !has_prog && !has_mark) { printf("CMS: Nothing to update. Provide NAME/PROGRAMME/MARK.\n"); return; }
    if (has_name) { strncpy(records[idx].name, s_name, MAX_NAME-1); records[idx].name[MAX_NAME-1]=0; }
    if (has_prog) { strncpy(records[idx].programme, s_prog, MAX_PROG-1); records[idx].programme[MAX_PROG-1]=0; }
    if (has_mark) { float m=0.0f; if (!parse_mark(s_mark, &m)) { printf("CMS: Invalid Mark. Must be 0..100.\n"); return; } records[idx].mark = m; }
    printf("CMS: The record with ID=%d is successfully updated.\n", id);
    UndoEntry u = {0}; u.type = OP_UPDATE; u.before = before; u.after = records[idx]; u.had_before = 1; push_undo(u);
    maybe_autosave();
}

static void cmd_delete(const char* line) {
    char s_id[64]; if (!parse_between(line, "ID", s_id, sizeof(s_id))) { printf("CMS: Please provide ID. e.g., DELETE ID=2301234\n"); return; }
    int id = atoi(s_id); if (id <= 0) { printf("CMS: Invalid ID. Must be a positive integer.\n"); return; }
    int idx = find_index_by_id(id); if (idx < 0) { printf("CMS: The record with ID=%d does not exist.\n", id); return; }
    printf("CMS: Are you sure you want to delete record with ID=%d? Type \"Y\" to Confirm or type \"N\" to cancel.\n", id);
    printf("You: ");
    char resp[32]; if (!fgets(resp, sizeof(resp), stdin)) return; trim(resp);
    if (resp[0] != 'Y' && resp[0] != 'y') { printf("CMS: The deletion is cancelled.\n"); return; }
    Student before = records[idx];
    for (int i = idx + 1; i < n_records; ++i) records[i-1] = records[i];
    n_records--; printf("CMS: The record with ID=%d is successfully deleted.\n", id);
    UndoEntry u = {0}; u.type = OP_DELETE; u.before = before; u.had_before = 1; push_undo(u);
    maybe_autosave();
}

static void cmd_undo(void) {
    if (undo_top <= 0) { printf("CMS: Nothing to UNDO.\n"); return; }
    UndoEntry u = undo_stack[--undo_top];
    if (u.type == OP_INSERT) {
        int idx = find_index_by_id(u.after.id);
        if (idx >= 0) { for (int i = idx + 1; i < n_records; ++i) records[i-1] = records[i]; n_records--; printf("CMS: UNDO successful (reverted last INSERT of ID=%d).\n", u.after.id); }
        else printf("CMS: UNDO failed (record not found).\n");
    } else if (u.type == OP_UPDATE) {
        int idx = find_index_by_id(u.before.id);
        if (idx >= 0) { records[idx] = u.before; printf("CMS: UNDO successful (reverted last UPDATE of ID=%d).\n", u.before.id); }
        else printf("CMS: UNDO failed (record not found).\n");
    } else if (u.type == OP_DELETE) {
        if (n_records >= MAX_RECORDS) { printf("CMS: UNDO failed (max records reached).\n"); return; }
        records[n_records++] = u.before; printf("CMS: UNDO successful (reverted last DELETE of ID=%d).\n", u.before.id);
    } else printf("CMS: UNDO failed (unknown op).\n");
    maybe_autosave();
}

static void cmd_help(void) {
    printf("Available commands:\n");
    printf("  OPEN <TeamName>\n");
    printf("  SHOW ALL [SORT BY ID|MARK [ASC|DESC]]\n");
    printf("  SHOW SUMMARY\n");
    printf("  FIND NAME=\"<keyword>\"\n");
    printf("  INSERT ID=<int> Name=\"<str>\" Programme=\"<str>\" Mark=<float 0..100>\n");
    printf("  QUERY  ID=<int>\n");
    printf("  UPDATE ID=<int> [Name=\"<str>\"] [Programme=\"<str>\"] [Mark=<float 0..100>]\n");
    printf("  DELETE ID=<int>\n");
    printf("  SAVE\n");
    printf("  SET AUTOSAVE ON|OFF  (currently %s)\n", autosave_enabled ? "ON" : "OFF");
    printf("  UNDO\n");
    printf("  HELP\n");
    printf("  EXIT\n");
}

static void ensure_filename_from_team(void) { if (team_name[0]) snprintf(db_filename, sizeof(db_filename), "%s-CMS.txt", team_name); }

static void cmd_find_name(const char* line) {
    char key[MAX_NAME]; if (!parse_between(line, "NAME", key, sizeof(key))) { printf("CMS: Provide NAME keyword. e.g., FIND NAME=\"michelle\"\n"); return; }
    if (!key[0]) { printf("CMS: Empty keyword.\n"); return; }
    char keylow[MAX_NAME]; int i=0; for (; key[i] && i<MAX_NAME-1; ++i) keylow[i] = (char)tolower((unsigned char)key[i]); keylow[i]=0;
    int found = 0; printf("CMS: Search results for NAME contains \"%s\":\n", key); print_header();
    for (int k = 0; k < n_records; ++k) {
        char namelow[MAX_NAME]; int j=0; for (; records[k].name[j] && j<MAX_NAME-1; ++j) namelow[j] = (char)tolower((unsigned char)records[k].name[j]); namelow[j]=0;
        if (strstr(namelow, keylow)) { print_row(&records[k]); found++; }
    }
    if (found == 0) printf("(no matches)\n"); else printf("(%d match%s)\n", found, found==1?"":"es");
}

int main(void) {
    print_declaration();
    printf("Type HELP to see available commands.\n\n");
    char line[MAX_LINE];
    while (1) {
        printf("You: "); if (!fgets(line, sizeof(line), stdin)) break; trim(line); if (!line[0]) continue;
        char cmd[MAX_LINE]; strncpy(cmd, line, sizeof(cmd)-1); cmd[sizeof(cmd)-1]=0;
        char up[MAX_LINE]; strncpy(up, line, sizeof(up)-1); up[sizeof(up)-1]=0; for (char* c = up; *c; ++c) *c = (char)toupper((unsigned char)*c);
        if (strncmp(up, "EXIT", 4) == 0 || strncmp(up, "QUIT", 4) == 0) { printf("CMS: Bye!\n"); break; }
        else if (strncmp(up, "HELP", 4) == 0) cmd_help();
        else if (strncmp(up, "OPEN", 4) == 0) {
            char* p = cmd + 4; while (*p && isspace((unsigned char)*p)) p++; if (!*p) { printf("CMS: Please provide a team name. e.g., OPEN P10-09\n"); continue; }
            strncpy(team_name, p, sizeof(team_name)-1); team_name[sizeof(team_name)-1]=0; trim(team_name); ensure_filename_from_team();
            if (load_db(db_filename)) printf("CMS: The database file \"%s\" is successfully opened. (%d records)\n", db_filename, n_records);
            else { n_records = 0; printf("CMS: New database will be created: \"%s\" (new file, 0 records). Use SAVE to create it.\n", db_filename); }
        }
        else if (strncmp(up, "SHOW ALL", 8) == 0) { char* args = cmd + 8; cmd_show_all(args); }
        else if (strncmp(up, "SHOW SUMMARY", 12) == 0) cmd_show_summary();
        else if (strncmp(up, "FIND ", 5) == 0) cmd_find_name(cmd);
        else if (strncmp(up, "INSERT", 6) == 0) cmd_insert(cmd);
        else if (strncmp(up, "QUERY", 5) == 0) cmd_query(cmd);
        else if (strncmp(up, "UPDATE", 6) == 0) cmd_update(cmd);
        else if (strncmp(up, "DELETE", 6) == 0) cmd_delete(cmd);
        else if (strncmp(up, "SAVE", 4) == 0) {
            if (!db_filename[0]) printf("CMS: Please OPEN <TeamName> first to determine the database filename.\n");
            else if (save_db(db_filename)) printf("CMS: The database file \"%s\" is successfully saved.\n", db_filename);
            else printf("CMS: Failed to save database file. Check permissions/path.\n");
        }
        else if (strncmp(up, "SET AUTOSAVE", 12) == 0) {
            if (strstr(up, "ON")) { autosave_enabled = 1; printf("CMS: AUTOSAVE is ON.\n"); }
            else if (strstr(up, "OFF")) { autosave_enabled = 0; printf("CMS: AUTOSAVE is OFF.\n"); }
            else printf("CMS: Use SET AUTOSAVE ON|OFF\n");
        }
        else if (strncmp(up, "UNDO", 4) == 0) cmd_undo();
        else printf("CMS: Unknown command. Type HELP.\n");
    }
    return 0;
}
