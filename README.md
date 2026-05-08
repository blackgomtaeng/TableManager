# TableManager
Linux + C Lang. = scriptLinuxFile &amp; Terminal command


<img width="1024" height="1536" alt="image" src="https://github.com/user-attachments/assets/4883ccb3-5703-492e-8f5d-a62bea85e0d4" />

[TableManager 흐름도]
01 프로그램 시작 → 입력 인자 확인
02 인자가 부족하면 사용법 출력 후 종료, 파일이 있으면 로그 파일 생성
03 각 파일을 반복 처리하면서 확장자 검사
   .xlsx/.xls → analyze_all_sheets로 엑셀 분석
   그 외 → analyze_table로 일반 테이블 분석
04 결과는 화면과 로그 파일에 출력
05 마지막으로 프로그램 종료
