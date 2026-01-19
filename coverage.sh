#!/bin/bash
# Coverage report generator for Casm compiler
# Collects branch coverage data from test runs and generates reports
# Only measures src/ (compiler) coverage, excluding tests

set -e

COVERAGE_DIR="coverage_report"
COVERAGE_INFO="$COVERAGE_DIR/coverage.info"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo ""
echo "=========================================="
echo "Collecting Coverage Data"
echo "=========================================="

# Initialize coverage directory
mkdir -p "$COVERAGE_DIR"

# Remove old coverage info file (but keep .gcda files from tests)
rm -f "$COVERAGE_INFO" 2>/dev/null || true

# Collect coverage data from .gcda files
# .gcda files are generated in bin/ directory alongside the binary
# -b: collect branch coverage
# -r: relative paths
# --directory: where to find .gcda files
echo "Collecting coverage metrics..."
lcov --capture \
  --directory bin \
  --output-file "$COVERAGE_INFO" \
  --branch-coverage \
  2>/dev/null || {
  echo -e "${RED}✗ Failed to collect coverage data${NC}"
  exit 1
}

# Extract coverage statistics from coverage.info
extract_coverage_stats() {
  local info_file="$1"
  
  # Parse the coverage.info file for statistics
  # Count total branches and lines across all files
  
  local lines_hit=0
  local lines_total=0
  local branches_hit=0
  local branches_total=0
  local functions_hit=0
  local functions_total=0
  
  # Count records in coverage.info (one per file section)
  while IFS= read -r line; do
    case "$line" in
      LH:*) lines_hit=$((lines_hit + ${line#LH:})) ;;
      LF:*) lines_total=$((lines_total + ${line#LF:})) ;;
      BRH:*) branches_hit=$((branches_hit + ${line#BRH:})) ;;
      BRF:*) branches_total=$((branches_total + ${line#BRF:})) ;;
      FNH:*) functions_hit=$((functions_hit + ${line#FNH:})) ;;
      FNF:*) functions_total=$((functions_total + ${line#FNF:})) ;;
    esac
  done < "$info_file"
  
  echo "$lines_hit:$lines_total:$branches_hit:$branches_total:$functions_hit:$functions_total"
}

# Extract stats
STATS=$(extract_coverage_stats "$COVERAGE_INFO")
IFS=':' read -r LINES_HIT LINES_TOTAL BRANCHES_HIT BRANCHES_TOTAL FUNCTIONS_HIT FUNCTIONS_TOTAL <<< "$STATS"

# Calculate percentages
calc_percent() {
  local hit=$1
  local total=$2
  if [ "$total" -eq 0 ]; then
    echo "0.0"
  else
    awk "BEGIN {printf \"%.1f\", ($hit * 100.0) / $total}"
  fi
}

BRANCH_PERCENT=$(calc_percent "$BRANCHES_HIT" "$BRANCHES_TOTAL")
LINE_PERCENT=$(calc_percent "$LINES_HIT" "$LINES_TOTAL")
FUNCTION_PERCENT=$(calc_percent "$FUNCTIONS_HIT" "$FUNCTIONS_TOTAL")

# Generate HTML report
echo "Generating HTML report..."
genhtml \
  "$COVERAGE_INFO" \
  --output-directory "$COVERAGE_DIR" \
  --branch-coverage \
  --highlight \
  --legend \
  --title "Casm Compiler - Branch Coverage Report" \
  2>/dev/null || {
  echo -e "${RED}✗ Failed to generate HTML report${NC}"
  exit 1
}

# Print terminal summary
echo ""
echo "=========================================="
echo "       COVERAGE REPORT - BRANCH COVERAGE"
echo "=========================================="
echo ""

# Color code based on coverage percentage
color_percent() {
  local percent=$1
  if (( $(echo "$percent >= 80" | bc -l) )); then
    echo -e "${GREEN}$percent%${NC}"
  elif (( $(echo "$percent >= 60" | bc -l) )); then
    echo -e "${YELLOW}$percent%${NC}"
  else
    echo -e "${RED}$percent%${NC}"
  fi
}

BRANCH_COLOR=$(color_percent "$BRANCH_PERCENT")
LINE_COLOR=$(color_percent "$LINE_PERCENT")
FUNCTION_COLOR=$(color_percent "$FUNCTION_PERCENT")

echo "Branch Coverage:     $BRANCH_COLOR    ($BRANCHES_HIT / $BRANCHES_TOTAL branches)"
echo "Line Coverage:       $LINE_COLOR    ($LINES_HIT / $LINES_TOTAL lines)"
echo "Function Coverage:   $FUNCTION_COLOR   ($FUNCTIONS_HIT / $FUNCTIONS_TOTAL functions)"

echo ""
echo "=========================================="
echo ""

# Find least covered source files
echo "Coverage by File (Least Covered First):"
echo ""

# Parse coverage.info to extract per-file branch coverage
# Format: SF:filename ... BRH:branches_hit ... BRF:branches_found
awk '
  /^SF:/ {
    if (file != "" && brf > 0) {
      brpct = int((brh * 100.0) / brf + 0.5)
      print file ":" brpct ":" brh ":" brf
    }
    file = substr($0, 4)
    brh = 0
    brf = 0
  }
  /^BRH:/ { brh = substr($0, 5) }
  /^BRF:/ { brf = substr($0, 5) }
  END {
    if (file != "" && brf > 0) {
      brpct = int((brh * 100.0) / brf + 0.5)
      print file ":" brpct ":" brh ":" brf
    }
  }
' "$COVERAGE_INFO" | grep "src/" | sort -t: -k2 -n | head -10 | while IFS=: read -r file brpct brh brf; do
  if [ -z "$file" ] || [ "$brf" -eq 0 ]; then
    continue
  fi
  file_basename=$(basename "$file")
  printf "  %-30s  %3d%% branch  (%d/%d branches)\n" "$file_basename" "$brpct" "$brh" "$brf"
done

echo ""
echo "HTML Report:"
echo "  Open: $COVERAGE_DIR/index.html"
if command -v xdg-open &> /dev/null; then
  echo "  (Use 'xdg-open $COVERAGE_DIR/index.html' to open in default browser)"
elif command -v open &> /dev/null; then
  echo "  (Use 'open $COVERAGE_DIR/index.html' to open in default browser)"
fi

echo ""
echo "=========================================="
echo ""
