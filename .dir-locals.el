((nil . ((compile-command . "
set -e
# cd to project root, which contains .dir-locals.el
startdir=$PWD
until [ -f .dir-locals.el ]; do
  if [ \"$PWD\" = / ]; then cd \"$startdir\"; break; fi
  cd ..
done
unset startdir
# run full build in PROJECT_ROOT/build to avoid CMake clutter
cmake -S . -B build -DCMAKE_BUILD_TYPE=debugfull
make -k
"))))
