configure:
    cmake -S. -Bbuild -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -GNinja -DCMAKE_BUILD_TYPE=Debug

build: configure
    cmake --build build -j 12

run: build
    ./build/src/engine
