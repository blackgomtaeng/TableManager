#include "TableManager.h"   // TableManager 관련 함수와 구조체 선언 포함
#include <sys/stat.h>       // 파일 상태 확인(stat), 디렉토리 생성(mkdir) 등에 사용
#include <unistd.h>         // POSIX API (예: access, close 등)
#include <xlsxio_read.h>    // XLSX 파일 읽기 라이브러리

void analyze_all_sheets(xlsxioreader xlsxioread, const char *filename, FILE *out); // 모든 시트를 분석하는 함수 선언

int main(int argc, char* argv[]) {   // 프로그램 시작점, 인자 개수와 값 전달
    if (argc < 2) {   // 인자가 2개 미만이면 (즉, 파일 입력이 없으면)
        printf("Usage: ./TableManager <file1> <file2> ...\n"); // 사용법 출력
        return 1;   // 프로그램 종료
    }

    struct stat st = {0};   // 파일 상태 구조체 초기화
    if (stat("tempTableManager", &st) == -1)   // tempTableManager 디렉토리 존재 여부 확인
        mkdir("tempTableManager", 0700);       // 없으면 새로 생성 (권한: 소유자만 접근 가능)

    time_t now = time(NULL);   // 현재 시간 가져오기
    char logname[128];         // 로그 파일 이름 버퍼
    strftime(logname, sizeof(logname), "tempTableManager/tempTM%Y%m%d.txt", localtime(&now)); // 날짜 기반 로그 파일명 생성
    FILE *logfp = fopen(logname, "a");   // 로그 파일 append 모드로 열기
    if (!logfp) {   // 로그 파일 열기 실패 시
        printf("Failed to open log file.\n"); // 에러 메시지 출력
        return 1;   // 프로그램 종료
    }

    for (int i = 1; i < argc; i++) {   // 입력받은 파일들을 하나씩 처리
        const char *ext = strrchr(argv[i], '.');   // 파일 확장자 확인
        if (!ext) continue;   // 확장자가 없으면 건너뜀

        if (strcmp(ext, ".xlsx") == 0 || strcmp(ext, ".xls") == 0) {   // 엑셀 파일인 경우
            xlsxioreader xlsxioread = xlsxioread_open(argv[i]);   // XLSX 파일 열기
            if (!xlsxioread) {   // 열기 실패 시
                printf("Failed to load %s\n\n", argv[i]);   // 에러 메시지 출력
                fprintf(logfp, "Failed to load %s\n\n", argv[i]);   // 로그 기록
                continue;   // 다음 파일로 넘어감
            }

            analyze_all_sheets(xlsxioread, argv[i], stdout);   // 표준 출력으로 분석 결과 출력
            analyze_all_sheets(xlsxioread, argv[i], logfp);    // 로그 파일에도 분석 결과 기록

            xlsxioread_close(xlsxioread);   // XLSX 파일 닫기
        } else {   // 일반 텍스트 기반 테이블 파일인 경우
            Table *t = load_table(argv[i]);   // 테이블 로드
            if (!t) {   // 로드 실패 시
                printf("Failed to load %s\n\n", argv[i]);   // 에러 메시지 출력
                fprintf(logfp, "Failed to load %s\n\n", argv[i]);   // 로그 기록
                continue;   // 다음 파일로 넘어감
            }
            analyze_table(t, argv[i], stdout);   // 표준 출력으로 분석 결과 출력
            analyze_table(t, argv[i], logfp);    // 로그 파일에도 분석 결과 기록
            destroy_table(t);   // 메모리 해제
        }
    }

    fclose(logfp);   // 로그 파일 닫기
    return 0;   // 프로그램 정상 종료
}
