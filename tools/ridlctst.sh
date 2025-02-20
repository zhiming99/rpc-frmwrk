#!/bin/sh
a=(./ridl/fulltest.ridl ./monitor/appmon/appmon.ridl)
for i in ${a[@]}; do
rm *.cpp *.h *.new
ridlc -lsf -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --server -bsf -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --client -lsf -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --client -sf -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --client -s -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --client -f -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --server -lsf -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --server -sf -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --server -s -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --server -f -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc -bsf -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --client -bsf -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --client -bf -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --client -bs -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --client -b -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --server -bsf -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --server -bf -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --server -bs -O . $i
if ! make debug -j8; then
    exit $?
fi

rm *.cpp *.h *.new
ridlc --server -b -O . $i
if ! make debug -j8; then
    exit $?
fi
done

ridlc --server --sync_mode IUserManager=async --sync_mode \
IAppStore.ListApps=sync --services AppManager,SimpleAuth -l -O . \
monitor/appmon/appmon.ridl || exit $?

ridlc --client --sync_mode IUserManager=sync --sync_mode \
IAppStore=sync --services AppManager,SimpleAuth -bsf -O . \
monitor/appmon/appmon.ridl || exit $?
