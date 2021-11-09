cd src
SDKSOURCES=$(find . -name *.cpp)
echo "SDKSOURCES=" $SDKSOURCES > Makefile.inc
cd ..
cd include
SDKHEADERS=$(find . -name \*.h -o -name \*.hpp)
echo "SDKHEADERS=" $SDKHEADERS > Makefile.inc
cd ..
aclocal  && autoheader  && automake --add-missing  && autoconf
