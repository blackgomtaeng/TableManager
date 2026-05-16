# TableManager
Linux + C Lang. = scriptLinuxFile &amp; Terminal command

xlsxio을 통해 제작하였습니다. 

++++++++++++++++++++++++++++++++++++++++++++++++++++++++


Git을 설치한 상태에서 시작하는 명령어입니다. 
다음과 같이 실행할 경우에는 사용자가 원하는 
   Makefile로 접근할 경우에는 주석 중에서 [02 Makefile 사용]을 복사하여 사용하시고,
   bash 즉 일반 터미널로 접근 시에는 [01 bash 터미널 접근]을 복사하여 사용하시길 바랍니다.
   즉, 사용하는 방법은 2가지이라서 제시한 것이니 원하는 바를 접근하여 사용하시기 바랍니다.
   만약 문제가 생긴다면 다른 환경이기 때문에 별도로 생성하여 진행하시기 바랍니다. 
   기준은 github codespace를 중심으로 사용한 것이므로 언제든 환경이 변경될 수 있다는 점을 감안해야 합니다.
아래의 명령어를 복사하고 터미널 속에 붙여 넣어서 실행해주세요.

git clone https://github.com/blackgomtaeng/TableManager.git   # 깃허브 코드스페이스에서 프로젝트 클론 후 다운로드 진행
cd TableManager                                               # 위에 있는 생성된 폴더인 TableManager로 접근

chmod +x build.sh                                             # [01 bash 터미널 접근] 스크립트 실행 권한 부여
./buildTM.sh                                                  # [01 bash 터미널 접근] 스크립트 실행
./TableManager <file1> <file2> <파일 갯수만큼>                 # [01 bash 터미널 접근] 업로드파일들을 올릴 것. 1개 아니면 2개이상 사용가능

make clean && make && make run                                # [02 Makefile 사용] 별도 or 한줄로 권장하는 패턴


++++++++++++++++++++++++++++++++++++++++++++++++++++++++



linux_specs.sh는 기본 리눅스 환경의 사양들을 확인하는 명령어이며 
   기본 다운로드 폴더 경로에서 확인하면 됩니다.
   추가로 linuxSpecs 다음에 현재기간에 맞춰서 출력하도록 제작을 해두었으니 그것을 보고 참고하시면 가장 좋을 것입니다.
   스크립트 실행은 아래와 같이 접근하시면 됩니다.

chmod +x linux_specs.sh && ./linux_specs.sh
