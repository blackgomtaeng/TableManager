#!/bin/bash
# ================================
# 📊 Linux System Health Check Script with Logging
# ================================

# 다운로드 폴더 경로 (필요 시 사용자 환경에 맞게 수정)
DOWNLOAD_DIR="$HOME/Downloads"

# 현재 날짜 (YYYYMMDD)
CURRENT_DATE=$(date +"%Y%m%d")

# 로그 파일 경로
LOG_FILE="$DOWNLOAD_DIR/linuxSpecs-$CURRENT_DATE.log"

# 실행 시간 기록
{
echo "===== 🕒 실행 시간: $(date +"%Y-%m-%d %H:%M:%S") ====="
echo

echo "===== 🖥️ CPU 정보 ====="
lscpu
echo
cat /proc/cpuinfo | head -20
echo
uptime
cat /proc/loadavg
echo

echo "===== 💾 메모리 정보 ====="
free -h
echo
cat /proc/meminfo | head -20
echo
vmstat 1 5
cat /proc/uptime
echo

echo "===== 📂 디스크/저장소 정보 ====="
df -h
echo
lsblk
echo
mount | head -20
cat /proc/partitions
echo

echo "===== 🌐 네트워크 정보 ====="
ip a
echo
ss -tuln
echo
cat /proc/net/dev
echo
netstat -i
echo

echo "===== ⚙️ 커널/시스템 정보 ====="
uname -a
echo
hostnamectl
echo
dmesg | tail -20
echo
cat /proc/version
echo
lsmod | head -20
echo

echo "===== 👥 사용자/프로세스 정보 ====="
top -b -n 1 | head -20
echo
ps aux --sort=-%mem | head -10
echo
who
echo
w
echo
id
echo

echo "===== ✅ 시스템 점검 완료 ====="
echo
} >> "$LOG_FILE"

echo "로그 파일이 저장되었습니다: $LOG_FILE"
