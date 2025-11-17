\
/*
 * INF1002 Programming Fundamentals — Class Management System (CMS) — v2
 * Portable C99 (Windows/macOS/Linux)
 *
 * New features (v2):
 *  1) SET AUTOSAVE ON|OFF (default OFF) — automatically saves after INSERT/UPDATE/DELETE
 *  2) FIND NAME="<keyword>" — case-insensitive substring search on names
 *  3) Grade bands in SHOW SUMMARY (A>=80, B 70-79.99, C 60-69.99, D 50-59.99, F<50)
 *  4) Numeric validation: ID must be >0; Mark must be in [0,100]
 *  5) Consistent formatting: all marks shown with 2 decimal places; clearer OPEN/SAVE messages
 *
 * Core features:
 *  - OPEN, SHOW ALL, INSERT, QUERY, UPDATE, DELETE, SAVE
 *  - Sorting: SHOW ALL SORT BY ID|MARK ASC|DESC
 *  - Summary: SHOW SUMMARY (count, avg, hi/lo with names, grade bands)
 *  - Unique: UNDO (revert last INSERT/UPDATE/DELETE)
 *  - HELP, EXIT
 *
 * Build:
 *   gcc -std=c99 -O2 cms.c -o cms
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _MSC_VER
#define strdup _strdup
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
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

typedef enum {
    OP_NONE, OP_INSERT, OP_UPDATE, OP_DELETE
} OpType;

typedef struct {
    OpType type;
    Student before;
    Student after;
    int had_before;
} UndoEntry;

static Student records[MAX_RECORDS];
static int n_records = 0;

static UndoEntry undo_stack[128];
static int undo_top = 0;

static char db_filename[260] = "";
static char team_name[128] = "";

// settings
static int autosave_on = 0;

static void print_declaration(void) {
    printf("\nDeclaration\n");
    printf("SIT's policy on copying does not allow the students to copy source code as well as assessment solutions\n");
    printf("from another person AI or other places. It is the students' responsibility to guarantee that their\n");
    printf("assessment solutions are their own work. Meanwhile, the students must also ensure that their work is\n");
    printf("not accessible by others. Where such plagiarism is detected, both of the assessments involved will\n");
    printf("receive ZERO mark.\n\n");
    printf("We hereby declare that:\n");
    printf("- We fully understand and agree to the abovementioned plagiarism policy.\n");
    printf("- We did not copy any code from others or from other places.\n");
    printf("- We did not share our codes with others or upload to any other places for public access and will not do that in the future.\n");
    printf("- We agree that our project will receive Zero mark if there is any plagiarism detected.\n");
    printf("- We agree that we will not disclose any information or material of the group project to others or upload to any other places for public access.\n");
    printf("- We agree that we did not copy any code directly from AI generated sources\n\n");
    printf("Declared by: Group Name: P10-09\n");
    printf("Team members:\n");
    printf("1. David Yong Jing Xiang\n2. Amal Nadiy\n3. Lim Kai Hin\n4. Liaw Jun De\n");
    printf("Date: (please insert the date when you submit your group project)\n\n");
}

static void trim(char* s) {
    int start = 0;
    while (s[start] && isspace((unsigned char)s[start])) start++;
    if (start) memmove(s, s+start, strlen(s+start)+1);
    int end = (int)strlen(s)-1;
    while (end>=0 && isspace((unsigned char)s[end])) { s[end]='\0'; end--; }
}

static void strtoupper_inplace(char* s) {
    for (; *s; ++s) *s = (char)toupper((unsigned char)*s);
}

static int cmp_id_asc(const void* a, const void* b) {
    const Student* x = (const Student*)a;
    const Student* y = (const Student*)b;
    return (x->id > y->id) - (x->id < y->id);
}
static int cmp_id_desc(const void* a, const void* b) { return -cmp_id_asc(a,b); }

static int cmp_mark_asc(const void* a, const void* b) {
    const Student* x = (const Student*)a;
    const Student* y = (const Student*)b;
    if (x->mark < y->mark) return -1;
    if (x->mark > y->mark) return 1;
    return cmp_id_asc(a,b);
}
static int cmp_mark_desc(const void* a, const void* b) { return -cmp_mark_asc(a,b); }

static int cmp_programme_asc(const void* a, const void* b) {
    const Student* x = (const Student*)a;
    const Student* y = (const Student*)b;
    return strcmp(x->programme, y->programme);
}
static int cmp_programme_desc(const void* a, const void* b) { return -cmp_programme_asc(a,b); }


static int find_index_by_id(int id) {
    for (int i=0;i<n_records;++i) if (records[i].id==id) return i;
    return -1;
}

static int parse_between(const char* line, const char* key, char* out, size_t outsz) {
    const char* p = line;
    size_t klen = strlen(key);
    while (*p) {
        if (strncasecmp(p, key, klen)==0 && p[klen]=='=') {
            p += klen+1;
            if (*p=='\"') {
                p++;
                const char* q = strchr(p, '\"');
                if (!q) return 0;
                size_t len = (size_t)(q-p); if (len>=outsz) len=outsz-1;
                memcpy(out,p,len); out[len]='\0'; return 1;
            } else {
                const char* q = p;
                while (*q) {
                    if (strncasecmp(q," ID=",4)==0||strncasecmp(q," NAME=",6)==0||
                        strncasecmp(q," PROGRAMME=",11)==0||strncasecmp(q," MARK=",6)==0) break;
                    q++;
                }
                while (q>p && isspace((unsigned char)q[-1])) q--;
                size_t len=(size_t)(q-p); if (len>=outsz) len=outsz-1;
                memcpy(out,p,len); out[len]='\0'; return 1;
            }
        }
        p++;
    }
    return 0;
}

static int load_db(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) return 0;
    char line[MAX_LINE];
    n_records = 0;
    while (fgets(line,sizeof(line),f)) {
        trim(line); if (!line[0]) continue;
        char* p1 = strtok(line,"|");
        char* p2 = strtok(NULL,"|");
        char* p3 = strtok(NULL,"|");
        char* p4 = strtok(NULL,"|");
        if (!p1||!p2||!p3||!p4) continue;
        Student s; s.id=atoi(p1);
        strncpy(s.name,p2,MAX_NAME-1); s.name[MAX_NAME-1]=0;
        strncpy(s.programme,p3,MAX_PROG-1); s.programme[MAX_PROG-1]=0;
        s.mark=(float)atof(p4);
        if (n_records<MAX_RECORDS) records[n_records++]=s;
    }
    fclose(f);
    return 1;
}

static int save_db(const char* filename) {
    FILE* f = fopen(filename,"w");
    if (!f) return 0;
    for (int i=0;i<n_records;++i) {
        fprintf(f,"%d|%s|%s|%.2f\n", records[i].id, records[i].name, records[i].programme, records[i].mark);
    }
    fclose(f);
    return 1;
}

static void push_undo(UndoEntry e) {
    if (undo_top < (int)(sizeof(undo_stack)/sizeof(undo_stack[0]))) {
        undo_stack[undo_top++] = e;
    } else {
        memmove(undo_stack, undo_stack+1, (undo_top-1)*sizeof(UndoEntry));
        undo_stack[undo_top-1] = e;
    }
}

static void maybe_autosave(void) {
    if (autosave_on && db_filename[0]) {
        if (save_db(db_filename)) {
            printf("CMS: Autosave complete → \"%s\".\n", db_filename);
        } else {
            printf("CMS: Autosave FAILED. Please SAVE manually and check permissions.\n");
        }
    }
}

static void print_record_header(void) { printf("ID\tName\tProgramme\tMark\n"); }
static void print_record(const Student* s) { printf("%d\t%s\t%s\t%.2f\n", s->id, s->name, s->programme, s->mark); }

static void cmd_show_all(const char* args) {
    Student* tmp = (Student*)malloc(sizeof(Student)*(size_t)n_records);
    if (!tmp) { printf("CMS: Memory error.\n"); return; }
    memcpy(tmp, records, sizeof(Student)*(size_t)n_records);

    int sort_by=0, desc=0;
    if (args && *args) {
        char up[MAX_LINE]; strncpy(up,args,sizeof(up)-1); up[sizeof(up)-1]=0;
        strtoupper_inplace(up);
        if (strstr(up,"SORT BY ID")) sort_by=1;
        else if (strstr(up,"SORT BY MARK")) sort_by=2;
        else if (strstr(up,"SORT BY PROGRAMME")) sort_by=3;
        if (strstr(up,"DESC")) desc=1;
        if (sort_by==1) qsort(tmp,(size_t)n_records,sizeof(Student), desc?cmp_id_desc:cmp_id_asc);
        else if (sort_by==2) qsort(tmp,(size_t)n_records,sizeof(Student), desc?cmp_mark_desc:cmp_mark_asc);
        else if (sort_by==3) qsort(tmp,(size_t)n_records,sizeof(Student), desc?cmp_programme_desc:cmp_programme_asc);
    }
    printf("CMS: Here are all the records found in the table \"StudentRecords\" (%d total).\n", n_records);
    print_record_header();
    for (int i=0;i<n_records;++i) print_record(&tmp[i]);
    free(tmp);
}

static void cmd_show_summary(void) {
    if (n_records==0) { printf("CMS: No records loaded.\n"); return; }
    int total=n_records;
    float sum=0.0f;
    int hi_idx=0, lo_idx=0;
    int A=0,B=0,C=0,D=0,Fc=0;
    for (int i=0;i<n_records;++i) {
        float m=records[i].mark;
        sum += m;
        if (m>records[hi_idx].mark) hi_idx=i;
        if (m<records[lo_idx].mark) lo_idx=i;
        if (m>=80) A++;
        else if (m>=70) B++;
        else if (m>=60) C++;
        else if (m>=50) D++;
        else Fc++;
    }
    float avg=sum/(float)total;
    printf("CMS: SUMMARY\n");
    printf("Total students: %d\n", total);
    printf("Average mark : %.2f\n", avg);
    printf("Highest mark : %.2f (%s)\n", records[hi_idx].mark, records[hi_idx].name);
    printf("Lowest mark  : %.2f (%s)\n", records[lo_idx].mark, records[lo_idx].name);
    printf("Grade bands  : A=%d  B=%d  C=%d  D=%d  F=%d\n", A,B,C,D,Fc);
}

static void cmd_query(const char* line) {
    char buf[64];
    if (!parse_between(line,"ID",buf,sizeof(buf))) { printf("CMS: Please provide ID. e.g., QUERY ID=2401234\n"); return; }
    int id = atoi(buf);
    int idx = find_index_by_id(id);
    if (idx<0) { printf("CMS: The record with ID=%d does not exist.\n", id); return; }
    printf("CMS: The record with ID=%d is found in the data table.\n", id);
    print_record_header();
    print_record(&records[idx]);
}

static int parse_and_validate_id(const char* line, int* out_id) {
    char s_id[64];
    if (!parse_between(line,"ID",s_id,sizeof(s_id))) return 0;
    for (char* p=s_id; *p; ++p) if (!isdigit((unsigned char)*p) && *p!='-') { return -1; }
    int id = atoi(s_id);
    if (id<=0) return -2;
    *out_id = id; return 1;
}

static int parse_and_validate_mark(const char* line, float* out_mark) {
    char s_mark[64];
    if (!parse_between(line,"MARK",s_mark,sizeof(s_mark))) return 0;
    float m = (float)atof(s_mark);
    if (m<0.0f || m>100.0f) return -1;
    *out_mark = m; return 1;
}


static void cmd_show_programme_summary(void) {
    if (n_records==0) {
        printf("CMS: No records loaded.\n");
        return;
    }
    typedef struct {
        char programme[MAX_PROG];
        int count;
        float total_mark;
    } ProgrammeSummary;

    ProgrammeSummary ps[256];
    int n_prog = 0;

    for (int i=0; i<n_records; ++i) {
        int idx = -1;
        for (int j=0; j<n_prog; ++j) {
            if (strcmp(ps[j].programme, records[i].programme) == 0) {
                idx = j;
                break;
            }
        }
        if (idx == -1) {
            if (n_prog >= (int)(sizeof(ps)/sizeof(ps[0]))) {
                continue; /* safety guard, should not happen for typical datasets */
            }
            idx = n_prog++;
            strncpy(ps[idx].programme, records[i].programme, MAX_PROG-1);
            ps[idx].programme[MAX_PROG-1] = '\0';
            ps[idx].count = 0;
            ps[idx].total_mark = 0.0f;
        }
        ps[idx].count += 1;
        ps[idx].total_mark += records[i].mark;
    }

    printf("CMS: Programme summary (per programme):\n");
    printf("%-30s %-10s %-10s\n", "Programme", "Count", "AvgMark");
    printf("--------------------------------------------------------------\n");
    for (int i=0; i<n_prog; ++i) {
        float avg = ps[i].total_mark / (float)ps[i].count;
        printf("%-30s %-10d %-10.2f\n", ps[i].programme, ps[i].count, avg);
    }
}


static void cmd_insert(const char* line) {
    int id; int id_ok = parse_and_validate_id(line,&id);
    if (id_ok<=0) { printf("CMS: Please provide a valid positive integer ID.\n"); return; }
    if (find_index_by_id(id)>=0) { printf("CMS: The record with ID=%d already exists.\n", id); return; }

    char s_name[MAX_NAME], s_prog[MAX_PROG];
    float mark; int has_mark = parse_and_validate_mark(line,&mark);
    if (!parse_between(line,"NAME",s_name,sizeof(s_name)) ||
        !parse_between(line,"PROGRAMME",s_prog,sizeof(s_prog)) ||
        has_mark<=0) {
        printf("CMS: Missing or invalid fields. Required: NAME, PROGRAMME, MARK (0..100).\n");
        return;
    }
    if (n_records>=MAX_RECORDS) { printf("CMS: Cannot insert, reached max records.\n"); return; }
    Student s; s.id=id;
    strncpy(s.name,s_name,MAX_NAME-1); s.name[MAX_NAME-1]=0;
    strncpy(s.programme,s_prog,MAX_PROG-1); s.programme[MAX_PROG-1]=0;
    s.mark=mark;
    records[n_records++]=s;
    printf("CMS: A new record with ID=%d is successfully inserted.\n", id);

    UndoEntry u={0}; u.type=OP_INSERT; u.after=s; push_undo(u);
    maybe_autosave();
}

static void cmd_update(const char* line) {
    int id; int id_ok = parse_and_validate_id(line,&id);
    if (id_ok<=0) { printf("CMS: Please provide a valid ID for UPDATE.\n"); return; }
    int idx=find_index_by_id(id);
    if (idx<0) { printf("CMS: The record with ID=%d does not exist.\n", id); return; }

    char s_name[MAX_NAME], s_prog[MAX_PROG];
    int has_name=0, has_prog=0, has_mark=0; float mark=0.0f;
    if (parse_between(line,"NAME",s_name,sizeof(s_name))) has_name=1;
    if (parse_between(line,"PROGRAMME",s_prog,sizeof(s_prog))) has_prog=1;
    int mk = parse_and_validate_mark(line,&mark);
    if (mk==1) { has_mark=1; }
    else if (mk==-1) { printf("CMS: MARK must be within 0..100.\n"); return; }

    if (!has_name && !has_prog && !has_mark) { printf("CMS: Nothing to update. Provide NAME/PROGRAMME/MARK.\n"); return; }

    Student before = records[idx];
    if (has_name) { strncpy(records[idx].name,s_name,MAX_NAME-1); records[idx].name[MAX_NAME-1]=0; }
    if (has_prog) { strncpy(records[idx].programme,s_prog,MAX_PROG-1); records[idx].programme[MAX_PROG-1]=0; }
    if (has_mark) { records[idx].mark=mark; }

    printf("CMS: The record with ID=%d is successfully updated.\n", id);

    UndoEntry u={0}; u.type=OP_UPDATE; u.before=before; u.after=records[idx]; u.had_before=1; push_undo(u);
    maybe_autosave();
}

static void cmd_delete(const char* line) {
    int id; int id_ok = parse_and_validate_id(line,&id);
    if (id_ok<=0) { printf("CMS: Please provide a valid ID for DELETE.\n"); return; }
    int idx=find_index_by_id(id);
    if (idx<0) { printf("CMS: The record with ID=%d does not exist.\n", id); return; }

    printf("CMS: Are you sure you want to delete record with ID=%d? Type \"Y\" to Confirm or type \"N\" to cancel.\n", id);
    printf("You: ");
    char resp[32];
    if (!fgets(resp,sizeof(resp),stdin)) return;
    trim(resp);
    if (resp[0]!='Y' && resp[0]!='y') { printf("CMS: The deletion is cancelled.\n"); return; }

    Student before=records[idx];
    for (int i=idx+1;i<n_records;++i) records[i-1]=records[i];
    n_records--;
    printf("CMS: The record with ID=%d is successfully deleted.\n", id);

    UndoEntry u={0}; u.type=OP_DELETE; u.before=before; u.had_before=1; push_undo(u);
    maybe_autosave();
}

static void cmd_undo(void) {
    if (undo_top<=0) { printf("CMS: Nothing to UNDO.\n"); return; }
    UndoEntry u = undo_stack[--undo_top];
    if (u.type==OP_INSERT) {
        int idx=find_index_by_id(u.after.id);
        if (idx>=0) { for (int i=idx+1;i<n_records;++i) records[i-1]=records[i]; n_records--; printf("CMS: UNDO successful (reverted last INSERT of ID=%d).\n",u.after.id); }
        else printf("CMS: UNDO failed (record not found).\n");
    } else if (u.type==OP_UPDATE) {
        int idx=find_index_by_id(u.before.id);
        if (idx>=0) { records[idx]=u.before; printf("CMS: UNDO successful (reverted last UPDATE of ID=%d).\n",u.before.id); }
        else printf("CMS: UNDO failed (record not found).\n");
    } else if (u.type==OP_DELETE) {
        if (n_records>=MAX_RECORDS) { printf("CMS: UNDO failed (max records reached).\n"); return; }
        records[n_records++]=u.before;
        printf("CMS: UNDO successful (reverted last DELETE of ID=%d).\n",u.before.id);
    } else {
        printf("CMS: UNDO failed (unknown op).\n");
    }
    maybe_autosave();
}


static void cmd_find_name(const char* line) {
    char key_name[MAX_NAME];
    char key_prog[MAX_PROG];
    int has_name = parse_between(line,"NAME",key_name,sizeof(key_name));
    int has_prog = parse_between(line,"PROGRAMME",key_prog,sizeof(key_prog));

    if (!has_name && !has_prog) {
        printf("CMS: Please provide NAME or PROGRAMME keyword, e.g., FIND NAME=\"michelle\" or FIND PROGRAMME=\"Digital Supply Chain\".\n");
        return;
    }

    if (has_name) {
        char key_lc[MAX_NAME]; strncpy(key_lc,key_name,sizeof(key_lc)-1); key_lc[sizeof(key_lc)-1]=0;
        for (char* p=key_lc; *p; ++p) *p=(char)tolower((unsigned char)*p);
        int found=0;
        printf("CMS: Search results for name contains \"%s\":\n", key_name);
        print_record_header();
        for (int i=0;i<n_records;++i) {
            char name_lc[MAX_NAME]; strncpy(name_lc,records[i].name,sizeof(name_lc)-1); name_lc[sizeof(name_lc)-1]=0;
            for (char* p=name_lc; *p; ++p) *p=(char)tolower((unsigned char)*p);
            if (strstr(name_lc,key_lc)) { print_record(&records[i]); found=1; }
        }
        if (!found) printf("(no matches)\n");
        return;
    }

    if (has_prog) {
        char key_lc[MAX_PROG]; strncpy(key_lc,key_prog,sizeof(key_lc)-1); key_lc[sizeof(key_lc)-1]=0;
        for (char* p=key_lc; *p; ++p) *p=(char)tolower((unsigned char)*p);
        int found=0;
        printf("CMS: Search results for programme contains \"%s\":\n", key_prog);
        print_record_header();
        for (int i=0;i<n_records;++i) {
            char prog_lc[MAX_PROG]; strncpy(prog_lc,records[i].programme,sizeof(prog_lc)-1); prog_lc[sizeof(prog_lc)-1]=0;
            for (char* p=prog_lc; *p; ++p) *p=(char)tolower((unsigned char)*p);
            if (strstr(prog_lc,key_lc)) { print_record(&records[i]); found=1; }
        }
        if (!found) printf("(no matches)\n");
    }
}

static void cmd_help(void) {
    printf("Available commands:\n");
    printf("  OPEN <TeamName>\n");
    printf("  SHOW ALL [SORT BY ID|MARK|PROGRAMME [ASC|DESC]]\n");
    printf("  SHOW SUMMARY\n");
    printf("  SHOW PROGRAMME SUMMARY\n");
    printf("  INSERT ID=<int> Name=\"<str>\" Programme=\"<str>\" Mark=<float>\n");
    printf("  QUERY  ID=<int>\n");
    printf("  UPDATE ID=<int> [Name=\"<str>\"] [Programme=\"<str>\"] [Mark=<float> (0..100)]\n");
    printf("  DELETE ID=<int>\n");
    printf("  FIND NAME=\"<keyword>\"\n");
    printf("  FIND PROGRAMME=\"<keyword>\"\n");
    printf("  SET AUTOSAVE ON|OFF\n");
    printf("  SAVE\n");
    printf("  UNDO\n");
    printf("  HELP\n");
    printf("  EXIT\n");
}

static void ensure_filename_from_team(void) {
    if (team_name[0]) snprintf(db_filename,sizeof(db_filename),"%s-CMS.txt", team_name);
}

int main(void) {
    print_declaration();
    printf("Type HELP to see available commands.\n\n");

    char line[MAX_LINE];
    while (1) {
        printf("You: ");
        if (!fgets(line,sizeof(line),stdin)) break;
        trim(line); if (!line[0]) continue;
        char cmd[MAX_LINE]; strncpy(cmd,line,sizeof(cmd)-1); cmd[sizeof(cmd)-1]=0;
        char up[MAX_LINE]; strncpy(up,line,sizeof(up)-1); up[sizeof(up)-1]=0; strtoupper_inplace(up);

        if (strncmp(up,"EXIT",4)==0 || strncmp(up,"QUIT",4)==0) {
            printf("CMS: Bye!\n"); break;
        } else if (strncmp(up,"HELP",4)==0) {
            cmd_help();
        } else if (strncmp(up,"OPEN",4)==0) {
            char* p = cmd+4; while (*p && isspace((unsigned char)*p)) p++;
            if (!*p) { printf("CMS: Please provide a team name. e.g., OPEN P10-09\n"); continue; }
            strncpy(team_name,p,sizeof(team_name)-1); team_name[sizeof(team_name)-1]=0; trim(team_name);
            ensure_filename_from_team();
            if (load_db(db_filename)) {
                printf("CMS: Opened \"%s\" (%d records).\n", db_filename, n_records);
            } else {
                n_records=0;
                printf("CMS: New database will be created on SAVE → \"%s\" (0 records currently).\n", db_filename);
            }
        } else if (strncmp(up,"SHOW ALL",8)==0) {
            char* args = cmd+8; cmd_show_all(args);
        } else if (strncmp(up,"SHOW PROGRAMME SUMMARY",22)==0) {
            cmd_show_programme_summary();
        } else if (strncmp(up,"SHOW SUMMARY",12)==0) {
            cmd_show_summary();
        } else if (strncmp(up,"INSERT",6)==0) {
            cmd_insert(cmd);
        } else if (strncmp(up,"QUERY",5)==0) {
            cmd_query(cmd);
        } else if (strncmp(up,"UPDATE",6)==0) {
            cmd_update(cmd);
        } else if (strncmp(up,"DELETE",6)==0) {
            cmd_delete(cmd);
        } else if (strncmp(up,"FIND",4)==0) {
            cmd_find_name(cmd);
        } else if (strncmp(up,"SET AUTOSAVE",12)==0) {
            if (strstr(up,"ON")) { autosave_on=1; printf("CMS: AUTOSAVE is ON.\n"); }
            else if (strstr(up,"OFF")) { autosave_on=0; printf("CMS: AUTOSAVE is OFF.\n"); }
            else { printf("CMS: Usage → SET AUTOSAVE ON|OFF\n"); }
        } else if (strncmp(up,"SAVE",4)==0) {
            if (!db_filename[0]) { printf("CMS: Please OPEN <TeamName> first.\n"); }
            else {
                if (save_db(db_filename)) printf("CMS: The database file \"%s\" is successfully saved.\n", db_filename);
                else printf("CMS: Failed to save database file. Check permissions.\n");
            }
        } else if (strncmp(up,"UNDO",4)==0) {
            cmd_undo();
        } else {
            printf("CMS: Unknown command. Type HELP.\n");
        }
    }
    return 0;
}
