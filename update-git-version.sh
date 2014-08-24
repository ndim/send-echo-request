#!/bin/sh
#
# Update git-version.sh, but only if it has actually changed so that
# make does not rebuild everything every time.

set -e
cd "$(dirname "$0")"

comment="Unknown error trying to generate git version string"
git_version="unknown"

if test -f .git/info/exclude
then
    if git --version > /dev/null
    then
	if git diff-files --quiet && git diff-index --cached --quiet HEAD
	then
	    dirty=""
	else
	    dirty="-dirty"
	fi

	if tmp="$(git describe --all --long)"
	then
	    comment="output of \"git describe --all --long\" plus dirty status"
	    git_version="$tmp$dirty"
	else
	    comment="Error running command: git describe --all --long"
	fi
    else
	comment="Cannot run cmd: git --version"
    fi
else
    comment="Could not find file .git/info/exclude"
fi

cat>git-version.h.new<<EOF
/* DO NOT EDIT THIS FILE.
 * It was generated by the $(basename "$0") script.
 */

#ifndef GIT_VERSION_H
#define GIT_VERSION_H

/* ${comment} */
#define GIT_VERSION_STR "${git_version}"

#endif /* ! GIT_VERSION_H */
EOF

if test -f "git-version.h"
then
    if cmp -s "git-version.h.new" "git-version.h"
    then
	rm "git-version.h.new"
    else
	mv -f "git-version.h.new" "git-version.h"
    fi
else
    mv -f "git-version.h.new" "git-version.h"
fi
