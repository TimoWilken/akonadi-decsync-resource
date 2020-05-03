((nil . ((flycheck-clang-include-path
          . ("/usr/include/qt/"
             "/usr/include/qt/QtCore/"
             "/usr/include/qt/QtDBus/"
             "/usr/include/qt/QtNetwork/"
             "/usr/include/qt/QtWidgets/"
             "/usr/include/qt/QtGui/"
             "/usr/include/KF5/KCoreAddons/"
             "/usr/include/KF5/KConfigCore/"
             "/usr/include/KF5/KConfigGui/"
             "/usr/include/KF5/KI18n/"
             "/usr/include/KF5/AkonadiCore/"
             "/usr/include/KF5/AkonadiAgentBase/"))

         (compile-command . "
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
cd build
make -k
"))))
