#ifndef TABLEMANAGER_H   // 헤더 가드 시작
#define TABLEMANAGER_H   // 헤더 가드 정의

#include <stdbool.h>     // bool 타입 사용
#include <stddef.h>      // size_t 등 정의
#include <stdio.h>       // 파일 입출력
#include <stdlib.h>      // 메모리 관리, exit 등
#include <string.h>      // 문자열 처리
#include <stdarg.h>      // 가변 인자 처리
#include <time.h>        // 시간 관련 함수
#include <limits.h>      // 상수 정의
#include <errno.h>       // 오류 코드
#include <unistd.h>      // POSIX 함수
#include <sys/stat.h>    // 파일 상태 확인
#include <xlsxio_read.h> // XLSX 파일 읽기 라이브러리

#ifndef PATH_MAX
#define PATH_MAX 4096    // 경로 최대 길이 정의
#endif

typedef struct {
    FILE *primary;       // 기본 출력 파일
    FILE *secondary;     // 보조 출력 파일
} OutputSink;

extern char default_path[PATH_MAX]; // 기본 경로
extern char temp_folder[PATH_MAX];  // 임시 폴더 경로

typedef struct {
    char **rows;         // 데이터 행
    size_t row_count;    // 행 개수
    size_t column_count; // 열 개수
    char **headers;      // 열 헤더
    char **sheets;       // 시트 이름
    size_t sheet_count;  // 시트 개수
    char filepath[512];  // 파일 경로
    char timestamp[64];  // 타임스탬프
} Table;

Table *create_table(size_t rows, size_t cols);              // 테이블 생성
void destroy_table(Table *t);                               // 테이블 메모리 해제
Table *load_table(const char *filename);                    // 테이블 로드
Table *deserialize_table(const char *filename);             // 파일에서 테이블 역직렬화
void serialize_table(const Table *t, const char *filename); // 테이블 직렬화 저장
void analyze_table(const Table *t, const char *filename, OutputSink *out); // 테이블 분석
void analyze_all_sheets(xlsxioreader xlsxioread, const char *filename, OutputSink *out); // 모든 시트 분석
void set_table_metadata(Table *t, const char *filename);    // 메타데이터 설정
bool ensure_folder_exists(const char *path);                // 폴더 존재 확인 및 생성
bool compose_path(char *dest, size_t dest_size, const char *dir, const char *name); // 경로 합성
const char *file_extension(const char *filename);           // 파일 확장자 추출
bool is_excel_file(const char *ext);                        // 엑셀 파일 여부 확인
bool analyze_file(const char *filename, OutputSink *out);   // 파일 분석
void set_default_path(const char *path);                    // 기본 경로 설정
void set_temp_folder(const char *folder);                   // 임시 폴더 설정
FILE *open_log_file(const char *folder, char *path, size_t path_size); // 로그 파일 열기

#endif   // 헤더 가드 종료
