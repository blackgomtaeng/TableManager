#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <unistd.h>
#include "TableManager.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>

char default_path[PATH_MAX] = "./";
char temp_folder[PATH_MAX] = "tempTableManager";

static char *duplicate_string(const char *source)
{
    if (!source) return strdup(""); // NULL이면 빈 문자열 반환
    return strdup(source);
}

static void free_string_array(char **items, size_t count)
{
    if (!items) return; // NULL이면 바로 종료
    for (size_t i = 0; i < count; i++) free(items[i]); // 각 문자열 해제
    free(items); // 배열 해제
}

static bool reserve_cell_buffer(char ***data, int **rows, int **cols, size_t *capacity)
{
    size_t next = *capacity ? *capacity * 2 : 128; // 용량 두 배 또는 초기값
    char **new_data = realloc(*data, next * sizeof(char *));
    int *new_rows = realloc(*rows, next * sizeof(int));
    int *new_cols = realloc(*cols, next * sizeof(int));
    if (!new_data || !new_rows || !new_cols) { // 실패 시 메모리 해제
        free(new_data);
        free(new_rows);
        free(new_cols);
        return false;
    }
    *data = new_data;
    *rows = new_rows;
    *cols = new_cols;
    *capacity = next;
    return true;
}

Table *create_table(size_t rows, size_t cols)
{
    Table *t = malloc(sizeof(Table));
    if (!t) return NULL; // 메모리 할당 실패
    t->row_count = rows;
    t->column_count = cols;
    t->rows = calloc(rows * cols, sizeof(char *));
    t->headers = calloc(cols, sizeof(char *));
    if ((!t->rows && rows * cols > 0) || (!t->headers && cols > 0)) { // 실패 시 정리
        free(t->rows);
        free(t->headers);
        free(t);
        return NULL;
    }
    for (size_t i = 0; i < rows * cols; i++) t->rows[i] = duplicate_string(""); // 빈 문자열 초기화
    for (size_t j = 0; j < cols; j++) t->headers[j] = duplicate_string(""); // 헤더 초기화
    t->sheets = NULL;
    t->sheet_count = 0;
    t->filepath[0] = '\0';
    t->timestamp[0] = '\0';
    return t;
}

void destroy_table(Table *t)
{
    if (!t) return; // NULL이면 종료
    free_string_array(t->rows, t->row_count * t->column_count);
    free_string_array(t->headers, t->column_count);
    free_string_array(t->sheets, t->sheet_count);
    free(t);
}

static char *col_index_to_letter(size_t col)
{
    static char buf[16];
    size_t idx = 0;
    col++;
    while (col > 0 && idx + 1 < sizeof(buf)) { // 열 번호를 문자로 변환
        col--;
        buf[idx++] = 'A' + (col % 26);
        col /= 26;
    }
    buf[idx] = '\0';
    for (size_t i = 0; i < idx / 2; i++) { // 문자열 뒤집기
        char tmp = buf[i];
        buf[i] = buf[idx - 1 - i];
        buf[idx - 1 - i] = tmp;
    }
    return buf;
}

bool ensure_folder_exists(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) return S_ISDIR(st.st_mode); // 이미 존재하면 디렉토리 여부 확인
    return mkdir(path, 0755) == 0 || errno == EEXIST; // 없으면 생성
}

bool compose_path(char *dest, size_t dest_size, const char *dir, const char *name)
{
    if (!dest || !dir || !name) return false; // NULL 체크
    if (snprintf(dest, dest_size, "%s/%s", dir, name) >= (int)dest_size) return false; // 버퍼 초과 방지
    return true;
}

static bool output_vprintf(OutputSink *sink, const char *format, va_list args)
{
    if (!sink || !format) return false; // NULL 체크

    if (sink->primary) {
        va_list copy;
        va_copy(copy, args);
        if (vfprintf(sink->primary, format, copy) < 0) { // 출력 실패
            va_end(copy);
            return false;
        }
        va_end(copy);
    }

    if (sink->secondary) {
        va_list copy;
        va_copy(copy, args);
        if (vfprintf(sink->secondary, format, copy) < 0) { // 출력 실패
            va_end(copy);
            return false;
        }
        va_end(copy);
    }

    return true;
}

static bool output_printf(OutputSink *sink, const char *format, ...)
{
    if (!sink || !format) return false; // NULL 체크

    va_list args;
    va_start(args, format);
    bool result = output_vprintf(sink, format, args); // 가변 인자 전달
    va_end(args);
    return result;
}

const char *file_extension(const char *filename)
{
    if (!filename) return ""; // NULL이면 빈 문자열 반환
    const char *ext = strrchr(filename, '.');
    return ext ? ext : ""; // 확장자 반환
}

bool is_excel_file(const char *ext)
{
    if (!ext || !*ext) return false; // NULL 또는 빈 문자열
    return strcasecmp(ext, ".xlsx") == 0 || strcasecmp(ext, ".xls") == 0; // 엑셀 확장자 확인
}

void set_default_path(const char *path)
{
    if (!path) return; // NULL이면 종료
    strncpy(default_path, path, sizeof(default_path) - 1);
    default_path[sizeof(default_path) - 1] = '\0'; // 널 종료 보장
}

void set_temp_folder(const char *folder)
{
    if (!folder) return; // NULL이면 종료
    strncpy(temp_folder, folder, sizeof(temp_folder) - 1);
    temp_folder[sizeof(temp_folder) - 1] = '\0'; // 널 종료 보장
}

static bool write_log_name(char *path, size_t path_size, const char *folder)
{
    if (!path || !folder) return false; // NULL 체크
    time_t now = time(NULL);
    struct tm tm;
    if (!localtime_r(&now, &tm)) return false; // 시간 변환 실패
    char filename[64];
    if (strftime(filename, sizeof(filename), "tempTM%Y%m%d.txt", &tm) == 0) return false; // 포맷 실패
    return compose_path(path, path_size, folder, filename); // 경로 생성
}

FILE *open_log_file(const char *folder, char *path, size_t path_size)
{
    if (!ensure_folder_exists(folder)) return NULL; // 폴더 없으면 생성 실패
    if (!write_log_name(path, path_size, folder)) return NULL; // 로그 이름 생성 실패
    FILE *fp = fopen(path, "a");
    return fp; // 파일 포인터 반환
}

static bool has_nonblank(const char *value)
{
    if (!value) return false; // NULL 체크
    for (size_t i = 0; value[i]; i++) {
        if (value[i] != ' ' && value[i] != '\t' && value[i] != '\n' && value[i] != '\r') return true; // 공백 아닌 문자 발견
    }
    return false; // 모두 공백
}

static Table *initialize_table_with_headers(size_t row_count, size_t column_count)
{
    Table *t = create_table(row_count, column_count);
    if (!t) return NULL; // 테이블 생성 실패
    t->sheets = malloc(sizeof(char *));
    if (!t->sheets) { // 시트 메모리 실패
        destroy_table(t);
        return NULL;
    }
    t->sheets[0] = duplicate_string("Default"); // 기본 시트 이름
    t->sheet_count = 1;
    return t;
}

static Table *parse_csv(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) return NULL; // 파일 열기 실패
    int rows = 0;
    int cols = 0;
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), fp)) { // 첫 줄 읽기
        char *token = strtok(buffer, ",\n");
        while (token) { // 열 개수 계산
            cols++;
            token = strtok(NULL, ",\n");
        }
        rows++;
    }
    while (fgets(buffer, sizeof(buffer), fp)) rows++; // 행 개수 계산
    rewind(fp);
    if (rows <= 1 || cols <= 0) { // 데이터 없음
        fclose(fp);
        return NULL;
    }
    Table *t = initialize_table_with_headers((size_t)(rows - 1), (size_t)cols);
    if (!t) { // 테이블 생성 실패
        fclose(fp);
        return NULL;
    }
    if (fgets(buffer, sizeof(buffer), fp)) { // 헤더 읽기
        int col = 0;
        char *token = strtok(buffer, ",\n");
        while (token && col < cols) {
            free(t->headers[col]);
            t->headers[col] = duplicate_string(token);
            col++;
            token = strtok(NULL, ",\n");
        }
    }
    for (size_t row = 0; row < t->row_count && fgets(buffer, sizeof(buffer), fp); row++) {
        int col = 0;
        char *token = strtok(buffer, ",\n");
        while (token && col < (int)t->column_count) {
            free(t->rows[row * t->column_count + col]);
            t->rows[row * t->column_count + col] = duplicate_string(token);
            col++;
            token = strtok(NULL, ",\n");
        }
        while (col < (int)t->column_count) { // 빈 셀 채우기
            free(t->rows[row * t->column_count + col]);
            t->rows[row * t->column_count + col] = duplicate_string("");
            col++;
        }
    }
    fclose(fp);
    set_table_metadata(t, filename);
    return t;
}

static Table *parse_xlsx(const char *filename)
{
    xlsxioreader reader = xlsxioread_open(filename);
    if (!reader) return NULL; // 파일 열기 실패
    Table *t = malloc(sizeof(Table));
    if (!t) { // 메모리 실패
        xlsxioread_close(reader);
        return NULL;
    }
    t->headers = NULL;
    t->rows = NULL;
    t->sheets = NULL;
    t->sheet_count = 0;
    t->row_count = 0;
    t->column_count = 0;
    xlsxioreadersheetlist sheetlist = xlsxioread_sheetlist_open(reader);
    if (!sheetlist) { // 시트 목록 실패
        xlsxioread_close(reader);
        free(t);
        return NULL;
    }
    const char *sheetname;
    size_t sheet_count = 0;
    while ((sheetname = xlsxioread_sheetlist_next(sheetlist)) != NULL) {
        char **new_sheets = realloc(t->sheets, sizeof(char *) * (sheet_count + 1));
        if (!new_sheets) break; // 메모리 실패
        t->sheets = new_sheets;
        t->sheets[sheet_count++] = duplicate_string(sheetname);
    }
    xlsxioread_sheetlist_close(sheetlist);
    t->sheet_count = sheet_count;
    if (sheet_count > 0) {
        xlsxioreadersheet sheet = xlsxioread_sheet_open(reader, t->sheets[0], XLSXIOREAD_SKIP_NONE);
        if (sheet) {
            char **cellData = NULL;
            int *cellRows = NULL;
            int *cellCols = NULL;
            size_t capacity = 0;
            size_t cellCount = 0;
            int maxRow = -1;
            int maxCol = -1;
            int row = 0;
            XLSXIOCHAR *value;
            while (xlsxioread_sheet_next_row(sheet)) {
                int col = 0;
                while ((value = xlsxioread_sheet_next_cell(sheet)) != NULL) {
                    if (cellCount == capacity && !reserve_cell_buffer(&cellData, &cellRows, &cellCols, &capacity)) break; // 버퍼 확장 실패
                    cellData[cellCount] = duplicate_string(value);
                    cellRows[cellCount] = row;
                    cellCols[cellCount] = col;
                    if (row > maxRow) maxRow = row;
                    if (col > maxCol) maxCol = col;
                    cellCount++;
                    xlsxioread_free(value);
                    col++;
                }
                row++;
            }
            size_t data_rows = maxRow > 0 ? (size_t)maxRow : 0;
            size_t columns = maxCol >= 0 ? (size_t)maxCol + 1 : 0;
            Table *copy = initialize_table_with_headers(data_rows, columns);
            if (!copy) { // 테이블 생성 실패
                for (size_t i = 0; i < cellCount; i++) free(cellData[i]);
                free(cellData);
                free(cellRows);
                free(cellCols);
                xlsxioread_sheet_close(sheet);
                xlsxioread_close(reader);
                destroy_table(t);
                return NULL;
            }
            free(t->sheets);
            *t = *copy;
            free(copy);
            for (size_t i = 0; i < cellCount; i++) {
                int r = cellRows[i];
                int c = cellCols[i];
                if (r == 0) { // 헤더 처리
                    free(t->headers[c]);
                    t->headers[c] = duplicate_string(cellData[i]);
                } else if ((size_t)(r - 1) < t->row_count && (size_t)c < t->column_count) {
                    size_t idx = (size_t)(r - 1) * t->column_count + (size_t)c;
                    free(t->rows[idx]);
                    t->rows[idx] = duplicate_string(cellData[i]);
                }
                free(cellData[i]);
            }
            free(cellData);
            free(cellRows);
            free(cellCols);
            xlsxioread_sheet_close(sheet);
        }
    }
    xlsxioread_close(reader);
    set_table_metadata(t, filename);
    return t;
}

Table *load_table(const char *filename)
{
    const char *ext = file_extension(filename);
    if (!ext[0]) return NULL; // 확장자 없음
    if (strcasecmp(ext, ".csv") == 0) return parse_csv(filename); // CSV 처리
    if (is_excel_file(ext)) return parse_xlsx(filename); // 엑셀 처리
    return NULL; // 지원하지 않는 확장자
}

void serialize_table(const Table *t, const char *filename)
{
    if (!t || !filename) return; // NULL 체크
    FILE *fp = fopen(filename, "w");
    if (!fp) return; // 파일 열기 실패
    for (size_t j = 0; j < t->column_count; j++)
        fprintf(fp, "%s%s", t->headers[j], j + 1 == t->column_count ? "\n" : ","); // 헤더 출력
    for (size_t i = 0; i < t->row_count; i++)
        for (size_t j = 0; j < t->column_count; j++)
            fprintf(fp, "%s%s", t->rows[i * t->column_count + j], j + 1 == t->column_count ? "\n" : ","); // 데이터 출력
    fclose(fp);
}

Table *deserialize_table(const char *filename)
{
    return load_table(filename); // 파일 로드
}

void analyze_table(const Table *t, const char *filename, OutputSink *out)
{
    if (!t || !filename || !out) return; // NULL 체크
    int existCount = 0;
    char **nonExistCoords = NULL;
    int nonExistCount = 0;
    int allocatedSize = 0;
    int firstRow = -1;
    int firstCol = -1;
    int lastCol = -1;
    int actualFirstRow = -1;
    int actualLastRow = -1;
    for (size_t i = 0; i < t->row_count; i++) {
        for (size_t j = 0; j < t->column_count; j++) {
            const char *val = t->rows[i * t->column_count + j];
            if (has_nonblank(val)) { // 값 존재
                existCount++;
                if (firstRow == -1) {
                    firstRow = (int)i;
                    actualFirstRow = (int)i + 2;
                }
                actualLastRow = (int)i + 2;
                if (firstCol == -1) firstCol = (int)j;
                if ((int)j > lastCol) lastCol = (int)j;
                continue;
            }
            if (nonExistCount >= allocatedSize) {
                allocatedSize = allocatedSize ? allocatedSize * 2 : 100;
                char **tmp = realloc(nonExistCoords, sizeof(char *) * allocatedSize);
                if (!tmp) break; // 메모리 실패
                nonExistCoords = tmp;
            }
            char buf[32];
            snprintf(buf, sizeof(buf), "(%s, %d)", col_index_to_letter(j), (int)i + 2);
            nonExistCoords[nonExistCount++] = duplicate_string(buf);
        }
    }
    output_printf(out, "[%s]\n", filename);
    for (size_t s = 0; s < t->sheet_count; s++)
        output_printf(out, "   sheet명(%zu): %s\n", s + 1, t->sheets[s]); // 시트명 출력
    if (firstRow != -1)
        output_printf(out, "   시작: %s, %d    종료: %s, %d\n", col_index_to_letter(firstCol), actualFirstRow,
                      col_index_to_letter(lastCol), actualLastRow); // 시작/종료 위치
    else
        output_printf(out, "   시작: -, -    종료: -, -\n"); // 데이터 없음
    output_printf(out, "   실존 데이터 갯수: %d\n", existCount);
    output_printf(out, "   가로(열) × 세로(행) : %zu × %zu\n", t->column_count, t->row_count);
    output_printf(out, "   비실존 데이터 갯수: %d\n", nonExistCount);
    output_printf(out, "   비실존 데이터 좌표값: ");
    for (int k = 0; k < nonExistCount; k++) {
        output_printf(out, "%s ", nonExistCoords[k]);
        free(nonExistCoords[k]);
    }
    free(nonExistCoords);
    output_printf(out, "\n");
    output_printf(out, "   업로드 파일의 경로 위치: %s\n", t->filepath);
    output_printf(out, "   확인한 기간 및 시간: %s\n\n", t->timestamp);
}

void analyze_all_sheets(xlsxioreader xlsxioread, const char *filename, OutputSink *out)
{
    if (!xlsxioread || !filename || !out) return; // NULL 체크
    xlsxioreadersheetlist sheetlist = xlsxioread_sheetlist_open(xlsxioread);
    if (!sheetlist) return; // 시트 목록 실패
    const char *sheetname;
    while ((sheetname = xlsxioread_sheetlist_next(sheetlist)) != NULL) {
        xlsxioreadersheet sheet = xlsxioread_sheet_open(xlsxioread, sheetname, XLSXIOREAD_SKIP_NONE);
        if (!sheet) continue; // 시트 열기 실패
        size_t cellCount = 0;
        size_t capacity = 0;
        char **cellData = NULL;
        int *cellRows = NULL;
        int *cellCols = NULL;
        int maxRow = -1;
        int maxCol = -1;
        int row = 0;
        XLSXIOCHAR *value;
        while (xlsxioread_sheet_next_row(sheet)) {
            int col = 0;
            while ((value = xlsxioread_sheet_next_cell(sheet)) != NULL) {
                if (cellCount == capacity && !reserve_cell_buffer(&cellData, &cellRows, &cellCols, &capacity)) {
                    xlsxioread_free(value);
                    break;
                }
                cellData[cellCount] = duplicate_string(value);
                cellRows[cellCount] = row;
                cellCols[cellCount] = col;
                if (row > maxRow) maxRow = row;
                if (col > maxCol) maxCol = col;
                cellCount++;
                xlsxioread_free(value);
                col++;
            }
            row++;
        }
        output_printf(out, "[%s - Sheet: %s]\n", filename, sheetname);
        int existCount = 0;
        char **nonExistCoords = NULL;
        int nonExistCount = 0;
        int allocatedSize = 0;
        int firstRow = -1;
        int firstCol = -1;
        int lastCol = -1;
        int actualFirstRow = -1;
        int actualLastRow = -1;
        for (int i = 1; i <= maxRow; i++) {
            for (int j = 0; j <= maxCol; j++) {
                const char *val = "";
                for (size_t k = 0; k < cellCount; k++) {
                    if (cellRows[k] == i && cellCols[k] == j) {
                        val = cellData[k];
                        break;
                    }
                }
                if (has_nonblank(val)) { // 값 존재
                    existCount++;
                    if (firstRow == -1) {
                        firstRow = i;
                        actualFirstRow = i + 1;
                    }
                    actualLastRow = i + 1;
                    if (firstCol == -1) firstCol = j;
                    if (j > lastCol) lastCol = j;
                    continue;
                }
                if (nonExistCount >= allocatedSize) {
                    allocatedSize = allocatedSize ? allocatedSize * 2 : 100;
                    char **tmp = realloc(nonExistCoords, sizeof(char *) * allocatedSize);
                    if (!tmp) break; // 메모리 실패
                    nonExistCoords = tmp;
                }
                char buf[32];
                snprintf(buf, sizeof(buf), "(%s, %d)", col_index_to_letter(j), i + 1);
                nonExistCoords[nonExistCount++] = duplicate_string(buf);
            }
        }
        if (firstRow != -1)
            output_printf(out, "   시작: %s, %d    종료: %s, %d\n", col_index_to_letter(firstCol), actualFirstRow,
                          col_index_to_letter(lastCol), actualLastRow); // 시작/종료 위치
        else
            output_printf(out, "   시작: -, -    종료: -, -\n"); // 데이터 없음
        output_printf(out, "   실존 데이터 갯수: %d\n", existCount);
        output_printf(out, "   가로(열) × 세로(행) : %d × %d\n", maxCol + 1, maxRow);
        output_printf(out, "   비실존 데이터 갯수: %d\n", nonExistCount);
        output_printf(out, "   비실존 데이터 좌표값: ");
        for (int k = 0; k < nonExistCount; k++) {
            output_printf(out, "%s ", nonExistCoords[k]);
            free(nonExistCoords[k]);
        }
        free(nonExistCoords);
        output_printf(out, "\n\n");
        for (size_t i = 0; i < cellCount; i++) free(cellData[i]); // 셀 데이터 해제
        free(cellData);
        free(cellRows);
        free(cellCols);
        xlsxioread_sheet_close(sheet);
    }
    xlsxioread_sheetlist_close(sheetlist);
}

void set_table_metadata(Table *t, const char *filename)
{
    if (!t || !filename) return; // NULL 체크
    strncpy(t->filepath, filename, sizeof(t->filepath) - 1);
    t->filepath[sizeof(t->filepath) - 1] = '\0';

    time_t now = time(NULL);
    struct tm tm;
    if (localtime_r(&now, &tm)) // 시간 변환 성공
        strftime(t->timestamp, sizeof(t->timestamp), "%Y.%m.%d %H:%M:%S", &tm);
    else
        t->timestamp[0] = '\0'; // 실패 시 빈 문자열
}

bool analyze_file(const char *filename, OutputSink *out)
{
    if (!filename || !out) return false; // NULL 체크
    const char *ext = file_extension(filename);
    if (is_excel_file(ext)) {
        xlsxioreader reader = xlsxioread_open(filename);
        if (!reader) return false; // 엑셀 파일 열기 실패
        analyze_all_sheets(reader, filename, out);
        xlsxioread_close(reader);
        return true;
    }
    Table *t = load_table(filename);
    if (!t) return false; // 테이블 로드 실패
    set_table_metadata(t, filename);
    analyze_table(t, filename, out);
    destroy_table(t);
    return true;
}
