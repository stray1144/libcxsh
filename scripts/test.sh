export BUILD_DIRECTORY=$1

$BUILD_DIRECTORY/test/lex
if [ $? -ne 0 ]; then
	echo "Didn't passed test lex.c"
	exit 1
fi

$BUILD_DIRECTORY/test/reotest
if [ $? -ne 0 ]; then
	echo "Didn't passed test reotest.c"
	exit 1
fi

$BUILD_DIRECTORY/test/buffer
if [ $? -ne 0 ]; then
	echo "Didn't passed test buffer.c"
	exit 1
fi

$BUILD_DIRECTORY/test/semantize
if [ $? -ne 0 ]; then
	echo "Didn't passed test semantize.c"
	exit 1
fi
