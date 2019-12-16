#!/usr/bin/env bash

# Ard: helper for Arduino sketch folder

set -e -o nounset -o pipefail


grep="git grep"
bases="Mpe Misc Prototype"
sections=.sections.list


update_section_lists()
{
  eval $grep -l '{{{' $bases | while read fn
  do

    mkdir -p $(dirname .build/$fn )

    test .build/$fn.list -nt $fn || {
      test ! -e .build/$fn.list || rm .build/$fn.list

      eval $grep -h '{{{' $fn | while read match
      do
        echo $( echo "$match" | sed 's/[^A-Za-z0-9 _-]//g' ) >> .build/$fn.list
      done
    }
  done
}

compare_sections()
{
  find .build -iname '*.list' | while read list
  do

    echo
    echo $list
    diff -y $list .sections.list || continue

  done
}

check_ino_tab()
{
  gsed=sed
  . ~/project/user-scripts/src/sh/lib/match.lib.sh

  find $bases -iname '*.ino' | while read fn
  do
    pref=$(dirname "$fn")
    grep -q $( match_grep "$pref" ) ino.tab || {
      echo "Missing board target for $fn"
    }
  done
}

compile_all()
{
  while read stt id prefix fqbn defines
  do
    case "$stt" in

      '' | '#'* ) continue ;;
      -* ) continue ;;
    esac

    arduino compile -b $fqbn $prefix </dev/tty || return
    echo "Compiled '$id' firmware"
  done < ino.tab
}

help()
{
  cat <<EOM
  update-section-lists   Extract fold-names from *.ino files (prepare for comparison)
  compare-sections       Compare extracted fold-names with standard section names (.sections.list)

  check-ino-tab          Check that each sketch has a compilable board target in ino.tab
  compile-all            Compile all (enabled) targets from ino.tab
EOM
}

test -n "${1-}" || set -- compare-sections
$(echo $1 | tr '-' '_')
