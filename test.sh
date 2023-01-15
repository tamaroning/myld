function test_exec() {
    echo -n "test: $1 ... " > /dev/tty

    # check if build with ld finishes successfully
    tests/$1/ld-build.sh
    tests/$1/ld-a.out
    actual=$?
    if [ $actual != 0 ]; then
        echo -n "build with ld failed (exitcode: $actual)" > /dev/tty
    fi

    # build with myld
    tests/$1/build.sh
    if [ $? != 0 ]; then
        echo "build failed" > /dev/tty
        exit 1
    fi
    # execute binary
    tests/$1/myld-a.out
    actual=$?
    # check result
    if [ $actual == 0 ]; then
        echo "ok" > /dev/tty
    else
        echo "failed (exitcode: $actual)" > /dev/tty
    fi
}

cd `dirname $0`

test_exec "simple1"
test_exec "simple2"
test_exec "simple3"
test_exec "static1"
