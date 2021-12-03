# Clang-based source code Slicer

How to run: 

```
ln -s /usr/lib/llvm-8/lib/ <Slicer-Path>/lib
```

` ./slicer -extra-arg="-xc" -extra-arg="-isystem /usr/include" -extra-arg="-isystem /usr/include/x86_64-linux-gnu" -extra-arg="-isystem /usr/lib/llvm-8/lib/clang/8.0.0/include" -extra-arg="-I<project-header-path>" <file>.c --`