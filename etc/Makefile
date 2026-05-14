# TableManager Makefile
# 빌드 및 컴파일 명령어 자동화

# 컴파일러 설정
CC = gcc
CFLAGS = -Wall -Wextra -g -O2
LDFLAGS = -lxlsx

# 소스 파일 및 객체 파일
SOURCES = main.c TableManager.c
OBJECTS = main.o TableManager.o
EXECUTABLE = TableManager

# 기본 타겟
all: deps build

# 의존성 설치 (Ubuntu/Debian)
deps:
    @echo "→ 시스템 업데이트 및 라이브러리 설치 중..."
    sudo apt-get update
    sudo apt-get install -y libxlsxio-dev
    @echo "→ 라이브러리 확인:"
    pkg-config --cflags --libs xlsxio_read

# 빌드 타겟
build: $(EXECUTABLE)
    @echo "✓ 빌드 완료: $(EXECUTABLE)"

# 링크 단계
$(EXECUTABLE): $(OBJECTS)
    @echo "→ 링크 중: $@"
    $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 컴파일 단계
%.o: %.c
    @echo "→ 컴파일 중: $<"
    $(CC) $(CFLAGS) -c $< -o $@

# 정리
clean:
    @echo "→ 오브젝트 파일 및 실행파일 정리 중..."
    rm -f $(OBJECTS) $(EXECUTABLE)
    @echo "✓ 정리 완료"

# 재빌드
rebuild: clean build

# 컴파일러 정보
info:
    @echo "===== 빌드 설정 정보 ====="
    @echo "컴파일러: $(CC)"
    @echo "컴파일 플래그: $(CFLAGS)"
    @echo "링크 플래그: $(LDFLAGS)"
    @echo "소스 파일: $(SOURCES)"
    @echo "객체 파일: $(OBJECTS)"
    @echo "실행파일: $(EXECUTABLE)"

# 도움말
help:
    @echo "사용 가능한 명령어:"
    @echo "  make              - 의존성 설치 후 빌드"
    @echo "  make deps         - 빌드 의존성 설치 및 확인"
    @echo "  make build        - 소스 파일 컴파일 및 링크"
    @echo "  make rebuild      - 완전 재빌드"
    @echo "  make clean        - 객체파일, 실행파일 제거"
    @echo "  make info         - 빌드 설정 정보 출력"
    @echo "  make help         - 이 도움말 출력"

.PHONY: all deps build rebuild clean info help
