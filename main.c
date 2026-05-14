#include "TableManager.h"
#include <stdio.h>
#include <string.h>

static int parse_arguments(int argc, char *argv[])
{
    if (argc > 1 && strncmp(argv[1], "CTFN=", 5) == 0) {
        set_temp_folder(argv[1] + 5); // 임시 폴더 설정
        return 2;
    }

    set_default_path(argv[1]); // 기본 경로 설정
    if (argc > 2 && strncmp(argv[2], "CTFN=", 5) == 0) {
        set_temp_folder(argv[2] + 5); // 임시 폴더 설정
        return 3;
    }

    return 2; // 파일 인자 시작 위치 반환
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: ./TableManager [default_path] [CTFN=folder] <file1> <file2> ...\n"); // 사용법 출력
        return 1;
    }

    int file_arg_start = parse_arguments(argc, argv); // 인자 파싱
    if (file_arg_start >= argc) {
        fprintf(stderr, "No input files provided.\n"); // 입력 파일 없음
        return 1;
    }

    char log_dir[512];
    if (!compose_path(log_dir, sizeof(log_dir), default_path, temp_folder)) {
        fprintf(stderr, "Path length exceeded\n"); // 경로 길이 초과
        return 1;
    }

    if (!ensure_folder_exists(log_dir)) {
        fprintf(stderr, "Failed to create directory: %s\n", log_dir); // 디렉토리 생성 실패
        return 1;
    }

    char log_path[512];
    FILE *logfp = open_log_file(log_dir, log_path, sizeof(log_path));
    if (!logfp) {
        fprintf(stderr, "Failed to open log file in %s\n", log_dir); // 로그 파일 열기 실패
        return 1;
    }

    OutputSink output = { .primary = stdout, .secondary = logfp }; // 출력 대상 설정
    for (int index = file_arg_start; index < argc; index++) {
        const char *filename = argv[index];
        if (!analyze_file(filename, &output)) {
            fprintf(stderr, "Failed to analyze %s\n", filename); // 분석 실패
            fprintf(logfp, "Failed to analyze %s\n", filename); // 로그 기록
        }
    }

    fclose(logfp); // 로그 파일 닫기
    return 0;
}
