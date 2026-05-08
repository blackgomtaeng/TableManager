# TableManager
Linux + C Lang. = scriptLinuxFile &amp; Terminal command

[TableManager 흐름도]
01 입력 처리: main.c에서 전달받은 여러 파일을 순회하며 확장자에 따라 분기.

02 엑셀 파일(.xlsx/.xls):
   → xlsxioread_open으로 파일을 열고
   → analyze_all_sheets()로 모든 시트를 분석 후 결과 출력.

03 CSV 등 일반 파일:
   → load_table()로 테이블 구조 생성
   → analyze_table()로 데이터 존재 여부, 좌표, 통계 분석
   → destroy_table()로 메모리 해제.

04 로그 관리: tempTableManager 폴더에 날짜별 로그 파일 생성, 결과를 화면과 로그에 동시 기록.
05 출력 결과: 각 파일의 데이터 존재 개수, 비어 있는 셀 좌표, 시트명, 파일 경로, 분석 시각을 표시.
