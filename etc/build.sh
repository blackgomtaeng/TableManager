#!/bin/bash

##############################################################################
# TableManager 빌드 스크립트
# 목적: 컴파일러 명령어를 단계별로 실행하고 각 단계를 명확하게 표시
##############################################################################

# 색상 정의 (가독성 향상)
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 컴파일러 및 플래그 설정
CC="gcc"
CFLAGS="-Wall -Wextra -g -O2"
LDFLAGS="-lxlsx"

# 소스 파일들
SOURCES=("main.c" "TableManager.c")
OBJECTS=("main.o" "TableManager.o")
EXECUTABLE="TableManager"

##############################################################################
# 함수들
##############################################################################

print_header() {
    echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║  TableManager 빌드 스크립트 시작       ║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
    echo ""
}

print_step() {
    echo -e "${YELLOW}[Step] $1${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ 오류: $1${NC}"
    exit 1
}

prepare_environment() {
    print_step "0️⃣ 빌드 환경 준비"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    echo "  → 패키지 목록 업데이트"
    sudo apt-get update
    
    echo "  → libxlsxio-dev 설치"
    sudo apt-get install -y libxlsxio-dev
    
    echo "  → 설치 확인"
    if pkg-config --cflags --libs xlsxio_read > /dev/null 2>&1; then
        echo "    ✓ libxlsxio 설치 및 확인 완료"
    else
        print_error "libxlsxio 설치 확인 실패!"
    fi
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
}

show_compiler_info() {
    print_step "1️⃣ 컴파일러 정보 확인"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  컴파일러: $CC"
    echo "  버전: $($CC --version | head -n 1)"
    echo "  컴파일 플래그: $CFLAGS"
    echo "  링크 플래그: $LDFLAGS"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    
    if ! command -v $CC &> /dev/null; then
        print_error "컴파일러 '$CC'를 찾을 수 없습니다!"
    fi
    print_success "컴파일러 준비 완료"
}

clean_objects() {
    print_step "2️⃣ 기존 오브젝트 파일 정리"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    for obj in "${OBJECTS[@]}"; do
        if [ -f "$obj" ]; then
            echo "  → $obj 제거"
            rm -f "$obj"
        fi
    done
    if [ -f "$EXECUTABLE" ]; then
        echo "  → $EXECUTABLE 제거"
        rm -f "$EXECUTABLE"
    fi
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    print_success "정리 완료"
    echo ""
}

compile_sources() {
    print_step "3️⃣ 소스 파일 컴파일"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    for i in "${!SOURCES[@]}"; do
        src="${SOURCES[$i]}"
        obj="${OBJECTS[$i]}"
        
        if [ ! -f "$src" ]; then
            print_error "소스 파일 '$src'를 찾을 수 없습니다!"
        fi
        
        echo "  컴파일: $src → $obj"
        echo "  명령어: $CC $CFLAGS -c $src -o $obj"
        
        if $CC $CFLAGS -c "$src" -o "$obj"; then
            print_success "$obj 생성 완료"
        else
            print_error "컴파일 실패: $src"
        fi
    done
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
}

link_objects() {
    print_step "4️⃣ 오브젝트 파일 링크"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    for obj in "${OBJECTS[@]}"; do
        if [ ! -f "$obj" ]; then
            print_error "오브젝트 파일 '$obj'를 찾을 수 없습니다!"
        fi
    done
    
    echo "  링크: ${OBJECTS[@]} → $EXECUTABLE"
    echo "  명령어: $CC $CFLAGS -o $EXECUTABLE ${OBJECTS[@]} $LDFLAGS"
    
    if $CC $CFLAGS -o "$EXECUTABLE" "${OBJECTS[@]}" $LDFLAGS; then
        print_success "$EXECUTABLE 생성 완료"
    else
        print_error "링크 실패"
    fi
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
}

verify_executable() {
    print_step "5️⃣ 실행파일 확인"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if [ -f "$EXECUTABLE" ] && [ -x "$EXECUTABLE" ]; then
        filesize=$(ls -lh "$EXECUTABLE" | awk '{print $5}')
        echo "  실행파일: $EXECUTABLE (크기: $filesize)"
        echo "  경로: $(pwd)/$EXECUTABLE"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        print_success "실행파일 확인 완료"
    else
        print_error "실행파일 '$EXECUTABLE'을 찾을 수 없습니다!"
    fi
    echo ""
}

show_usage() {
    print_step "6️⃣ 사용 방법"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  실행 방법:"
    echo "    ./$EXECUTABLE <파일1> <파일2> ..."
    echo ""
    echo "  예시:"
    echo "    ./$EXECUTABLE data.csv report.xlsx"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
}

show_summary() {
    echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║  빌드 완료!                            ║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
    echo ""
}

##############################################################################
# 메인 실행
##############################################################################

print_header
prepare_environment
show_compiler_info
clean_objects
compile_sources
link_objects
verify_executable
show_usage
show_summary
