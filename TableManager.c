#define _POSIX_C_SOURCE 200809L   // POSIX 표준 기능 활성화 (예: strdup, realpath 등)
#include <stdlib.h>               // 메모리 할당/해제 함수 포함
#include <stdio.h>                // 파일 입출력 함수 포함
#include <string.h>               // 문자열 처리 함수 포함
#include <time.h>                 // 시간 관련 함수 포함
#include "TableManager.h"         // 테이블 관리용 헤더파일 포함
#include <xlsxio_read.h>          // XLSX 파일 읽기 라이브러리 포함

char *realpath(const char *path, char *resolved_path);   // 파일 경로를 절대경로로 변환
char *strdup(const char *s);                             // 문자열 복제 함수

Table* create_table(int rows, int cols) {                // 테이블 생성 함수
    Table *t = malloc(sizeof(Table));                    // Table 구조체 메모리 할당
    t->row_count = rows;                                 // 행 개수 설정
    t->column_count = cols;                              // 열 개수 설정
    t->rows = calloc(rows * cols, sizeof(char*));        // 데이터 저장 공간 초기화
    t->headers = calloc(cols, sizeof(char*));            // 헤더 저장 공간 초기화
    t->sheets = NULL;                                    // 시트 초기화
    t->sheet_count = 0;                                  // 시트 개수 초기화
    return t;                                            // 생성된 테이블 반환
}

void destroy_table(Table *t) {                           // 테이블 메모리 해제 함수
    if (!t) return;                                      // NULL 체크
    for (int i = 0; i < t->row_count * t->column_count; i++)
        if (t->rows[i]) free(t->rows[i]);                // 각 셀 데이터 해제
    for (int j = 0; j < t->column_count; j++)
        if (t->headers[j]) free(t->headers[j]);          // 헤더 해제
    if (t->sheets) {                                     // 시트 존재 시 해제
        for (int k = 0; k < t->sheet_count; k++)
            if (t->sheets[k]) free(t->sheets[k]);
        free(t->sheets);
    }
    free(t->rows);                                       // 행 데이터 배열 해제
    free(t->headers);                                    // 헤더 배열 해제
    free(t);                                             // 구조체 자체 해제
}

static char* col_index_to_letter(int col) {              // 열 인덱스를 엑셀 열 문자로 변환
    static char buf[10];                                 // 변환 결과 저장 버퍼
    memset(buf, 0, sizeof(buf));
    int idx = 0;
    col++;
    while (col > 0) {                                    // 26진수 변환
        col--;
        buf[idx++] = 'A' + (col % 26);
        col /= 26;
    }
    buf[idx] = '\0';
    for (int i = 0; i < idx / 2; i++) {                  // 문자열 뒤집기
        char tmp = buf[i];
        buf[i] = buf[idx - 1 - i];
        buf[idx - 1 - i] = tmp;
    }
    return buf;
}

static Table* parse_csv(const char *filename) {          // CSV 파일을 읽어 테이블 구조체로 변환
    FILE *fp = fopen(filename, "r");                     // 파일 열기
    if (!fp) return NULL;                                // 파일 열기 실패 시 NULL 반환
    int rows = 0, cols = 0;                              // 행, 열 개수 초기화
    char buffer[1024];                                   // 한 줄 버퍼

    if (fgets(buffer, sizeof(buffer), fp)) {             // 첫 줄 읽기 (헤더)
        char *token = strtok(buffer, ",\n");             // 콤마 기준으로 토큰 분리
        while (token) { cols++; token = strtok(NULL, ",\n"); } // 열 개수 계산
        rows++;                                          // 첫 줄은 헤더 → 행 개수 증가
    }
    while (fgets(buffer, sizeof(buffer), fp)) rows++;    // 나머지 줄 개수 세기
    rewind(fp);                                          // 파일 포인터 처음으로 되돌리기

    if (rows <= 1 || cols <= 0) {                        // 데이터가 없으면 NULL 반환
        fclose(fp);
        return NULL;
    }

    Table *t = create_table(rows - 1, cols);             // 헤더 제외한 행 개수로 테이블 생성
    if (fgets(buffer, sizeof(buffer), fp)) {             // 헤더 행 읽기
        int col = 0;
        char *token = strtok(buffer, ",\n");
        while (token) {
            t->headers[col] = strdup(token);             // 헤더 저장
            col++;
            token = strtok(NULL, ",\n");
        }
    }
    int row = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {          // 데이터 행 반복
        int col = 0;
        char *token = strtok(buffer, ",\n");
        while (token && col < cols) {
            t->rows[row * t->column_count + col] = strdup(token); // 셀 값 저장
            col++;
            token = strtok(NULL, ",\n");
        }
        while (col < cols) {                             // 부족한 열은 빈 문자열로 채움
            t->rows[row * t->column_count + col] = strdup("");
            col++;
        }
        row++;
    }
    fclose(fp);                                          // 파일 닫기

    t->sheet_count = 1;                                  // 시트 개수 1개
    t->sheets = malloc(sizeof(char*));                   // 시트 배열 할당
    t->sheets[0] = strdup("Default");                    // 기본 시트 이름 저장

    realpath(filename, t->filepath);                     // 파일 절대경로 저장
    time_t now = time(NULL);                             // 현재 시간 가져오기
    strftime(t->timestamp, sizeof(t->timestamp), "%Y.%m.%d %H:%M:%S", localtime(&now)); // 타임스탬프 저장

    return t;                                            // 테이블 반환
}

static Table* parse_xlsx(const char *filename) {         // XLSX 파일을 읽어 테이블 구조체로 변환
    xlsxioreader xlsxioread = xlsxioread_open(filename); // XLSX 파일 열기
    if (!xlsxioread) return NULL;                        // 실패 시 NULL 반환

    Table *t = malloc(sizeof(Table));                    // 테이블 구조체 메모리 할당
    t->headers = NULL;                                   // 초기화
    t->rows = NULL;
    t->sheets = NULL;
    t->sheet_count = 0;
    t->row_count = 0;
    t->column_count = 0;

    xlsxioreadersheetlist sheetlist = xlsxioread_sheetlist_open(xlsxioread); // 시트 목록 열기
    const char *sheetname;
    int count = 0;
    while ((sheetname = xlsxioread_sheetlist_next(sheetlist)) != NULL) {     // 시트 이름 반복
        char **new_sheets = realloc(t->sheets, sizeof(char*) * (count + 1)); // 시트 배열 확장
        if (!new_sheets) break;
        t->sheets = new_sheets;
        t->sheets[count] = strdup(sheetname);            // 시트 이름 저장
        count++;
    }
    xlsxioread_sheetlist_close(sheetlist);               // 시트 목록 닫기
    t->sheet_count = count;                              // 시트 개수 저장

    if (count > 0) {                                     // 첫 번째 시트 읽기
        const char *sheetname = t->sheets[0];
        xlsxioreadersheet sheet = xlsxioread_sheet_open(xlsxioread, sheetname, XLSXIOREAD_SKIP_NONE);
        if (sheet) {
            int maxRow = -1, maxCol = -1;                // 최대 행/열 추적
            char *cellData[100000];                      // 셀 데이터 저장
            int cellCount = 0;
            int *cellRows = malloc(sizeof(int) * 100000);
            int *cellCols = malloc(sizeof(int) * 100000);

            int row = 0, col = 0;
            XLSXIOCHAR *value;

            while (xlsxioread_sheet_next_row(sheet)) {   // 행 반복
                col = 0;
                while ((value = xlsxioread_sheet_next_cell(sheet)) != NULL) { // 셀 반복
                    if (cellCount < 100000) {
                        cellData[cellCount] = strdup(value ? value : "");    // 셀 값 저장
                        cellRows[cellCount] = row;
                        cellCols[cellCount] = col;
                        if (row > maxRow) maxRow = row;                      // 최대 행 갱신
                        if (col > maxCol) maxCol = col;                      // 최대 열 갱신
                        cellCount++;
                    }
                    if (value) xlsxioread_free(value);   // 메모리 해제
                    col++;
                }
                row++;
            }

            t->row_count = maxRow;                       // 헤더 제외한 데이터 행 개수
            t->column_count = maxCol + 1;                // 열 개수

            t->headers = calloc(t->column_count, sizeof(char*));             // 헤더 배열 초기화
            t->rows = calloc(t->row_count * t->column_count, sizeof(char*)); // 데이터 배열 초기화

            for (int i = 0; i < t->row_count * t->column_count; i++)
                t->rows[i] = strdup("");                 // 빈 문자열로 초기화
            for (int j = 0; j < t->column_count; j++)
                t->headers[j] = strdup("");              // 헤더 초기화

            for (int i = 0; i < cellCount; i++) {        // 셀 데이터 복사
                int r = cellRows[i];
                int c = cellCols[i];

                if (r == 0) {                            // 첫 행은 헤더
                    if (t->headers[c]) free(t->headers[c]);
                    t->headers[c] = strdup(cellData[i]);
                } else {                                 // 나머지는 데이터 행
                    int idx = (r - 1) * t->column_count + c;
                    if (idx < t->row_count * t->column_count) {
                        if (t->rows[idx]) free(t->rows[idx]);
                        t->rows[idx] = strdup(cellData[i]);
                    }
                }
                free(cellData[i]);                       // 임시 데이터 해제
            }

            free(cellRows);
            free(cellCols);
            xlsxioread_sheet_close(sheet);               // 시트 닫기
        }
    }

    realpath(filename, t->filepath);                     // 파일 절대경로 저장
    time_t now = time(NULL);                             // 현재 시간 가져오기
    strftime(t->timestamp, sizeof(t->timestamp), "%Y.%m.%d %H:%M:%S", localtime(&now)); // 타임스탬프 저장

    xlsxioread_close(xlsxioread);                        // XLSX 파일 닫기
    return t;                                            // 테이블 반환
}

Table* load_table(const char *filename) {                     // 파일 확장자에 따라 테이블 로드
    const char *ext = strrchr(filename, '.');                  // 파일 확장자 추출
    if (!ext) return NULL;                                     // 확장자 없으면 NULL 반환
    if (strcmp(ext, ".csv") == 0) return parse_csv(filename);  // CSV 파일이면 parse_csv 호출
    if (strcmp(ext, ".xlsx") == 0 || strcmp(ext, ".xls") == 0) return parse_xlsx(filename); // XLSX/XLS 파일이면 parse_xlsx 호출
    return NULL;                                               // 지원하지 않는 확장자면 NULL 반환
}

void serialize_table(Table *t, const char *filename) {         // 테이블을 CSV 형식으로 저장
    FILE *fp = fopen(filename, "w");                           // 쓰기 모드로 파일 열기
    if (!fp) return;                                           // 파일 열기 실패 시 종료
    for (int j = 0; j < t->column_count; j++)                  // 헤더 출력
        fprintf(fp, "%s%s", t->headers[j], j == t->column_count - 1 ? "\n" : ",");
    for (int i = 0; i < t->row_count; i++)                     // 데이터 행 출력
        for (int j = 0; j < t->column_count; j++)
            fprintf(fp, "%s%s", t->rows[i * t->column_count + j], j == t->column_count - 1 ? "\n" : ",");
    fclose(fp);                                                // 파일 닫기
}

Table* deserialize_table(const char *filename) {               // CSV 파일을 테이블로 복원
    return parse_csv(filename);                                // parse_csv 호출
}

void analyze_table(Table *t, const char *filename, FILE *out) { // 테이블 분석 결과 출력
    int existCount = 0;                                        // 실제 데이터 개수
    char **nonExistCoords = NULL;                              // 비어있는 셀 좌표 저장
    int nonExistCount = 0;                                     // 비어있는 셀 개수
    int allocatedSize = 0;                                     // 좌표 배열 크기 관리

    int firstRow = -1, firstCol = -1, lastCol = -1;            // 데이터 시작/끝 위치 추적
    int actualFirstRow = -1, actualLastRow = -1;               // 실제 출력용 행 번호

    for (int i = 0; i < t->row_count; i++) {                   // 모든 행 반복
        for (int j = 0; j < t->column_count; j++) {            // 모든 열 반복
            char *val = t->rows[i * t->column_count + j];      // 셀 값 가져오기

            int hasData = 0;                                   // 데이터 존재 여부
            if (val != NULL && strlen(val) > 0) {              // 값이 존재하면 검사
                for (int k = 0; val[k] != '\0'; k++) {
                    if (val[k] != ' ' && val[k] != '\t' && val[k] != '\n' && val[k] != '\r') {
                        hasData = 1;                           // 공백이 아닌 값 존재
                        break;
                    }
                }
            }

            if (hasData) {                                     // 데이터가 존재하는 경우
                existCount++;                                  // 존재 데이터 개수 증가
                if (firstRow == -1) {                          // 첫 데이터 위치 기록
                    firstRow = i;
                    actualFirstRow = i + 2;                    // 헤더 포함 → +2
                }
                actualLastRow = i + 2;                         // 마지막 데이터 행 갱신
                if (firstCol == -1) firstCol = j;              // 첫 열 기록
                if (j > lastCol) lastCol = j;                  // 마지막 열 갱신
            } else {                                           // 데이터가 없는 경우
                if (nonExistCount >= allocatedSize) {          // 좌표 배열 확장 필요 시
                    allocatedSize = (allocatedSize == 0) ? 100 : allocatedSize * 2;
                    char **tmp = realloc(nonExistCoords, sizeof(char*) * allocatedSize);
                    if (!tmp) break;
                    nonExistCoords = tmp;
                }

                char buf[32];                                  // 좌표 문자열 생성
                snprintf(buf, sizeof(buf), "(%s, %d)", col_index_to_letter(j), i + 2); // 열 문자, 행 번호
                nonExistCoords[nonExistCount] = strdup(buf);   // 좌표 저장
                nonExistCount++;
            }
        }
    }

    fprintf(out, "[%s]\n", filename);                          // 파일 이름 출력
    for (int s = 0; s < t->sheet_count; s++)                   // 시트 이름 출력
        fprintf(out, "   sheet명(%d): %s\n", s + 1, t->sheets[s]);

    if (firstRow != -1) {                                      // 데이터 시작/종료 위치 출력
        fprintf(out, "   시작: %s, %d    종료: %s, %d\n",
                col_index_to_letter(firstCol), actualFirstRow,
                col_index_to_letter(lastCol), actualLastRow);
    } else {
        fprintf(out, "   시작: -, -    종료: -, -\n");         // 데이터 없음
    }

    fprintf(out, "   실존 데이터 갯수: %d\n", existCount);     // 실제 데이터 개수 출력
    fprintf(out, "   가로(열) × 세로(행) : %d × %d\n", t->column_count, t->row_count); // 테이블 크기 출력
    fprintf(out, "   비실존 데이터 갯수: %d\n", nonExistCount); // 비어있는 셀 개수 출력
    fprintf(out, "   비실존 데이터 좌표값: ");                 // 비어있는 셀 좌표 출력
    for (int k = 0; k < nonExistCount; k++) {
        fprintf(out, "%s ", nonExistCoords[k]);
        free(nonExistCoords[k]);                               // 좌표 메모리 해제
    }
    if (nonExistCoords) free(nonExistCoords);                  // 좌표 배열 해제

    fprintf(out, "\n");
    fprintf(out, "   업로드 파일의 경로 위치: %s\n", t->filepath); // 파일 경로 출력
    fprintf(out, "   확인한 기간 및 시간: %s\n\n", t->timestamp); // 분석 시간 출력
}

void analyze_all_sheets(xlsxioreader xlsxioread, const char *filename, FILE *out) {   // 모든 시트를 분석하는 함수
    xlsxioreadersheetlist sheetlist = xlsxioread_sheetlist_open(xlsxioread);          // 시트 목록 열기
    const char *sheetname;
    
    while ((sheetname = xlsxioread_sheetlist_next(sheetlist)) != NULL) {              // 각 시트 반복
        xlsxioreadersheet sheet = xlsxioread_sheet_open(xlsxioread, sheetname, XLSXIOREAD_SKIP_NONE); // 시트 열기
        if (!sheet) continue;                                                         // 시트 열기 실패 시 건너뜀

        int maxRow = -1, maxCol = -1;                                                 // 최대 행/열 추적
        char *cellData[100000];                                                       // 셀 데이터 저장
        int cellCount = 0;
        int *cellRows = malloc(sizeof(int) * 100000);                                 // 셀 행 인덱스 저장
        int *cellCols = malloc(sizeof(int) * 100000);                                 // 셀 열 인덱스 저장

        int row = 0, col = 0;
        XLSXIOCHAR *value;

        while (xlsxioread_sheet_next_row(sheet)) {                                    // 행 반복
            col = 0;
            while ((value = xlsxioread_sheet_next_cell(sheet)) != NULL) {             // 셀 반복
                if (cellCount < 100000) {
                    cellData[cellCount] = strdup(value ? value : "");                 // 셀 값 저장
                    cellRows[cellCount] = row;                                        // 행 번호 기록
                    cellCols[cellCount] = col;                                        // 열 번호 기록
                    if (row > maxRow) maxRow = row;                                   // 최대 행 갱신
                    if (col > maxCol) maxCol = col;                                   // 최대 열 갱신
                    cellCount++;
                }
                if (value) xlsxioread_free(value);                                    // 메모리 해제
                col++;
            }
            row++;
        }

        fprintf(out, "[%s - Sheet: %s]\n", filename, sheetname);                      // 시트 이름 출력
        
        int existCount = 0;                                                           // 실제 데이터 개수
        char **nonExistCoords = NULL;                                                 // 비어있는 셀 좌표 저장
        int nonExistCount = 0;
        int allocatedSize = 0;
        int firstRow = -1, firstCol = -1, lastCol = -1;                               // 데이터 시작/끝 위치 추적
        int actualFirstRow = -1, actualLastRow = -1;

        for (int i = 1; i <= maxRow; i++) {                                           // 모든 행 반복
            for (int j = 0; j <= maxCol; j++) {                                       // 모든 열 반복
                char *val = "";                                                       // 기본값 빈 문자열
                
                for (int k = 0; k < cellCount; k++) {                                 // 해당 좌표의 값 찾기
                    if (cellRows[k] == i && cellCols[k] == j) {
                        val = cellData[k];
                        break;
                    }
                }

                int hasData = 0;                                                      // 데이터 존재 여부
                if (val != NULL && strlen(val) > 0) {                                 // 값이 존재하면 검사
                    for (int k = 0; val[k] != '\0'; k++) {
                        if (val[k] != ' ' && val[k] != '\t' && val[k] != '\n' && val[k] != '\r') {
                            hasData = 1;                                              // 공백이 아닌 값 존재
                            break;
                        }
                    }
                }

                if (hasData) {                                                        // 데이터가 존재하는 경우
                    existCount++;                                                     // 존재 데이터 개수 증가
                    if (firstRow == -1) {                                             // 첫 데이터 위치 기록
                        firstRow = i;
                        actualFirstRow = i + 1;                                       // 실제 출력용 행 번호
                    }
                    actualLastRow = i + 1;                                            // 마지막 데이터 행 갱신
                    if (firstCol == -1) firstCol = j;                                 // 첫 열 기록
                    if (j > lastCol) lastCol = j;                                     // 마지막 열 갱신
                } else {                                                              // 데이터가 없는 경우
                    if (nonExistCount >= allocatedSize) {                             // 좌표 배열 확장 필요 시
                        allocatedSize = (allocatedSize == 0) ? 100 : allocatedSize * 2;
                        char **tmp = realloc(nonExistCoords, sizeof(char*) * allocatedSize);
                        if (!tmp) break;
                        nonExistCoords = tmp;
                    }

                    char buf[32];                                                     // 좌표 문자열 생성
                    snprintf(buf, sizeof(buf), "(%s, %d)", col_index_to_letter(j), i + 1); // 열 문자, 행 번호
                    nonExistCoords[nonExistCount] = strdup(buf);                      // 좌표 저장
                    nonExistCount++;
                }
            }
        }

        fprintf(out, "   시작: %s, %d    종료: %s, %d\n",                             // 데이터 시작/종료 위치 출력
                col_index_to_letter(firstCol), actualFirstRow,
                col_index_to_letter(lastCol), actualLastRow);
        fprintf(out, "   실존 데이터 갯수: %d\n", existCount);                        // 실제 데이터 개수 출력
        fprintf(out, "   가로(열) × 세로(행) : %d × %d\n", maxCol + 1, maxRow);       // 테이블 크기 출력
        fprintf(out, "   비실존 데이터 갯수: %d\n", nonExistCount);                   // 비어있는 셀 개수 출력
        fprintf(out, "   비실존 데이터 좌표값: ");                                    // 비어있는 셀 좌표 출력
        for (int k = 0; k < nonExistCount; k++) {
            fprintf(out, "%s ", nonExistCoords[k]);
            free(nonExistCoords[k]);                                                  // 좌표 메모리 해제
        }
        if (nonExistCoords) free(nonExistCoords);                                     // 좌표 배열 해제
        fprintf(out, "\n\n");

        for (int i = 0; i < cellCount; i++)                                           // 셀 데이터 메모리 해제
            free(cellData[i]);
        free(cellRows);
        free(cellCols);
        xlsxioread_sheet_close(sheet);                                                // 시트 닫기
    }

    xlsxioread_sheetlist_close(sheetlist);                                            // 시트 목록 닫기
}
