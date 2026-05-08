#ifndef TABLEMANAGER_H   // 헤더 파일 중복 포함 방지
#define TABLEMANAGER_H   // TABLEMANAGER_H 매크로 정의

#ifndef _POSIX_C_SOURCE   // POSIX 표준 기능 사용을 위한 매크로 설정
#define _POSIX_C_SOURCE 200809L   // POSIX.1-2008 표준 활성화
#endif

#include <stdio.h>        // 표준 입출력 함수 사용
#include <stdlib.h>       // 메모리 할당, 해제 함수 사용
#include <string.h>       // 문자열 처리 함수 사용
#include <time.h>         // 시간 관련 함수 사용

typedef struct {
    char **rows;          // 테이블의 데이터 행들을 저장하는 포인터 배열
    int row_count;        // 행(row)의 개수
    int column_count;     // 열(column)의 개수
    char **headers;       // 열 이름(헤더)들을 저장하는 포인터 배열
    char **sheets;        // 시트 이름들을 저장하는 포인터 배열
    int sheet_count;      // 시트 개수
    char filepath[512];   // 테이블 파일 경로 저장
    char timestamp[64];   // 테이블 생성/수정 시간 저장
} Table;                  // 테이블 구조체 정의

Table* create_table(int rows, int cols);             // 새로운 테이블 생성 함수
void destroy_table(Table *t);                        // 테이블 메모리 해제 함수
Table* load_table(const char *filename);             // 파일에서 테이블 불러오기 함수
void serialize_table(Table *t, const char *filename);// 테이블을 파일로 직렬화(저장) 함수
Table* deserialize_table(const char *filename);      // 파일에서 테이블 역직렬화(복원) 함수
void analyze_table(Table *t, const char *filename, FILE *out); // 테이블 분석 결과 출력 함수

#endif   // TABLEMANAGER_H 매크로 끝
