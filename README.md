# cs526-project

HelloWorld: Your First Pass
===========================
The **HelloWorld** pass from
[HelloWorld.cpp](https://github.com/banach-space/llvm-tutor/blob/main/HelloWorld/HelloWorld.cpp)
is a self-contained *reference example*. The corresponding
[CMakeLists.txt](https://github.com/banach-space/llvm-tutor/blob/main/HelloWorld/CMakeLists.txt)
implements the minimum set-up for an out-of-source pass.

For every function defined in the input module, **HelloWorld** prints its name
and the number of arguments that it takes. You can build it like this:

```bash
export LLVM_DIR=<installation/dir/of/llvm/16>
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1 && cmake --build ./build/
```

Before you can test it, you need to prepare an input file:

```bash
# Make sure inside build folder generate an LLVM test file
$LLVM_DIR/bin/clang -disable-O0-optnone -S -emit-llvm ./tests/input_for_hello.c -o ./build/input_for_hello.ll
```

Finally, run **HelloWorld** with
[**opt**](http://llvm.org/docs/CommandGuide/opt.html) (use `libHelloWorld.so`
on Linux and `libHelloWorld.dylib` on Mac OS):

```bash
# Run the pass
$LLVM_DIR/bin/opt -load-pass-plugin ./build/lib/libHelloWorld.so -passes=hello-world -disable-output ./build/input_for_hello.ll
# Expected output
(llvm-tutor) Hello from: foo
(llvm-tutor)   number of arguments: 1
(llvm-tutor) Hello from: bar
(llvm-tutor)   number of arguments: 2
(llvm-tutor) Hello from: fez
(llvm-tutor)   number of arguments: 3
(llvm-tutor) Hello from: main
(llvm-tutor)   number of arguments: 2
```

The **HelloWorld** pass doesn't modify the input module. The `-disable-output`
flag is used to prevent **opt** from printing the output bitcode file.
