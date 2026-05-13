#!/bin/bash
set -e   # 에러 발생 시 즉시 종료

echo "=== TableManager 빌드 스크립트 시작 ==="

sudo apt-get update -y   # 패키지 목록 업데이트
sudo apt-get install -y make libxlsxio-dev   # make와 XLSX 라이브러리 설치

if command -v gcc-12 >/dev/null 2>&1; then   # gcc-12 설치 여부 확인
    COMPILER=gcc-12   # 신버전 GCC 선택
    echo "신버전 GCC(gcc-12) 감지됨 → 사용합니다."
elif command -v gcc >/dev/null 2>&1; then   # 구버전 gcc 확인
    COMPILER=gcc   # 구버전 GCC 선택
    echo "구버전 GCC(gcc) 감지됨 → 사용합니다."
elif command -v clang >/dev/null 2>&1; then   # clang 확인
    COMPILER=clang   # Clang 선택
    echo "Clang 감지됨 → 사용합니다."
else
    echo "컴파일러가 설치되어 있지 않습니다. GCC 설치 중..."
    sudo apt-get install -y gcc g++   # 기본 GCC 설치
    COMPILER=gcc   # 설치 후 GCC 사용
fi

echo "컴파일러: $COMPILER"
$COMPILER -o TableManager main.c TableManager.c -lxlsxio_read -lxlsxio -lexpat   # 프로그램 빌드

echo "=== 빌드 완료! ==="
echo "./TableManager sample.csv   # CSV 파일 실행 예시"
echo "./TableManager sample.xlsx  # XLSX 파일 실행 예시"
