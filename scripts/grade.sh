#!/bin/bash

WORKDIR=$(dirname "$(dirname "$(readlink -f "$0")")")

build() {
  build_target=$1
  cd "$WORKDIR" && mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. >/dev/null && make "$build_target" -j >/dev/null
  if [[ $? != 0 ]]; then
    echo "Error: Compile error, try to run make build and debug"
    exit 1
  fi
}

test_python() {
  local score_str="LAB7 SCORE"
  local testcase_dir=${WORKDIR}/testdata/python/testcases
  local ref_dir=${WORKDIR}/testdata/python/refs
  local runtime_path=${WORKDIR}/src/tiger/runtime/runtime.cc
  local heap_path=${WORKDIR}/src/tiger/runtime/gc/heap/derived_heap.cc
  local testcase_name

  build tiger-compiler
  for testcase in "$testcase_dir"/*.py; do
    testcase_name=$(basename "$testcase" | cut -f1 -d".")
    local ref=${ref_dir}/${testcase_name}.out
    local assem=${testcase}.tig.s
    local external=${testcase}.cc
    # echo "compiling [$testcase]"
    ./tiger-compiler "$testcase" &>/dev/null
    rm test.out &>/dev/null
    g++ -Wl,--wrap,getchar -m64 "$assem" "$external" "$runtime_path" "$heap_path" -o test.out &>/dev/null
    if [ ! -s test.out ]; then
      echo "Error: Link error [$testcase_name]"
      full_score=0
      continue
    fi

    ./test.out >&/tmp/output.txt
    if [[ -f  "$ref" ]]; 
    then
      cp "$ref" /tmp/ref.txt
    else
      python3 "$testcase" &>/tmp/ref.txt
    fi
    diff -w -B /tmp/output.txt /tmp/ref.txt
    if [[ $? != 0 ]]; then
      echo "Error: Output mismatch [$testcase_name]"
      full_score=0
      continue
    fi
    echo "Pass $testcase_name"
  done
  rm "$testcase_dir"/*.py.tig &>/dev/null
  rm "$testcase_dir"/*.py.tig.s &>/dev/null
  rm "$testcase_dir"/*.py.cc &>/dev/null
}

main() {
  local scope=$1

  if [[ ! $(pwd) == "$WORKDIR" ]]; then
    echo "Error: Please run this grading script in the root dir of the project"
    exit 1
  fi

  if [[ ! $(uname -s) == "Linux" ]]; then
    echo "Error: Please run this grading script in a Linux system"
    exit 1
  fi

  test_python
}

main "$1"
