function test_exec() {
    echo -n "test: $1 ... " > /dev/tty

    # check if build with ld finishes successfully
    tests/$1/build.sh /usr/bin/ld
    tests/$1/a.out > /dev/null
    actual=$?
    if [ $actual != 0 ]; then
        echo -n "build with ld failed (exitcode: $actual)" > /dev/tty
    fi

    # build with myld
    tests/$1/build.sh 1> tests/$1/build.log 2> /dev/null
    if [ $? != 0 ]; then
        echo "link failed (See tests/$1/build.log)" > /dev/tty
        return
    fi
    # execute binary
    tests/$1/myld-a.out build/myld
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
test_exec "static2"
test_exec "static3"
