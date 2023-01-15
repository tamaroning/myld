function test_exec() {
    echo -n "test: $1 ... " > /dev/tty

    ./$1/build.sh
    if [ $? != 0 ]; then
        echo "build failed" > /dev/tty
        exit 1
    fi

    ./$1/myld-a.out
    actual=$?

    if [ $actual == 0 ]; then
        echo "ok" > /dev/tty
    else
        echo "failed"
        echo "expected 0, but got $actual" > /dev/tty
    fi
}

cd `dirname $0`
test_exec "simple"
test_exec "simple2"
